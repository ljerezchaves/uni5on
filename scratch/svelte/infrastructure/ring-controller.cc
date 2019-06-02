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
#include "../logical/slice-controller.h"
#include "../metadata/enb-info.h"
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
    .AddAttribute ("Routing", "The ring routing strategy.",
                   EnumValue (RingController::SPO),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::SPO,
                                    RoutingStrategyStr (RingController::SPO),
                                    RingController::SPF,
                                    RoutingStrategyStr (RingController::SPF)))
  ;
  return tid;
}

RingController::RoutingStrategy
RingController::GetRoutingStrategy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_strategy;
}

std::string
RingController::RoutingStrategyStr (RoutingStrategy strategy)
{
  switch (strategy)
    {
    case RingController::SPO:
      return "spo";
    case RingController::SPF:
      return "spf";
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

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // Reset the shortest path for the S1-U interface (the handover procedure may
  // have changed the eNB switch index.)
  SetShortestPath (ringInfo, LteIface::S1);

  // Part 1: Check for the available resources on the S5 interface.
  bool s5Ok = HasAvailableResources (ringInfo, LteIface::S5);
  if (!s5Ok)
    {
      NS_ASSERT_MSG (rInfo->IsBlocked (), "This bearer should be blocked.");
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because there are no resources for the S5 interface.");
    }

  // Part 2: Check for the available resources on the S1-U interface.
  // To avoid errors when reserving bit rates, check for overlapping links.
  LinkInfoSet_t s5Links;
  GetLinkSet (ringInfo, LteIface::S5, &s5Links);
  bool s1Ok = HasAvailableResources (ringInfo, LteIface::S1, &s5Links);
  if (!s1Ok)
    {
      NS_ASSERT_MSG (rInfo->IsBlocked (), "This bearer should be blocked.");
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because there are no resources for the S1-U interface.");
    }

  return (s5Ok && s1Ok);
}

bool
RingController::BearerReserve (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->IsBlocked (), "Bearer should not be blocked.");
  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= BitRateReserve (ringInfo, LteIface::S5);
  success &= BitRateReserve (ringInfo, LteIface::S1);
  return success;
}

bool
RingController::BearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= BitRateRelease (ringInfo, LteIface::S5);
  success &= BitRateRelease (ringInfo, LteIface::S1);
  return success;
}

bool
RingController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsInstalled (), "Rules must not be installed.");
  NS_LOG_INFO ("Installing ring rules for teid " << rInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= RulesInstall (ringInfo, LteIface::S5);
  success &= RulesInstall (ringInfo, LteIface::S1);
  return success;
}

bool
RingController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsInstalled (), "Rules must be installed.");
  NS_LOG_INFO ("Removing ring rules for teid " << rInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= RulesRemove (ringInfo, LteIface::S5);
  success &= RulesRemove (ringInfo, LteIface::S1);
  return success;
}

bool
RingController::BearerUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsInstalled (), "Rules must be installed.");
  NS_ASSERT_MSG (rInfo->GetEnbCellId () != dstEnbInfo->GetCellId (),
                 "Don't update UE's eNB info before BearerUpdate.");
  NS_LOG_INFO ("Updating ring rules for teid " << rInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // Each slice has a single P-GW and S-GW, so handover only changes the eNB.
  // Thus, we only need to modify the S1-U backhaul rules.
  bool success = true;
  success &= RulesUpdate (ringInfo, LteIface::S1, dstEnbInfo);
  return success;
}

void
RingController::NotifyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // Let's create its ring routing metadata.
  Ptr<RingInfo> ringInfo = CreateObject<RingInfo> (rInfo);

  // Set the downlink shortest path for both S1-U and S5 interfaces.
  SetShortestPath (ringInfo, LteIface::S5);
  SetShortestPath (ringInfo, LteIface::S1);

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

  // Iterate over links configuring the ring routing groups.
  // The following commands works as LINKS ARE CREATED IN CLOCKWISE DIRECTION.
  // Groups must be created first to avoid OpenFlow BAD_OUT_GROUP error code.
  for (auto const &lInfo : LinkInfo::GetList ())
    {
      // ---------------------------------------------------------------------
      // Group table
      //
      // Configure groups to forward packets in both ring directions.
      {
        // Routing group for clockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group="    << RingInfo::CLOCK
            << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (0);
        DpctlExecute (lInfo->GetSwDpId (0), cmd.str ());
      }
      {
        // Routing group for counterclockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group="    << RingInfo::COUNT
            << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (1);
        DpctlExecute (lInfo->GetSwDpId (1), cmd.str ());
      }
    }
}

