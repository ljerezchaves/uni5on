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
                   EnumValue (RingController::SPF),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::SPO, "spo",
                                    RingController::SPF, "spf"))
  ;
  return tid;
}

std::string
RingController::RoutingStrategyStr (RoutingStrategy strategy)
{
  switch (strategy)
    {
    case RingController::SPO:
      return "SPO";
    case RingController::SPF:
      return "SPF";
    default:
      return "-";
    }
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
      // metadata field. Send the packet to slicing table.
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

void
RingController::NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  // Installing groups and meters for ring network. Note that following
  // commands works as connections are created in clockwise direction, and
  // switches inside cInfo are saved in the same direction.
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

  if (GetSlicingMode () == OperationMode::ON)
    {
      // Connecting this controller to ConnectionInfo trace source
      // when the network slicing mechanism is enable.
      cInfo->TraceConnectWithoutContext (
        "MeterAdjusted", MakeCallback (
          &RingController::MeterAdjusted, this));

      // Meter flags OFPMF_KBPS.
      std::string flagsStr ("0x0001");

      NS_LOG_DEBUG ("Creating meter for connection info " <<
                    cInfo->GetSwDpId (0) << " to " << cInfo->GetSwDpId (1));

      // Non-GBR meter for clockwise direction.
      kbps = cInfo->GetMeterBitRate (ConnectionInfo::FWD) / 1000;
      cmd02 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::CLOCK
            << " drop:rate=" << kbps;
      DpctlSchedule (cInfo->GetSwDpId (0), cmd02.str ());
      NS_LOG_DEBUG ("Forward link set to " << kbps << " Kbps");


      // Non-GBR meter for counterclockwise direction.
      kbps = cInfo->GetMeterBitRate (ConnectionInfo::BWD) / 1000;
      cmd12 << "meter-mod cmd=add"
            << ",flags=" << flagsStr
            << ",meter=" << RingRoutingInfo::COUNTER
            << " drop:rate=" << kbps;
      DpctlSchedule (cInfo->GetSwDpId (1), cmd12.str ());
      NS_LOG_DEBUG ("Backward link set to " << kbps << " Kbps");
    }
}

void
RingController::TopologyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  // Let's create its ring routing metadata.
  Ptr<RingRoutingInfo> ringInfo = CreateObject<RingRoutingInfo> (rInfo);

  // Set internal switch indexes.
  ringInfo->SetPgwSwIdx  (GetSwitchIndex (rInfo->GetPgwS5Addr ()));
  ringInfo->SetSgwSwIdx  (GetSwitchIndex (rInfo->GetSgwS5Addr ()));
  ringInfo->SetPgwSwDpId (GetDpId (ringInfo->GetPgwSwIdx ()));
  ringInfo->SetSgwSwDpId (GetDpId (ringInfo->GetSgwSwIdx ()));

  // Set as default path the one with lower hops.
  ringInfo->SetDefaultPath (
    FindShortestPath (ringInfo->GetPgwSwIdx (), ringInfo->GetSgwSwIdx ()));
}

bool
RingController::TopologyBearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo && gbrInfo->IsReserved ())
    {
      Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");
      NS_LOG_INFO ("Releasing resources for bearer " << rInfo->GetTeid ());
      ReleaseGbrBitRate (ringInfo, gbrInfo, rInfo->GetSlice ());
    }
  return true;
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  // If the bearer is already blocked, there's nothing more to do.
  if (rInfo->IsBlocked ())
    {
      return false;
    }

  // Reset the ring routing info to the shortest path.
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  ringInfo->ResetPath ();

  // For Non-GBR bearers (which includes the default bearer), and for bearers
  // that only transverse local switch (local routing): let's accept it without
  // guarantees. Note that in current implementation, these bearers are always
  // routed over the shortest path.
  if (!rInfo->IsGbr () || ringInfo->IsLocalPath ())
    {
      return true;
    }

  // It only makes sense to check and reserve bandwidth for GBR bearers.
  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  NS_ASSERT_MSG (gbrInfo, "Invalid configuration for GBR bearer request.");

  // Check for the requested bit rate over the shortest path.
  if (HasGbrBitRate (ringInfo, gbrInfo, rInfo->GetSlice ()))
    {
      NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeid () <<
                   " over the shortest path");
      return ReserveGbrBitRate (ringInfo, gbrInfo, rInfo->GetSlice ());
    }

  // The requested bit rate is not available over the shortest path. When
  // using the SPF routing strategy, invert the routing path and check for the
  // requested bit rate over the longest path.
  if (m_strategy == RingController::SPF)
    {
      ringInfo->InvertPath ();
      if (HasGbrBitRate (ringInfo, gbrInfo, rInfo->GetSlice ()))
        {
          NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeid () <<
                       " over the longest (inverted) path");
          return ReserveGbrBitRate (ringInfo, gbrInfo, rInfo->GetSlice ());
        }
    }

  // Nothing more to do. Block the traffic.
  NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid ());
  rInfo->SetBlocked (true, RoutingInfo::BANDWIDTH);
  return false;
}

