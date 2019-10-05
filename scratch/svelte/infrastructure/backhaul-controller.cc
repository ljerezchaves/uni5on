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

#include <algorithm>
#include "backhaul-controller.h"
#include "../metadata/link-info.h"
#include "backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulController");
NS_OBJECT_ENSURE_REGISTERED (BackhaulController);

BackhaulController::BackhaulController ()
{
  NS_LOG_FUNCTION (this);
}

BackhaulController::~BackhaulController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BackhaulController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BackhaulController")
    .SetParent<OFSwitch13Controller> ()

    .AddAttribute ("DynamicGuard",
                   "Dynamic inter-slice link guard bit rate.",
                   DataRateValue (DataRate ("4Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_dynGuard),
                   MakeDataRateChecker ())
    .AddAttribute ("DynamicTimeout",
                   "Dynamic inter-slice adjustment timeout.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&BackhaulController::m_dynTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("ExtraStep",
                   "Extra bit rate adjustment step.",
                   DataRateValue (DataRate ("4Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("MeterStep",
                   "Meter bit rate adjustment step.",
                   DataRateValue (DataRate ("2Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_meterStep),
                   MakeDataRateChecker ())
    .AddAttribute ("QosQueues",
                   "QoS output queues operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_qosQueues),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SliceMode",
                   "Inter-slice operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceMode::NONE),
                   MakeEnumAccessor (&BackhaulController::m_sliceMode),
                   MakeEnumChecker (SliceMode::NONE,
                                    SliceModeStr (SliceMode::NONE),
                                    SliceMode::SHAR,
                                    SliceModeStr (SliceMode::SHAR),
                                    SliceMode::STAT,
                                    SliceModeStr (SliceMode::STAT),
                                    SliceMode::DYNA,
                                    SliceModeStr (SliceMode::DYNA)))
    .AddAttribute ("SpareUse",
                   "Use spare link bit rate for sharing purposes.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_spareUse),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SwBlockPolicy",
                   "Switch overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_swBlockPolicy),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SwBlockThs",
                   "Switch overloaded block threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&BackhaulController::m_swBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
  ;
  return tid;
}

uint64_t
BackhaulController::GetDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid switch index.");
  return m_switchDevices.Get (idx)->GetDatapathId ();
}

uint16_t
BackhaulController::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchDevices.GetN ();
}

OpMode
BackhaulController::GetSwBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_swBlockPolicy;
}

double
BackhaulController::GetSwBlockThreshold (void) const
{
  NS_LOG_FUNCTION (this);

  return m_swBlockThs;
}

SliceMode
BackhaulController::GetInterSliceMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceMode;
}

OpMode
BackhaulController::GetQosQueuesMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_qosQueues;
}

OpMode
BackhaulController::GetSpareUseMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_spareUse;
}

void
BackhaulController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sliceCtrlById.clear ();
  OFSwitch13Controller::DoDispose ();
}

void
BackhaulController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Schedule the first slicing extra timeout operation only when in
  // dynamic inter-slicing operation mode.
  if (GetInterSliceMode () == SliceMode::DYNA)
    {
      Simulator::Schedule (
        m_dynTimeout, &BackhaulController::SlicingDynamicTimeout, this);
    }

  OFSwitch13Controller::NotifyConstructionCompleted ();
}

void
BackhaulController::DpctlSchedule (Time delay, uint64_t dpId,
                                   const std::string textCmd)
{
  NS_LOG_FUNCTION (this << delay << dpId << textCmd);

  Simulator::Schedule (delay, &BackhaulController::DpctlExecute,
                       this, dpId, textCmd);
}

double
BackhaulController::GetFlowTableUse (uint16_t idx, uint8_t tableId) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid index.");
  return m_switchDevices.Get (idx)->GetFlowTableUsage (tableId);
}

std::tuple<Ptr<LinkInfo>, LinkInfo::LinkDir, LinkInfo::LinkDir>
BackhaulController::GetLinkInfo (uint16_t idx1, uint16_t idx2) const
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  uint64_t dpId1 = GetDpId (idx1);
  uint64_t dpId2 = GetDpId (idx2);
  Ptr<LinkInfo> lInfo = LinkInfo::GetPointer (dpId1, dpId2);
  LinkInfo::LinkDir dir = lInfo->GetLinkDir (dpId1, dpId2);
  return std::make_tuple (lInfo, dir, LinkInfo::InvertDir (dir));
}

