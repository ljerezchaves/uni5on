/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <string>
#include "ring-controller.h"
#include "epc-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingController");
NS_OBJECT_ENSURE_REGISTERED (RingController);

RingController::RingController ()
{
  NS_LOG_FUNCTION (this);
}

RingController::~RingController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent<EpcController> ()
    .AddConstructor<RingController> ()
    .AddAttribute ("Strategy",
                   "The ring routing strategy.",
                   EnumValue (RingController::SPO),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::SPO, "spo",
                                    RingController::SPF, "spf"))
    .AddAttribute ("DebarIncStep",
                   "DeBaR increase adjustment step.",
                   DoubleValue (0.025), // 2.5% of GBR quota
                   MakeDoubleAccessor (&RingController::m_debarStep),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("EnableShortDebar",
                   "Enable GBR Distance-Based Reservation algorithm (DeBaR) "
                   "in shortest path.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RingController::m_debarShortPath),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableLongDebar",
                   "Enable GBR Distance-Based Reservation algorithm (DeBaR) "
                   "in longest (inverted) paths.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RingController::m_debarLongPath),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ipSwitchTable.clear ();
  EpcController::DoDispose ();
}

void
RingController::NotifyS5Attach (
  Ptr<OFSwitch13Device> swtchDev, uint32_t portNo, Ptr<NetDevice> gwDev)
{
  NS_LOG_FUNCTION (this << swtchDev << portNo << gwDev);

  // Save the pair S/P-GW IP address / switch index.
  std::pair<Ipv4Address, uint16_t> entry (
    EpcNetwork::GetIpv4Addr (gwDev), GetSwitchIndex (swtchDev));
  std::pair<IpSwitchMap_t::iterator, bool> ret;
  ret = m_ipSwitchTable.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("This IP already existis in switch index table.");
    }

  // Chain up.
  EpcController::NotifyS5Attach (swtchDev, portNo, gwDev);
}

void
RingController::NotifySwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  // Connecting this controller to ConnectionInfo trace source
  cInfo->TraceConnectWithoutContext (
    "NonGbrAdjusted", MakeCallback (&RingController::NonGbrAdjusted, this));

  // Installing groups and meters for ring network. Note that following
  // commands works as connections are created in clockwise direction, and
  // switchs inside cInfo are saved in the same direction.
  std::ostringstream cmd01, cmd11, cmd02, cmd12;
  uint64_t kbps = 0;

  // Routing group for clockwise packet forwarding.
  cmd01 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::CLOCK
        << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (0);
  DpctlSchedule (cInfo->GetSwDpId (0), cmd01.str ());

  // Routing group for counterclockwise packet forwarding.
  cmd11 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::COUNTER
        << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (1);
  DpctlSchedule (cInfo->GetSwDpId (1), cmd11.str ());

  if (GetNonGbrCoexistence ())
    {
      // Meter flags OFPMF_KBPS.
      std::string flagsStr ("0x0001");

      // Non-GBR meter for clockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::FWD) / 1000;
      cmd02 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::CLOCK
            << " drop:rate=" << kbps;
      DpctlSchedule (cInfo->GetSwDpId (0), cmd02.str ());

      // Non-GBR meter for counterclockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::BWD) / 1000;
      cmd12 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::COUNTER
            << " drop:rate=" << kbps;
      DpctlSchedule (cInfo->GetSwDpId (1), cmd12.str ());
    }
}

void
RingController::NotifyTopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // Save the collection of switch devices and create the spanning tree.
  m_ofDevices = devices;
  CreateSpanningTree ();

  // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Configure routes to keep forwarding packets already in the ring until they
  // reach the destination switch.
  for (uint16_t swIdx = 0; swIdx < GetNSwitches (); swIdx++)
    {
      uint16_t nextIdx = NextSwitchIndex (swIdx, RingRoutingInfo::CLOCK);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (swIdx, nextIdx);

      // ---------------------------------------------------------------------
      // Table 2 -- Routing table -- [from higher to lower priority]
      //
      // GTP packets being forwarded by this switch. Write the output group
      // into action set based on input port. Write the same group number into
      // metadata field. Send the packet to Coexistence QoS table.
      std::ostringstream cmd0, cmd1;
      char metadataStr [12];

      sprintf (metadataStr, "0x%x", RingRoutingInfo::COUNTER);
      cmd0 << "flow-mod cmd=add,table=2,prio=128"
           << ",flags=" << flagsStr
           << " meta=0x0,in_port=" << cInfo->GetPortNo (0)
           << " write:group=" << RingRoutingInfo::COUNTER
           << " meta:" << metadataStr
           << " goto:3";
      DpctlSchedule (cInfo->GetSwDpId (0), cmd0.str ());

      sprintf (metadataStr, "0x%x", RingRoutingInfo::CLOCK);
      cmd1 << "flow-mod cmd=add,table=2,prio=128"
           << ",flags=" << flagsStr
           << " meta=0x0,in_port=" << cInfo->GetPortNo (1)
           << " write:group=" << RingRoutingInfo::CLOCK
           << " meta:" << metadataStr
           << " goto:3";
      DpctlSchedule (cInfo->GetSwDpId (1), cmd1.str ());
    }
}

