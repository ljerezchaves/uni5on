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

  BackhaulController::DoDispose ();
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

  // FIXME Aqui tem que marcar a qual link slice que este tráfego pertence

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

      // Installing meters entries for each link in the ring network when the
      // link slicing mechanism is enabled.
      // ---------------------------------------------------------------------
      // Meter table
      //
      // FIXME Revisar o link slicing.
      if (GetSlicingMode () != OpMode::OFF)
        {
          NS_LOG_DEBUG ("Creating slicing meters for link info " <<
                        lInfo->GetSwDpId (0) << " - " << lInfo->GetSwDpId (1));

          // Connect this controller to LinkInfo meter adjusted trace source.
          lInfo->TraceConnectWithoutContext (
            "MeterAdjusted", MakeCallback (
              &RingController::MeterAdjusted, this));

          std::ostringstream cmdm1, cmdm2, cmdm3, cmdm4;
          uint64_t kbps = 0;

          if (GetSlicingMode () == OpMode::ON)
            {
              // DFT Non-GBR meter for clockwise FWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::FWD, LinkSlice::DFT);
              cmdm1 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=1 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (0), cmdm1.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::DFT) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::FWD) <<
                            " link set to " << kbps << " Kbps");

              // DFT Non-GBR meter for counterclockwise BWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::BWD, LinkSlice::DFT);
              cmdm2 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=2 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (1), cmdm2.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::DFT) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::BWD) <<
                            " link set to " << kbps << " Kbps");

              // M2M Non-GBR meter for clockwise FWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::FWD, LinkSlice::M2M);
              cmdm3 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=3 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (0), cmdm3.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::M2M) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::FWD) <<
                            " link set to " << kbps << " Kbps");

              // M2M Non-GBR meter for counterclockwise BWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::BWD, LinkSlice::M2M);
              cmdm4 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=4 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (1), cmdm4.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::M2M) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::BWD) <<
                            " link set to " << kbps << " Kbps");
            }
          else if (GetSlicingMode () == OpMode::AUTO)
            {
              // Non-GBR meter for clockwise FWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::FWD, LinkSlice::ALL);
              cmdm1 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=1 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (0), cmdm1.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::ALL) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::FWD) <<
                            " link set to " << kbps << " Kbps");

              // Non-GBR meter for counterclockwise BWD direction.
              kbps = lInfo->GetFreeBitRate (LinkInfo::BWD, LinkSlice::ALL);
              cmdm2 << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                    << ",meter=2 drop:rate=" << kbps / 1000;
              DpctlSchedule (lInfo->GetSwDpId (1), cmdm2.str ());
              NS_LOG_DEBUG ("Link slice " << LinkSliceStr (LinkSlice::ALL) <<
                            ": " << LinkInfo::DirectionStr (LinkInfo::BWD) <<
                            " link set to " << kbps << " Kbps");
            }
        }
    }
}

bool
RingController::TopologyRoutingInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Installing ring rules for bearer teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Building the dpctl command + arguments string.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=1,flags="
      << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Configuring downlink routing.
  if (rInfo->HasDownlinkTraffic ())
    {
      // Building the match string for both (S1-U and S5) interfaces
      // No match on source IP because we may have several P-GW TFT switches.
      std::ostringstream matchS5, matchS1u;
      matchS5 << " eth_type=0x800,ip_proto=17"
              << ",ip_dst=" << rInfo->GetSgwS5Addr ()
              << ",gtpu_teid=" << rInfo->GetTeidHex ();

      matchS1u << " eth_type=0x800,ip_proto=17"
               << ",ip_dst=" << rInfo->GetEnbS1uAddr ()
               << ",gtpu_teid=" << rInfo->GetTeidHex ();

      // Set the IP DSCP field when necessary.
      std::ostringstream act;
      if (rInfo->GetDscpValue ())
        {
          // Build the apply set_field action instruction string.
          act << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
        }

      // Build the metatada, write and goto instructions string.
      // FIXME Essas regras de act tem que ser diferentes tb.
      act << " meta:" << ringInfo->GetDownPath (LteIface::S5) << " goto:2";

      // Installing downlink rules into switch connected to P-GW and S-GW.
      DpctlExecute (GetDpId (ringInfo->GetPgwInfraSwIdx ()), cmd.str () + matchS5.str () + act.str ());
      DpctlExecute (GetDpId (ringInfo->GetSgwInfraSwIdx ()), cmd.str () + matchS1u.str () + act.str ());
    }
