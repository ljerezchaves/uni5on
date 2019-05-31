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

  // Reset the shortest path for both S1-U and S5 interfaces.
  // Routing paths may change after handover procedures.
  SetShortestPath (ringInfo);

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
  GetLinks (ringInfo, LteIface::S5, &s5Links);
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

  // The only resource that is reserved is the GBR bit rate over links.
  // For Non-GBR bearers (which includes the default bearer) and for bearers
  // that only transverse the local switch (local routing for both S1-U and S5
  // interfaces): there's no GBR bit rate to reserve.
  if (rInfo->IsNonGbr () || ringInfo->AreLocalPaths ()) // FIXME Verificar se e local na funcao
    {
      return true;
    }
  return BitRateReserve (ringInfo);
}

bool
RingController::BearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");

  // For bearers without reserved resources: nothing to release.
  if (!rInfo->IsGbrReserved ())
    {
      return true;
    }

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");
  return BitRateRelease (ringInfo);
}

bool
RingController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // -------------------------------------------------------------------------
  // Slice table -- [from higher to lower priority]
  //
  NS_LOG_INFO ("Installing ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Building the dpctl command.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",table="  << GetSliceTable (rInfo->GetSliceId ())
      << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",prio="   << rInfo->GetPriority ()
      << ",idle="   << rInfo->GetTimeout ();

  // Building the DSCP set field instruction.
  std::ostringstream dscp;
  if (rInfo->GetDscpValue ())
    {
      dscp << " apply:set_field=ip_dscp:" << rInfo->GetDscpValue ();
    }

  // Configuring downlink routing.
  if (rInfo->HasDlTraffic ())
    {
      RingInfo::RingPath s5DownPath = ringInfo->GetDlPath (LteIface::S5);
      RingInfo::RingPath s1DownPath = ringInfo->GetDlPath (LteIface::S1);

      // Building the match string for both S1-U and S5 interfaces.
      // Using GTP TEID to identify the bearer and the IP destination address
      // to identify the logical interface.
      std::ostringstream mS5, mS1;
      mS5 << " eth_type="    << IPV4_PROT_NUM
          << ",ip_proto="    << UDP_PROT_NUM
          << ",ip_dst="      << rInfo->GetSgwS5Addr ()
          << ",gtpu_teid="   << rInfo->GetTeidHex ();
      mS1 << " eth_type="    << IPV4_PROT_NUM
          << ",ip_proto="    << UDP_PROT_NUM
          << ",ip_dst="      << rInfo->GetEnbS1uAddr ()
          << ",gtpu_teid="   << rInfo->GetTeidHex ();

      // Build the instructions string.
      std::ostringstream iS5, iS1;
      iS5 << " write:group=" << s5DownPath
          << " goto:"        << BANDW_TAB;
      iS1 << " write:group=" << s1DownPath
          << " goto:"        << BANDW_TAB;

      // Installing down rules for S5 interface (from P-GW to S-GW)
      if (!ringInfo->IsLocalPath (LteIface::S5))
        {
          // Rules for the switch connected to the P-GW with DSCP instruction.
          uint16_t curr = rInfo->GetPgwInfraSwIdx ();
          DpctlExecute (GetDpId (curr),
                        cmd.str () + mS5.str () + dscp.str () + iS5.str ());

          // Rules for other switches without DSCP instruction.
          curr = NextSwitchIndex (curr, s5DownPath);
          while (curr != rInfo->GetSgwInfraSwIdx ())
            {
              DpctlExecute (GetDpId (curr),
                            cmd.str () + mS5.str () + iS5.str ());
              curr = NextSwitchIndex (curr, s5DownPath);
            }
        }

      // Installing down rules for S1-U interface (from S-GW to eNB)
      if (!ringInfo->IsLocalPath (LteIface::S1))
        {
          // Rules for the switch connected to the S-GW with DSCP instruction.
          uint16_t curr = rInfo->GetSgwInfraSwIdx ();
          DpctlExecute (GetDpId (curr),
                        cmd.str () + mS1.str () + dscp.str () + iS1.str ());

          // Rules for other switches without DSCP instruction.
          curr = NextSwitchIndex (curr, s1DownPath);
          while (curr != rInfo->GetEnbInfraSwIdx ())
            {
              DpctlExecute (GetDpId (curr),
                            cmd.str () + mS1.str () + iS1.str ());
              curr = NextSwitchIndex (curr, s1DownPath);
            }
        }
    }

  // Configuring uplink routing.
  if (rInfo->HasUlTraffic ())
    {
      RingInfo::RingPath s1UpPath = ringInfo->GetUlPath (LteIface::S1);
      RingInfo::RingPath s5UpPath = ringInfo->GetUlPath (LteIface::S5);

      // Building the match string for both S1-U and S5 interfaces.
      // Using GTP TEID to identify the bearer and the IP destination address
      // to identify the logical interface.
      std::ostringstream mS1, mS5;
      mS1 << " eth_type="   << IPV4_PROT_NUM
          << ",ip_proto="   << UDP_PROT_NUM
          << ",ip_dst="     << rInfo->GetSgwS1uAddr ()
          << ",gtpu_teid="  << rInfo->GetTeidHex ();
      mS5 << " eth_type="   << IPV4_PROT_NUM
          << ",ip_proto="   << UDP_PROT_NUM
          << ",ip_dst="     << rInfo->GetPgwS5Addr ()
          << ",gtpu_teid="  << rInfo->GetTeidHex ();

      // Build the instructions string.
      std::ostringstream iS1, iS5;
      iS1 << " write:group=" << s1UpPath
          << " goto:"        << BANDW_TAB;
      iS5 << " write:group=" << s5UpPath
          << " goto:"        << BANDW_TAB;

      // Installing up rules for S1-U interface (from eNB to S-GW)
      if (!ringInfo->IsLocalPath (LteIface::S1))
        {
          // Rules for the switch connected to the eNB with DSCP instruction.
          uint16_t curr = rInfo->GetEnbInfraSwIdx ();
          DpctlExecute (GetDpId (curr),
                        cmd.str () + mS1.str () + dscp.str () + iS1.str ());

          // Rules for other switches without DSCP instruction.
          curr = NextSwitchIndex (curr, s1UpPath);
          while (curr != rInfo->GetSgwInfraSwIdx ())
            {
              DpctlExecute (GetDpId (curr),
                            cmd.str () + mS1.str () + iS1.str ());
              curr = NextSwitchIndex (curr, s1UpPath);
            }
        }

      // Installing up rules for S5 interface (from S-GW to P-GW)
      if (!ringInfo->IsLocalPath (LteIface::S5))
        {
          // Rules for the switch connected to the S-GW with DSCP instruction.
          uint16_t curr = rInfo->GetSgwInfraSwIdx ();
          DpctlExecute (GetDpId (curr),
                        cmd.str () + mS5.str () + dscp.str () + iS5.str ());

          // Rules for other switches without DSCP instruction.
          curr = NextSwitchIndex (curr, s5UpPath);
          while (curr != rInfo->GetPgwInfraSwIdx ())
            {
              DpctlExecute (GetDpId (curr),
                            cmd.str () + mS5.str () + iS5.str ());
              curr = NextSwitchIndex (curr, s5UpPath);
            }
        }
    }
  return true;
}