double
BackhaulController::GetEwmaCpuUse (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid index.");
  Ptr<OFSwitch13Device> device;
  Ptr<OFSwitch13StatsCalculator> stats;
  device = m_switchDevices.Get (idx);
  stats = device->GetObject<OFSwitch13StatsCalculator> ();
  NS_ABORT_MSG_IF (!stats, "Enable OFSwitch13 datapath stats.");

  return static_cast<double> (stats->GetEwmaCpuLoad ().GetBitRate ()) /
         static_cast<double> (device->GetCpuCapacity ().GetBitRate ());
}

Ptr<SliceController>
BackhaulController::GetSliceController (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  auto it = m_sliceCtrlById.find (slice);
  NS_ASSERT_MSG (it != m_sliceCtrlById.end (), "Slice controller not found.");
  return it->second;
}

const SliceControllerList_t&
BackhaulController::GetSliceControllerList (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrls;
}

uint16_t
BackhaulController::GetSliceTable (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  return static_cast<int> (slice) + 2;
}

void
BackhaulController::NotifyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());
}

void
BackhaulController::NotifyEpcAttach (
  Ptr<OFSwitch13Device> swDev, uint32_t portNo, Ptr<NetDevice> epcDev)
{
  NS_LOG_FUNCTION (this << swDev << portNo << epcDev);

  // -------------------------------------------------------------------------
  // Input table -- [from higher to lower priority]
  //
  // IP packets addressed to EPC elements connected to this EPC port.
  // Write the output port into action set.
  // Send the packet directly to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=256"
        << ",table="        << INPUT_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_dst="       << Ipv4AddressHelper::GetAddress (epcDev)
        << " write:output=" << portNo
        << " goto:"         << OUTPT_TAB;
    DpctlExecute (swDev->GetDatapathId (), cmd.str ());
  }
  //
  // X2-C packets entering the backhaul network from this EPC port.
  // Set the DSCP field for Expedited Forwarding.
  // Send the packet to the classification table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="                    << INPUT_TAB
        << ",flags="                    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="                 << IPV4_PROT_NUM
        << ",ip_proto="                 << UDP_PROT_NUM
        << ",udp_src="                  << X2C_PORT
        << ",udp_dst="                  << X2C_PORT
        << ",in_port="                  << portNo
        << " apply:set_field=ip_dscp:"  << Ipv4Header::DSCP_EF
        << " goto:"                     << CLASS_TAB;
    DpctlExecute (swDev->GetDatapathId (), cmd.str ());
  }
}

// Comparator for slice priorities.
static bool
PriorityComp (Ptr<SliceController> ctrl1, Ptr<SliceController> ctrl2)
{
  return ctrl1->GetPriority () < ctrl2->GetPriority ();
}

void
BackhaulController::NotifySlicesBuilt (ApplicationContainer &controllers)
{
  NS_LOG_FUNCTION (this);

  ApplicationContainer::Iterator it;
  for (it = controllers.Begin (); it != controllers.End (); ++it)
    {
      Ptr<SliceController> controller = DynamicCast<SliceController> (*it);
      SliceId slice = controller->GetSliceId ();
      int quota = controller->GetQuota ();

      // Saving controller application pointers.
      m_sliceCtrls.push_back (controller);
      std::pair<SliceId, Ptr<SliceController> > entry (slice, controller);
      auto ret = m_sliceCtrlById.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Existing slice controller.");

      // Iterate over links configuring the initial quotas.
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          bool success = true;
          success &= lInfo->UpdateQuota (LinkInfo::FWD, slice, quota);
          success &= lInfo->UpdateQuota (LinkInfo::BWD, slice, quota);
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");
        }
    }

  // Sort m_sliceCtrls in increasing priority order.
  std::stable_sort (m_sliceCtrls.begin (), m_sliceCtrls.end (), PriorityComp);

  // ---------------------------------------------------------------------
  // Meter table
  //
  // Install inter-slicing meters, depending on the InterSliceMode attribute.
  switch (GetInterSliceMode ())
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install high-priority individual Non-GBR meter entries for slices
          // with disabled bandwidth sharing and the low-priority shared
          // Non-GBR meter entry for other slices.
          SlicingMeterInstall (lInfo, SliceId::ALL);
          for (auto const &ctrl : GetSliceControllerList ())
            {
              if (ctrl->GetSharing () == OpMode::OFF)
                {
                  SlicingMeterInstall (lInfo, ctrl->GetSliceId ());
                }
            }
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install individual Non-GBR meter entries.
          for (auto const &ctrl : GetSliceControllerList ())
            {
              SlicingMeterInstall (lInfo, ctrl->GetSliceId ());
            }
        }
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }
}

