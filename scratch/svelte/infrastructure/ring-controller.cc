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

void
RingController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

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
  if (!rInfo->IsGbr () || (ringInfo->IsLocalPath (LteIface::S1U)
                           && ringInfo->IsLocalPath (LteIface::S5)))
    {
      return true;
    }

  // It only makes sense to check for available bandwidth for GBR bearers.
  NS_ASSERT_MSG (rInfo->IsGbr (), "Invalid configuration for GBR request.");

  // Check for the requested bit rate over the shortest path.
  if (HasBitRate (ringInfo))
    {
      NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                   " over the shortest path");
      return BitRateReserve (ringInfo);
    }

  // The requested bit rate is not available over the shortest path. When
  // using the SPF routing strategy, invert the routing path and check for the
  // requested bit rate over the longest path.
  if (m_strategy == RingController::SPF)
    {
      // Let's try inverting only the S1-U interface.
      if (!ringInfo->IsLocalPath (LteIface::S1U))
        {
          ringInfo->ResetToDefaults ();
          ringInfo->InvertPath (LteIface::S1U);
          if (HasBitRate (ringInfo))
            {
              NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                           " over the inverted S1-U path");
              return BitRateReserve (ringInfo);
            }
        }

      // Let's try inverting only the S5 interface.
      if (!ringInfo->IsLocalPath (LteIface::S5))
        {
          ringInfo->ResetToDefaults ();
          ringInfo->InvertPath (LteIface::S5);
          if (HasBitRate (ringInfo))
            {
              NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                           " over the inverted S5 path");
              return BitRateReserve (ringInfo);
            }
        }

      // Let's try inverting both the S1-U and S5 interface.
      if (!ringInfo->IsLocalPath (LteIface::S1U)
          && !ringInfo->IsLocalPath (LteIface::S5))
        {
          ringInfo->ResetToDefaults ();
          ringInfo->InvertPath (LteIface::S1U);
          ringInfo->InvertPath (LteIface::S5);
          if (HasBitRate (ringInfo))
            {
              NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                           " over the inverted S1-U and S5 paths");
              return BitRateReserve (ringInfo);
            }
        }
    }

  // Nothing more to do. Block the traffic.
  NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex ());
  BlockBearer (rInfo, RoutingInfo::LINKBAND);
  return false;
}

bool
RingController::BearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // For bearers without reserved resources: nothing to release.
  if (!rInfo->IsGbrReserved ())
    {
      return true;
    }

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");
  return BitRateRelease (ringInfo);
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

  BackhaulController::NotifyBearerCreated (rInfo);
}

void
RingController::NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices)
{
  NS_LOG_FUNCTION (this);

  // Chain up first, as we need to save the switch devices.
  BackhaulController::NotifyTopologyBuilt (devices);

  // Create the spanning tree for this topology.
  CreateSpanningTree ();

  // The following commands works as LINKS ARE CREATED IN CLOCKWISE DIRECTION.
  // Do not merge the two following loops. Groups must be created first to
  // avoid OpenFlow error messages with BAD_OUT_GROUP code.

  // Iterate over links configuring the groups.
  for (auto const &lInfo : LinkInfo::GetList ())
    {
      // ---------------------------------------------------------------------
      // Group table
      //
      // Configure groups to keep forwarding packets in the ring until they
      // reach the destination switch.
      //
      {
        // Routing group for clockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group=" << RingInfo::CLOCK
            << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (0);
        DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
      }
      {
        // Routing group for counterclockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group=" << RingInfo::COUNTER
            << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (1);
        DpctlSchedule (lInfo->GetSwDpId (1), cmd.str ());
      }
    }

  // Iterate over links configuring the forwarding rules.
  for (auto const &lInfo : LinkInfo::GetList ())
    {
      // ---------------------------------------------------------------------
      // Routing table -- [from higher to lower priority]
      //
      // GTP packets being forwarded by this switch. Write the output group
      // into action set based on input port. Write the same group number into
      // metadata field.
      // Send the packet to the bandwidth table.
      {
        // Clockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "flow-mod cmd=add,table=" << ROUTE_TAB
            << ",prio=128,flags="
            << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
            << " meta=0x0,in_port=" << lInfo->GetPortNo (0)
            << " write:group=" << RingInfo::COUNTER
            << " meta:" << RingInfo::COUNTER
            << " goto:" << BANDW_TAB;
        DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
      }
      {
        // Counterclockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "flow-mod cmd=add,table=" << ROUTE_TAB
            << ",prio=128,flags="
            << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
            << " meta=0x0,in_port=" << lInfo->GetPortNo (1)
            << " write:group=" << RingInfo::CLOCK
            << " meta:" << RingInfo::CLOCK
            << " goto:" << BANDW_TAB;
        DpctlSchedule (lInfo->GetSwDpId (1), cmd.str ());
      }
    }
}