void
RingController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the OpenFlow switch datapath ID.
  uint64_t swDpId = swtch->GetDpId ();

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // Skip slice classification for X2-C packets, routing them always in the
  // clockwise direction.
  // Write the output group into action set.
  // Send the packet directly to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="                    << CLASS_TAB
        << ",flags="                    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="                 << IPV4_PROT_NUM
        << ",ip_proto="                 << UDP_PROT_NUM
        << ",udp_src="                  << X2C_PORT
        << ",udp_dst="                  << X2C_PORT
        << " write:group="              << RingInfo::CLOCK
        << " goto:"                     << OUTPT_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Apply Non-GBR meter band.
  // Send the packet to the output table.
  switch (GetInterSliceMode ())
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      break;

    case SliceMode::SHAR:
      // Apply high-priority individual Non-GBR meter entries for slices with
      // disabled bandwidth sharing and the low-priority shared Non-GBR meter
      // entry for other slices.
      SlicingMeterApply (swtch, SliceId::ALL);
      for (auto const &ctrl : GetSliceControllerList ())
        {
          if (ctrl->GetSharing () == OpMode::OFF)
            {
              SlicingMeterApply (swtch, ctrl->GetSliceId ());
            }
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      // Apply individual Non-GBR meter entries for each slice.
      for (auto const &ctrl : GetSliceControllerList ())
        {
          SlicingMeterApply (swtch, ctrl->GetSliceId ());
        }
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }

  BackhaulController::HandshakeSuccessful (swtch);
}

bool
RingController::BitRateRequest (
  Ptr<RingInfo> ringInfo, LteIface iface, LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface << overlap);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Ignoring this check for Non-GBR bearers, aggregated bearers,
  // and local-routing bearers.
  if (rInfo->IsNonGbr () || rInfo->IsAggregated ()
      || ringInfo->IsLocalPath (iface))
    {
      return true;
    }

  return BitRateRequest (
    rInfo->GetSrcDlInfraSwIdx (iface),
    rInfo->GetDstDlInfraSwIdx (iface),
    rInfo->GetGbrDlBitRate (),
    rInfo->GetGbrUlBitRate (),
    ringInfo->GetDlPath (iface),
    rInfo->GetSliceId (),
    GetSliceController (rInfo->GetSliceId ())->GetGbrBlockThs (),
    overlap);
}

bool
RingController::BitRateRequest (
  uint16_t srcIdx, uint16_t dstIdx, int64_t fwdBitRate, int64_t bwdBitRate,
  RingInfo::RingPath path, SliceId slice, double blockThs,
  LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << fwdBitRate <<
                   bwdBitRate << path << slice << blockThs << overlap);

  // Walk through links in the given routing path, requesting for the bit rate.
  LinkInfo::LinkDir fwdDir, bwdDir;
  Ptr<LinkInfo> lInfo;
  bool ok = true;
  while (ok && srcIdx != dstIdx)
    {
      uint16_t next = GetNextSwIdx (srcIdx, path);
      std::tie (lInfo, fwdDir, bwdDir) = GetLinkInfo (srcIdx, next);
      if (overlap && overlap->find (lInfo) != overlap->end ())
        {
          // Ensure that overlapping links have the requested bandwidth for
          // both directions, otherwise the BitRateReserve method will fail.
          int64_t sumBitRate = fwdBitRate + bwdBitRate;
          ok &= lInfo->HasBitRate (fwdDir, slice, sumBitRate, blockThs);
          ok &= lInfo->HasBitRate (bwdDir, slice, sumBitRate, blockThs);
        }
      else
        {
          ok &= lInfo->HasBitRate (fwdDir, slice, fwdBitRate, blockThs);
          ok &= lInfo->HasBitRate (bwdDir, slice, bwdBitRate, blockThs);
        }
      srcIdx = next;
    }

  return ok;
}

