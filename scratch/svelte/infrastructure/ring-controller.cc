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
#include "../metadata/gbr-info.h"
#include "../metadata/meter-info.h"
#include "../metadata/routing-info.h"
#include "backhaul-network.h"

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
    .SetParent<BackhaulController> ()
    .AddConstructor<RingController> ()
    .AddAttribute ("Strategy", "The ring routing strategy.",
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
      return "shortest path only";
    case RingController::SPF:
      return "shortest path first";
    default:
      return "-";
    }
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  BackhaulController::DoDispose ();
}

void
RingController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  BackhaulController::NotifyConstructionCompleted ();
}

bool
RingController::BearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // If the bearer is already blocked, there's nothing more to do.
  if (rInfo->IsBlocked ())
    {
      return false;
    }

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // Reset the ring routing info to the shortest path.
  ringInfo->ResetToDefaults ();

  // For Non-GBR bearers (which includes the default bearer) and for bearers
  // that only transverse local switch (local routing for both S1-U and S5
  // interfaces): let's accept it without guarantees. Note that in current
  // implementation, these bearers are always routed over the shortest path.
  if (!rInfo->IsGbr () || ringInfo->IsLocalPath ())
    {
      return true;
    }

  // It only makes sense to check for available bandwidth for GBR bearers.
  Ptr<GbrInfo> gbrInfo = rInfo->GetGbrInfo ();
  NS_ASSERT_MSG (gbrInfo, "Invalid configuration for GBR bearer request.");

  // Check for the requested bit rate over the shortest path.
  if (HasBitRate (ringInfo, gbrInfo))
    {
      NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                   " over the shortest path");
      return BitRateReserve (ringInfo, gbrInfo);
    }

  // The requested bit rate is not available over the shortest path. When
  // using the SPF routing strategy, invert the routing path and check for the
  // requested bit rate over the longest path.
  if (m_strategy == RingController::SPF)
    {
      // FIXME Qual interface inverter?
      ringInfo->InvertPath (LteIface::S5);
      if (HasBitRate (ringInfo, gbrInfo))
        {
          NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                       " over the longest (inverted) path");
          return BitRateReserve (ringInfo, gbrInfo);
        }
    }

  // Nothing more to do. Block the traffic.
  NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex ());
  BlockBearer (rInfo, RoutingInfo::NOBANDWIDTH);
  return false;
}

bool
RingController::BearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // For bearers without reserved resources: nothing to release.
  Ptr<GbrInfo> gbrInfo = rInfo->GetGbrInfo ();
  if (!gbrInfo || !gbrInfo->IsReserved ())
    {
      return true;
    }

  return BitRateRelease (rInfo->GetObject<RingInfo> (), gbrInfo);
}

void
RingController::NotifyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // Let's create its ring routing metadata.
  Ptr<RingInfo> ringInfo = CreateObject<RingInfo> (rInfo);

  // Set default paths to those with lower hops.
  RingInfo::RingPath s5DownPath = FindShortestPath (
      rInfo->GetPgwInfraSwIdx (), rInfo->GetSgwInfraSwIdx ());
  ringInfo->SetDefaultPath (s5DownPath, LteIface::S5);

  RingInfo::RingPath s1uDownPath = FindShortestPath (
      rInfo->GetSgwInfraSwIdx (), rInfo->GetEnbInfraSwIdx ());
  ringInfo->SetDefaultPath (s1uDownPath, LteIface::S1U);

  NS_LOG_DEBUG ("Bearer teid " << rInfo->GetTeidHex () << " default downlink "
                "S1-U path to " << RingInfo::RingPathStr (s1uDownPath) <<
                " and S5 path to " << RingInfo::RingPathStr (s5DownPath));

  // Chain up.
  BackhaulController::NotifyBearerCreated (rInfo);
}

