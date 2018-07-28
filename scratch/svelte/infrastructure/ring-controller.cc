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
#include "backhaul-network.h"
#include "ring-controller.h"
#include "../lte-interface.h"
#include "../logical/metadata/routing-info.h"

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

void
RingController::NotifyTopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // Save the collection of switch devices and create the spanning tree.
  m_switchDevices = devices;
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

  // -------------------------------------------------------------------------
  // Group table
  //
  // Routing group for clockwise packet forwarding.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::CLOCK
       << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (0);
  DpctlSchedule (cInfo->GetSwDpId (0), cmd1.str ());

  // Routing group for counterclockwise packet forwarding.
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::COUNTER
       << " weight=0,port=any,group=any output=" << cInfo->GetPortNo (1);
  DpctlSchedule (cInfo->GetSwDpId (1), cmd2.str ());

  // -------------------------------------------------------------------------
  // Meter table
  //
  // Set up Non-GBR meters when the network slicing mechanism is enabled.
  if (GetSlicingMode () != OperationMode::OFF)
    {
      NS_LOG_DEBUG ("Creating slicing meters for connection info " <<
                    cInfo->GetSwDpId (0) << " to " << cInfo->GetSwDpId (1));

      // Connect this controller to ConnectionInfo meter ajdusted trace source.
      cInfo->TraceConnectWithoutContext (
        "MeterAdjusted", MakeCallback (
          &RingController::MeterAdjusted, this));

      // Meter flags OFPMF_KBPS.
      std::string flagsStr ("0x0001");
      std::ostringstream cmdm1, cmdm2, cmdm3, cmdm4;
      uint64_t kbps = 0;

      if (GetSlicingMode () == OperationMode::ON)
        {
          // DFT Non-GBR meter for clockwise FWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::FWD, Slice::DFT);
          cmdm1 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=1 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (0), cmdm1.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::DFT) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::FWD) <<
                        " link set to " << kbps << " Kbps");

          // DFT Non-GBR meter for counterclockwise BWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::BWD, Slice::DFT);
          cmdm2 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=2 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (1), cmdm2.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::DFT) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::BWD) <<
                        " link set to " << kbps << " Kbps");

          // M2M Non-GBR meter for clockwise FWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::FWD, Slice::M2M);
          cmdm3 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=3 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (0), cmdm3.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::M2M) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::FWD) <<
                        " link set to " << kbps << " Kbps");

          // M2M Non-GBR meter for counterclockwise BWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::BWD, Slice::M2M);
          cmdm4 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=4 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (1), cmdm4.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::M2M) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::BWD) <<
                        " link set to " << kbps << " Kbps");
        }
      else if (GetSlicingMode () == OperationMode::AUTO)
        {
          // Non-GBR meter for clockwise FWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::FWD, Slice::ALL);
          cmdm1 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=1 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (0), cmdm1.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::ALL) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::FWD) <<
                        " link set to " << kbps << " Kbps");

          // Non-GBR meter for counterclockwise BWD direction.
          kbps = cInfo->GetFreeBitRate (ConnectionInfo::BWD, Slice::ALL);
          cmdm2 << "meter-mod cmd=add,flags=" << flagsStr
                << ",meter=2 drop:rate=" << kbps / 1000;
          DpctlSchedule (cInfo->GetSwDpId (1), cmdm2.str ());
          NS_LOG_DEBUG ("Slice " << SliceStr (Slice::ALL) << ": " <<
                        ConnectionInfo::DirectionStr (ConnectionInfo::BWD) <<
                        " link set to " << kbps << " Kbps");
        }
    }
}

void
RingController::TopologyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  // FIXME Aqui tem que marcar qual o cinfo slice que este tráfego pertence

  // Let's create its ring routing metadata.
  Ptr<RingRoutingInfo> ringInfo = CreateObject<RingRoutingInfo> (rInfo);

  // Set default paths to those with lower hops.
  RingRoutingInfo::RoutingPath s1uDownPath = FindShortestPath (
      rInfo->GetSgwInfraSwIdx (), rInfo->GetEnbInfraSwIdx ());
  RingRoutingInfo::RoutingPath s5DownPath = FindShortestPath (
      rInfo->GetPgwInfraSwIdx (), rInfo->GetSgwInfraSwIdx ());
  ringInfo->SetDefaultPath (s1uDownPath, LteInterface::S1U);
  ringInfo->SetDefaultPath (s5DownPath, LteInterface::S5);

  NS_LOG_DEBUG ("Bearer teid " << rInfo->GetTeid () <<
                " default downlink S1-U path to " << s1uDownPath <<
                " and S5 path to " << s5DownPath);
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

