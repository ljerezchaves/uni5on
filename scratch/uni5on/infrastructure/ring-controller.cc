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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <string>
#include "ring-controller.h"
#include "transport-network.h"
#include "../mano-apps/link-sharing.h"
#include "../metadata/bearer-info.h"
#include "../metadata/enb-info.h"
#include "../slices/slice-controller.h"

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
    .SetParent<TransportController> ()
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

  TransportController::DoDispose ();
}

bool
RingController::BearerRequest (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // Reset the shortest path for the S1-U interface (the handover procedure may
  // have changed the eNB switch index.)
  SetShortestPath (ringInfo, EpsIface::S1);

  // Part 1: Check for the available resources on the S5 interface.
  bool s5Ok = HasAvailableResources (ringInfo, EpsIface::S5);
  if (!s5Ok)
    {
      NS_ASSERT_MSG (bInfo->IsBlocked (), "This bearer should be blocked.");
      NS_LOG_WARN ("Blocking bearer teid " << bInfo->GetTeidHex () <<
                   " because there are no resources for the S5 interface.");
    }

  // Part 2: Check for the available resources on the S1-U interface.
  // To avoid errors when reserving bit rates, check for overlapping links.
  LinkInfoSet_t s5Links;
  GetLinkSet (ringInfo, EpsIface::S5, &s5Links);
  bool s1Ok = HasAvailableResources (ringInfo, EpsIface::S1, &s5Links);
  if (!s1Ok)
    {
      NS_ASSERT_MSG (bInfo->IsBlocked (), "This bearer should be blocked.");
      NS_LOG_WARN ("Blocking bearer teid " << bInfo->GetTeidHex () <<
                   " because there are no resources for the S1-U interface.");
    }

  return (s5Ok && s1Ok);
}

bool
RingController::BearerReserve (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo);

  NS_ASSERT_MSG (!bInfo->IsBlocked (), "Bearer should not be blocked.");
  NS_ASSERT_MSG (!bInfo->IsAggregated (), "Bearer should not be aggregated.");

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= BitRateReserve (ringInfo, EpsIface::S5);
  success &= BitRateReserve (ringInfo, EpsIface::S1);
  return success;
}

bool
RingController::BearerRelease (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo);

  NS_ASSERT_MSG (!bInfo->IsAggregated (), "Bearer should not be aggregated.");

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= BitRateRelease (ringInfo, EpsIface::S5);
  success &= BitRateRelease (ringInfo, EpsIface::S1);
  return success;
}