bool
RingController::TopologyRoutingInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  NS_LOG_INFO ("Installing ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Building the dpctl command + arguments string.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=" << GetSliceTable (rInfo->GetSliceId ())
      << ",flags="
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
  if (rInfo->HasDlTraffic ())
    {
      // Building the match string for both S1-U and S5 interfaces
      // No match on source IP because we may have several P-GW TFT switches.
      std::ostringstream mS5, mS1;
      mS5 << " eth_type=0x800,ip_proto=17"
          << ",ip_dst=" << rInfo->GetSgwS5Addr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();
      mS1 << " eth_type=0x800,ip_proto=17"
          << ",ip_dst=" << rInfo->GetEnbS1uAddr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();

      // Build the metatada and goto instructions string.
      std::ostringstream aS5, aS1;
      aS5 << " meta:" << ringInfo->GetDlPath (LteIface::S5)
          << " goto:" << ROUTE_TAB;
      aS1 << " meta:" << ringInfo->GetDlPath (LteIface::S1U)
          << " goto:" << ROUTE_TAB;

      // Installing down rules into switches connected to the P-GW and S-GW.
      DpctlExecute (GetDpId (rInfo->GetPgwInfraSwIdx ()),
                    cmd.str () + mS5.str () + dscp.str () + aS5.str ());
      DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()),
                    cmd.str () + mS1.str () + dscp.str () + aS1.str ());
    }

  // Configuring uplink routing.
  if (rInfo->HasUlTraffic ())
    {
      // Building the match string.
      std::ostringstream mS1, mS5;
      mS1 << " eth_type=0x800,ip_proto=17"
          << ",ip_src=" << rInfo->GetEnbS1uAddr ()
          << ",ip_dst=" << rInfo->GetSgwS1uAddr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();
      mS5 << " eth_type=0x800,ip_proto=17"
          << ",ip_src=" << rInfo->GetSgwS5Addr ()
          << ",ip_dst=" << rInfo->GetPgwS5Addr ()
          << ",gtpu_teid=" << rInfo->GetTeidHex ();

      // Build the metatada and goto instructions string.
      std::ostringstream aS1, aS5;
      aS1 << " meta:" << ringInfo->GetUlPath (LteIface::S1U)
          << " goto:" << ROUTE_TAB;
      aS5 << " meta:" << ringInfo->GetUlPath (LteIface::S5)
          << " goto:" << ROUTE_TAB;

      // Installing up rules into switches connected to the eNB and S-GW.
      DpctlExecute (GetDpId (rInfo->GetEnbInfraSwIdx ()),
                    cmd.str () + mS1.str () + dscp.str () + aS1.str ());
      DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()),
                    cmd.str () + mS5.str () + dscp.str () + aS5.str ());
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
  cmd << "flow-mod cmd=del,table=" << GetSliceTable (rInfo->GetSliceId ())
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",cookie_mask=" << COOKIE_STRICT_MASK_STR;

  // Removing rules from switches connected to the eNB, S-GW and P-GW.
  DpctlExecute (GetDpId (rInfo->GetPgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (rInfo->GetEnbInfraSwIdx ()), cmd.str ());

  return true;
}

void
RingController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  // Write the output group into action set based on metadata field.
  // Send the packet to the bandwidth table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64,table=" << ROUTE_TAB
        << " meta=" << RingInfo::CLOCK
        << " write:group=" << RingInfo::CLOCK
        << " goto:" << BANDW_TAB;
    DpctlExecute (swtch, cmd.str ());
  }
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64,table=" << ROUTE_TAB
        << " meta=" << RingInfo::COUNTER
        << " write:group=" << RingInfo::COUNTER
        << " goto:" << BANDW_TAB;
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Bandwitdh table -- [from higher to lower priority]
  //
  // We are using the IP DSCP field to identify Non-GBR traffic.
  // Apply Non-GBR meter band.
  // Send the packet to the output table.
  if (GetLinkSlicingMode () == OpMode::ON)
    {
      // Apply meter rules for each slice.
      for (int s = 0; s < SliceId::ALL; s++)
        {
          SliceId slice = static_cast<SliceId> (s);
          for (int d = 0; d <= LinkInfo::BWD; d++)
            {
              LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);
              RingInfo::RingPath ringPath = RingInfo::LinkDirToRingPath (dir);
              uint32_t meterId = GetSvelteMeterId (slice, d);

              // Non-GBR QCIs range is [5, 9].
              for (int q = 5; q <= 9; q++)
                {
                  EpsBearer::Qci qci = static_cast<EpsBearer::Qci> (q);
                  Ipv4Header::DscpType dscp = Qci2Dscp (qci);

                  // Apply this meter to the traffic of this slice only.
                  std::ostringstream cmd;
                  cmd << "flow-mod cmd=add,prio=16,table=" << BANDW_TAB
                      << " eth_type=0x800,ip_proto=17,meta=" << ringPath
                      << ",gtpu_teid=" << (meterId & TEID_SLICE_MASK)
                      << "/" << TEID_SLICE_MASK
                      << ",ip_dscp=" << static_cast<uint16_t> (dscp)
                      << " meter:" << meterId
                      << " goto:" << OUTPT_TAB;
                  DpctlExecute (swtch, cmd.str ());
                }
            }
        }
    }
  else if (GetLinkSlicingMode () == OpMode::AUTO)
    {
      SliceId slice = SliceId::ALL;
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);
          RingInfo::RingPath ringPath = RingInfo::LinkDirToRingPath (dir);
          uint32_t meterId = GetSvelteMeterId (slice, d);

          // Non-GBR QCIs range is [5, 9].
          for (int q = 5; q <= 9; q++)
            {
              EpsBearer::Qci qci = static_cast<EpsBearer::Qci> (q);
              Ipv4Header::DscpType dscp = Qci2Dscp (qci);

              // Apply this meter to the traffic of all slices.
              std::ostringstream cmd;
              cmd << "flow-mod cmd=add,prio=16,table=" << BANDW_TAB
                  << " eth_type=0x800,ip_proto=17,meta=" << ringPath
                  << ",ip_dscp=" << static_cast<uint16_t> (dscp)
                  << " meter:" << meterId
                  << " goto:" << OUTPT_TAB;
              DpctlExecute (swtch, cmd.str ());
            }
        }
    }

  BackhaulController::HandshakeSuccessful (swtch);
}

