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

#include "ring-controller.h"
#include <string>

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
    .SetParent (EpcController::GetTypeId ())
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
  m_connections.clear ();
  EpcController::DoDispose ();
}

void
RingController::NewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this);

  // Call base method which will connect trace sources and sinks, and save this
  // connection info for further usage.
  EpcController::NewSwitchConnection (cInfo);
  SaveConnectionInfo (cInfo);

  // Installing groups and meters for ring network. Note that following
  // commands works as connections are created in clockwise direction, and
  // switchs inside cInfo are saved in the same direction.
  std::ostringstream cmd01, cmd11, cmd02, cmd12;
  uint64_t kbps = 0;

  // Routing group for clockwise packet forwarding.
  cmd01 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::CLOCK
        << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (0);

  // Routing group for counterclockwise packet forwarding.
  cmd11 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::COUNTER
        << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (1);

  DpctlSchedule (cInfo->GetSwDpId (0), cmd01.str ());
  DpctlSchedule (cInfo->GetSwDpId (1), cmd11.str ());

  if (m_nonGbrCoexistence)
    {
      // Meter flags OFPMF_KBPS.
      std::string flagsStr ("0x0001");

      // Non-GBR meter for clockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::FWD) / 1000;
      cmd02 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::CLOCK
            << " drop:rate=" << kbps;

      // Non-GBR meter for counterclockwise direction.
      kbps = cInfo->GetNonGbrBitRate (ConnectionInfo::BWD) / 1000;
      cmd12 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::COUNTER
            << " drop:rate=" << kbps;

      DpctlSchedule (cInfo->GetSwDpId (0), cmd02.str ());
      DpctlSchedule (cInfo->GetSwDpId (1), cmd12.str ());
    }
}

void
RingController::TopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // Save the number of switches in network topology.
  m_noSwitches = devices.GetN ();

  // Call base method which will save devices and create the spanning tree.
  EpcController::TopologyBuilt (devices);

  // Flow-mod flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and
  // OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Configure routes to keep forwarding packets already in the ring until they
  // reach the destination switch.
  for (uint16_t sw1 = 0; sw1 < GetNSwitches (); sw1++)
    {
      uint16_t sw2 = NextSwitchIndex (sw1, RingRoutingInfo::CLOCK);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (sw1, sw2);

      // ---------------------------------------------------------------------
      // Table 2 -- Routing table -- [from higher to lower priority]
      //
      // GTP packets being forwarded by this switch. Write the output group
      // into action set based on input port. Write the same group number into
      // metadata field. Send the packet to Coexistence QoS table.
      std::ostringstream cmd0, cmd1;
      char metadataStr [9];

      sprintf (metadataStr, "0x%x", RingRoutingInfo::COUNTER);
      cmd0 << "flow-mod cmd=add,table=2,prio=128"
           << ",flags=" << flagsStr
           << " meta=0x0,in_port=" << cInfo->GetPortNo (0)
           << " write:group=" << RingRoutingInfo::COUNTER
           << " meta:" << metadataStr
           << " goto:3";

      sprintf (metadataStr, "0x%x", RingRoutingInfo::CLOCK);
      cmd1 << "flow-mod cmd=add,table=2,prio=128"
           << ",flags=" << flagsStr
           << " meta=0x0,in_port=" << cInfo->GetPortNo (1)
           << " write:group=" << RingRoutingInfo::CLOCK
           << " meta:" << metadataStr
           << " goto:3";

      DpctlSchedule (cInfo->GetSwDpId (0), cmd0.str ());
      DpctlSchedule (cInfo->GetSwDpId (1), cmd1.str ());
    }
}