bool
RingController::BearerInstall (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  NS_ASSERT_MSG (bInfo->IsGwInstalled (), "Gateway rules not installed.");
  NS_LOG_INFO ("Installing ring rules for teid " << bInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= RulesInstall (ringInfo, EpsIface::S5);
  success &= RulesInstall (ringInfo, EpsIface::S1);
  return success;
}

bool
RingController::BearerRemove (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  NS_ASSERT_MSG (!bInfo->IsGwInstalled (), "Gateway rules installed.");
  NS_LOG_INFO ("Removing ring rules for teid " << bInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  bool success = true;
  success &= RulesRemove (ringInfo, EpsIface::S5);
  success &= RulesRemove (ringInfo, EpsIface::S1);
  return success;
}

bool
RingController::BearerUpdate (Ptr<BearerInfo> bInfo, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  NS_ASSERT_MSG (bInfo->IsGwInstalled (), "Gateway rules not installed.");
  NS_ASSERT_MSG (bInfo->GetEnbCellId () != dstEnbInfo->GetCellId (),
                 "Don't update UE's eNB info before BearerUpdate.");
  NS_LOG_INFO ("Updating ring rules for teid " << bInfo->GetTeidHex ());

  Ptr<RingInfo> ringInfo = bInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // Each slice has a single P-GW and S-GW, so handover only changes the eNB.
  // Thus, we only need to modify the S1-U routing rules.
  bool success = true;
  success &= RulesUpdate (ringInfo, EpsIface::S1, dstEnbInfo);
  return success;
}

void
RingController::NotifyBearerCreated (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  // Let's create its ring routing metadata.
  Ptr<RingInfo> ringInfo = CreateObject<RingInfo> (bInfo);

  // Set the downlink shortest path for both S1-U and S5 interfaces.
  SetShortestPath (ringInfo, EpsIface::S5);
  SetShortestPath (ringInfo, EpsIface::S1);

  TransportController::NotifyBearerCreated (bInfo);
}

void
RingController::NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices)
{
  NS_LOG_FUNCTION (this);

  // Chain up first, as we need to save the switch devices.
  TransportController::NotifyTopologyBuilt (devices);

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

  // TODO Differentiate between transport and eNB switch.

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // Skip slice classification for X2-C packets.
  // Route them always in the clockwise direction.
  // Write the output group into action set.
  // Send the packet directly to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=128"
        << ",table="          << CLASS_TAB
        << ",flags="          << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="       << IPV4_PROT_NUM
        << ",ip_proto="       << UDP_PROT_NUM
        << ",ip_dst="         << TransportNetwork::m_x2Addr
        << "/"                << TransportNetwork::m_x2Mask.GetPrefixLength ()
        << " write:group="    << RingInfo::CLOCK
        << " goto:"           << OUTPT_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  TransportController::HandshakeSuccessful (swtch);
}

bool
RingController::BitRateRequest (
  Ptr<RingInfo> ringInfo, EpsIface iface, LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface << overlap);

  // Ignoring this check for Non-GBR bearers, aggregated bearers,
  // and local-routing bearers.
  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  if (bInfo->IsNonGbr () || ringInfo->IsLocalPath (iface)
      || (bInfo->IsAggregated () && GetAggBitRateCheck () == OpMode::OFF))
    {
      return true;
    }

  return BitRateRequest (
    bInfo->GetSrcDlInfraSwIdx (iface),
    bInfo->GetDstDlInfraSwIdx (iface),
    bInfo->GetGbrDlBitRate (),
    bInfo->GetGbrUlBitRate (),
    ringInfo->GetDlPath (iface),
    bInfo->GetSliceId (),
    GetSliceController (bInfo->GetSliceId ())->GetGbrBlockThs (),
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
  Ptr<RingInfo> ringInfo, EpsIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  NS_ASSERT_MSG (!bInfo->IsBlocked (), "Bearer should not be blocked.");
  NS_ASSERT_MSG (!bInfo->IsAggregated (), "Bearer should not be aggregated.");
  NS_ASSERT_MSG (!bInfo->IsGbrReserved (iface), "Bit rate already reserved.");

  NS_LOG_INFO ("Reserving resources for teid " << bInfo->GetTeidHex () <<
               " on interface " << EpsIfaceStr (iface));

  // Ignoring bearers without guaranteed bit rate or local-routing bearers.
  if (!bInfo->HasGbrBitRate () || ringInfo->IsLocalPath (iface))
    {
      return true;
    }
  NS_ASSERT_MSG (bInfo->IsGbr (), "Non-GBR bearers should not get here.");

  bool success = BitRateReserve (
      bInfo->GetSrcDlInfraSwIdx (iface),
      bInfo->GetDstDlInfraSwIdx (iface),
      bInfo->GetGbrDlBitRate (),
      bInfo->GetGbrUlBitRate (),
      ringInfo->GetDlPath (iface),
      bInfo->GetSliceId ());
  bInfo->SetGbrReserved (iface, success);
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
      GetSharingApp ()->MeterAdjust (lInfo, slice);
      srcIdx = next;
    }

  NS_ASSERT_MSG (ok, "Error when reserving bit rate.");
  return ok;
}

