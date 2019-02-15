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

  // Reset the ring routing info to the shortest path.
  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");
  ringInfo->ResetToDefaults ();

  // Check for the available resources over the shortest path.
  if (HasAvailableResources (ringInfo))
    {
      NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
                   " over the shortest path");
      return true;
    }

  // FIXME This part is not working by now because we can't reset blocked status.
  // // The requested resources are not available over the shortest path. When
  // // using the SPF routing strategy, invert the path for S1/S5 interfaces and
  // // check for available resources again.
  // if (m_strategy == RingController::SPF)
  //   {
  //     // Let's try inverting only the S1-U interface.
  //     if (!ringInfo->IsLocalPath (LteIface::S1U))
  //       {
  //         rInfo->ResetBlocked (); // FIXME Remove this.
  //         ringInfo->ResetToDefaults ();
  //         ringInfo->InvertIfacePath (LteIface::S1U);
  //         if (HasAvailableResources (ringInfo))
  //           {
  //             NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
  //                          " over the inverted S1-U path");
  //             return BearerReserve (ringInfo);
  //           }
  //       }

  //     // Let's try inverting only the S5 interface.
  //     if (!ringInfo->IsLocalPath (LteIface::S5))
  //       {
  //         rInfo->ResetBlocked (); // FIXME Remove this.
  //         ringInfo->ResetToDefaults ();
  //         ringInfo->InvertIfacePath (LteIface::S5);
  //         if (HasAvailableResources (ringInfo))
  //           {
  //             NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
  //                          " over the inverted S5 path");
  //             return BearerReserve (ringInfo);
  //           }
  //       }

  //     // Let's try inverting both the S1-U and S5 interface.
  //     if (!ringInfo->IsLocalPath (LteIface::S1U)
  //         && !ringInfo->IsLocalPath (LteIface::S5))
  //       {
  //         rInfo->ResetBlocked (); // FIXME Remove this.
  //         ringInfo->ResetToDefaults ();
  //         ringInfo->InvertIfacePath (LteIface::S1U);
  //         ringInfo->InvertIfacePath (LteIface::S5);
  //         if (HasAvailableResources (ringInfo))
  //           {
  //             NS_LOG_INFO ("Routing bearer teid " << rInfo->GetTeidHex () <<
  //                          " over the inverted S1-U and S5 paths");
  //             return BearerReserve (ringInfo);
  //           }
  //       }
  //   }

  // If we get here it's because one of the resources is not available.
  // It is expected that the HasAvailableResources method set the block reason.
  NS_ASSERT_MSG (rInfo->IsBlocked (), "This bearer should be blocked.");
  return false;
}

bool
RingController::BearerReserve (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->IsBlocked (), "Bearer should not be blocked.");

  Ptr<RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ringInfo for this bearer.");

  // The only resource that is reserved is the GBR bit rate over links.
  // For Non-GBR bearers (which includes the default bearer) and for bearers
  // that only transverse local switch (local routing for both S1-U and S5
  // interfaces): there's nothing to reserve.
  if (!rInfo->IsGbr () || ringInfo->AreBothLocalPaths ())
    {
      return true;
    }
  return BitRateReserve (ringInfo);
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

bool
RingController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
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
          << ",ip_dst="     << rInfo->GetEnbS1uAddr ()
          << ",gtpu_teid="  << rInfo->GetTeidHex ();

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
RingController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsInstalled (), "Rules must be installed.");
  NS_LOG_INFO ("Removing ring rules for teid " << rInfo->GetTeidHex ());

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",table="        << GetSliceTable (rInfo->GetSliceId ())
      << ",cookie="       << rInfo->GetTeidHex ()
      << ",cookie_mask="  << COOKIE_STRICT_MASK_STR;

  // Removing rules from switches connected to the eNB, S-GW and P-GW.
  DpctlExecute (GetDpId (rInfo->GetPgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (rInfo->GetSgwInfraSwIdx ()), cmd.str ());
  DpctlExecute (GetDpId (rInfo->GetEnbInfraSwIdx ()), cmd.str ());

  return true;
}