bool
RingController::TopologyInstallRouting (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Installing ring rules for bearer teid " << rInfo->GetTeid ());

  // Getting ring routing information.
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);

  // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Printing the cookie and buffer values in dpctl string format.
  char cookieStr [20], metadataStr [12];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Building the dpctl command + arguments string.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=1"
      << ",flags=" << flagsStr
      << ",cookie=" << cookieStr
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Configuring downlink routing.
  if (rInfo->HasDownlinkTraffic ())
    {
      // Building the match string.
      // No match on source IP because we may have several P-GW TFT switches.
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_dst=" << rInfo->GetSgwS5Addr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // For GBR bearers, set the IP DSCP field.
      std::ostringstream act;
      if (rInfo->IsGbr ())
        {
          // Build the apply set_field action instruction string.
          act << " apply:set_field=ip_dscp:"
              << rInfo->GetObject<GbrInfo> ()->GetDscp ();
        }

      // Build the metatada, write and goto instructions string.
      sprintf (metadataStr, "0x%x", ringInfo->GetDownPath ());
      act << " meta:" << metadataStr << " goto:2";

      // Installing the rule into input switch.
      // In downlink the input ring switch is the one connected to the P-GW.
      std::string commandStr = cmd.str () + match.str () + act.str ();
      DpctlExecute (ringInfo->GetPgwSwDpId (), commandStr);
    }

  // Configuring uplink routing.
  if (rInfo->HasUplinkTraffic ())
    {
      // Building the match string.
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_src=" << rInfo->GetSgwS5Addr ()
            << ",ip_dst=" << rInfo->GetPgwS5Addr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // For GBR bearers, set the IP DSCP field.
      std::ostringstream act;
      if (rInfo->IsGbr ())
        {
          // Build the apply set_field action instruction string.
          act << " apply:set_field=ip_dscp:"
              << rInfo->GetObject<GbrInfo> ()->GetDscp ();
        }

      // Build the metatada, write and goto instructions string.
      sprintf (metadataStr, "0x%x", ringInfo->GetUpPath ());
      act << " meta:" << metadataStr << " goto:2";

      // Installing the rule into input switch.
      // In uplink the input ring switch is the one connected to the S-GW.
      std::string commandStr = cmd.str () + match.str () + act.str ();
      DpctlExecute (ringInfo->GetSgwSwDpId (), commandStr);
    }
  return true;
}