bool
RingController::BitRateRelease (
  Ptr<RingInfo> ringInfo, EpsIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  NS_LOG_INFO ("Releasing resources for teid " << bInfo->GetTeidHex () <<
               " on interface " << EpsIfaceStr (iface));

  // Ignoring when there is no bit rate to release.
  if (!bInfo->IsGbrReserved (iface))
    {
      return true;
    }

  bool success = BitRateRelease (
      bInfo->GetSrcDlInfraSwIdx (iface),
      bInfo->GetDstDlInfraSwIdx (iface),
      bInfo->GetGbrDlBitRate (),
      bInfo->GetGbrUlBitRate (),
      ringInfo->GetDlPath (iface),
      bInfo->GetSliceId ());
  bInfo->SetGbrReserved (iface, !success);
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
      GetSharingApp ()->MeterAdjust (lInfo, slice);
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
  Ptr<RingInfo> ringInfo, EpsIface iface, LinkInfoSet_t *links) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  NS_ASSERT_MSG (links && links->empty (), "Set of links should be empty.");

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  uint16_t curr = bInfo->GetSrcDlInfraSwIdx (iface);
  uint16_t last = bInfo->GetDstDlInfraSwIdx (iface);
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
  Ptr<RingInfo> ringInfo, EpsIface iface, LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Check for the available resources on the default path.
  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
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
      bInfo->SetBlocked (BearerInfo::BRTPNBWD);
      NS_LOG_WARN ("Blocking bearer teid " << bInfo->GetTeidHex () <<
                   " because at least one transport link is overloaded.");
    }
  if (!cpuOk)
    {
      bInfo->SetBlocked (BearerInfo::BRTPNCPU);
      NS_LOG_WARN ("Blocking bearer teid " << bInfo->GetTeidHex () <<
                   " because at least one transport switch is overloaded.");
    }
  if (!tabOk)
    {
      bInfo->SetBlocked (BearerInfo::BRTPNTAB);
      NS_LOG_WARN ("Blocking bearer teid " << bInfo->GetTeidHex () <<
                   " because at least one transport switch table is full.");
    }

  return (bwdOk && cpuOk && tabOk);
}

bool
RingController::RulesInstall (
  Ptr<RingInfo> ringInfo, EpsIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  NS_ASSERT_MSG (!bInfo->IsIfInstalled (iface), "Ring rules installed.");
  bool success = true;

  // No rules to install for local-routing bearers.
  if (ringInfo->IsLocalPath (iface))
    {
      return true;
    }

  // -------------------------------------------------------------------------
  // Slice table -- [from higher to lower priority]
  //
  // Cookie and MBR meter ID for new rules.
  uint32_t mbrMeterId = GlobalIds::MeterIdMbrCreate (iface, bInfo->GetTeid ());
  uint64_t cookie = GlobalIds::CookieCreate (
      iface, bInfo->GetPriority (), bInfo->GetTeid ());

  // Building the dpctl command.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",table="  << GetSliceTable (bInfo->GetSliceId ())
      << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
      << ",cookie=" << GetUint64Hex (cookie)
      << ",prio="   << bInfo->GetPriority ()
      << ",idle="   << bInfo->GetTimeout ();
  std::string cmdStr = cmd.str ();

  // Configuring downlink routing.
  if (bInfo->HasDlTraffic ())
    {
      if (bInfo->HasMbrDl ())
        {
          NS_ASSERT_MSG (!bInfo->IsMbrDlInstalled (iface), "Meter installed.");

          // Install downlink MBR meter entry on the input switch.
          std::ostringstream met;
          met << "meter-mod cmd=add,flags=1,meter=" << mbrMeterId
              << " drop:rate=" << bInfo->GetMbrDlBitRate () / 1000;
          std::string metStr = met.str ();

          DpctlExecute (GetDpId (bInfo->GetSrcDlInfraSwIdx (iface)), metStr);
          bInfo->SetMbrDlInstalled (iface, true);
        }

      success &= RulesInstall (
          bInfo->GetSrcDlInfraSwIdx (iface),
          bInfo->GetDstDlInfraSwIdx (iface),
          ringInfo->GetDlPath (iface),
          bInfo->GetTeid (),
          bInfo->GetDstDlAddr (iface),
          bInfo->GetDscpValue (),
          bInfo->IsMbrDlInstalled (iface) ? mbrMeterId : 0,
          cmdStr);
    }

  // Configuring uplink routing.
  if (bInfo->HasUlTraffic ())
    {
      if (bInfo->HasMbrUl ())
        {
          NS_ASSERT_MSG (!bInfo->IsMbrUlInstalled (iface), "Meter installed.");

          // Install uplink MBR meter entry on the input switch.
          std::ostringstream met;
          met << "meter-mod cmd=add,flags=1,meter=" << mbrMeterId
              << " drop:rate=" << bInfo->GetMbrUlBitRate () / 1000;
          std::string metStr = met.str ();

          DpctlExecute (GetDpId (bInfo->GetSrcUlInfraSwIdx (iface)), metStr);
          bInfo->SetMbrUlInstalled (iface, true);
        }

      success &= RulesInstall (
          bInfo->GetSrcUlInfraSwIdx (iface),
          bInfo->GetDstUlInfraSwIdx (iface),
          ringInfo->GetUlPath (iface),
          bInfo->GetTeid (),
          bInfo->GetDstUlAddr (iface),
          bInfo->GetDscpValue (),
          bInfo->IsMbrUlInstalled (iface) ? mbrMeterId : 0,
          cmdStr);
    }

  // Update the installed flag for this interface.
  bInfo->SetIfInstalled (iface, success);
  return success;
}