//   // If the bearer is already blocked, there's nothing more to do.
//   if (rInfo->IsBlocked ())
//     {
//       return false;
//     }
//
//   // Reset the ring routing info to the shortest path.
//   Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
//   ringInfo->ResetPath ();
//
//   // Update the slice bandwidth usage on aggregation info.
//   Ptr<S5AggregationInfo> aggInfo = rInfo->GetObject<S5AggregationInfo> ();
//   aggInfo->SetSliceUsage (GetSliceUsage (rInfo->GetSlice ()));
//
//   // For Non-GBR bearers (which includes the default bearer), for bearers that
//   // only transverse local switch (local routing), and for HTC aggregated
//   // bearers: let's accept it without guarantees. Note that in current
//   // implementation, these bearers are always routed over the shortest path.
//   if (!rInfo->IsGbr () || ringInfo->IsLocalPath ()
//       || (rInfo->IsHtc () && rInfo->IsAggregated ()))
//     {
//       return true;
//     }
//
//   // It only makes sense to check for available bandwidth for GBR bearers.
//   Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
//   NS_ASSERT_MSG (gbrInfo, "Invalid configuration for GBR bearer request.");
//
//   // Check for the requested bit rate over the shortest path.
//   if (HasBitRate (ringInfo, gbrInfo, rInfo->GetSlice ()))
//     {
//       NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeid () <<
//                    " over the shortest path");
//       return true;
//     }
//
//   // The requested bit rate is not available over the shortest path. When
//   // using the SPF routing strategy, invert the routing path and check for the
//   // requested bit rate over the longest path.
//   if (m_strategy == RingController::SPF)
//     {
//       ringInfo->InvertPath ();
//       if (HasBitRate (ringInfo, gbrInfo, rInfo->GetSlice ()))
//         {
//           NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeid () <<
//                        " over the longest (inverted) path");
//           return true;
//         }
//     }
//
//   // Nothing more to do. Block the traffic.
//   NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid ());
//   rInfo->SetBlocked (true, RoutingInfo::NOBANDWIDTH);
  return false;
}

bool
RingController::TopologyBitRateRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

//   // For bearers without reserved resources: nothing to release.
//   Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
//   if (!gbrInfo || !gbrInfo->IsReserved ())
//     {
//       return true;
//     }
//
//   NS_LOG_INFO ("Releasing resources for bearer " << rInfo->GetTeid ());
//
//   Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
//   NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");
//
  bool success = true;
//   uint16_t curr = ringInfo->GetPgwSwIdx ();
//   while (success && curr != ringInfo->GetSgwSwIdx ())
//     {
//       uint16_t next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
//       Ptr<ConnectionInfo> cInfo = GetConnectionInfo (curr, next);
//
//       uint64_t currId = GetDpId (curr);
//       uint64_t nextId = GetDpId (next);
//       success &= cInfo->ReleaseBitRate (currId, nextId, rInfo->GetSlice (),
//                                         gbrInfo->GetDownBitRate ());
//       success &= cInfo->ReleaseBitRate (nextId, currId, rInfo->GetSlice (),
//                                         gbrInfo->GetUpBitRate ());
//       curr = next;
//     }
//   NS_ASSERT_MSG (success, "Error when releasing resources.");
//   gbrInfo->SetReserved (!success);
  return success;
}