bool
RingController::BitRateReserve (
  Ptr<RingInfo> ringInfo, LteIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_ASSERT_MSG (!rInfo->IsBlocked (), "Bearer should not be blocked.");
  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");
  NS_ASSERT_MSG (!rInfo->IsGbrReserved (iface), "Bit rate already reserved.");

  NS_LOG_INFO ("Reserving resources for teid " << rInfo->GetTeidHex () <<
               " on interface " << LteIfaceStr (iface));

  // Ignoring bearers without guaranteed bit rate or local-routing bearers.
  if (!rInfo->HasGbrBitRate () || ringInfo->IsLocalPath (iface))
    {
      return true;
    }
  NS_ASSERT_MSG (rInfo->IsGbr (), "Non-GBR bearers should not get here.");

  bool success = BitRateReserve (
      rInfo->GetSrcDlInfraSwIdx (iface),
      rInfo->GetDstDlInfraSwIdx (iface),
      rInfo->GetGbrDlBitRate (),
      rInfo->GetGbrUlBitRate (),
      ringInfo->GetDlPath (iface),
      rInfo->GetSliceId ());
  rInfo->SetGbrReserved (iface, success);
  return success;
}

bool
RingController::BitRateReserve (
  uint16_t srcIdx, uint16_t dstIdx, int64_t fwdBitRate, int64_t bwdBitRate,
  RingInfo::RingPath path, SliceId slice)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << fwdBitRate <<
                   bwdBitRate << path << slice);

  // Walk through links in the given routing path, reserving the bit rate.
  LinkInfo::LinkDir fwdDir, bwdDir;
  Ptr<LinkInfo> lInfo;
  bool ok = true;
  while (ok && srcIdx != dstIdx)
    {
      uint16_t next = GetNextSwIdx (srcIdx, path);
      std::tie (lInfo, fwdDir, bwdDir) = GetLinkInfo (srcIdx, next);
      ok &= lInfo->UpdateResBitRate (fwdDir, slice, fwdBitRate);
      ok &= lInfo->UpdateResBitRate (bwdDir, slice, bwdBitRate);
      SlicingMeterAdjust (lInfo, slice);
      srcIdx = next;
    }

  NS_ASSERT_MSG (ok, "Error when reserving bit rate.");
  return ok;
}

bool
RingController::BitRateRelease (
  Ptr<RingInfo> ringInfo, LteIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_LOG_INFO ("Releasing resources for teid " << rInfo->GetTeidHex () <<
               " on interface " << LteIfaceStr (iface));

  // Nothing to release when no guaranteed bit rate was reserved.
  if (!rInfo->IsGbrReserved (iface))
    {
      return true;
    }

  bool success = BitRateRelease (
      rInfo->GetSrcDlInfraSwIdx (iface),
      rInfo->GetDstDlInfraSwIdx (iface),
      rInfo->GetGbrDlBitRate (),
      rInfo->GetGbrUlBitRate (),
      ringInfo->GetDlPath (iface),
      rInfo->GetSliceId ());
  rInfo->SetGbrReserved (iface, !success);
  return success;
}

bool
RingController::BitRateRelease (
  uint16_t srcIdx, uint16_t dstIdx, int64_t fwdBitRate, int64_t bwdBitRate,
  RingInfo::RingPath path, SliceId slice)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << fwdBitRate <<
                   bwdBitRate << path << slice);

  // Walk through links in the given routing path, releasing the bit rate.
  LinkInfo::LinkDir fwdDir, bwdDir;
  Ptr<LinkInfo> lInfo;
  bool ok = true;
  while (ok && srcIdx != dstIdx)
    {
      uint16_t next = GetNextSwIdx (srcIdx, path);
      std::tie (lInfo, fwdDir, bwdDir) = GetLinkInfo (srcIdx, next);
      ok &= lInfo->UpdateResBitRate (fwdDir, slice, -fwdBitRate);
      ok &= lInfo->UpdateResBitRate (bwdDir, slice, -bwdBitRate);
      SlicingMeterAdjust (lInfo, slice);
      srcIdx = next;
    }

  NS_ASSERT_MSG (ok, "Error when releasing bit rate.");
  return ok;
}

void
RingController::CreateSpanningTree (void)
{
  NS_LOG_FUNCTION (this);

  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD) with OFPPC_NO_FWD config (0x20).
  uint16_t half = (GetNSwitches () / 2);
  Ptr<LinkInfo> lInfo =
    LinkInfo::GetPointer (GetDpId (half), GetDpId (half + 1));
  NS_LOG_DEBUG ("Disabling link from " << half << " to " <<
                half + 1 << " for broadcast messages.");
  {
    std::ostringstream cmd;
    cmd << "port-mod"
        << " port=" << lInfo->GetPortNo (0)
        << ",addr=" << lInfo->GetPortAddr (0)
        << ",conf=0x00000020,mask=0x00000020";
    DpctlExecute (lInfo->GetSwDpId (0), cmd.str ());
  }
  {
    std::ostringstream cmd;
    cmd << "port-mod"
        << " port=" << lInfo->GetPortNo (1)
        << ",addr=" << lInfo->GetPortAddr (1)
        << ",conf=0x00000020,mask=0x00000020";
    DpctlExecute (lInfo->GetSwDpId (1), cmd.str ());
  }
}