bool
RingController::RulesInstall (
  uint16_t srcIdx, uint16_t dstIdx, RingInfo::RingPath path, uint32_t teid,
  Ipv4Address dstAddr, uint16_t dscp, uint32_t meter, std::string cmdStr)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << path << teid <<
                   dstAddr << dscp << meter << cmdStr);

  NS_ASSERT_MSG (srcIdx != dstIdx, "Can't install rules for local routing.");

  // Building the match string (using GTP TEID to identify the bearer and
  // the IP destination address to identify the logical interface).
  std::ostringstream mat;
  mat << " eth_type="   << IPV4_PROT_NUM
      << ",ip_proto="   << UDP_PROT_NUM
      << ",ip_dst="     << dstAddr
      << ",gtpu_teid="  << GetUint32Hex (teid);
  std::string matStr = mat.str ();

  // Building the instructions string for the first switch.
  std::ostringstream ins1;
  if (meter)
    {
      ins1 << " meter:" << meter;
    }
  if (dscp)
    {
      ins1 << " apply:set_field=ip_dscp:" << dscp;
    }
  std::string ins1Str = ins1.str ();

  // Building the instructions string for all switches.
  std::ostringstream ins;
  ins << " write:group=" << path
      << " meta:"        << path
      << " goto:"        << BANDW_TAB;
  std::string insStr = ins.str ();

  // Installing OpenFlow routing rules.
  DpctlExecute (GetDpId (srcIdx), cmdStr + matStr + ins1Str + insStr);
  srcIdx = GetNextSwIdx (srcIdx, path);
  while (srcIdx != dstIdx)
    {
      DpctlExecute (GetDpId (srcIdx), cmdStr + matStr + insStr);
      srcIdx = GetNextSwIdx (srcIdx, path);
    }
  return true;
}

bool
RingController::RulesRemove (
  Ptr<RingInfo> ringInfo, EpsIface iface)
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // No rules installed for this interface.
  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  if (!bInfo->IsIfInstalled (iface))
    {
      return true;
    }

  // Building the dpctl command. Matching cookie for interface and TEID.
  uint64_t cookie = GlobalIds::CookieCreate (iface, 0, bInfo->GetTeid ());
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",table="        << GetSliceTable (bInfo->GetSliceId ())
      << ",cookie="       << GetUint64Hex (cookie)
      << ",cookie_mask="  << GetUint64Hex (COOKIE_IFACE_TEID_MASK);
  std::string cmdStr = cmd.str ();

  RingInfo::RingPath dlPath = ringInfo->GetDlPath (iface);
  uint16_t curr = bInfo->GetSrcDlInfraSwIdx (iface);
  uint16_t last = bInfo->GetDstDlInfraSwIdx (iface);
  while (curr != last)
    {
      DpctlExecute (GetDpId (curr), cmdStr);
      curr = GetNextSwIdx (curr, dlPath);
    }
  DpctlExecute (GetDpId (curr), cmdStr);

  // Remove installed MBR meter entries.
  if (bInfo->HasMbr ())
    {
      uint32_t mbrMeterId = GlobalIds::MeterIdMbrCreate (iface, bInfo->GetTeid ());

      std::ostringstream met;
      met << "meter-mod cmd=del,meter=" << mbrMeterId;
      std::string metStr = met.str ();

      if (bInfo->IsMbrDlInstalled (iface))
        {
          DpctlExecute (GetDpId (bInfo->GetSrcDlInfraSwIdx (iface)), metStr);
          bInfo->SetMbrDlInstalled (iface, false);
        }
      if (bInfo->IsMbrUlInstalled (iface))
        {
          DpctlExecute (GetDpId (bInfo->GetSrcUlInfraSwIdx (iface)), metStr);
          bInfo->SetMbrUlInstalled (iface, false);
        }
    }

  // Update the installed flag for this interface.
  bInfo->SetIfInstalled (iface, false);
  return true;
}