double
RingController::TopologyLinkUsage (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid ());

  // Update the aggregation metadata with link bandwidth usage.
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  uint16_t pgwIdx = ringInfo->GetPgwSwIdx ();
  uint16_t sgwIdx = ringInfo->GetSgwSwIdx ();

  // FIXME Should we consider only the current traffic slice?
  return std::max (GetPathUseRatio (pgwIdx, sgwIdx, ringInfo->GetDownPath ()),
                   GetPathUseRatio (sgwIdx, pgwIdx, ringInfo->GetUpPath ()));
}

bool
RingController::TopologyRoutingInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Installing ring rules for bearer teid " << rInfo->GetTeid ());

  // Getting ring routing information.
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();

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

      // Set the IP DSCP field when necessary.
      std::ostringstream act;
      if (rInfo->GetDscp ())
        {
          // Build the apply set_field action instruction string.
          act << " apply:set_field=ip_dscp:" << rInfo->GetDscp ();
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

      // Set the IP DSCP field when necessary.
      std::ostringstream act;
      if (rInfo->GetDscp ())
        {
          // Build the apply set_field action instruction string.
          act << " apply:set_field=ip_dscp:" << rInfo->GetDscp ();
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
RingController::TopologyRoutingRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Removing ring rules for bearer teid " << rInfo->GetTeid ());

  // Print the cookie value in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Getting ring routing information.
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();

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

  std::ostringstream cmd1, cmd2;
  cmd1 << "port-mod port=" << cInfo->GetPortNo (0)
       << ",addr=" <<  cInfo->GetPortMacAddr (0)
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (cInfo->GetSwDpId (0), cmd1.str ());

  cmd2 << "port-mod port=" << cInfo->GetPortNo (1)
       << ",addr=" << cInfo->GetPortMacAddr (1)
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (cInfo->GetSwDpId (1), cmd2.str ());
}

RingRoutingInfo::RoutingPath
RingController::FindShortestPath (uint16_t srcIdx, uint16_t dstIdx) const
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

Ptr<ConnectionInfo>
RingController::GetConnectionInfo (uint16_t idx1, uint16_t idx2) const
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  return ConnectionInfo::GetPointer (GetDpId (idx1), GetDpId (idx2));
}

uint64_t
RingController::GetDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_ofDevices.GetN (), "Invalid switch index.");
  return m_ofDevices.Get (idx)->GetDatapathId ();
}

uint16_t
RingController::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ofDevices.GetN ();
}

double
RingController::GetPathUseRatio (uint16_t srcIdx, uint16_t dstIdx,
                                 RingRoutingInfo::RoutingPath path) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path);

  uint64_t useBitRate = 0;
  uint64_t maxBitRate = std::numeric_limits<uint64_t>::max ();

  uint16_t next;
  DataRate bitRate;
  Ptr<ConnectionInfo> cInfo;
  while (srcIdx != dstIdx)
    {
      next = NextSwitchIndex (srcIdx, path);
      cInfo = GetConnectionInfo (srcIdx, next);

      // FIXME Essa função olha pro uso do enlace pra decidir se o tráfego pode
      // ou não agregar. Ela deveria olhar para o enlace como um todo? ou
      // apenas para o slice ao qual o tráfego pertence? A função GetEwmaThp
      // pode aceitar como último parametro o slice.
      bitRate = cInfo->GetEwmaThp (GetDpId (srcIdx), GetDpId (next));
      useBitRate = std::max (useBitRate, bitRate.GetBitRate ());
      maxBitRate = std::min (maxBitRate, cInfo->GetLinkBitRate ());
      srcIdx = next;
    }
  return static_cast<double> (useBitRate) / maxBitRate;
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