void
RingController::NotifyTopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // Save the collection of switch devices and create the spanning tree.
  m_switchDevices = devices;
  CreateSpanningTree ();

  // NOTE that following commands works as LINKS ARE CREATED IN CLOCKWISE
  // DIRECTION, and switches inside each lInfo are saved in the same order.

  for (uint16_t swIdx = 0; swIdx < GetNSwitches (); swIdx++)
    {
      uint16_t nextIdx = NextSwitchIndex (swIdx, RingInfo::CLOCK);
      Ptr<LinkInfo> lInfo = GetLinkInfo (swIdx, nextIdx);

      // ---------------------------------------------------------------------
      // Meter table
      //
      // Installing meters entries when the link slicing mechanism is enabled.
      if (GetLinkSlicingMode () != OpMode::OFF)
        {
          SlicingMeterInstall (lInfo);

          // Connect this controller to LinkInfo meter adjusted trace source.
          lInfo->TraceConnectWithoutContext (
            "MeterAdjusted", MakeCallback (
              &RingController::SlicingMeterAdjusted, this));
        }

      // ---------------------------------------------------------------------
      // Group table
      //
      // Configure groups to keep forwarding packets in the ring until they
      // reach the destination switch.
      //
      // Routing group for clockwise packet forwarding.
      std::ostringstream cmd1;
      cmd1 << "group-mod cmd=add,type=ind,group=" << RingInfo::CLOCK
           << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (0);
      DpctlSchedule (lInfo->GetSwDpId (0), cmd1.str ());

      // Routing group for counterclockwise packet forwarding.
      std::ostringstream cmd2;
      cmd2 << "group-mod cmd=add,type=ind,group=" << RingInfo::COUNTER
           << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (1);
      DpctlSchedule (lInfo->GetSwDpId (1), cmd2.str ());
    }

  // Do not merge the following loop with the previous one. Groups must be
  // created first to avoid OpenFlow error messages with BAD_OUT_GROUP code.
  for (uint16_t swIdx = 0; swIdx < GetNSwitches (); swIdx++)
    {
      uint16_t nextIdx = NextSwitchIndex (swIdx, RingInfo::CLOCK);
      Ptr<LinkInfo> lInfo = GetLinkInfo (swIdx, nextIdx);

      // ---------------------------------------------------------------------
      // Table 2 -- Routing table -- [from higher to lower priority]
      //
      // GTP packets being forwarded by this switch. Write the output group
      // into action set based on input port. Write the same group number into
      // metadata field. Send the packet to slicing table.
      //
      // Clockwise packet forwarding.
      std::ostringstream cmd3;
      cmd3 << "flow-mod cmd=add,table=2,prio=128,flags="
           << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
           << " meta=0x0,in_port=" << lInfo->GetPortNo (0)
           << " write:group=" << RingInfo::COUNTER
           << " meta:" << RingInfo::COUNTER
           << " goto:3";
      DpctlSchedule (lInfo->GetSwDpId (0), cmd3.str ());

      // Counterclockwise packet forwarding.
      std::ostringstream cmd4;
      cmd4 << "flow-mod cmd=add,table=2,prio=128,flags="
           << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
           << " meta=0x0,in_port=" << lInfo->GetPortNo (1)
           << " write:group=" << RingInfo::CLOCK
           << " meta:" << RingInfo::CLOCK
           << " goto:3";
      DpctlSchedule (lInfo->GetSwDpId (1), cmd4.str ());
    }
}