bool
RingController::RulesUpdate (
  Ptr<RingInfo> ringInfo, EpsIface iface, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << iface << dstEnbInfo);

  NS_ASSERT_MSG (iface == EpsIface::S1, "Only S1-U interface supported.");

  // During this procedure, the eNB was not updated in the bInfo yet.
  // So, the following methods will return information for the old eNB.
  // bInfo->GetEnbCellId ()                   // eNB cell ID
  // bInfo->GetEnbInfraSwIdx ()               // eNB switch index
  // bInfo->GetDstDlInfraSwIdx (EpsIface::S1) // eNB switch index
  // bInfo->GetSrcUlInfraSwIdx (EpsIface::S1) // eNB switch index
  // bInfo->GetEnbS1uAddr ()                  // eNB S1-U address
  // bInfo->GetDstDlAddr (EpsIface::S1)       // eNB S1-U address
  // bInfo->GetSrcUlAddr (EpsIface::S1)       // eNB S1-U address
  //
  // We can't just modify the OpenFlow rules in the transport switches because
  // we need to change the match fields. So, we will schedule the removal of
  // old low-priority rules from the old routing path and install new rules in
  // the new routing path (may be the same), using a higher priority and the
  // dstEnbInfo metadata.

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();

  // MBR meter ID for this bearer (won't change on update).
  uint32_t mbrMeterId = GlobalIds::MeterIdMbrCreate (iface, bInfo->GetTeid ());
  bool success = true;

  // Schedule the removal of old low-priority OpenFlow rules.
  if (bInfo->IsIfInstalled (iface))
    {
      // Cookie for old rules. Using old low-priority.
      uint64_t oldCookie = GlobalIds::CookieCreate (
          iface, bInfo->GetPriority (), bInfo->GetTeid ());

      // Building the dpctl command. Strict matching cookie.
      std::ostringstream del;
      del << "flow-mod cmd=del"
          << ",table="        << GetSliceTable (bInfo->GetSliceId ())
          << ",cookie="       << GetUint64Hex (oldCookie)
          << ",cookie_mask="  << GetUint64Hex (COOKIE_STRICT_MASK);
      std::string delStr = del.str ();

      // Walking through the old S1-U downlink path.
      RingInfo::RingPath dlPath = ringInfo->GetDlPath (iface);
      uint16_t curr = bInfo->GetSgwInfraSwIdx ();
      uint16_t last = bInfo->GetEnbInfraSwIdx ();
      while (curr != last)
        {
          DpctlSchedule (MilliSeconds (250), GetDpId (curr), delStr);
          curr = GetNextSwIdx (curr, dlPath);
        }
      DpctlSchedule (MilliSeconds (250), GetDpId (curr), delStr);

      // Update the installation flag.
      bInfo->SetIfInstalled (iface, false);
    }

  // When changing the switch index, we must release any possible reserved bit
  // rate from the old path, update the ring routing path to the new (shortest)
  // one, and reserve the bit rate on the new path. For bearers with MBR meter,
  // also remove it from the old switch and install it into the new switch.
  if (bInfo->GetEnbInfraSwIdx () != dstEnbInfo->GetInfraSwIdx ())
    {
      // Release the bit rate from the old path.
      if (bInfo->IsGbrReserved (iface))
        {
          bool ok = BitRateRelease (
              bInfo->GetSgwInfraSwIdx (),
              bInfo->GetEnbInfraSwIdx (),
              bInfo->GetGbrDlBitRate (),
              bInfo->GetGbrUlBitRate (),
              ringInfo->GetDlPath (iface),
              bInfo->GetSliceId ());
          bInfo->SetGbrReserved (iface, !ok);
        }

      // Update the new shortest path from the S-GW to the target eNB.
      RingInfo::RingPath newDlPath = GetShortPath (
          bInfo->GetSgwInfraSwIdx (),
          dstEnbInfo->GetInfraSwIdx ());
      ringInfo->SetShortDlPath (iface, newDlPath);

      // Try to reserve the bit rate on the new path.
      if (bInfo->HasGbrBitRate ())
        {
          // Check for the available bit rate in the new path and reserve it.
          // There's no need to check for overlapping paths as the bit rate for
          // the S5 interface is already reserved.
          bool hasBitRate = BitRateRequest (
              bInfo->GetSgwInfraSwIdx (),
              dstEnbInfo->GetInfraSwIdx (),         // Target eNB switch idx.
              bInfo->GetGbrDlBitRate (),
              bInfo->GetGbrUlBitRate (),
              ringInfo->GetDlPath (iface),          // New downlink path.
              bInfo->GetSliceId (),
              GetSliceController (bInfo->GetSliceId ())->GetGbrBlockThs ());
          if (hasBitRate)
            {
              bool ok = BitRateReserve (
                  bInfo->GetSgwInfraSwIdx (),
                  dstEnbInfo->GetInfraSwIdx (),     // Target eNB switch idx.
                  bInfo->GetGbrDlBitRate (),
                  bInfo->GetGbrUlBitRate (),
                  ringInfo->GetDlPath (iface),      // New downlink path.
                  bInfo->GetSliceId ());
              bInfo->SetGbrReserved (iface, ok);
            }
        }

      // Remove the MBR meters from the old switches.
      if (bInfo->HasMbr ())
        {
          std::ostringstream del;
          del << "meter-mod cmd=del,meter=" << mbrMeterId;
          std::string delStr = del.str ();

          // In the uplink, the eNB switch will change for sure (we've already
          // tested it!). So, schedule the removal of MBR meters from the old
          // eNB switch.
          if (bInfo->IsMbrUlInstalled (iface))
            {
              DpctlSchedule (MilliSeconds (300),
                             GetDpId (bInfo->GetEnbInfraSwIdx ()), delStr);
              bInfo->SetMbrUlInstalled (iface, false);
            }

          // In the downlink, the S-GW switch won't change. But, there's the
          // speciall case when the new routing path becomes a local one and we
          // must remove the meter. This must be checked here, after updating
          // the new shortest path from the S-GW to the target eNB.
          if (bInfo->IsMbrDlInstalled (iface) && ringInfo->IsLocalPath (iface))
            {
              DpctlSchedule (MilliSeconds (300),
                             GetDpId (bInfo->GetSgwInfraSwIdx ()), delStr);
              bInfo->SetMbrDlInstalled (iface, false);
            }
        }
    }

  // Install new high-priority OpenFlow rules for non-local routing paths.
  if (!ringInfo->IsLocalPath (iface))
    {
      // Cookie for new rules. Using new high-priority.
      uint64_t newCookie = GlobalIds::CookieCreate (
          iface, bInfo->GetPriority () + 1, bInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add"
          << ",table="  << GetSliceTable (bInfo->GetSliceId ())
          << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (newCookie)
          << ",prio="   << bInfo->GetPriority () + 1
          << ",idle="   << bInfo->GetTimeout ();
      std::string cmdStr = cmd.str ();

      // Configuring downlink routing.
      if (bInfo->HasDlTraffic ())
        {
          if (bInfo->HasMbrDl () && !bInfo->IsMbrDlInstalled (iface))
            {
              // Install downlink MBR meter entry on the input switch.
              std::ostringstream met;
              met << "meter-mod cmd=add,flags=1,meter=" << mbrMeterId
                  << " drop:rate=" << bInfo->GetMbrDlBitRate () / 1000;
              std::string metStr = met.str ();

              DpctlExecute (GetDpId (bInfo->GetSgwInfraSwIdx ()), metStr);
              bInfo->SetMbrDlInstalled (iface, true);
            }

          success &= RulesInstall (
              bInfo->GetSgwInfraSwIdx (),
              dstEnbInfo->GetInfraSwIdx (),         // Target eNB switch idx.
              ringInfo->GetDlPath (iface),          // New downlink path.
              bInfo->GetTeid (),
              dstEnbInfo->GetS1uAddr (),            // Target eNB address.
              bInfo->GetDscpValue (),
              bInfo->IsMbrDlInstalled (iface) ? mbrMeterId : 0,
              cmdStr);
        }

      // Configuring uplink routing.
      if (bInfo->HasUlTraffic ())
        {
          if (bInfo->HasMbrUl () && !bInfo->IsMbrUlInstalled (iface))
            {
              // Install uplink MBR meter entry on the input switch.
              std::ostringstream met;
              met << "meter-mod cmd=add,flags=1,meter=" << mbrMeterId
                  << " drop:rate=" << bInfo->GetMbrUlBitRate () / 1000;
              std::string metStr = met.str ();

              DpctlExecute (GetDpId (dstEnbInfo->GetInfraSwIdx ()), metStr);
              bInfo->SetMbrUlInstalled (iface, true);
            }

          success &= RulesInstall (
              dstEnbInfo->GetInfraSwIdx (),         // Target eNB switch idx.
              bInfo->GetSgwInfraSwIdx (),
              ringInfo->GetUlPath (iface),          // New uplink path.
              bInfo->GetTeid (),
              bInfo->GetSgwS1uAddr (),
              bInfo->GetDscpValue (),
              bInfo->IsMbrUlInstalled (iface) ? mbrMeterId : 0,
              cmdStr);
        }

      // Update the installed flag for this interface.
      bInfo->SetIfInstalled (iface, success);
    }

  return success;
}