void
RingController::NonGbrAdjusted (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  if (m_nonGbrCoexistence)
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

bool
RingController::TopologyInstallRouting (Ptr<RoutingInfo> rInfo,
                                        uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << rInfo->GetPriority ()
                        << buffer);
  NS_ASSERT_MSG (rInfo->IsActive (), "Rule not active.");

  // Getting rInfo associated metadata.
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  bool meterInstalled = false;

  // Increasing the priority every time we (re)install routing rules.
  rInfo->IncreasePriority ();

  // Install P-GW TFT rules
  InstallPgwTftRules (rInfo, buffer);

  // Flow-mod flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and
  // OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Printing the cookie and buffer values in dpctl string format.
  char cookieStr [9], bufferStr [12], metadataStr [9];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
  sprintf (bufferStr, "%u",   buffer);

  // Building the dpctl command + arguments string.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=1"
      << ",buffer=" << bufferStr
      << ",flags=" << flagsStr
      << ",cookie=" << cookieStr
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Configuring downlink routing.
  if (rInfo->HasDownlinkTraffic ())
    {
      std::ostringstream match, act;

      // In downlink the input switch is the gateway.
      uint16_t swIdx = rInfo->GetSgwSwIdx ();

      // Building the match string
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_src=" << rInfo->GetSgwAddr ()
            << ",ip_dst=" << rInfo->GetEnbAddr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // Check for meter entry.
      if (meterInfo && meterInfo->HasDown ())
        {
          if (!meterInfo->IsInstalled ())
            {
              // Install the per-flow meter entry.
              DpctlExecute (GetDatapathId (swIdx),
                            meterInfo->GetDownAddCmd ());
              meterInstalled = true;
            }

          // Building the per-flow meter instruction string.
          act << " meter:" << rInfo->GetTeid ();
        }

      // For GBR bearers, mark the IP DSCP field.
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
      std::string commandStr = cmd.str () + match.str () + act.str ();
      DpctlExecute (GetDatapathId (swIdx), commandStr);
    }

  // Configuring uplink routing.
  if (rInfo->HasUplinkTraffic ())
    {
      std::ostringstream match, act;

      // In uplink the input switch is the eNB.
      uint16_t swIdx = rInfo->GetEnbSwIdx ();

      // Building the match string.
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_src=" << rInfo->GetEnbAddr ()
            << ",ip_dst=" << rInfo->GetSgwAddr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // Check for meter entry.
      if (meterInfo && meterInfo->HasUp ())
        {
          if (!meterInfo->IsInstalled ())
            {
              // Install the per-flow meter entry.
              DpctlExecute (GetDatapathId (swIdx), meterInfo->GetUpAddCmd ());
              meterInstalled = true;
            }

          // Building the per-flow meter instruction string.
          act << " meter:" << rInfo->GetTeid ();
        }

      // For GBR bearers, mark the IP DSCP field.
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
      std::string commandStr = cmd.str () + match.str () + act.str ();
      DpctlExecute (GetDatapathId (swIdx), commandStr);
    }

  // Updating meter installation flag.
  if (meterInstalled)
    {
      meterInfo->SetInstalled (true);
    }

  rInfo->SetInstalled (true);
  NS_LOG_INFO ("Topology routing installed for bearer " << rInfo->GetTeid ());
  return true;
}

bool
RingController::TopologyRemoveRouting (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // We will only remove meter entries from switch. This will automatically
  // remove referring flow rules. The other rules will expired due idle
  // timeout.
  return RemoveMeterRules (rInfo);
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // Reseting ring routing info to the shortest path.
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  ringInfo->ResetToDefaultPaths ();

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
            NS_LOG_INFO ("Routing bearer " << teid << " over shortest path.");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else
          {
            NS_LOG_WARN ("No resources for bearer " << teid << ". Block!");
            return false;
          }
        break;
      }
    case RingController::SPF:
      {
        if (dlShortBw >= dlRequest && ulShortBw >= ulRequest)
          {
            NS_LOG_INFO ("Routing bearer " << teid << " over shortest path.");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else if (dlLongBw >= dlRequest && ulLongBw >= ulRequest)
          {
            // Let's invert the path and reserve the bit rate
            ringInfo->InvertBothPaths ();
            NS_LOG_INFO ("Routing bearer " << teid << " over longest path.");
            return ReserveGbrBitRate (ringInfo, gbrInfo);
          }
        else
          {
            NS_LOG_WARN ("No resources for bearer " << teid << ". Block!");
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
  NS_LOG_FUNCTION (this << rInfo);

  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo && gbrInfo->IsReserved ())
    {
      Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");
      NS_LOG_INFO ("Releasing resources for bearer " << rInfo->GetTeid ());
      ReleaseGbrBitRate (ringInfo, gbrInfo);
    }
  return true;
}

void
RingController::TopologyCreateSpanningTree ()
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
  macAddr2 = Mac48Address::ConvertFrom (cInfo->GetPortDev (1)->GetAddress ());

  cmd1 << "port-mod port=" << cInfo->GetPortNo (0)
       << ",addr=" <<  macAddr1
       << ",conf=0x00000020,mask=0x00000020";

  cmd2 << "port-mod port=" << cInfo->GetPortNo (1)
       << ",addr=" << macAddr2
       << ",conf=0x00000020,mask=0x00000020";

  DpctlSchedule (cInfo->GetSwDpId (0), cmd1.str ());
  DpctlSchedule (cInfo->GetSwDpId (1), cmd2.str ());
}