void
BackhaulController::NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices)
{
  NS_LOG_FUNCTION (this);

  // Save the collection of switch devices.
  m_switchDevices = devices;
}

ofl_err
BackhaulController::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this error message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Backhaul controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with error message: " << msgStr
            << std::endl;
  return 0;
}

ofl_err
BackhaulController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = CookieGetTeid (msg->stats->cookie);
  uint16_t prio = msg->stats->priority;

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free_flow_removed (msg, true, 0);

  NS_LOG_DEBUG ("Flow removed: " << msgStr);

  // Check for existing routing information for this bearer.
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "Routing metadata not found");

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer is inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for inactive bearer teid " << rInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the bearer
  // priority was increased and this removed flow rule is an old one.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for bearer teid " << rInfo->GetTeidHex () <<
                   " with old priority " << prio);
      return 0;
    }

  // 3) The application is running, the bearer is active, and the bearer
  // priority is the same of the removed rule. This is a critical situation!
  // For some reason, the flow rule was removed so we are going to abort the
  // program to avoid wrong results.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Rule removed for active bearer. " <<
                "OpenFlow flow removed message: " << msgStr);
  return 0;
}

ofl_err
BackhaulController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_structs_match_to_string (msg->match, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this packet-in message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Backhaul controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with packet-in message: " << msgStr
            << std::endl;
  return 0;
}

void
BackhaulController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the OpenFlow switch datapath ID.
  uint64_t swDpId = swtch->GetDpId ();

  // For the switches on the backhaul network, install following rules:
  // -------------------------------------------------------------------------
  // Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  //
  // Table miss entry.
  // Send the packet to the classification table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << INPUT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET
        << " goto:"  << CLASS_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // Classify GTP-U packets on the corresponding logical slice using
  // the GTP-U TEID masked value.
  // Send the packet to the corresponding slice table.
  for (int s = 0; s < SliceId::ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      uint32_t sliceTeid = TeidCreate (slice, 0, 0);

      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64"
          << ",table="      << CLASS_TAB
          << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET
          << " eth_type="   << IPV4_PROT_NUM
          << ",ip_proto="   << UDP_PROT_NUM
          << ",udp_src="    << GTPU_PORT
          << ",udp_dst="    << GTPU_PORT
          << ",gtpu_teid="  << (sliceTeid & TEID_SLICE_MASK)
          << "/"            << TEID_SLICE_MASK
          << " goto:"       << GetSliceTable (slice);
      DpctlExecute (swDpId, cmd.str ());
    }
  //
  // Entries will be installed here by the topology HandshakeSuccessful.

  // -------------------------------------------------------------------------
  // Slice tables (one for each slice) -- [from higher to lower priority]
  //
  // Entries will be installed here by BearerInstall function.

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Entries will be installed here by the topology HandshakeSuccessful.
  //
  // Table miss entry.
  // Send the packet to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table="  << BANDW_TAB
        << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
        << " goto:"   << OUTPT_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Output table -- [from higher to lower priority]
  //
  // Classify IP packets on the corresponding output queue using
  // the IP DSCP value.
  // No goto instruction to trigger action set execution.
  if (GetQosQueuesMode () == OpMode::ON)
    {
      // QoS output queues rules.
      for (auto const &it : Dscp2QueueMap ())
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,prio=32"
              << ",table="        << OUTPT_TAB
              << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
              << " eth_type="     << IPV4_PROT_NUM
              << ",ip_dscp="      << static_cast<uint16_t> (it.first)
              << " write:queue="  << static_cast<uint32_t> (it.second);
          DpctlExecute (swDpId, cmd.str ());
        }
    }
  //
  // Table miss entry.
  // No goto instruction to trigger action set execution.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << OUTPT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET;
    DpctlExecute (swDpId, cmd.str ());
  }
}