bool
RingController::HasBitRate (Ptr<const RingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  SliceId slice = rInfo->GetSliceId ();
  uint64_t dlRate = rInfo->GetGbrDlBitRate ();
  uint64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // When checking for the available bit rate on backhaul links, the S5 and
  // S1-U routing paths may overlap if one of them is CLOCK and the other is
  // COUNTER. We must ensure that the overlapping links have the requested
  // bandwidth for both interfaces, otherwise the BitRateReserve () method will
  // fail. So we save the links used by S5 interface and check them when going
  // through S1-U interface.
  std::set<Ptr<LinkInfo> > s5Links;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (success && curr != rInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);
      success &= lInfo->HasBitRate (currId, nextId, slice, dlRate);
      success &= lInfo->HasBitRate (nextId, currId, slice, ulRate);
      curr = next;

      // Save this link as used by S5 interface.
      auto ret = s5Links.insert (lInfo);
      NS_ABORT_MSG_IF (ret.second == false, "Error when saving link in map.");
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (success && curr != rInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      Ptr<LinkInfo> lInfo = GetLinkInfo (curr, next);
      uint64_t currId = GetDpId (curr);
      uint64_t nextId = GetDpId (next);

      // Check if this link was used by S5 interface.
      auto it = s5Links.find (lInfo);
      if (it != s5Links.end ())
        {
          // When checking for downlink resources for S1-U interface we also
          // must ensure that the links has uplink resources for S5 interface,
          // and vice-versa.
          success &= lInfo->HasBitRate (currId, nextId, slice,
                                        dlRate + ulRate);
          success &= lInfo->HasBitRate (nextId, currId, slice,
                                        ulRate + dlRate);
        }
      else
        {
          success &= lInfo->HasBitRate (currId, nextId, slice, dlRate);
          success &= lInfo->HasBitRate (nextId, currId, slice, ulRate);
        }
      curr = next;
    }
  return success;
}

bool
RingController::BitRateReserve (Ptr<RingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_LOG_INFO ("Reserving resources for teid " << rInfo->GetTeidHex ());

  SliceId slice = rInfo->GetSliceId ();
  uint64_t dlRate = rInfo->GetGbrDlBitRate ();
  uint64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (success && curr != rInfo->GetSgwInfraSwIdx ())
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
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (success && curr != rInfo->GetEnbInfraSwIdx ())
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
  rInfo->SetGbrReserved (success);
  return success;
}

bool
RingController::BitRateRelease (Ptr<RingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_LOG_INFO ("Releasing resources for teid " << rInfo->GetTeidHex ());

  SliceId slice = rInfo->GetSliceId ();
  uint64_t dlRate = rInfo->GetGbrDlBitRate ();
  uint64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (success && curr != rInfo->GetSgwInfraSwIdx ())
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
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (success && curr != rInfo->GetEnbInfraSwIdx ())
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
  rInfo->SetGbrReserved (!success);
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
  {
    std::ostringstream cmd;
    cmd << "port-mod port=" << lInfo->GetPortNo (0)
        << ",addr=" << lInfo->GetPortMacAddr (0)
        << ",conf=0x00000020,mask=0x00000020";
    DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
  }
  {
    std::ostringstream cmd;
    cmd << "port-mod port=" << lInfo->GetPortNo (1)
        << ",addr=" << lInfo->GetPortMacAddr (1)
        << ",conf=0x00000020,mask=0x00000020";
    DpctlSchedule (lInfo->GetSwDpId (1), cmd.str ());
  }
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