uint16_t
RingController::GetNSwitches (void) const
{
  return m_noSwitches;
}

Ptr<RingRoutingInfo>
RingController::GetRingRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo == 0)
    {
      // This is the first time in simulation we are querying ring information
      // for this bearer. Let's create and aggregate its ring routing metadata.
      ringInfo = CreateObject<RingRoutingInfo> (rInfo);
      rInfo->AggregateObject (ringInfo);

      // Considering default paths those with lower hops.
      RingRoutingInfo::RoutingPath dlPath, ulPath;
      dlPath = FindShortestPath (rInfo->GetSgwSwIdx (), rInfo->GetEnbSwIdx ());
      ulPath = FindShortestPath (rInfo->GetEnbSwIdx (), rInfo->GetSgwSwIdx ());
      ringInfo->SetDefaultPaths (dlPath, ulPath);
    }
  return ringInfo;
}

void
RingController::SaveConnectionInfo (Ptr<ConnectionInfo> cInfo)
{
  // Respecting the increasing switch index order when saving connection data.
  uint16_t swIndex1 = cInfo->GetSwIdx (0);
  uint16_t swIndex2 = cInfo->GetSwIdx (1);
  uint16_t port1 = cInfo->GetPortNo (0);
  uint16_t port2 = cInfo->GetPortNo (1);

  SwitchPair_t key;
  key.first  = std::min (swIndex1, swIndex2);
  key.second = std::max (swIndex1, swIndex2);
  std::pair<SwitchPair_t, Ptr<ConnectionInfo> > entry (key, cInfo);
  std::pair<ConnInfoMap_t::iterator, bool> ret;
  ret = m_connections.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New connection info saved:"
                    << " switch " << swIndex1 << " port " << port1
                    << " switch " << swIndex2 << " port " << port2);
      return;
    }
  NS_FATAL_ERROR ("Error saving connection info.");
}

Ptr<ConnectionInfo>
RingController::GetConnectionInfo (uint16_t sw1, uint16_t sw2)
{
  // Respecting the increasing switch index order when getting connection data.
  SwitchPair_t key;
  key.first = std::min (sw1, sw2);
  key.second = std::max (sw1, sw2);
  ConnInfoMap_t::iterator it = m_connections.find (key);
  if (it != m_connections.end ())
    {
      return it->second;
    }
  NS_FATAL_ERROR ("No connection information available.");
}

RingRoutingInfo::RoutingPath
RingController::FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) < GetNSwitches ());

  // Check for local routing.
  if (srcSwitchIdx == dstSwitchIdx)
    {
      return RingRoutingInfo::LOCAL;
    }

  // Identify the shortest routing path from src to dst switch index.
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstSwitchIdx - srcSwitchIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  return (clockwiseDistance <= maxHops) ?
         RingRoutingInfo::CLOCK :
         RingRoutingInfo::COUNTER;
}

uint16_t
RingController::HopCounter (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                            RingRoutingInfo::RoutingPath routingPath)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) < GetNSwitches ());

  // Check for local routing.
  if (routingPath == RingRoutingInfo::LOCAL)
    {
      NS_ASSERT (srcSwitchIdx == dstSwitchIdx);
      return 0;
    }

  // Count the number of hops from src to dst switch index.
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  int distance = dstSwitchIdx - srcSwitchIdx;
  if (routingPath == RingRoutingInfo::COUNTER)
    {
      distance = srcSwitchIdx - dstSwitchIdx;
    }
  if (distance < 0)
    {
      distance += GetNSwitches ();
    }
  return distance;
}