bool
RingController::BearerUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

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
        aS1 << " meta:" << ringInfo->GetUlPath (LteIface::S1U)
            << " goto:" << ROUTE_TAB;
        aS5 << " meta:" << ringInfo->GetUlPath (LteIface::S5)
            << " goto:" << ROUTE_TAB;

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
      // Configure groups to forward packets in both ring directions.
      {
        // Routing group for clockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group="    << RingInfo::CLOCK
            << " weight=0,port=any,group=any output=" << lInfo->GetPortNo (0);
        DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
      }
      {
        // Routing group for counterclockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "group-mod cmd=add,type=ind,group="    << RingInfo::COUNTER
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
      // IP packets being forwarded by this switch, except for those addressed
      // to EPC elements connected to EPC ports (a high-priority match rule was
      // installed by NotifyEpcAttach function for this case) and for those
      // just classified by the corresponding slice table (a high-priority
      // match rule was installed by HandshakeSuccessful function for this
      // case). Write the output group into action set based on the input port,
      // forwarding the packet in the same ring direction so it can reach the
      // destination switch. Write the same group number into metadata field.
      // Send the packet to the bandwidth table.
      {
        // Counterclockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "flow-mod cmd=add,prio=64"
            << ",table="        << ROUTE_TAB
            << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
            << " eth_type="     << IPV4_PROT_NUM
            << ",meta=0x0"
            << ",in_port="      << lInfo->GetPortNo (0)
            << " write:group="  << RingInfo::COUNTER
            << " meta:"         << RingInfo::COUNTER
            << " goto:"         << BANDW_TAB;
        DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
      }
      {
        // Clockwise packet forwarding.
        std::ostringstream cmd;
        cmd << "flow-mod cmd=add,prio=64"
            << ",table="        << ROUTE_TAB
            << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
            << " eth_type="     << IPV4_PROT_NUM
            << ",meta=0x0"
            << ",in_port="      << lInfo->GetPortNo (1)
            << " write:group="  << RingInfo::CLOCK
            << " meta:"         << RingInfo::CLOCK
            << " goto:"         << BANDW_TAB;
        DpctlSchedule (lInfo->GetSwDpId (1), cmd.str ());
      }
    }
}