void
RingController::GetLinkSet (
  Ptr<RingInfo> ringInfo, LteIface iface, LinkInfoSet_t *links) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  NS_ASSERT_MSG (links && links->empty (), "Set of links should be empty.");

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  uint16_t curr = rInfo->GetSrcDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetDstDlInfraSwIdx (iface);
  RingInfo::RingPath path = ringInfo->GetDlPath (iface);

  // Walk through the downlink path.
  LinkInfo::LinkDir dlDir, ulDir;
  Ptr<LinkInfo> lInfo;
  while (curr != last)
    {
      uint16_t next = GetNextSwIdx (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      auto ret = links->insert (lInfo);
      NS_ABORT_MSG_IF (ret.second == false, "Error saving link info.");
      curr = next;
    }
}

uint16_t
RingController::GetNextSwIdx (
  uint16_t srcIdx, RingInfo::RingPath path) const
{
  NS_LOG_FUNCTION (this << srcIdx << path);

  NS_ASSERT_MSG (GetNSwitches () > 0, "Invalid number of switches.");
  NS_ASSERT_MSG (path != RingInfo::UNDEF, "Invalid ring routing path.");
  NS_ASSERT_MSG (path != RingInfo::LOCAL, "Invalid ring routing path.");

  return path == RingInfo::CLOCK ?
         (srcIdx + 1) % GetNSwitches () :
         (srcIdx == 0 ? GetNSwitches () - 1 : (srcIdx - 1));
}

uint16_t
RingController::GetNumHops (
  uint16_t srcIdx, uint16_t dstIdx, RingInfo::RingPath path) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx);

  NS_ASSERT_MSG (path != RingInfo::UNDEF, "Invalid ring routing path.");
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
  if (path == RingInfo::COUNT)
    {
      distance = srcIdx - dstIdx;
    }
  if (distance < 0)
    {
      distance += GetNSwitches ();
    }
  return distance;
}

RingInfo::RingPath
RingController::GetShortPath (
  uint16_t srcIdx, uint16_t dstIdx) const
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
         RingInfo::COUNT;
}

bool
RingController::HasAvailableResources (
  Ptr<RingInfo> ringInfo, LteIface iface, LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Check for the available resources on the default path.
  bool bwdOk = BitRateRequest (ringInfo, iface, overlap);
  bool cpuOk = SwitchCpuRequest (ringInfo, iface);
  bool tabOk = SwitchTableRequest (ringInfo, iface);
  if ((bwdOk == false || cpuOk == false || tabOk == false)
      && GetRoutingStrategy () == RingController::SPF)
    {
      // We don't have the resources in the default path.
      // Let's invert the routing path and check again.
      ringInfo->InvertPath (iface);
      bwdOk = BitRateRequest (ringInfo, iface, overlap);
      cpuOk = SwitchCpuRequest (ringInfo, iface);
      tabOk = SwitchTableRequest (ringInfo, iface);
    }

  // Set the blocked flagged when necessary.
  if (!bwdOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKBAND);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because at least one backhaul link is overloaded.");
    }
  if (!cpuOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKLOAD);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because at least one backhaul switch is overloaded.");
    }
  if (!tabOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKTABLE);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because at least one backhaul switch table is full.");
    }

  return (bwdOk && cpuOk && tabOk);
}