bool
RingController::TopologyRemoveRouting (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Removing ring rules for bearer teid " << rInfo->GetTeid ());

  // Print the cookie value in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Getting ring routing information.
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,table=1"
      << ",cookie=" << cookieStr
      << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.

  // Remove downlink routing.
  if (rInfo->HasDownlinkTraffic ())
    {
      DpctlExecute (ringInfo->GetPgwSwDpId (), cmd.str ());
    }

  // Remove uplink routing.
  if (rInfo->HasUplinkTraffic ())
    {
      DpctlExecute (ringInfo->GetSgwSwDpId (), cmd.str ());
    }
  return true;
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  // Reseting ring routing info to the shortest path.
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  ringInfo->ResetToDefaultPaths ();

  if (GetS5TrafficAggregation () && !rInfo->IsDefault ())
    {
      // When the traffic aggregation is enable, we always accept dedicated
      // bearer requests without guarantees.
      rInfo->SetAggregated (true);
      return true;
    }

  if (!rInfo->IsGbr ())
    {
      // For Non-GBR bearers (which includes the default bearer), let's accept
      // it without guarantees. Note that in current implementation, Non-GBR
      // bearers are always routed over the shortest path. However, nothing
      // prevents the use of a more sophisticated routing approach.
      return true;
    }

  if (ringInfo->IsLocalPath ())
    {
      // For GBR bearers that only transverse local switch (local routing),
      // let's accept it without guarantees.
      return true;
    }

  uint32_t teid = rInfo->GetTeid ();
  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  NS_ASSERT_MSG (gbrInfo, "Invalid configuration for bearer request.");

  // Getting available downlink and uplink bit rates in both paths.
  std::pair<uint64_t, uint64_t> shortPathBand, longPathBand;
  shortPathBand = GetAvailableGbrBitRate (ringInfo, true);
  longPathBand  = GetAvailableGbrBitRate (ringInfo, false);

  uint64_t dlShortBw = shortPathBand.first;
  uint64_t ulShortBw = shortPathBand.second;
  uint64_t dlLongBw  = longPathBand.first;
  uint64_t ulLongBw  = longPathBand.second;

  // Getting bit rate requests.
  uint64_t dlRequest = gbrInfo->GetDownBitRate ();
  uint64_t ulRequest = gbrInfo->GetUpBitRate ();

  NS_LOG_DEBUG (teid << " req down "   << dlRequest << ", up " << ulRequest);
  NS_LOG_DEBUG (teid << " short down " << dlShortBw << ", up " << ulShortBw);
  NS_LOG_DEBUG (teid << " long down "  << dlLongBw  << ", up " << ulLongBw);

  switch (m_strategy)
    {
    case RingController::SPO:
      {
        if (dlShortBw >= dlRequest && ulShortBw >= ulRequest)
          {
            NS_LOG_INFO ("Routing bearer teid " << teid << ": shortest path");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else
          {
            NS_LOG_WARN ("Blocking bearer teid " << teid);
            return false;
          }
        break;
      }
    case RingController::SPF:
      {
        if (dlShortBw >= dlRequest && ulShortBw >= ulRequest)
          {
            NS_LOG_INFO ("Routing bearer teid " << teid << ": shortest path");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else if (dlLongBw >= dlRequest && ulLongBw >= ulRequest)
          {
            // Let's invert the path and reserve the bit rate
            ringInfo->InvertBothPaths ();
            NS_LOG_INFO ("Routing bearer teid " << teid << ": longest path");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else
          {
            NS_LOG_WARN ("Blocking bearer teid " << teid);
            return false;
          }
        break;
      }
    }
  NS_ABORT_MSG ("Invalid Routing strategy.");
}

bool
RingController::TopologyBearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo && gbrInfo->IsReserved ())
    {
      Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");
      NS_LOG_INFO ("Releasing resources for bearer " << rInfo->GetTeid ());
      ReleaseGbrBitRate (ringInfo, gbrInfo);
    }
  rInfo->SetAggregated (false);
  return true;
}

void
RingController::CreateSpanningTree (void)
{
  NS_LOG_FUNCTION (this);

  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD). Here we are disabling the farthest gateway link,
  // configuring its ports to OFPPC_NO_FWD config (0x20).
  uint16_t half = (GetNSwitches () / 2);
  Ptr<ConnectionInfo> cInfo = GetConnectionInfo (half, half + 1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " <<
                half + 1 << " for broadcast messages.");

  Mac48Address macAddr1, macAddr2;
  std::ostringstream cmd1, cmd2;

  macAddr1 = Mac48Address::ConvertFrom (cInfo->GetPortDev (0)->GetAddress ());
  cmd1 << "port-mod port=" << cInfo->GetPortNo (0)
       << ",addr=" <<  macAddr1
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (cInfo->GetSwDpId (0), cmd1.str ());

  macAddr2 = Mac48Address::ConvertFrom (cInfo->GetPortDev (1)->GetAddress ());
  cmd2 << "port-mod port=" << cInfo->GetPortNo (1)
       << ",addr=" << macAddr2
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (cInfo->GetSwDpId (1), cmd2.str ());
}

void
RingController::NonGbrAdjusted (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  if (GetNonGbrCoexistence ())
    {
      std::ostringstream cmd1, cmd2;
      uint64_t kbps = 0;

      // Meter flags OFPMF_KBPS.
      std::string flagsStr ("0x0001");

      // Update Non-GBR meter for clockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::FWD) / 1000;
      cmd1 << "meter-mod cmd=mod"
           << ",flags=" << flagsStr
           << ",meter=" << RingRoutingInfo::CLOCK
           << " drop:rate=" << kbps;
      DpctlExecute (cInfo->GetSwDpId (0), cmd1.str ());

      // Update Non-GBR meter for counterclockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::BWD) / 1000;
      cmd2 << "meter-mod cmd=mod"
           << ",flags=" << flagsStr
           << ",meter=" << RingRoutingInfo::COUNTER
           << " drop:rate=" << kbps;
      DpctlExecute (cInfo->GetSwDpId (1), cmd2.str ());
    }
}