bool
RingController::TopologyRoutingInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Installing ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Building the dpctl command + arguments string.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=1,flags="
      << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  std::ostringstream dscp;
  if (rInfo->GetDscpValue ())
    {
      dscp << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
    }

  // Configuring downlink routing.
  if (rInfo->HasDownlinkTraffic ())
    {
      // Building the match string for both S1-U and S5 interfaces
      // No match on source IP because we may have several P-GW TFT switches.
      std::ostringstream mS5, mS1u;
      mS5 << " eth_type=0x800,ip_proto=17"
          << ",ip_dst=" << rInfo->GetSgwS5Addr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();
      mS1u << " eth_type=0x800,ip_proto=17"
           << ",ip_dst=" << rInfo->GetEnbS1uAddr ()
           << ",gtpu_teid=" << rInfo->GetTeidHex ();

      // Build the metatada and goto instructions string.
      std::ostringstream actS5, actS1u;
      actS5 << " meta:" << ringInfo->GetDownPath (LteIface::S5) << " goto:2";
      actS1u << " meta:" << ringInfo->GetDownPath (LteIface::S1U) << " goto:2";

      // Installing down rules into switches connected to the P-GW and S-GW.
      DpctlExecute (GetDpId (ringInfo->GetPgwInfraSwIdx ()),
                    cmd.str () + mS5.str () + dscp.str () + actS5.str ());
      DpctlExecute (GetDpId (ringInfo->GetSgwInfraSwIdx ()),
                    cmd.str () + mS1u.str () + dscp.str () + actS1u.str ());
    }

  // Configuring uplink routing.
  if (rInfo->HasUplinkTraffic ())
    {
      // Building the match string.
      std::ostringstream mS1u, mS5;
      mS1u << " eth_type=0x800,ip_proto=17"
           << ",ip_src=" << rInfo->GetEnbS1uAddr ()
           << ",ip_dst=" << rInfo->GetSgwS1uAddr ()
           << ",gtpu_teid=" << rInfo->GetTeidHex ();
      mS5 << " eth_type=0x800,ip_proto=17"
          << ",ip_src=" << rInfo->GetSgwS5Addr ()
          << ",ip_dst=" << rInfo->GetPgwS5Addr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();

      // Build the metatada and goto instructions string.
      std::ostringstream actS1u, actS5;
      actS1u << " meta:" << ringInfo->GetUpPath (LteIface::S1U) << " goto:2";
      actS5 << " meta:" << ringInfo->GetUpPath (LteIface::S5) << " goto:2";

      // Installing up rules into switches connected to the eNB and S-GW.
      DpctlExecute (GetDpId (ringInfo->GetEnbInfraSwIdx ()),
                    cmd.str () + mS1u.str () + dscp.str () + actS1u.str ());
      DpctlExecute (GetDpId (ringInfo->GetSgwInfraSwIdx ()),
                    cmd.str () + mS5.str () + dscp.str () + actS5.str ());
    }
  return true;
}

bool
RingController::TopologyRoutingRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Removing ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,table=1"
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",cookie_mask=" << COOKIE_STRICT_MASK;

  // Removing rules from switches connected to the eNB, S-GW and P-GW.
  DpctlExecute (GetDpId (ringInfo->GetPgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (ringInfo->GetSgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (ringInfo->GetEnbInfraSwIdx ()), cmd.str ());

  return true;
}

void
RingController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // -------------------------------------------------------------------------
  // Table 3 -- Slicing table -- [from higher to lower priority]
  //
  if (GetLinkSlicingMode () == OpMode::ON)
    {
      // FIXME This should be automatic depending on the number of slices.

      // DFT Non-GBR packets are filtered by DSCP fields DSCP_AF11 and
      // DSCP_BE. Apply Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF11 (DSCP decimal 10)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x1,ip_dscp=10"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x2,ip_dscp=10"
                    " meter:2 goto:4");

      // DSCP_BE (DSCP decimal 0)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x1,ip_dscp=0"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x2,ip_dscp=0"
                    " meter:2 goto:4");

      // MTC Non-GBR packets are filtered by DSCP field DSCP_AF31.
      // Apply MTC Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF31 (DSCP decimal 26)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x1,ip_dscp=26"
                    " meter:3 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x2,ip_dscp=26"
                    " meter:4 goto:4");
    }
  else if (GetLinkSlicingMode () == OpMode::AUTO)
    {
      // When the network slicing operation mode is AUTO, the Non-GBR traffic
      // of all slices will be monitored together. Here is how we are using
      // meter IDs:
      // Meter ID 1 -> clockwise FWD direction
      // Meter ID 2 -> counterclockwise BWD direction

      // Non-GBR packets are filtered by DSCP fields DSCP_AF31, DSCP_AF11, and
      // DSCP_BE. Apply Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF31 (DSCP decimal 26)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x1,ip_dscp=26"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x2,ip_dscp=26"
                    " meter:2 goto:4");

      // DSCP_AF11 (DSCP decimal 10)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x1,ip_dscp=10"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x2,ip_dscp=10"
                    " meter:2 goto:4");

      // DSCP_BE (DSCP decimal 0)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x1,ip_dscp=0"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x2,ip_dscp=0"
                    " meter:2 goto:4");
    }



  // Chain up.
  BackhaulController::HandshakeSuccessful (swtch);
}