bool
RingController::RulesInstall (
  Ptr<RingInfo> ringInfo, LteIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  NS_ASSERT_MSG (!ringInfo->IsInstalled (iface), "OpenFlow rules installed.");

  // No rules to install for local-routing bearers.
  if (ringInfo->IsLocalPath (iface))
    {
      return true;
    }

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // -------------------------------------------------------------------------
  // Slice table -- [from higher to lower priority]
  //
  // Building the dpctl command.
  uint64_t cookie = CookieCreate (
      iface, rInfo->GetPriority (), rInfo->GetTeid ());
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",table="  << GetSliceTable (rInfo->GetSliceId ())
      << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
      << ",cookie=" << GetUint64Hex (cookie)
      << ",prio="   << rInfo->GetPriority ()
      << ",idle="   << rInfo->GetTimeout ();
  std::string cmdStr = cmd.str ();

  // Building the DSCP set field instruction.
  std::ostringstream dscp;
  if (rInfo->GetDscpValue ())
    {
      dscp << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
    }
  std::string dscpStr = dscp.str ();

  // Configuring downlink routing.
  if (rInfo->HasDlTraffic ())
    {
      RingInfo::RingPath dlPath = ringInfo->GetDlPath (iface);

      // Building the match string. Using GTP TEID to identify the bearer and
      // the IP destination address to identify the logical interface.
      std::ostringstream mat;
      mat << " eth_type="    << IPV4_PROT_NUM
          << ",ip_proto="    << UDP_PROT_NUM
          << ",ip_dst="      << rInfo->GetDstDlAddr (iface)
          << ",gtpu_teid="   << rInfo->GetTeidHex ();
      std::string matStr = mat.str ();

      // Building the instructions string.
      std::ostringstream ins;
      ins << " write:group=" << dlPath
          << " goto:"        << BANDW_TAB;
      std::string insStr = ins.str ();

      // Installing OpenFlow rules
      uint16_t curr = rInfo->GetSrcDlInfraSwIdx (iface);
      uint16_t last = rInfo->GetDstDlInfraSwIdx (iface);

      // DSCP rules just in the first switch.
      DpctlExecute (GetDpId (curr), cmdStr + matStr + dscpStr + insStr);
      curr = GetNextSwIdx (curr, dlPath);
      while (curr != last)
        {
          DpctlExecute (GetDpId (curr), cmdStr + matStr + insStr);
          curr = GetNextSwIdx (curr, dlPath);
        }
    }

  // Configuring uplink routing.
  if (rInfo->HasUlTraffic ())
    {
      RingInfo::RingPath ulPath = ringInfo->GetUlPath (iface);

      // Building the match string. Using GTP TEID to identify the bearer and
      // the IP destination address to identify the logical interface.
      std::ostringstream mat;
      mat << " eth_type="    << IPV4_PROT_NUM
          << ",ip_proto="    << UDP_PROT_NUM
          << ",ip_dst="      << rInfo->GetDstUlAddr (iface)
          << ",gtpu_teid="   << rInfo->GetTeidHex ();
      std::string matStr = mat.str ();

      // Building the instructions string.
      std::ostringstream ins;
      ins << " write:group=" << ulPath
          << " goto:"        << BANDW_TAB;
      std::string insStr = ins.str ();

      // Installing OpenFlow rules
      uint16_t curr = rInfo->GetSrcUlInfraSwIdx (iface);
      uint16_t last = rInfo->GetDstUlInfraSwIdx (iface);

      // DSCP rules just in the first switch.
      DpctlExecute (GetDpId (curr), cmdStr + matStr + dscpStr + insStr);
      curr = GetNextSwIdx (curr, ulPath);
      while (curr != last)
        {
          DpctlExecute (GetDpId (curr), cmdStr + matStr + insStr);
          curr = GetNextSwIdx (curr, ulPath);
        }
    }

  // Update the installed flag for this interface.
  ringInfo->SetInstalled (iface, true);
  return true;
}

bool
RingController::RulesRemove (
  Ptr<RingInfo> ringInfo, LteIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Building the dpctl command. Matching cookie for interface and TEID.
  uint64_t cookie = CookieCreate (iface, 0, rInfo->GetTeid ());
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",table="        << GetSliceTable (rInfo->GetSliceId ())
      << ",cookie="       << GetUint64Hex (cookie)
      << ",cookie_mask="  << GetUint64Hex (COOKIE_IFACE_TEID_MASK);
  std::string cmdStr = cmd.str ();

  RingInfo::RingPath dlPath = ringInfo->GetDlPath (iface);
  uint16_t curr = rInfo->GetSrcDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetDstDlInfraSwIdx (iface);
  while (curr != last)
    {
      DpctlExecute (GetDpId (curr), cmdStr);
      curr = GetNextSwIdx (curr, dlPath);
    }
  DpctlExecute (GetDpId (curr), cmdStr);

  // Update the installed flag for this interface.
  ringInfo->SetInstalled (iface, false);
  return true;
}

bool
RingController::RulesUpdate (
  Ptr<RingInfo> ringInfo, LteIface iface, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << iface << dstEnbInfo);

  NS_ASSERT_MSG (iface == LteIface::S1, "Only S1-U interface supported.");
  return true;
}