std::pair<uint64_t, uint64_t>
RingController::GetAvailableGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                        bool useShortPath)
{
  NS_LOG_FUNCTION (this << ringInfo << useShortPath);

  uint16_t sgwIdx      = ringInfo->GetSgwSwIdx ();
  uint16_t enbIdx      = ringInfo->GetEnbSwIdx ();
  uint64_t downBitRate = std::numeric_limits<uint64_t>::max ();
  uint64_t upBitRate   = std::numeric_limits<uint64_t>::max ();
  uint64_t bitRate     = 0;
  uint16_t current     = enbIdx;
  double   debarFactor = 1.0;

  RingRoutingInfo::RoutingPath upPath = FindShortestPath (enbIdx, sgwIdx);
  if (!useShortPath)
    {
      upPath = RingRoutingInfo::InvertPath (upPath);
    }

  // From the eNB to the gateway switch index, get the bit rate for each link.
  while (current != sgwIdx)
    {
      uint16_t next = NextSwitchIndex (current, upPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);

      // Check for available bit rate in uplink direction.
      bitRate = cInfo->GetAvailableGbrBitRate (current, next, debarFactor);
      if (bitRate < upBitRate)
        {
          upBitRate = bitRate;
        }

      // Check for available bit rate in downlink direction.
      bitRate = cInfo->GetAvailableGbrBitRate (next, current, debarFactor);
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

  NS_LOG_INFO ("Reserving resources for GBR bearer " << ringInfo->GetTeid ());
  PerLinkReserve (ringInfo->GetSgwSwIdx (), ringInfo->GetEnbSwIdx (),
                  ringInfo->GetDownPath (), gbrInfo->GetDownBitRate ());
  PerLinkReserve (ringInfo->GetEnbSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetUpPath (), gbrInfo->GetUpBitRate ());
  gbrInfo->SetReserved (true);
  return true;
}

bool
RingController::ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Releasing resources for GBR bearer " << ringInfo->GetTeid ());
  PerLinkRelease (ringInfo->GetSgwSwIdx (), ringInfo->GetEnbSwIdx (),
                  ringInfo->GetDownPath (), gbrInfo->GetDownBitRate ());
  PerLinkRelease (ringInfo->GetEnbSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetUpPath (), gbrInfo->GetUpBitRate ());
  gbrInfo->SetReserved (false);
  return true;
}

bool
RingController::PerLinkReserve (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                RingRoutingInfo::RoutingPath routingPath,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx
                        << routingPath << bitRate);

  bool success = true;
  uint16_t current = srcSwitchIdx;
  while (success && current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      success = cInfo->ReserveGbrBitRate (current, next, bitRate);
      current = next;
    }

  NS_ASSERT_MSG (success, "Error when reserving resources.");
  return success;
}

bool
RingController::PerLinkRelease (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                RingRoutingInfo::RoutingPath routingPath,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx
                        << routingPath << bitRate);

  bool success = true;
  uint16_t current = srcSwitchIdx;
  while (success && current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      success = cInfo->ReleaseGbrBitRate (current, next, bitRate);
      current = next;
    }

  NS_ASSERT_MSG (success, "Error when reserving resources.");
  return success;
}

uint16_t
RingController::NextSwitchIndex (uint16_t current,
                                 RingRoutingInfo::RoutingPath routingPath)
{
  NS_LOG_FUNCTION (this << current << routingPath);

  NS_ASSERT_MSG (routingPath != RingRoutingInfo::LOCAL,
                 "Not supposed to get here for local routing.");

  return routingPath == RingRoutingInfo::CLOCK ?
         (current + 1) % GetNSwitches () :
         (current == 0 ? GetNSwitches () - 1 : (current - 1));
}

bool
RingController::RemoveMeterRules (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->IsActive () && !rInfo->IsInstalled (),
                 "Can't delete meter for valid traffic.");

  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->IsInstalled ())
    {
      NS_LOG_DEBUG ("Removing meter entries.");
      if (meterInfo->HasDown ())
        {
          DpctlExecute (GetDatapathId (rInfo->GetSgwSwIdx ()),
                        meterInfo->GetDelCmd ());
        }
      if (meterInfo->HasUp ())
        {
          DpctlExecute (GetDatapathId (rInfo->GetEnbSwIdx ()),
                        meterInfo->GetDelCmd ());
        }
      meterInfo->SetInstalled (false);
    }
  return true;
}

};  // namespace ns3