Ptr<RingRoutingInfo>
RingController::GetRingRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo == 0)
    {
      // This is the first time in simulation we are querying ring information
      // for this bearer. Let's create its ring routing metadata.
      ringInfo = CreateObject<RingRoutingInfo> (rInfo);

      // Set internal switch indexes.
      ringInfo->SetPgwSwIdx  (GetSwitchIndex (rInfo->GetPgwS5Addr ()));
      ringInfo->SetSgwSwIdx  (GetSwitchIndex (rInfo->GetSgwS5Addr ()));
      ringInfo->SetPgwSwDpId (GetDpId (ringInfo->GetPgwSwIdx ()));
      ringInfo->SetSgwSwDpId (GetDpId (ringInfo->GetSgwSwIdx ()));

      // Considering default paths those with lower hops.
      RingRoutingInfo::RoutingPath dlPath, ulPath;
      dlPath = FindShortestPath (ringInfo->GetPgwSwIdx (),
                                 ringInfo->GetSgwSwIdx ());
      ulPath = FindShortestPath (ringInfo->GetSgwSwIdx (),
                                 ringInfo->GetPgwSwIdx ());
      ringInfo->SetDefaultPaths (dlPath, ulPath);
    }
  return ringInfo;
}

Ptr<ConnectionInfo>
RingController::GetConnectionInfo (uint16_t idx1, uint16_t idx2)
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  return ConnectionInfo::GetPointer (GetDpId (idx1), GetDpId (idx2));
}

uint16_t
RingController::GetSwitchIndex (Ipv4Address ipAddr) const
{
  NS_LOG_FUNCTION (this << ipAddr);

  IpSwitchMap_t::const_iterator ret;
  ret = m_ipSwitchTable.find (ipAddr);
  if (ret != m_ipSwitchTable.end ())
    {
      return ret->second;
    }
  NS_FATAL_ERROR ("IP not registered in switch index table.");
}

uint16_t
RingController::GetSwitchIndex (Ptr<OFSwitch13Device> dev) const
{
  NS_LOG_FUNCTION (this << dev);

  uint16_t idx;
  for (idx = 0; idx < GetNSwitches (); idx++)
    {
      if (m_ofDevices.Get (idx) == dev)
        {
          break;
        }
    }
  NS_ASSERT_MSG (idx < GetNSwitches (), "Switch not found in collection.");
  return idx;
}

uint16_t
RingController::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ofDevices.GetN ();
}

uint64_t
RingController::GetDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_ofDevices.GetN (), "Invalid switch index.");
  return m_ofDevices.Get (idx)->GetDatapathId ();
}

std::pair<uint64_t, uint64_t>
RingController::GetAvailableGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                        bool useShortPath)
{
  NS_LOG_FUNCTION (this << ringInfo << useShortPath);

  uint16_t pgwIdx      = ringInfo->GetPgwSwIdx ();
  uint16_t sgwIdx      = ringInfo->GetSgwSwIdx ();
  uint64_t downBitRate = std::numeric_limits<uint64_t>::max ();
  uint64_t upBitRate   = std::numeric_limits<uint64_t>::max ();
  uint64_t bitRate     = 0;
  uint16_t current     = sgwIdx;
  double   debarFactor = 1.0;

  RingRoutingInfo::RoutingPath upPath = FindShortestPath (sgwIdx, pgwIdx);
  if (!useShortPath)
    {
      upPath = RingRoutingInfo::InvertPath (upPath);
    }

  // From the eNB to the gateway switch index, get the bit rate for each link.
  while (current != pgwIdx)
    {
      uint16_t next = NextSwitchIndex (current, upPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);

      // Check for available bit rate in uplink direction.
      bitRate = cInfo->GetAvailableGbrBitRate (
          GetDpId (current), GetDpId (next), debarFactor);
      if (bitRate < upBitRate)
        {
          upBitRate = bitRate;
        }

      // Check for available bit rate in downlink direction.
      bitRate = cInfo->GetAvailableGbrBitRate (
          GetDpId (next), GetDpId (current), debarFactor);
      if (bitRate < downBitRate)
        {
          downBitRate = bitRate;
        }
      current = next;

      // If enable, apply the GBR Distance-Based Reservation algorithm (DeBaR)
      // when looking for the available bit rate in routing path.
      if ((m_debarShortPath && useShortPath)
          || (m_debarLongPath && !useShortPath))
        {
          // Avoiding negative DeBaR factor.
          debarFactor = std::max (debarFactor - m_debarStep, 0.0);
        }
    }

  // Return the pair of available GBR bit rates (downlink and uplink).
  return std::pair<uint64_t, uint64_t> (downBitRate, upBitRate);
}