void
BackhaulController::SlicingDynamicTimeout (void)
{
  NS_LOG_FUNCTION (this);

  // Adjust the extra bit rates in both directions for each backhaul link.
  for (auto &lInfo : LinkInfo::GetList ())
    {
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          SlicingExtraAdjust (lInfo, static_cast<LinkInfo::LinkDir> (d));
        }
    }

  // Schedule the next slicing extra timeout operation.
  Simulator::Schedule (
    m_dynTimeout, &BackhaulController::SlicingDynamicTimeout, this);
}

void
BackhaulController::SlicingExtraAdjust (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << lInfo << dir);

  NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::DYNA,
                 "Invalid inter-slice operation mode.");

  const LinkInfo::EwmaTerm longTerm = LinkInfo::LTERM;
  int64_t guardRate = static_cast<int64_t> (m_dynGuard.GetBitRate ());
  int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());

  // Sum the quota and use bit rate from slices with enabled bandwidth sharing.
  int64_t quoShareBitRate = 0;
  int64_t useShareBitRate = 0;
  for (auto const &ctrl : GetSliceControllerList ())
    {
      if (ctrl->GetSharing () == OpMode::ON)
        {
          SliceId slice = ctrl->GetSliceId ();
          quoShareBitRate += lInfo->GetQuoBitRate (dir, slice);
          useShareBitRate += lInfo->GetUseBitRate (longTerm, dir, slice);
        }
    }
  // Sum the spare bit rate when enabled.
  if (GetSpareUseMode () == OpMode::ON)
    {
      quoShareBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
    }

  int64_t thsShareBitRate = quoShareBitRate - guardRate;
  if (useShareBitRate <= thsShareBitRate)
    {
      // Link usage is below the safeguard threshold. Iterate over slices in
      // decreasing priority order, assigning some extra bit rate to those
      // slices that may benefit from it.
      int64_t idlShareBitRate = quoShareBitRate - useShareBitRate;
      int maxSteps = idlShareBitRate / stepRate;

      for (auto it = m_sliceCtrls.rbegin (); it != m_sliceCtrls.rend (); ++it)
        {
          // Ignoring slices with disabled bandwidth sharing.
          if ((*it)->GetSharing () == OpMode::OFF)
            {
              continue;
            }

          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (longTerm, dir, slice);
          int64_t sliceOve = lInfo->GetOveBitRate (longTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " over "         << sliceOve <<
                        " idle "         << sliceIdl);

          if (maxSteps > 0 && (sliceIdl < (stepRate / 2)))
            {
              // Increase the extra bit rate by one step.
              NS_LOG_DEBUG ("Increase extra bit rate.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              SlicingMeterAdjust (lInfo, slice);
              maxSteps--;
            }
          else if ((sliceIdl > (stepRate * 2)) && (sliceExt >= stepRate))
            {
              // Decrease one extra bit rate step from those slices that are
              // not using it to reduce unnecessary bit rate overbooking.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              SlicingMeterAdjust (lInfo, slice);
            }
        }
    }
  else
    {
      // Link usage is over the safeguard threshold. Iterate over slices in
      // increasing priority order, removing some extra bit rate from those
      // slices that are using more than its quota to get the link usage below
      // the safeguard threshold again.
      int64_t getBackBitRate = useShareBitRate - thsShareBitRate;

      for (auto it = m_sliceCtrls.begin (); it != m_sliceCtrls.end (); ++it)
        {
          // Ignoring slices with disabled bandwidth sharing.
          if ((*it)->GetSharing () == OpMode::OFF)
            {
              continue;
            }

          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (longTerm, dir, slice);
          int64_t sliceOve = lInfo->GetOveBitRate (longTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " over "         << sliceOve <<
                        " idle "         << sliceIdl);

          bool removed = false;
          while ((sliceExt >= stepRate) && (getBackBitRate > 0))
            {
              removed = true;
              NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              sliceExt -= stepRate;
              if (sliceIdl > stepRate)
                {
                  sliceIdl -= stepRate;
                }
              else
                {
                  getBackBitRate -= stepRate - sliceIdl;
                  sliceIdl = 0;
                }
            }

          if (removed)
            {
              SlicingMeterAdjust (lInfo, slice);
            }
          else if ((sliceIdl > (stepRate * 2)) && (sliceExt >= stepRate))
            {
              // Decrease one extra bit rate step from those slices that are
              // not using it to reduce unnecessary bit rate overbooking.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              SlicingMeterAdjust (lInfo, slice);
            }
        }
    }
}