bool
RingController::HasGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                               Ptr<const GbrInfo> gbrInfo, Slice slice) const
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo << slice);

  uint16_t next;
  uint64_t currId;
  uint64_t nextId;
  bool success = true;
  Ptr<ConnectionInfo> cInfo;
  uint16_t curr = ringInfo->GetPgwSwIdx ();
  while (success && curr != ringInfo->GetSgwSwIdx ())
    {
      next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
      currId = GetDpId (curr);
      nextId = GetDpId (next);

      cInfo = GetConnectionInfo (curr, next);
      success &= cInfo->HasBitRate (currId, nextId, slice,
                                    gbrInfo->GetDownBitRate ());
      success &= cInfo->HasBitRate (nextId, currId, slice,
                                    gbrInfo->GetUpBitRate ());
      curr = next;
    }
  return success;
}

uint16_t
RingController::HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                            RingRoutingInfo::RoutingPath path) const
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

void
RingController::MeterAdjusted (Ptr<const ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  std::ostringstream cmd1, cmd2;
  uint64_t kbps = 0;

  // Meter flags OFPMF_KBPS.
  std::string flagsStr ("0x0001");

  NS_LOG_DEBUG ("Updating meter for connection info " <<
                cInfo->GetSwDpId (0) << " to " << cInfo->GetSwDpId (1));

  // Update the meter for clockwise direction.
  kbps = cInfo->GetMeterBitRate (ConnectionInfo::FWD) / 1000;
  cmd1 << "meter-mod cmd=mod"
       << ",flags=" << flagsStr
       << ",meter=" << RingRoutingInfo::CLOCK
       << " drop:rate=" << kbps;
  DpctlExecute (cInfo->GetSwDpId (0), cmd1.str ());
  NS_LOG_DEBUG ("Forward link set to " << kbps << " Kbps");

  // Update the meter for counterclockwise direction.
  kbps = cInfo->GetMeterBitRate (ConnectionInfo::BWD) / 1000;
  cmd2 << "meter-mod cmd=mod"
       << ",flags=" << flagsStr
       << ",meter=" << RingRoutingInfo::COUNTER
       << " drop:rate=" << kbps;
  DpctlExecute (cInfo->GetSwDpId (1), cmd2.str ());
  NS_LOG_DEBUG ("Backward link set to " << kbps << " Kbps");
}

uint16_t
RingController::NextSwitchIndex (uint16_t idx,
                                 RingRoutingInfo::RoutingPath path) const
{
  NS_LOG_FUNCTION (this << idx << path);

  NS_ASSERT_MSG (path != RingRoutingInfo::LOCAL,
                 "Not supposed to get here for local routing.");

  return path == RingRoutingInfo::CLOCK ?
         (idx + 1) % GetNSwitches () :
         (idx == 0 ? GetNSwitches () - 1 : (idx - 1));
}

bool
RingController::ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<GbrInfo> gbrInfo, Slice slice)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Releasing resources for GBR bearer.");

  uint16_t next;
  uint64_t currId;
  uint64_t nextId;
  bool success = true;
  Ptr<ConnectionInfo> cInfo;
  uint16_t curr = ringInfo->GetPgwSwIdx ();
  while (success && curr != ringInfo->GetSgwSwIdx ())
    {
      next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
      currId = GetDpId (curr);
      nextId = GetDpId (next);

      cInfo = GetConnectionInfo (curr, next);
      success &= cInfo->ReleaseBitRate (currId, nextId, slice,
                                        gbrInfo->GetDownBitRate ());
      success &= cInfo->ReleaseBitRate (nextId, currId, slice,
                                        gbrInfo->GetUpBitRate ());
      curr = next;
    }
  NS_ASSERT_MSG (success, "Error when releasing resources.");
  gbrInfo->SetReserved (!success);
  return success;
}

bool
RingController::ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<GbrInfo> gbrInfo, Slice slice)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Reserving resources for GBR bearer.");

  uint16_t next;
  uint64_t currId;
  uint64_t nextId;
  bool success = true;
  Ptr<ConnectionInfo> cInfo;
  uint16_t curr = ringInfo->GetPgwSwIdx ();
  while (success && curr != ringInfo->GetSgwSwIdx ())
    {
      next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
      currId = GetDpId (curr);
      nextId = GetDpId (next);

      cInfo = GetConnectionInfo (curr, next);
      success &= cInfo->ReserveBitRate (currId, nextId, slice,
                                        gbrInfo->GetDownBitRate ());
      success &= cInfo->ReserveBitRate (nextId, currId, slice,
                                        gbrInfo->GetUpBitRate ());
      curr = next;
    }
  NS_ASSERT_MSG (success, "Error when reserving resources.");
  gbrInfo->SetReserved (success);
  return success;
}

};  // namespace ns3