bool
RingController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsInstalled (), "Rules must be installed.");
  NS_LOG_INFO ("Removing ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",table="        << GetSliceTable (rInfo->GetSliceId ())
      << ",cookie="       << rInfo->GetTeidHex ()
      << ",cookie_mask="  << COOKIE_STRICT_MASK_STR;

  // Removing rules from all switches in the path from P-GW to eNB.
  RingInfo::RingPath downPath = ringInfo->GetDlPath (LteIface::S5);
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  while (curr != rInfo->GetSgwInfraSwIdx ())
    {
      DpctlExecute (GetDpId (curr), cmd.str ());
      curr = NextSwitchIndex (curr, downPath);
    }
  downPath = ringInfo->GetDlPath (LteIface::S1);
  while (curr != rInfo->GetEnbInfraSwIdx ())
    {
      DpctlExecute (GetDpId (curr), cmd.str ());
      curr = NextSwitchIndex (curr, downPath);
    }
  DpctlExecute (GetDpId (curr), cmd.str ());

  return true;
}

bool
RingController::BearerUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // FIXME
  NS_ABORT_MSG ("This method is not working yet.");

  // FIXME Ao checkar recursos no S1,
  // nao esquecer de ringInfo->ResetS5Links (); antes
  // O melhor é dar um jeito de fazer isso mesmo se nao tiver recursos.

  NS_ASSERT_MSG (rInfo->IsInstalled (), "Rules must be installed.");
  NS_ASSERT_MSG (rInfo->GetEnbCellId () != dstEnbInfo->GetCellId (),
                 "Don't update UE's eNB info before BearerUpdate.");

  NS_LOG_INFO ("Updating ring rules for teid " << rInfo->GetTeidHex ());

  // Getting ring routing information.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();

  // We can't just modify the OpenFlow rules in the backhaul switches because
  // we need to change the match fields. So, we will install new rules using
  // the rInfo with higher priority and the dstEnbInfo, and then we will remove
  // the old rules with lower priority.
  //
  // Install new high-priority OpenFlow rules.
  {
    // Building the dpctl command.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add"
        << ",table="  << GetSliceTable (rInfo->GetSliceId ())
        << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
        << ",cookie=" << rInfo->GetTeidHex ()
        << ",prio="   << rInfo->GetPriority ()
        << ",idle="   << rInfo->GetTimeout ();

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
        mS5 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_dst="     << rInfo->GetSgwS5Addr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();
        mS1 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_dst="     << dstEnbInfo->GetS1uAddr ()  // Target eNB
            << ",gtpu_teid="  << rInfo->GetTeidHex ();

        // Build the metatada and goto instructions string.
        std::ostringstream aS5, aS1;
        aS5 << " meta:" << ringInfo->GetDlPath (LteIface::S5)
            << " goto:" << BANDW_TAB;
        aS1 << " meta:" << ringInfo->GetDlPath (LteIface::S1)
            << " goto:" << BANDW_TAB;

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
        mS1 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_src="     << dstEnbInfo->GetS1uAddr ()  // Target eNB
            << ",ip_dst="     << rInfo->GetSgwS1uAddr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();
        mS5 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_src="     << rInfo->GetSgwS5Addr ()
            << ",ip_dst="     << rInfo->GetPgwS5Addr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();

        // Build the metatada and goto instructions string.
        std::ostringstream aS1, aS5;
        aS1 << " meta:" << ringInfo->GetUlPath (LteIface::S1)
            << " goto:" << BANDW_TAB;
        aS5 << " meta:" << ringInfo->GetUlPath (LteIface::S5)
            << " goto:" << BANDW_TAB;

        // Installing up rules into switches connected to the eNB and S-GW.
        DpctlExecute (GetDpId (dstEnbInfo->GetInfraSwIdx ()),  // Target eNB
                      cmd.str () + mS1.str () + dscp.str () + aS1.str ());
        DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()),
                      cmd.str () + mS5.str () + dscp.str () + aS5.str ());
      }
  }

  // Remove old low-priority OpenFlow rules.
  {
    // Building the dpctl command.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=dels"
        << ",table="  << GetSliceTable (rInfo->GetSliceId ())
        << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
        << ",cookie=" << rInfo->GetTeidHex ()
        << ",prio="   << rInfo->GetPriority () - 1 // Old priority!
        << ",idle="   << rInfo->GetTimeout ();

    // Configuring downlink routing.
    if (rInfo->HasDlTraffic ())
      {
        // Building the match string for both S1-U and S5 interfaces
        // No match on source IP because we may have several P-GW TFT switches.
        std::ostringstream mS5, mS1;
        mS5 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_dst="     << rInfo->GetSgwS5Addr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();
        mS1 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_dst="     << rInfo->GetEnbS1uAddr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();

        // Removing down rules from switches connected to the P-GW and S-GW.
        DpctlExecute (GetDpId (rInfo->GetPgwInfraSwIdx ()),
                      cmd.str () + mS5.str ());
        DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()),
                      cmd.str () + mS1.str ());
      }

    // Configuring uplink routing.
    if (rInfo->HasUlTraffic ())
      {
        // Building the match string.
        std::ostringstream mS1, mS5;
        mS1 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_src="     << rInfo->GetEnbS1uAddr ()
            << ",ip_dst="     << rInfo->GetSgwS1uAddr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();
        mS5 << " eth_type="   << IPV4_PROT_NUM
            << ",ip_proto="   << UDP_PROT_NUM
            << ",ip_src="     << rInfo->GetSgwS5Addr ()
            << ",ip_dst="     << rInfo->GetPgwS5Addr ()
            << ",gtpu_teid="  << rInfo->GetTeidHex ();

        // Removing up rules from switches connected to the eNB and S-GW.
        DpctlExecute (GetDpId (rInfo->GetEnbInfraSwIdx ()),
                      cmd.str () + mS1.str ());
        DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()),
                      cmd.str () + mS5.str ());
      }
  }
  return true;
}