void
RingController::SetShortestPath (
  Ptr<RingInfo> ringInfo, EpsIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  RingInfo::RingPath dlPath = GetShortPath (
      bInfo->GetSrcDlInfraSwIdx (iface),
      bInfo->GetDstDlInfraSwIdx (iface));
  ringInfo->SetShortDlPath (iface, dlPath);

  NS_LOG_DEBUG ("Bearer teid " << bInfo->GetTeidHex () <<
                " interface "  << EpsIfaceStr (iface) <<
                " short path " << RingInfo::RingPathStr (dlPath));
}

void
RingController::SharingMeterApply (
  uint64_t swDpId, LinkInfo::LinkDir dir, SliceId slice)
{
  NS_LOG_FUNCTION (this << swDpId << dir << slice);

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Apply the Non-GBR meter band.
  // Send the packet to the output table.

  // Build the command string.
  // Using a low-priority rule for ALL slice.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",prio="       << (slice == SliceId::ALL ? 32 : 64)
      << ",table="      << BANDW_TAB
      << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET;

  uint32_t meterId = GlobalIds::MeterIdSlcCreate (slice, dir);

  // We are using the IP DSCP field to identify Non-GBR traffic.
  for (auto &qci : GetNonGbrQcis ())
    {
      Ipv4Header::DscpType dscp = Qci2Dscp (qci);

      // Build the match string.
      std::ostringstream mtc;
      mtc << " eth_type="   << IPV4_PROT_NUM
          << ",meta="       << RingInfo::LinkDirToRingPath (dir)
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

  // Chain up
  TransportController::SharingMeterApply (swDpId, dir, slice);
}

bool
RingController::SwitchCpuRequest (
  Ptr<RingInfo> ringInfo, EpsIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check when the BlockPolicy mode is OFF.
  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  if (GetSwBlockPolicy () == OpMode::OFF)
    {
      return true;
    }

  return SwitchCpuRequest (
    bInfo->GetSrcDlInfraSwIdx (iface),
    bInfo->GetDstDlInfraSwIdx (iface),
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
  Ptr<RingInfo> ringInfo, EpsIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check for aggregated bearers.
  Ptr<BearerInfo> bInfo = ringInfo->GetBearerInfo ();
  if (bInfo->IsAggregated ())
    {
      return true;
    }

  return SwitchTableRequest (
    bInfo->GetSrcDlInfraSwIdx (iface),
    bInfo->GetDstDlInfraSwIdx (iface),
    ringInfo->GetDlPath (iface),
    GetSwBlockThreshold (),
    GetSliceTable (bInfo->GetSliceId ()));
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