bool
RingController::HasBitRate (Ptr<const RingInfo> ringInfo,
                            Ptr<const GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  SliceId slice = ringInfo->GetSliceId ();
  uint64_t dlRate = gbrInfo->GetDownBitRate ();
  uint64_t ulRate = gbrInfo->GetUpBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = ringInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDownPath (LteIface::S5);
  while (success && curr != ringInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->HasBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->HasBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDownPath (LteIface::S1U);
  while (success && curr != ringInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->HasBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->HasBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  return success;
}

bool
RingController::BitRateReserve (Ptr<const RingInfo> ringInfo,
                                Ptr<GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Reserving resources for teid " << ringInfo->GetTeidHex ());

  SliceId slice = ringInfo->GetSliceId ();
  uint64_t dlRate = gbrInfo->GetDownBitRate ();
  uint64_t ulRate = gbrInfo->GetUpBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = ringInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDownPath (LteIface::S5);
  while (success && curr != ringInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->ReserveBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->ReserveBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDownPath (LteIface::S1U);
  while (success && curr != ringInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->ReserveBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->ReserveBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  NS_ASSERT_MSG (success, "Error when reserving resources.");
  gbrInfo->SetReserved (success);
  return success;
}

bool
RingController::BitRateRelease (Ptr<const RingInfo> ringInfo,
                                Ptr<GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  NS_LOG_INFO ("Releasing resources for teid " << ringInfo->GetTeidHex ());

  SliceId slice = ringInfo->GetSliceId ();
  uint64_t dlRate = gbrInfo->GetDownBitRate ();
  uint64_t ulRate = gbrInfo->GetUpBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = ringInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDownPath (LteIface::S5);
  while (success && curr != ringInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->ReleaseBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->ReleaseBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDownPath (LteIface::S1U);
  while (success && curr != ringInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->ReleaseBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->ReleaseBitRate (nextId, currId, slice, ulRate);
      curr = next;
    }

  NS_ASSERT_MSG (success, "Error when releasing resources.");
  gbrInfo->SetReserved (!success);
  return success;
}

void
RingController::CreateSpanningTree (void)
{
  NS_LOG_FUNCTION (this);

  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD) with OFPPC_NO_FWD config (0x20).
  uint16_t half = (GetNSwitches () / 2);
  Ptr<LinkInfo> lInfo = GetLinkInfo (half, half + 1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " <<
                half + 1 << " for broadcast messages.");

  std::ostringstream cmd1, cmd2;
  cmd1 << "port-mod port=" << lInfo->GetPortNo (0)
       << ",addr=" << lInfo->GetPortMacAddr (0)
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (lInfo->GetSwDpId (0), cmd1.str ());

  cmd2 << "port-mod port=" << lInfo->GetPortNo (1)
       << ",addr=" << lInfo->GetPortMacAddr (1)
       << ",conf=0x00000020,mask=0x00000020";
  DpctlSchedule (lInfo->GetSwDpId (1), cmd2.str ());
}

RingInfo::RingPath
RingController::FindShortestPath (uint16_t srcIdx, uint16_t dstIdx) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx);

  NS_ASSERT (std::max (srcIdx, dstIdx) < GetNSwitches ());

  // Check for local routing.
  if (srcIdx == dstIdx)
    {
      return RingInfo::LOCAL;
    }

  // Identify the shortest routing path from src to dst switch index.
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstIdx - srcIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  return (clockwiseDistance <= maxHops) ?
         RingInfo::CLOCK :
         RingInfo::COUNTER;
}

uint16_t
RingController::HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                            RingInfo::RingPath path) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx);

  NS_ASSERT (std::max (srcIdx, dstIdx) < GetNSwitches ());

  // Check for local routing.
  if (path == RingInfo::LOCAL)
    {
      NS_ASSERT (srcIdx == dstIdx);
      return 0;
    }

  // Count the number of hops from src to dst switch index.
  NS_ASSERT (srcIdx != dstIdx);
  int distance = dstIdx - srcIdx;
  if (path == RingInfo::COUNTER)
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
RingController::SlicingMeterAdjusted (Ptr<const LinkInfo> lInfo,
                                      LinkInfo::Direction dir, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice);

  NS_ASSERT_MSG (GetLinkSlicingMode () != OpMode::OFF, "Not supposed to "
                 "adjust slicing meters when network slicing mode is OFF.");

  // Identify the meter ID based on slicing operation mode.
  uint32_t meterId = 0;
  if (GetLinkSlicingMode () == OpMode::ON)
    {
      // Compose the meter ID.
      meterId = static_cast<uint32_t> (1);
      meterId <<= 24;
      meterId |= static_cast<uint32_t> (slice);
      meterId <<= 4;
      meterId |= static_cast<uint32_t> (dir);
    }
  else if (GetLinkSlicingMode () == OpMode::AUTO)
    {
      meterId = static_cast<uint32_t> (2);
      meterId <<= 28;
      meterId |= static_cast<uint32_t> (dir);
    }

  NS_LOG_INFO ("Updating slicing meter for link info " <<
               lInfo->GetSwDpId (0) << " - " << lInfo->GetSwDpId (1));

  // -------------------------------------------------------------------------
  // Meter table
  //
  // Update the proper slicing meter.
  uint64_t kbps = lInfo->GetFreeBitRate (dir, slice);
  std::ostringstream cmd;
  cmd << "meter-mod cmd=mod"
      << ",flags=" << OFPMF_KBPS
      << ",meter=" << meterId
      << " drop:rate=" << kbps / 1000;
  DpctlExecute (lInfo->GetSwDpId (static_cast<uint8_t> (dir)), cmd.str ());
  NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) <<
                ": " << LinkInfo::DirectionStr (dir) <<
                " link updated to " << kbps << " Kbps");
}