void
RingController::NotifyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // Let's create its ring routing metadata.
  Ptr<RingInfo> ringInfo = CreateObject<RingInfo> (rInfo);

  // Set the downlink shortest path for both S1-U and S5 interfaces.
  SetShortestPath (ringInfo);

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
RingController::HasAvailableResources (Ptr<RingInfo> ringInfo) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  bool success = true;
  success &= BwBearerRequest (ringInfo);
  success &= SwBearerRequest (ringInfo);
  return success;
}

bool
RingController::HasAvailableResources (
  Ptr<RingInfo> ringInfo, LteIface iface, LinkInfoSet_t *overlap) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Check for the available resources on the default path.
  bool bwdOk = HasLinkBitRate (ringInfo, iface, overlap);
  bool cpuOk = HasSwitchCpu (ringInfo, iface);
  bool tabOk = HasSwitchTable (ringInfo, iface);
  if ((bwdOk == false || cpuOk == false || tabOk == false)
      && GetRoutingStrategy () == RingController::SPF)
    {
      // We don't have the resources in the default path.
      // Let's invert the routing path and check again.
      ringInfo->InvertPath (iface);
      bwdOk = HasLinkBitRate (ringInfo, iface, overlap);
      cpuOk = HasSwitchCpu (ringInfo, iface);
      tabOk = HasSwitchTable (ringInfo, iface);
    }

  // Set the blocked flagged when necessary.
  if (!bwdOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKBAND);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because tat least one backhaul link is overloaded.");
    }
  if (!cpuOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKLOAD);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because tat least one backhaul switch is overloaded.");
    }
  if (!tabOk)
    {
      rInfo->SetBlocked (RoutingInfo::BACKTABLE);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because tat least one backhaul switch table is full.");
    }

  return (bwdOk && cpuOk && tabOk);
}