bool
RingController::TopologyBitRateReserve (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

//   // If the bearer is already blocked, there's nothing more to do.
//   if (rInfo->IsBlocked ())
//     {
//       return false;
//     }
//
//   Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
//   NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");
//
//   // For Non-GBR bearers (which includes the default bearer), for bearers that
//   // only transverse local switch (local routing), and for HTC aggregated
//   // bearers: don't reserve bit rate resources.
//   if (!rInfo->IsGbr () || ringInfo->IsLocalPath ()
//       || (rInfo->IsHtc () && rInfo->IsAggregated ()))
//     {
//       return true;
//     }
//
//   NS_LOG_INFO ("Reserving resources for bearer " << rInfo->GetTeid ());
//
//   // It only makes sense to reserve bandwidth for GBR bearers.
//   Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
//   NS_ASSERT_MSG (gbrInfo, "Invalid configuration for GBR bearer request.");
//
  bool success = true;
//   uint16_t curr = ringInfo->GetPgwSwIdx ();
//   while (success && curr != ringInfo->GetSgwSwIdx ())
//     {
//       uint16_t next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
//       Ptr<ConnectionInfo> cInfo = GetConnectionInfo (curr, next);
//
//       uint64_t currId = GetDpId (curr);
//       uint64_t nextId = GetDpId (next);
//       success &= cInfo->ReserveBitRate (currId, nextId, rInfo->GetSlice (),
//                                         gbrInfo->GetDownBitRate ());
//       success &= cInfo->ReserveBitRate (nextId, currId, rInfo->GetSlice (),
//                                         gbrInfo->GetUpBitRate ());
//       curr = next;
//     }
//   NS_ASSERT_MSG (success, "Error when reserving resources.");
//   gbrInfo->SetReserved (success);
  return success;
}

bool
RingController::TopologyRoutingInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Installing ring rules for bearer teid " << rInfo->GetTeid ());

//   // Getting ring routing information.
//   Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
//
//   // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
//   std::string flagsStr ("0x0007");
//
//   // Printing the cookie and buffer values in dpctl string format.
//   char cookieStr [20], metadataStr [12];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Building the dpctl command + arguments string.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=add,table=1"
//       << ",flags=" << flagsStr
//       << ",cookie=" << cookieStr
//       << ",prio=" << rInfo->GetPriority ()
//       << ",idle=" << rInfo->GetTimeout ();
//
//   // Configuring downlink routing.
//   if (rInfo->HasDownlinkTraffic ())
//     {
//       // Building the match string.
//       // No match on source IP because we may have several P-GW TFT switches.
//       std::ostringstream match;
//       match << " eth_type=0x800,ip_proto=17"
//             << ",ip_dst=" << rInfo->GetSgwS5Addr ()
//             << ",gtpu_teid=" << rInfo->GetTeid ();
//
//       // Set the IP DSCP field when necessary.
//       std::ostringstream act;
//       if (rInfo->GetDscpValue ())
//         {
//           // Build the apply set_field action instruction string.
//           act << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
//         }
//
//       // Build the metatada, write and goto instructions string.
//       sprintf (metadataStr, "0x%x", ringInfo->GetDownPath ());
//       act << " meta:" << metadataStr << " goto:2";
//
//       // Installing the rule into input switch.
//       // In downlink the input ring switch is the one connected to the P-GW.
//       std::string commandStr = cmd.str () + match.str () + act.str ();
//       DpctlExecute (ringInfo->GetPgwSwDpId (), commandStr);
//     }
//
//   // Configuring uplink routing.
//   if (rInfo->HasUplinkTraffic ())
//     {
//       // Building the match string.
//       std::ostringstream match;
//       match << " eth_type=0x800,ip_proto=17"
//             << ",ip_src=" << rInfo->GetSgwS5Addr ()
//             << ",ip_dst=" << rInfo->GetPgwS5Addr ()
//             << ",gtpu_teid=" << rInfo->GetTeid ();
//
//       // Set the IP DSCP field when necessary.
//       std::ostringstream act;
//       if (rInfo->GetDscpValue ())
//         {
//           // Build the apply set_field action instruction string.
//           act << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
//         }
//
//       // Build the metatada, write and goto instructions string.
//       sprintf (metadataStr, "0x%x", ringInfo->GetUpPath ());
//       act << " meta:" << metadataStr << " goto:2";
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
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Removing ring rules for bearer teid " << rInfo->GetTeid ());

//   // Print the cookie value in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Getting ring routing information.
//   Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
//
//   // Remove flow entries for this TEID.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=del,table=1"
//       << ",cookie=" << cookieStr
//       << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
//
//   // Remove downlink routing.
//   if (rInfo->HasDownlinkTraffic ())
//     {
//       DpctlExecute (ringInfo->GetPgwSwDpId (), cmd.str ());
//     }
//
//   // Remove uplink routing.
//   if (rInfo->HasUplinkTraffic ())
//     {
//       DpctlExecute (ringInfo->GetSgwSwDpId (), cmd.str ());
//     }
  return true;
}

void
RingController::CreateSpanningTree (void)
{
  NS_LOG_FUNCTION (this);

  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD) with OFPPC_NO_FWD config (0x20).
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