bool
RingController::ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Reserving resources for GBR bearer.");
  PerLinkReserve (ringInfo->GetPgwSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetDownPath (), gbrInfo->GetDownBitRate ());
  PerLinkReserve (ringInfo->GetSgwSwIdx (), ringInfo->GetPgwSwIdx (),
                  ringInfo->GetUpPath (), gbrInfo->GetUpBitRate ());
  gbrInfo->SetReserved (true);
  return true;
}

bool
RingController::ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Releasing resources for GBR bearer.");
  PerLinkRelease (ringInfo->GetPgwSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetDownPath (), gbrInfo->GetDownBitRate ());
  PerLinkRelease (ringInfo->GetSgwSwIdx (), ringInfo->GetPgwSwIdx (),
                  ringInfo->GetUpPath (), gbrInfo->GetUpBitRate ());
  gbrInfo->SetReserved (false);
  return true;
}

bool
RingController::PerLinkReserve (uint16_t srcIdx, uint16_t dstIdx,
                                RingRoutingInfo::RoutingPath path,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path << bitRate);

  bool success = true;
  while (success && srcIdx != dstIdx)
    {
      uint16_t next = NextSwitchIndex (srcIdx, path);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (srcIdx, next);
      success = cInfo->ReserveGbrBitRate (
          GetDpId (srcIdx), GetDpId (next), bitRate);
      srcIdx = next;
    }
  NS_ASSERT_MSG (success, "Error when reserving resources.");
  return success;
}

bool
RingController::PerLinkRelease (uint16_t srcIdx, uint16_t dstIdx,
                                RingRoutingInfo::RoutingPath path,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path << bitRate);

  bool success = true;
  while (success && srcIdx != dstIdx)
    {
      uint16_t next = NextSwitchIndex (srcIdx, path);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (srcIdx, next);
      success = cInfo->ReleaseGbrBitRate (
          GetDpId (srcIdx), GetDpId (next), bitRate);
      srcIdx = next;
    }
  NS_ASSERT_MSG (success, "Error when reserving resources.");
  return success;
}

uint16_t
RingController::NextSwitchIndex (uint16_t idx,
                                 RingRoutingInfo::RoutingPath path)
{
  NS_LOG_FUNCTION (this << idx << path);

  NS_ASSERT_MSG (path != RingRoutingInfo::LOCAL,
                 "Not supposed to get here for local routing.");

  return path == RingRoutingInfo::CLOCK ?
         (idx + 1) % GetNSwitches () :
         (idx == 0 ? GetNSwitches () - 1 : (idx - 1));
}

RingRoutingInfo::RoutingPath
RingController::FindShortestPath (uint16_t srcIdx, uint16_t dstIdx)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx);

  NS_ASSERT (std::max (srcIdx, dstIdx) < GetNSwitches ());

  // Check for local routing.
  if (srcIdx == dstIdx)
    {
      return RingRoutingInfo::LOCAL;
    }

  // Identify the shortest routing path from src to dst switch index.
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstIdx - srcIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  return (clockwiseDistance <= maxHops) ?
         RingRoutingInfo::CLOCK :
         RingRoutingInfo::COUNTER;
}

uint16_t
RingController::HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                            RingRoutingInfo::RoutingPath path)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx);

  NS_ASSERT (std::max (srcIdx, dstIdx) < GetNSwitches ());

  // Check for local routing.
  if (path == RingRoutingInfo::LOCAL)
    {
      NS_ASSERT (srcIdx == dstIdx);
      return 0;
    }

  // Count the number of hops from src to dst switch index.
  NS_ASSERT (srcIdx != dstIdx);
  int distance = dstIdx - srcIdx;
  if (path == RingRoutingInfo::COUNTER)
    {
      distance = srcIdx - dstIdx;
    }
  if (distance < 0)
    {
      distance += GetNSwitches ();
    }
  return distance;
}

};  // namespace ns3