bool
RingController::BwBearerRequest (Ptr<RingInfo> ringInfo) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Single check: available bandwidth over backhaul links.
  // Only GBR non-agregated bearers transversing at least one backhaul link.
  if (rInfo->IsNonGbr () || rInfo->IsAggregated ()
      || ringInfo->AreLocalPaths ())
    {
      return true;
    }

  SliceId slice = rInfo->GetSliceId ();
  LinkInfo::LinkDir dlDir, ulDir;
  RingInfo::RingPath path;
  Ptr<LinkInfo> lInfo;
  uint16_t curr = 0;
  uint16_t last = 0;
  bool ok = true;

  // When checking for the available bit rate on backhaul links, the S5 and
  // S1-U routing paths may overlap if one of them is CLOCK and the other is
  // COUNT. We must ensure that the overlapping links have the requested
  // bandwidth for both interfaces, otherwise the BitRateReserve () method will
  // fail. So we save the links used by S5 interface and check them when going
  // through S1-U interface.
  std::set<Ptr<LinkInfo> > s5Links;

  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  double blockThs = GetSliceController (slice)->GetGbrBlockThs ();

  // S5 interface (from P-GW to S-GW)
  curr = rInfo->GetPgwInfraSwIdx ();
  last = rInfo->GetSgwInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S5);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= lInfo->HasBitRate (dlDir, slice, dlRate, blockThs);
      ok &= lInfo->HasBitRate (ulDir, slice, ulRate, blockThs);
      curr = next;

      // Save this link as used by S5 interface.
      auto ret = s5Links.insert (lInfo);
      NS_ABORT_MSG_IF (ret.second == false, "Error saving link in map.");
    }

  // S1-U interface (from S-GW to eNB)
  curr = rInfo->GetSgwInfraSwIdx ();
  last = rInfo->GetEnbInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S1);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);

      // Check if this link was used by S5 interface.
      auto it = s5Links.find (lInfo);
      if (it != s5Links.end ())
        {
          // When checking for downlink resources for S1-U interface we also
          // must ensure that the link has uplink resources for S5 interface,
          // and vice-versa.
          ok &= lInfo->HasBitRate (dlDir, slice, dlRate + ulRate, blockThs);
          ok &= lInfo->HasBitRate (ulDir, slice, ulRate + dlRate, blockThs);
        }
      else
        {
          ok &= lInfo->HasBitRate (dlDir, slice, dlRate, blockThs);
          ok &= lInfo->HasBitRate (ulDir, slice, ulRate, blockThs);
        }
      curr = next;
    }

  if (!ok)
    {
      rInfo->SetBlocked (RoutingInfo::BACKBAND);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because at least one backhaul link is overloaded.");
      return false;
    }

  return true;
}