void
RingController::SetShortestPath (
  Ptr<RingInfo> ringInfo, LteIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  RingInfo::RingPath dlPath = GetShortPath (
      rInfo->GetSrcDlInfraSwIdx (iface),
      rInfo->GetDstDlInfraSwIdx (iface));
  ringInfo->SetShortDlPath (iface, dlPath);

  NS_LOG_DEBUG ("Bearer teid " << rInfo->GetTeidHex () <<
                " interface "  << LteIfaceStr (iface) <<
                " short path " << RingInfo::RingPathStr (dlPath));
}

void
RingController::SlicingMeterApply (
  Ptr<const RemoteSwitch> swtch, SliceId slice)
{
  NS_LOG_FUNCTION (this << swtch << slice);

  // Get the OpenFlow switch datapath ID.
  uint64_t swDpId = swtch->GetDpId ();

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Build the command string.
  // Using a low-priority rule for ALL slice.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",prio="       << (slice == SliceId::ALL ? 32 : 64)
      << ",table="      << BANDW_TAB
      << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET;

  // Install rules on each port direction (FWD and BWD).
  for (int d = 0; d <= LinkInfo::BWD; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);
      RingInfo::RingPath path = RingInfo::LinkDirToRingPath (dir);
      uint32_t meterId = MeterIdCreate (slice, d);

      // We are using the IP DSCP field to identify Non-GBR traffic.
      // Non-GBR QCIs range is [5, 9].
      for (int q = 5; q <= 9; q++)
        {
          EpsBearer::Qci qci = static_cast<EpsBearer::Qci> (q);
          Ipv4Header::DscpType dscp = Qci2Dscp (qci);

          // Build the match string.
          std::ostringstream mtc;
          mtc << " eth_type="   << IPV4_PROT_NUM
              << ",meta="       << path
              << ",ip_dscp="    << static_cast<uint16_t> (dscp)
              << ",ip_proto="   << UDP_PROT_NUM;
          if (slice != SliceId::ALL)
            {
              // Filter traffic for individual slices.
              mtc << ",gtpu_teid="  << (meterId & TEID_SLICE_MASK)
                  << "/"            << TEID_SLICE_MASK;
            }

          // Build the instructions string.
          std::ostringstream act;
          act << " meter:"      << meterId
              << " goto:"       << OUTPT_TAB;

          DpctlExecute (swDpId, cmd.str () + mtc.str () + act.str ());
        }
    }
}

bool
RingController::SwitchCpuRequest (
  Ptr<RingInfo> ringInfo, LteIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check when the BlockPolicy mode is OFF.
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  if (GetSwBlockPolicy () == OpMode::OFF)
    {
      return true;
    }

  return SwitchCpuRequest (
    rInfo->GetSrcDlInfraSwIdx (iface),
    rInfo->GetDstDlInfraSwIdx (iface),
    ringInfo->GetDlPath (iface),
    GetSwBlockThreshold ());
}

bool
RingController::SwitchCpuRequest (
  uint16_t srcIdx, uint16_t dstIdx, RingInfo::RingPath path,
  double blockThs) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path << blockThs);

  // Walk through switches in the given routing path, requesting for CPU.
  bool ok = true;
  while (ok && srcIdx != dstIdx)
    {
      ok &= (GetEwmaCpuUse (srcIdx) < blockThs);
      srcIdx = GetNextSwIdx (srcIdx, path);
    }
  ok &= (GetEwmaCpuUse (srcIdx) < blockThs);

  return ok;
}

bool
RingController::SwitchTableRequest (
  Ptr<RingInfo> ringInfo, LteIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check for aggregated bearers.
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  if (rInfo->IsAggregated ())
    {
      return true;
    }

  return SwitchTableRequest (
    rInfo->GetSrcDlInfraSwIdx (iface),
    rInfo->GetDstDlInfraSwIdx (iface),
    ringInfo->GetDlPath (iface),
    GetSwBlockThreshold (),
    GetSliceTable (rInfo->GetSliceId ()));
}

bool
RingController::SwitchTableRequest (
  uint16_t srcIdx, uint16_t dstIdx, RingInfo::RingPath path,
  double blockThs, uint16_t table) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path << blockThs << table);

  // Walk through switches in the given routing path, requesting for table.
  bool ok = true;
  while (ok && srcIdx != dstIdx)
    {
      ok &= (GetFlowTableUse (srcIdx, table) < blockThs);
      srcIdx = GetNextSwIdx (srcIdx, path);
    }
  ok &= (GetFlowTableUse (srcIdx, table) < blockThs);

  return ok;
}

} // namespace ns3