void
RingController::SlicingMeterInstall (Ptr<const LinkInfo> lInfo)
{
  NS_LOG_FUNCTION (this << lInfo);

  NS_LOG_DEBUG ("Creating slicing meters for link info " <<
                lInfo->GetSwDpId (0) << " - " << lInfo->GetSwDpId (1));

  if (GetLinkSlicingMode () == OpMode::ON)
    {
      // Install meter rules for each slice.
      for (int s = 0; s < SliceId::ALL; s++)
        {
          SliceId slice = static_cast<SliceId> (s);

          for (int d = 0; d <= LinkInfo::BWD; d++)
            {
              LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);

              // Compose the meter ID.
              uint32_t meterId = static_cast<uint32_t> (1);
              meterId <<= 24;
              meterId |= static_cast<uint32_t> (s);
              meterId <<= 4;
              meterId |= static_cast<uint32_t> (d);

              uint64_t kbps = lInfo->GetFreeBitRate (dir, slice);
              std::ostringstream cmd;
              cmd << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                  << ",meter=" << meterId
                  << " drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
              NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) <<
                            ": " << LinkInfo::DirectionStr (dir) <<
                            " link set to " << kbps << " Kbps");
            }
        }
    }
  else if (GetLinkSlicingMode () == OpMode::AUTO)
    {
      // Install meter rules shared among slices.
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);

          // Compose the meter ID.
          uint32_t meterId = static_cast<uint32_t> (2);
          meterId <<= 28;
          meterId |= static_cast<uint32_t> (d);

          uint64_t kbps = lInfo->GetFreeBitRate (dir, SliceId::ALL);
          std::ostringstream cmd;
          cmd << "meter-mod cmd=add,flags=" << OFPMF_KBPS
              << ",meter=" << meterId
              << " drop:rate=" << kbps / 1000;
          DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
          NS_LOG_DEBUG ("Link slice " << SliceIdStr (SliceId::ALL) <<
                        ": " << LinkInfo::DirectionStr (dir) <<
                        " link set to " << kbps << " Kbps");
        }
    }
}

uint16_t
RingController::NextSwitchIndex (uint16_t idx, RingInfo::RingPath path) const
{
  NS_LOG_FUNCTION (this << idx << path);

  NS_ASSERT_MSG (path != RingInfo::LOCAL,
                 "Not supposed to get here for local routing.");

  return path == RingInfo::CLOCK ?
         (idx + 1) % GetNSwitches () :
         (idx == 0 ? GetNSwitches () - 1 : (idx - 1));
}

} // namespace ns3