bool
RingController::SwBearerRequest (Ptr<RingInfo> ringInfo) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  SliceId slice = rInfo->GetSliceId ();
  bool success = true;

  // First check: OpenFlow switch table usage (only non-aggregated bearers).
  // Block the bearer if the slice pipeline table usage is exceeding the block
  // threshold at any backhaul switch connected to EPC serving entities.
  if (!rInfo->IsAggregated ())
    {
      // FIXME. Com a mudança das tabelas do backhaul tem que percorrer todo o caminho.
      uint8_t table = GetSliceTable (slice);
      double sgwTabUse = GetFlowTableUse (rInfo->GetSgwInfraSwIdx (), table);
      double pgwTabUse = GetFlowTableUse (rInfo->GetPgwInfraSwIdx (), table);
      double enbTabUse = GetFlowTableUse (rInfo->GetEnbInfraSwIdx (), table);
      if ((rInfo->HasTraffic () && sgwTabUse >= GetSwBlockThreshold ())
          || (rInfo->HasDlTraffic () && pgwTabUse >= GetSwBlockThreshold ())
          || (rInfo->HasUlTraffic () && enbTabUse >= GetSwBlockThreshold ()))
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::BACKTABLE);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the backhaul switch table is full.");
        }
    }

  // Second check: OpenFlow switch processing load.
  // Block the bearer if the processing load is exceeding the block threshold
  // at any backhaul switch over the routing path for this bearer, respecting
  // the BlockPolicy attribute:
  // - If OFF : don't block the request.
  // - If ON  : block the request.
  if (GetSwBlockPolicy () == OpMode::ON)
    {
      RingInfo::RingPath path;
      uint16_t curr = 0;
      uint16_t last = 0;
      bool ok = true;

      // S5 interface (from P-GW to S-GW).
      curr = rInfo->GetPgwInfraSwIdx ();
      last = rInfo->GetSgwInfraSwIdx ();
      path = ringInfo->GetDlPath (LteIface::S5);
      while (ok && curr != last)
        {
          ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
          curr = NextSwitchIndex (curr, path);
        }

      // S1-U interface (from S-GW to eNB).
      curr = rInfo->GetSgwInfraSwIdx ();
      last = rInfo->GetEnbInfraSwIdx ();
      path = ringInfo->GetDlPath (LteIface::S1);
      while (ok && curr != last)
        {
          ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
          curr = NextSwitchIndex (curr, path);
        }

      // The last switch (eNB).
      ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());

      if (!ok)
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::BACKLOAD);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the backhaul switch is overloaded.");
        }
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
  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath path;
  uint16_t curr = 0;
  uint16_t last = 0;
  bool ok = true;

  Ptr<LinkInfo> lInfo;
  LinkInfo::LinkDir dlDir, ulDir;

  // S5 interface (from P-GW to S-GW)
  curr = rInfo->GetPgwInfraSwIdx ();
  last = rInfo->GetSgwInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S5);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= lInfo->UpdateResBitRate (dlDir, slice, dlRate);
      ok &= lInfo->UpdateResBitRate (ulDir, slice, ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  curr = rInfo->GetSgwInfraSwIdx ();
  last = rInfo->GetEnbInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S1);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= lInfo->UpdateResBitRate (dlDir, slice, dlRate);
      ok &= lInfo->UpdateResBitRate (ulDir, slice, ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  NS_ASSERT_MSG (ok, "Error when reserving resources.");
  rInfo->SetGbrReserved (ok);
  return ok;
}