void
BackhaulController::SlicingMeterAdjust (
  Ptr<LinkInfo> lInfo, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << slice);

  // Update inter-slicing meter, depending on the InterSliceMode attribute.
  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  switch (GetInterSliceMode ())
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      // Identify the Non-GBR meter entry to adjust: individual or shared.
      if (GetSliceController (slice)->GetSharing () == OpMode::ON)
        {
          slice = SliceId::ALL;
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      // Update the individual Non-GBR meter entry.
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }

  // Check for updated slicing meters in both link directions.
  for (int d = 0; d <= LinkInfo::BWD; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t unrBitRate = 0;
      if (slice == SliceId::ALL)
        {
          // Sum the bit rate from slices with enabled bandwidth sharing.
          for (auto const &ctrl : GetSliceControllerList ())
            {
              if (ctrl->GetSharing () == OpMode::ON)
                {
                  unrBitRate += lInfo->GetUnrBitRate (dir, ctrl->GetSliceId ());
                }
            }
          // Sum the spare bit rate when enabled.
          if (GetSpareUseMode () == OpMode::ON)
            {
              unrBitRate += lInfo->GetUnrBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          unrBitRate = lInfo->GetUnrBitRate (dir, slice);
        }

      uint64_t diffBitRate = std::abs (
          lInfo->GetMetBitRate (dir, slice) - unrBitRate);
      NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                    " direction "    << LinkInfo::LinkDirStr (dir) <<
                    " diff rate "    << diffBitRate);

      if (diffBitRate >= m_meterStep.GetBitRate ())
        {
          uint32_t meterId = MeterIdCreate (slice, d);
          int64_t meterKbps = Bps2Kbps (unrBitRate);
          bool success = lInfo->SetMetBitRate (dir, slice, meterKbps * 1000);
          NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

          NS_LOG_INFO ("Update slice " << SliceIdStr (slice) <<
                       " direction "   << LinkInfo::LinkDirStr (dir) <<
                       " meter ID "    << GetUint32Hex (meterId) <<
                       " bitrate "     << meterKbps << " Kbps");

          std::ostringstream cmd;
          cmd << "meter-mod cmd=mod"
              << ",flags="      << OFPMF_KBPS
              << ",meter="      << meterId
              << " drop:rate="  << meterKbps;
          DpctlExecute (lInfo->GetSwDpId (d), cmd.str ());
        }
    }
}

void
BackhaulController::SlicingMeterInstall (Ptr<LinkInfo> lInfo, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << slice);

  NS_ASSERT_MSG (GetInterSliceMode () != SliceMode::NONE,
                 "Invalid inter-slice operation mode.");

  // Install slicing meters in both link directions.
  for (int d = 0; d <= LinkInfo::BWD; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t quotaBitRate = 0;
      if (slice == SliceId::ALL)
        {
          NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::SHAR,
                         "Invalid inter-slice operation mode.");

          // Sum the bit rate from slices with enabled bandwidth sharing.
          for (auto const &ctrl : GetSliceControllerList ())
            {
              if (ctrl->GetSharing () == OpMode::ON)
                {
                  quotaBitRate += lInfo->GetQuoBitRate (dir, ctrl->GetSliceId ());
                }
            }
          // Sum the spare bit rate when enabled.
          if (GetSpareUseMode () == OpMode::ON)
            {
              quotaBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          quotaBitRate = lInfo->GetQuoBitRate (dir, slice);
        }

      uint32_t meterId = MeterIdCreate (slice, d);
      int64_t meterKbps = Bps2Kbps (quotaBitRate);
      bool success = lInfo->SetMetBitRate (dir, slice, meterKbps * 1000);
      NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

      NS_LOG_INFO ("Create slice " << SliceIdStr (slice) <<
                   " direction "   << LinkInfo::LinkDirStr (dir) <<
                   " meter ID "    << GetUint32Hex (meterId) <<
                   " bitrate "     << meterKbps << " Kbps");

      std::ostringstream cmd;
      cmd << "meter-mod cmd=add"
          << ",flags="      << OFPMF_KBPS
          << ",meter="      << meterId
          << " drop:rate="  << meterKbps;
      DpctlExecute (lInfo->GetSwDpId (d), cmd.str ());
    }
}

} // namespace ns3