double
RingController::GetSliceUsage (Slice slice) const
{
  NS_LOG_FUNCTION (this << slice);

  double sliceUsage = 0;
  uint16_t curr = 0;
  uint16_t next = NextSwitchIndex (curr, RingRoutingInfo::CLOCK);
  do
    {
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (curr, next);
      sliceUsage = std::max (
          sliceUsage, std::max (
            cInfo->GetThpSliceRatio (ConnectionInfo::FWD, slice),
            cInfo->GetThpSliceRatio (ConnectionInfo::BWD, slice)));
      curr = next;
      next = NextSwitchIndex (curr, RingRoutingInfo::CLOCK);
    }
  while (curr != 0);

  return sliceUsage;
}

// bool
// RingController::HasBitRate (Ptr<const RingRoutingInfo> ringInfo,
//                             Ptr<const GbrInfo> gbrInfo, Slice slice) const
// {
//   NS_LOG_FUNCTION (this << ringInfo << gbrInfo << slice);
//
//   bool success = true;
//   uint16_t curr = ringInfo->GetPgwSwIdx ();
//   while (success && curr != ringInfo->GetSgwSwIdx ())
//     {
//       uint16_t next = NextSwitchIndex (curr, ringInfo->GetDownPath ());
//       Ptr<ConnectionInfo> cInfo = GetConnectionInfo (curr, next);
//
//       uint64_t currId = GetDpId (curr);
//       uint64_t nextId = GetDpId (next);
//       success &= cInfo->HasBitRate (currId, nextId, slice,
//                                     gbrInfo->GetDownBitRate ());
//       success &= cInfo->HasBitRate (nextId, currId, slice,
//                                     gbrInfo->GetUpBitRate ());
//       curr = next;
//     }
//   return success;
// }

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
RingController::MeterAdjusted (Ptr<const ConnectionInfo> cInfo,
                               ConnectionInfo::Direction dir, Slice slice)
{
  NS_LOG_FUNCTION (this << cInfo << dir << slice);

  NS_ASSERT_MSG (GetSlicingMode () != OperationMode::OFF, "Not supposed to "
                 "adjust slicing meters when network slicing mode is OFF.");

  uint8_t  swDpId  = (dir == ConnectionInfo::FWD) ? 0 : 1;
  uint16_t meterId = (dir == ConnectionInfo::FWD) ? 1 : 2;

  if (GetSlicingMode () == OperationMode::ON)
    {
      // When the network slicing operation mode is ON, the Non-GBR traffic of
      // each slice will be monitored independently. So we have to identify the
      // meter ID based on the slice parameter.
      // * For the DFT slice, the meter IDs are: 1 for FWD and 2 for BWD.
      // * For the M2M slice, the meter IDs are: 3 for FWD and 4 for BWD.
      // * For the GBR slice, we don't have Non-GBR traffic on this slice, so
      //   we don't have meters to update here.
      if (slice == Slice::GBR)
        {
          return;
        }
      else if (slice == Slice::M2M)
        {
          meterId += 2;
        }
    }
  else if (GetSlicingMode () == OperationMode::AUTO)
    {
      // When the network slicing operation mode is AUTO, the Non-GBR traffic
      // of all slices will be monitored together. The meter IDs in use are:
      // 1 for FWD and 2 for BWD.
      slice = Slice::ALL;
    }

  NS_LOG_INFO ("Updating slicing meter for connection info " <<
               cInfo->GetSwDpId (0) << " to " << cInfo->GetSwDpId (1));

  // Meter flags OFPMF_KBPS.
  std::string flagsStr ("0x0001");
  std::ostringstream cmd;
  uint64_t kbps = 0;

  // -------------------------------------------------------------------------
  // Meter table
  //
  // Update the proper slicing meter.
  kbps = cInfo->GetFreeBitRate (dir, slice);
  cmd << "meter-mod cmd=mod"
      << ",flags=" << flagsStr
      << ",meter=" << meterId
      << " drop:rate=" << kbps / 1000;
  DpctlExecute (cInfo->GetSwDpId (swDpId), cmd.str ());
  NS_LOG_DEBUG ("Slice " << SliceStr (slice) << ": " <<
                ConnectionInfo::DirectionStr (dir) <<
                " link updated to " << kbps << " Kbps");
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

} // namespace ns3