bool
RingController::BitRateRelease (Ptr<RingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_LOG_INFO ("Releasing resources for teid " << rInfo->GetTeidHex ());

  SliceId slice = rInfo->GetSliceId ();
  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath path;
  uint16_t curr = 0;
  uint16_t last = 0;
  bool ok = true;

  Ptr<LinkInfo> lInfo;
  LinkInfo::LinkDir dlDir, ulDir;

  // S5 interface (from P-GW to S-GW)
  curr = rInfo->GetPgwInfraSwIdx ();
  last = rInfo->GetSgwInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S5);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= lInfo->UpdateResBitRate (dlDir, slice, -dlRate);
      ok &= lInfo->UpdateResBitRate (ulDir, slice, -ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  curr = rInfo->GetSgwInfraSwIdx ();
  last = rInfo->GetEnbInfraSwIdx ();
  path = ringInfo->GetDlPath (LteIface::S1);
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= lInfo->UpdateResBitRate (dlDir, slice, -dlRate);
      ok &= lInfo->UpdateResBitRate (ulDir, slice, -ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  NS_ASSERT_MSG (ok, "Error when releasing resources.");
  rInfo->SetGbrReserved (!ok);
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
         RingInfo::COUNT;
}

void
RingController::GetLinks (
  Ptr<RingInfo> ringInfo, LteIface iface, LinkInfoSet_t *links) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  NS_ASSERT_MSG (links && links->empty (), "Set of links should be empty.");

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  uint16_t curr = rInfo->GetFirstDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetLastDlInfraSwIdx (iface);
  RingInfo::RingPath path = ringInfo->GetDlPath (iface);

  // Walk through the downlink path.
  LinkInfo::LinkDir dlDir, ulDir;
  Ptr<LinkInfo> lInfo;
  while (curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      auto ret = links->insert (lInfo);
      NS_ABORT_MSG_IF (ret.second == false, "Error saving link info.");
      curr = next;
    }
}

bool
RingController::HasLinkBitRate (
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

  SliceId slice = rInfo->GetSliceId ();
  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  uint16_t curr = rInfo->GetFirstDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetLastDlInfraSwIdx (iface);
  RingInfo::RingPath path = ringInfo->GetDlPath (iface);
  double blockThs = GetSliceController (slice)->GetGbrBlockThs ();

  // Walk through the backhaul links in the downlink routing path,
  // checking for the available bandwidth.
  LinkInfo::LinkDir dlDir, ulDir;
  Ptr<LinkInfo> lInfo;
  bool ok = true;
  while (ok && curr != last)
    {
      uint16_t next = NextSwitchIndex (curr, path);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      if (overlap && overlap->find (lInfo) != overlap->end ())
        {
          // Ensure that overlapping links have the requested bandwidth for
          // both interfaces, otherwise the BitRateReserve () method will fail.
          ok &= lInfo->HasBitRate (dlDir, slice, dlRate + ulRate, blockThs);
          ok &= lInfo->HasBitRate (ulDir, slice, ulRate + dlRate, blockThs);
        }
      else
        {
          ok &= lInfo->HasBitRate (dlDir, slice, dlRate, blockThs);
          ok &= lInfo->HasBitRate (ulDir, slice, ulRate, blockThs);
        }
      curr = next;
    }
  return ok;
}