// FIXME
//   // Configuring uplink routing.
//   if (rInfo->HasUplinkTraffic ())
//     {
//       // Building the match string.
//       std::ostringstream match;
//       match << " eth_type=0x800,ip_proto=17"
//             << ",ip_src=" << rInfo->GetSgwS5Addr ()
//             << ",ip_dst=" << rInfo->GetPgwS5Addr ()
//             << ",gtpu_teid=" << rInfo->GetTeidHex ();
//
//       // Set the IP DSCP field when necessary.
//       std::ostringstream act;
//       if (rInfo->GetDscpValue ())
//         {
//           // Build the apply set_field action instruction string.
//           act << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
//         }
//
// FIXME Ver essa lógica que tá toda errada
//       // Build the metatada, write and goto instructions string.
//       sprintf (metadataStr, "0x%x", ringInfo->GetUpPath ());
//       act << " meta:" << ringInfo->GetUpPath (LteIface::S5) << " goto:2";
//
//       // Installing the rule into input switch.
//       // In uplink the input ring switch is the one connected to the S-GW.
//       std::string commandStr = cmd.str () + match.str () + act.str ();
//       DpctlExecute (ringInfo->GetSgwSwDpId (), commandStr);
//     }
  return true;
}

bool
RingController::TopologyRoutingRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Removing ring rules for bearer teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,table=1"
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",cookie_mask=" << COOKIE_STRICT_MASK;

  // FIXME Isso aqui vai dar erro? Desinstalar só de onde instalou.
  DpctlExecute (GetDpId (ringInfo->GetEnbInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (ringInfo->GetPgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (ringInfo->GetSgwInfraSwIdx ()), cmd.str ());

  return true;
}

bool
RingController::HasBitRate (Ptr<const RingInfo> ringInfo,
                            Ptr<const GbrInfo> gbrInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << gbrInfo);

  LinkSlice slice = ringInfo->GetLinkSlice ();
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

  NS_LOG_INFO ("Reserving resources for bearer " << ringInfo->GetTeidHex ());

  LinkSlice slice = ringInfo->GetLinkSlice ();
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

  NS_LOG_INFO ("Releasing resources for bearer " << ringInfo->GetTeidHex ());

  LinkSlice slice = ringInfo->GetLinkSlice ();
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
       << ",addr=" <<  lInfo->GetPortMacAddr (0)
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
RingController::MeterAdjusted (Ptr<const LinkInfo> lInfo,
                               LinkInfo::Direction dir, LinkSlice slice)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice);

  NS_ASSERT_MSG (GetSlicingMode () != OpMode::OFF, "Not supposed to "
                 "adjust slicing meters when network slicing mode is OFF.");

  uint8_t  swDpId  = (dir == LinkInfo::FWD) ? 0 : 1;
  uint16_t meterId = (dir == LinkInfo::FWD) ? 1 : 2;

  // FIXME Revisar o link slicing.
  if (GetSlicingMode () == OpMode::ON)
    {
      // When the network slicing operation mode is ON, the Non-GBR traffic of
      // each slice will be monitored independently. So we have to identify the
      // meter ID based on the slice parameter.
      // * For the DFT slice, the meter IDs are: 1 for FWD and 2 for BWD.
      // * For the M2M slice, the meter IDs are: 3 for FWD and 4 for BWD.
      // * For the GBR slice, we don't have Non-GBR traffic on this slice, so
      //   we don't have meters to update here.
      if (slice == LinkSlice::GBR)
        {
          return;
        }
      else if (slice == LinkSlice::M2M)
        {
          meterId += 2;
        }
    }
  else if (GetSlicingMode () == OpMode::AUTO)
    {
      // When the network slicing operation mode is AUTO, the Non-GBR traffic
      // of all slices will be monitored together. The meter IDs in use are:
      // 1 for FWD and 2 for BWD.
      slice = LinkSlice::ALL;
    }

  NS_LOG_INFO ("Updating slicing meter for link info " <<
               lInfo->GetSwDpId (0) << " - " << lInfo->GetSwDpId (1));

  std::ostringstream cmd;
  uint64_t kbps = 0;

  // -------------------------------------------------------------------------
  // Meter table
  //
  // Update the proper slicing meter.
  kbps = lInfo->GetFreeBitRate (dir, slice);
  cmd << "meter-mod cmd=mod"
      << ",flags=" << OFPMF_KBPS
      << ",meter=" << meterId
      << " drop:rate=" << kbps / 1000;
  DpctlExecute (lInfo->GetSwDpId (swDpId), cmd.str ());
  NS_LOG_DEBUG ("Link slice " << LinkSliceStr (slice) <<
                ": " << LinkInfo::DirectionStr (dir) <<
                " link updated to " << kbps << " Kbps");
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