void
RingController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  // IP packets being forwarded by this switch, except for those addressed to
  // EPC elements connected to EPC ports (a high-priority match rule was
  // installed by NotifyEpcAttach function for this case). These packets were
  // classified by the corresponding slice table and the metadata field has now
  // the necessary information for the routing decision. Write the output group
  // into action set based on metadata field.
  // Send the packet to the bandwidth table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=128"
        << ",table="        << ROUTE_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",meta="         << RingInfo::CLOCK
        << " write:group="  << RingInfo::CLOCK
        << " goto:"         << BANDW_TAB;
    DpctlExecute (swtch, cmd.str ());
  }
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=128"
        << ",table="        << ROUTE_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",meta="         << RingInfo::COUNTER
        << " write:group="  << RingInfo::COUNTER
        << " goto:"         << BANDW_TAB;
    DpctlExecute (swtch, cmd.str ());
  }

  // X2-C packets entering the backhaul network at this switch. Route the
  // packet in the clockwise direction.
  // Send the packet directly to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="        << ROUTE_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_proto="     << UDP_PROT_NUM
        << ",udp_src="      << X2C_PORT
        << ",udp_dst="      << X2C_PORT
        << " write:group="  << RingInfo::CLOCK
        << " goto:"         << OUTPT_TAB;
    DpctlExecute (swtch, cmd.str ());
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
RingController::BwBearerRequest (Ptr<RingInfo> ringInfo) const
{
  NS_LOG_FUNCTION (this << ringInfo);

  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();

  // Single check: available bandwidth over backhaul links.
  // The only resource that is reserved is the GBR bit rate over links.
  // For Non-GBR bearers (which includes the default bearer) and for bearers
  // that only transverse local switch (local routing for both S1-U and S5
  // interfaces): let's accept it without bandwidth guarantees.
  if (!rInfo->IsGbr () || ringInfo->AreBothLocalPaths ())
    {
      return true;
    }

  SliceId slice = rInfo->GetSliceId ();
  LinkInfo::LinkDir dlDir, ulDir;
  RingInfo::RingPath downPath;
  Ptr<LinkInfo> lInfo;
  uint16_t curr = 0;
  bool ok = true;

  // When checking for the available bit rate on backhaul links, the S5 and
  // S1-U routing paths may overlap if one of them is CLOCK and the other is
  // COUNTER. We must ensure that the overlapping links have the requested
  // bandwidth for both interfaces, otherwise the BitRateReserve () method will
  // fail. So we save the links used by S5 interface and check them when going
  // through S1-U interface.
  std::set<Ptr<LinkInfo> > s5Links;

  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  double blockThs = GetSliceController (slice)->GetGbrBlockThs ();

  // S5 interface (from P-GW to S-GW)
  curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (ok && curr != rInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      ok &= HasGbrBitRate (lInfo, dlDir, slice, dlRate, blockThs);
      ok &= HasGbrBitRate (lInfo, ulDir, slice, ulRate, blockThs);
      curr = next;

      // Save this link as used by S5 interface.
      auto ret = s5Links.insert (lInfo);
      NS_ABORT_MSG_IF (ret.second == false, "Error saving link in map.");
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (ok && curr != rInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);

      // Check if this link was used by S5 interface.
      auto it = s5Links.find (lInfo);
      if (it != s5Links.end ())
        {
          // When checking for downlink resources for S1-U interface we also
          // must ensure that the link has uplink resources for S5 interface,
          // and vice-versa.
          ok &= HasGbrBitRate (lInfo, dlDir, slice, dlRate + ulRate, blockThs);
          ok &= HasGbrBitRate (lInfo, ulDir, slice, ulRate + dlRate, blockThs);
        }
      else
        {
          ok &= HasGbrBitRate (lInfo, dlDir, slice, dlRate, blockThs);
          ok &= HasGbrBitRate (lInfo, ulDir, slice, ulRate, blockThs);
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

  // First check: OpenFlow switch table usage.
  // Block the bearer if the slice pipeline table usage is exceeding the block
  // threshold at any backhaul switch connected to EPC serving entities.
  uint8_t sliceTable = GetSliceTable (slice);
  double sgwTabUse = GetFlowTableUse (rInfo->GetSgwInfraSwIdx (), sliceTable);
  double pgwTabUse = GetFlowTableUse (rInfo->GetPgwInfraSwIdx (), sliceTable);
  double enbTabUse = GetFlowTableUse (rInfo->GetEnbInfraSwIdx (), sliceTable);
  if ((rInfo->HasTraffic () && sgwTabUse >= GetSwBlockThreshold ())
      || (rInfo->HasDlTraffic () && pgwTabUse >= GetSwBlockThreshold ())
      || (rInfo->HasUlTraffic () && enbTabUse >= GetSwBlockThreshold ()))
    {
      success = false;
      rInfo->SetBlocked (RoutingInfo::BACKTABLE);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because the backhaul switch table is full.");
    }

  // Second check: OpenFlow switch processing load.
  // Block the bearer if the processing load is exceeding the block threshold
  // at any backhaul switch over the routing path for this bearer, respecting
  // the BlockPolicy attribute:
  // - If OFF : don't block the request.
  // - If ON  : block the request.
  if (GetSwBlockPolicy () == OpMode::ON)
    {
      RingInfo::RingPath downPath;
      uint16_t curr = 0;
      bool ok = true;

      // S5 interface (from P-GW to S-GW).
      curr = rInfo->GetPgwInfraSwIdx ();
      downPath = ringInfo->GetDlPath (LteIface::S5);
      while (ok && curr != rInfo->GetSgwInfraSwIdx ())
        {
          ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
          curr = NextSwitchIndex (curr, downPath);
        }

      // S1-U interface (from S-GW to eNB).
      downPath = ringInfo->GetDlPath (LteIface::S1U);
      while (ok && curr != rInfo->GetEnbInfraSwIdx ())
        {
          ok &= (GetEwmaCpuUse (curr) < GetSwBlockThreshold ());
          curr = NextSwitchIndex (curr, downPath);
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
  RingInfo::RingPath downPath;
  bool success = true;

  Ptr<LinkInfo> lInfo;
  LinkInfo::LinkDir dlDir, ulDir;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (success && curr != rInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      success &= lInfo->UpdateResBitRate (dlDir, slice, dlRate);
      success &= lInfo->UpdateResBitRate (ulDir, slice, ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (success && curr != rInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      success &= lInfo->UpdateResBitRate (dlDir, slice, dlRate);
      success &= lInfo->UpdateResBitRate (ulDir, slice, ulRate);
      SlicingMeterAdjust (lInfo, slice);
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
  int64_t dlRate = rInfo->GetGbrDlBitRate ();
  int64_t ulRate = rInfo->GetGbrUlBitRate ();
  RingInfo::RingPath downPath;
  bool success = true;

  Ptr<LinkInfo> lInfo;
  LinkInfo::LinkDir dlDir, ulDir;

  // S5 interface (from P-GW to S-GW)
  uint16_t curr = rInfo->GetPgwInfraSwIdx ();
  downPath = ringInfo->GetDlPath (LteIface::S5);
  while (success && curr != rInfo->GetSgwInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      success &= lInfo->UpdateResBitRate (dlDir, slice, -dlRate);
      success &= lInfo->UpdateResBitRate (ulDir, slice, -ulRate);
      SlicingMeterAdjust (lInfo, slice);
      curr = next;
    }

  // S1-U interface (from S-GW to eNB)
  downPath = ringInfo->GetDlPath (LteIface::S1U);
  while (success && curr != rInfo->GetEnbInfraSwIdx ())
    {
      uint16_t next = NextSwitchIndex (curr, downPath);
      std::tie (lInfo, dlDir, ulDir) = GetLinkInfo (curr, next);
      success &= lInfo->UpdateResBitRate (dlDir, slice, -dlRate);
      success &= lInfo->UpdateResBitRate (ulDir, slice, -ulRate);
      SlicingMeterAdjust (lInfo, slice);
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
    DpctlSchedule (lInfo->GetSwDpId (0), cmd.str ());
  }
  {
    std::ostringstream cmd;
    cmd << "port-mod"
        << " port=" << lInfo->GetPortNo (1)
        << ",addr=" << lInfo->GetPortAddr (1)
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

  NS_ASSERT_MSG (GetNSwitches () > 0, "Invalid number of switches.");
  NS_ASSERT_MSG (path != RingInfo::LOCAL,
                 "Not supposed to get here for local routing.");

  return path == RingInfo::CLOCK ?
         (idx + 1) % GetNSwitches () :
         (idx == 0 ? GetNSwitches () - 1 : (idx - 1));
}

void
RingController::SlicingMeterApply (Ptr<const RemoteSwitch> swtch,
                                   SliceId slice)
{
  NS_LOG_FUNCTION (this << swtch << slice);

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

          DpctlExecute (swtch, cmd.str () + mtc.str () + act.str ());
        }
    }
}

} // namespace ns3