bool
RingController::HasSwitchCpu (Ptr<RingInfo> ringInfo, LteIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check when the BlockPolicy mode is OFF.
  if (GetSwBlockPolicy () == OpMode::OFF)
    {
      return true;
    }

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  uint16_t curr = rInfo->GetFirstDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetLastDlInfraSwIdx (iface);
  RingInfo::RingPath path = ringInfo->GetDlPath (iface);

  // Walk through the OpenFlow switches in the downlink routing path,
  // checking for the CPU usage.
  bool ok = true;
  while (ok && curr != last)
    {
      ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
      curr = NextSwitchIndex (curr, path);
    }
  ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
  return ok;
}

bool
RingController::HasSwitchTable (Ptr<RingInfo> ringInfo, LteIface iface) const
{
  NS_LOG_FUNCTION (this << ringInfo << iface);

  // Ignoring this check for aggregated bearers.
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  if (rInfo->IsAggregated ())
    {
      return true;
    }

  SliceId slice = rInfo->GetSliceId ();
  uint8_t table = GetSliceTable (slice);
  uint16_t curr = rInfo->GetFirstDlInfraSwIdx (iface);
  uint16_t last = rInfo->GetLastDlInfraSwIdx (iface);
  RingInfo::RingPath path = ringInfo->GetDlPath (iface);

  // Walk through the OpenFlow switches in the downlink routing path,
  // checking for the slice table usage.
  bool ok = true;
  while (ok && curr != last)
    {
      ok &= (GetFlowTableUse (curr, table) < GetSwBlockThreshold ());
      curr = NextSwitchIndex (curr, path);
    }
  ok &= (GetFlowTableUse (curr, table) < GetSwBlockThreshold ());
  return ok;
}

uint16_t
RingController::HopCounter (
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

uint16_t
RingController::NextSwitchIndex (uint16_t idx, RingInfo::RingPath path) const
{
  NS_LOG_FUNCTION (this << idx << path);

  NS_ASSERT_MSG (GetNSwitches () > 0, "Invalid number of switches.");
  NS_ASSERT_MSG (path != RingInfo::UNDEF, "Invalid ring routing path.");
  NS_ASSERT_MSG (path != RingInfo::LOCAL, "Invalid ring routing path.");

  return path == RingInfo::CLOCK ?
         (idx + 1) % GetNSwitches () :
         (idx == 0 ? GetNSwitches () - 1 : (idx - 1));
}

void
RingController::SetShortestPath (Ptr<RingInfo> ringInfo) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  RingInfo::RingPath s5DownPath = FindShortestPath (
      rInfo->GetFirstDlInfraSwIdx (LteIface::S5),
      rInfo->GetLastDlInfraSwIdx (LteIface::S5));
  ringInfo->SetShortDlPath (LteIface::S5, s5DownPath);

  RingInfo::RingPath s1DownPath = FindShortestPath (
      rInfo->GetFirstDlInfraSwIdx (LteIface::S1),
      rInfo->GetLastDlInfraSwIdx (LteIface::S1));
  ringInfo->SetShortDlPath (LteIface::S1, s1DownPath);

  NS_LOG_DEBUG ("Bearer teid " << rInfo->GetTeidHex () <<
                " S1-U path: " << RingInfo::RingPathStr (s1DownPath) <<
                " S5 path: "   << RingInfo::RingPathStr (s5DownPath));
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
      uint32_t meterId = GetSvelteMeterId (slice, d);

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

} // namespace ns3
