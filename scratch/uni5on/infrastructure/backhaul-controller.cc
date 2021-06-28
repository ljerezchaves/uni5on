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

    .AddAttribute ("AggBitRateCheck",
                   "Check for the available bit rate for aggregated bearers.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::OFF),
                   MakeEnumAccessor (&BackhaulController::m_aggCheck),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("ExtraStep",
                   "Extra bit rate adjustment step.",
                   DataRateValue (DataRate ("12Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("GuardStep",
                   "Link guard bit rate.",
                   DataRateValue (DataRate ("10Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_guardStep),
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
    .AddAttribute ("SliceTimeout",
                   "Inter-slice adjustment timeout.",
                   TimeValue (Seconds (20)),
                   MakeTimeAccessor (&BackhaulController::m_sliceTimeout),
                   MakeTimeChecker ())
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
BackhaulController::GetAggBitRateCheck (void) const
{
  NS_LOG_FUNCTION (this);

  return m_aggCheck;
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
        m_sliceTimeout, &BackhaulController::SlicingDynamicTimeout, this);
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
BackhaulController::GetSliceControllerList (bool sharing) const
{
  NS_LOG_FUNCTION (this << sharing);

  return (sharing ? m_sliceCtrlsSha : m_sliceCtrlsAll);
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
        << ",table="        << INPUT_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_proto="     << UDP_PROT_NUM
        << ",ip_dst="       << BackhaulNetwork::m_x2Addr
        << "/"              << BackhaulNetwork::m_x2Mask.GetPrefixLength ()
        << ",in_port="      << portNo
        << " apply:set_field=ip_dscp:" << Ipv4Header::DSCP_EF
        << " goto:"         << CLASS_TAB;
    DpctlExecute (swDev->GetDatapathId (), cmd.str ());
  }
}

// Comparator for slice priorities.
static bool
PriComp (Ptr<SliceController> ctrl1, Ptr<SliceController> ctrl2)
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
      std::pair<SliceId, Ptr<SliceController>> entry (slice, controller);
      auto ret = m_sliceCtrlById.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Existing slice controller.");

      m_sliceCtrlsAll.push_back (controller);
      if (controller->GetSharing () == OpMode::ON)
        {
          m_sliceCtrlsSha.push_back (controller);
        }

      // Iterate over links configuring the initial quotas.
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          bool success = true;
          success &= lInfo->UpdateQuota (LinkInfo::FWD, slice, quota);
          success &= lInfo->UpdateQuota (LinkInfo::BWD, slice, quota);
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");
        }
    }

  // Sort slice controllers in increasing priority order.
  std::stable_sort (m_sliceCtrlsAll.begin (), m_sliceCtrlsAll.end (), PriComp);
  std::stable_sort (m_sliceCtrlsSha.begin (), m_sliceCtrlsSha.end (), PriComp);

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
  for (int s = 0; s < N_SLICE_IDS; s++)
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
      for (int d = 0; d < N_LINK_DIRS; d++)
        {
          SlicingExtraAdjust (lInfo, static_cast<LinkInfo::LinkDir> (d));
        }
    }

  // Schedule the next slicing extra timeout operation.
  Simulator::Schedule (
    m_sliceTimeout, &BackhaulController::SlicingDynamicTimeout, this);
}

void
BackhaulController::SlicingExtraAdjust (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << lInfo << dir);

  NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::DYNA,
                 "Invalid inter-slice operation mode.");

  const LinkInfo::EwmaTerm lTerm = LinkInfo::LTERM;
  int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());
  NS_ASSERT_MSG (stepRate > 0, "Invalid ExtraStep attribute value.");

  // Iterate over slices with enabled bandwidth sharing
  // to sum the quota bit rate and the used bit rate.
  int64_t maxShareBitRate = 0;
  int64_t useShareBitRate = 0;
  for (auto const &ctrl : GetSliceControllerList (true))
    {
      SliceId slice = ctrl->GetSliceId ();
      maxShareBitRate += lInfo->GetQuoBitRate (dir, slice);
      useShareBitRate += lInfo->GetUseBitRate (lTerm, dir, slice);
    }
  // When enable, sum the spare bit rate too.
  if (GetSpareUseMode () == OpMode::ON)
    {
      maxShareBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
    }

  // Get the idle bit rate (apart from the guard bit rate) that can be used as
  // extra bit rate by overloaded slices.
  int64_t guardBitRate = static_cast<int64_t> (m_guardStep.GetBitRate ());
  int64_t idlShareBitRate = maxShareBitRate - guardBitRate - useShareBitRate;

  if (idlShareBitRate > 0)
    {
      // We have some unused bit rate step that can be distributed as extra to
      // any overloaded slice. Iterate over slices with enabled bandwidth
      // sharing in decreasing priority order, assigning one extra bit rate to
      // those slices that may benefit from it. Also, gets back one extra bit
      // rate from underloaded slices to reduce unnecessary overbooking.
      for (auto it = m_sliceCtrlsSha.rbegin ();
           it != m_sliceCtrlsSha.rend (); ++it)
        {
          // Get the idle and extra bit rates for this slice.
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          if (sliceIdl < (stepRate / 2) && idlShareBitRate >= stepRate)
            {
              // This is an overloaded slice and we have idle bit rate.
              // Increase the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Increase extra bit rate.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate -= stepRate;
            }
          else if (sliceIdl >= (stepRate * 2) && sliceExt >= stepRate)
            {
              // This is an underloaded slice with some extra bit rate.
              // Decrease the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
            }
        }
    }
  else
    {
      // Link usage is over the safeguard threshold. First, iterate over slices
      // with enable bandwidth sharing and get back any unused extra bit rate
      // to reduce unnecessary overbooking.
      for (auto const &ctrl : GetSliceControllerList (true))
        {
          // Get the idle and extra bit rates for this slice.
          SliceId slice = ctrl->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // Remove all unused extra bit rate (step by step) from this slice.
          while (sliceIdl >= stepRate && sliceExt >= stepRate)
            {
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              sliceIdl -= stepRate;
              sliceExt -= stepRate;
            }
        }

      // At this point there is no slices with more than one step of unused
      // extra bit rate. Now, iterate again over slices with enabled bandwidth
      // sharing in increasing priority order, removing some extra bit rate
      // from those slices that are using more than its quota to get the link
      // usage below the safeguard threshold again.
      bool removedFlag = false;
      auto it = m_sliceCtrlsSha.begin ();
      auto sp = m_sliceCtrlsSha.begin ();
      while (it != m_sliceCtrlsSha.end () && idlShareBitRate < 0)
        {
          // Check if the slice priority has increased to update the sp.
          if ((*it)->GetPriority () > (*sp)->GetPriority ())
            {
              NS_ASSERT_MSG (!removedFlag, "Inconsistent removed flag.");
              sp = it;
            }

          // Get the idle and extra bit rates for this slice.
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // If possible, decrease the slice extra bit rate by one step.
          if (sliceExt >= stepRate)
            {
              removedFlag = true;
              NS_ASSERT_MSG (sliceIdl < stepRate, "Inconsistent bit rate.");
              NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate += stepRate - sliceIdl;
            }

          // Select the slice for the next loop iteration.
          auto nextIt = std::next (it);
          bool isLast = (nextIt == m_sliceCtrlsSha.end ());
          if ((!isLast && (*nextIt)->GetPriority () == (*it)->GetPriority ())
              || (removedFlag == false))
            {
              // Go to the next slice if it has the same priority of the
              // current one or if no more extra bit rate can be recovered from
              // slices with the current priority.
              it = nextIt;
            }
          else
            {
              // Go back to the first slice with the current priority (can be
              // the current slice) and reset the removed flag.
              NS_ASSERT_MSG (removedFlag, "Inconsistent removed flag.");
              it = sp;
              removedFlag = false;
            }
        }
    }

  // Update the slicing meters for all slices over this link.
  for (auto const &ctrl : GetSliceControllerList (true))
    {
      SlicingMeterAdjust (lInfo, ctrl->GetSliceId ());
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
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t meterBitRate = 0;
      if (slice == SliceId::ALL)
        {
          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (auto const &ctrl : GetSliceControllerList (true))
            {
              SliceId slc = ctrl->GetSliceId ();
              meterBitRate += lInfo->GetUnrBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (GetSpareUseMode () == OpMode::ON)
            {
              meterBitRate += lInfo->GetUnrBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetUnrBitRate (dir, slice);
        }

      int64_t currBitRate = lInfo->GetMetBitRate (dir, slice);
      uint64_t diffBitRate = std::abs (currBitRate - meterBitRate);
      NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                    " direction "    << LinkInfo::LinkDirStr (dir) <<
                    " diff rate "    << diffBitRate);

      if (diffBitRate >= m_meterStep.GetBitRate ())
        {
          uint32_t meterId = MeterIdSlcCreate (slice, d);
          int64_t meterKbps = Bps2Kbps (meterBitRate);
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
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t meterBitRate = 0;
      if (slice == SliceId::ALL)
        {
          NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::SHAR,
                         "Invalid inter-slice operation mode.");

          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (auto const &ctrl : GetSliceControllerList (true))
            {
              SliceId slc = ctrl->GetSliceId ();
              meterBitRate += lInfo->GetQuoBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (GetSpareUseMode () == OpMode::ON)
            {
              meterBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetQuoBitRate (dir, slice);
        }

      uint32_t meterId = MeterIdSlcCreate (slice, d);
      int64_t meterKbps = Bps2Kbps (meterBitRate);
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
