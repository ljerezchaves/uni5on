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
#include "../logical/slice-controller.h"
#include "../metadata/link-info.h"
#include "backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulController");
NS_OBJECT_ENSURE_REGISTERED (BackhaulController);

// Initializing BackhaulController static members.
BackhaulController::DscpQueueMap_t BackhaulController::m_queueByDscp;

BackhaulController::BackhaulController ()
{
  NS_LOG_FUNCTION (this);

  StaticInitialize ();

  // Populating slice controllers map.
  for (int s = 0; s < SliceId::ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      std::pair<SliceId, Ptr<SliceController> > entry (slice, 0);
      auto ret = m_sliceCtrlById.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Existing slice controller.");
    }
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
    .AddAttribute ("ExtraStep",
                   "Default extra bit rate adjustment step.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("4Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("MeterStep",
                   "Default meter bit rate adjustment step.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
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
    .AddAttribute ("SliceLinkThs",
                   "Inter-slice link usage threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&BackhaulController::m_sliceLinkThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
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
                   "Inter-slice timeout execution.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&BackhaulController::m_sliceTimeout),
                   MakeTimeChecker ())
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

  // Schedule the first inter-slicing timeout operation.
  Simulator::Schedule (
    m_sliceTimeout, &BackhaulController::SlicingTimeout, this);

  OFSwitch13Controller::NotifyConstructionCompleted ();
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

uint16_t
BackhaulController::GetSliceTable (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  return static_cast<int> (slice) + 2;
}

bool
BackhaulController::HasGbrBitRate (
  Ptr<const LinkInfo> lInfo, LinkInfo::LinkDir dir, SliceId slice,
  int64_t bitRate, double blockThs) const
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice << bitRate << blockThs);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");

  // Can't reserve more GBR bit rate than the minimum between the slice
  // quota bit rate and the slice maximum bit rate * GBR block threshold.
  int64_t blkBitRate = lInfo->GetMaxBitRate (dir, slice) * blockThs;
  int64_t quoBitRate = lInfo->GetQuoBitRate (dir, slice);
  int64_t resBitRate = lInfo->GetResBitRate (dir, slice);
  return (resBitRate + bitRate <= std::min (blkBitRate, quoBitRate));
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
  {
    // IP packets entering the ring network from this EPC port.
    // Send the packet to the classification table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64"
        << ",table="    << INPUT_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << ",in_port="  << portNo
        << " goto:"     << CLASS_TAB;
    DpctlSchedule (swDev->GetDatapathId (), cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  {
    // IP packets addressed to EPC elements connected to this EPC port.
    // Write the output port into action set.
    // Send the packet directly to the output table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=256"
        << ",table="        << ROUTE_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_dst="       << Ipv4AddressHelper::GetAddress (epcDev)
        << " write:output=" << portNo
        << " goto:"         << OUTPT_TAB;
    DpctlSchedule (swDev->GetDatapathId (), cmd.str ());
  }
}

// Comparator for slice priorities.
bool
SlicePrioComp (Ptr<SliceController> ctrl1, Ptr<SliceController> ctrl2)
{
  return ctrl1->GetPriority () < ctrl2->GetPriority ();
}

void
BackhaulController::NotifySlicesBuilt (ApplicationContainer &controllers)
{
  NS_LOG_FUNCTION (this);

  // Saving controller application pointers.
  ApplicationContainer::Iterator it;
  for (it = controllers.Begin (); it != controllers.End (); ++it)
    {
      Ptr<SliceController> ctrl = DynamicCast<SliceController> (*it);
      SliceId slice = ctrl->GetSliceId ();

      auto it = m_sliceCtrlById.find (slice);
      NS_ASSERT_MSG (it != m_sliceCtrlById.end (), "Invalid slice ID.");
      it->second = ctrl;

      m_sliceCtrlPrio.push_back (ctrl);
    }

  // Sort m_sliceCtrlPrio in increasing controller priority order.
  std::sort (m_sliceCtrlPrio.begin (), m_sliceCtrlPrio.end (), SlicePrioComp);

  // Configure initial link slice quotas for each slice.
  for (auto const &it : m_sliceCtrlById)
    {
      SliceId sliceId = it.first;
      int quota = 0;
      if (it.second)
        {
          quota = it.second->GetQuota ();
        }

      // Iterate over links setting the initial quotas.
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          bool success = true;
          success &= lInfo->UpdateQuota (LinkInfo::FWD, sliceId, quota);
          success &= lInfo->UpdateQuota (LinkInfo::BWD, sliceId, quota);
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");
        }
    }

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
          // Install high-priority individual Non-GBR meter entries
          // for slices with disabled bandwidth sharing.
          for (int s = 0; s < SliceId::ALL; s++)
            {
              SliceId slice = static_cast<SliceId> (s);
              if (GetSliceController (slice)->GetSharing () == OpMode::OFF)
                {
                  SlicingMeterInstall (lInfo, slice);
                }
            }
          // Install the low-priority shared Non-GBR meter entry
          // for other slices.
          SlicingMeterInstall (lInfo, SliceId::ALL);
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      // Install individual Non-GBR meter entries.
      for (auto &lInfo : LinkInfo::GetList ())
        {
          for (int s = 0; s < SliceId::ALL; s++)
            {
              SliceId slice = static_cast<SliceId> (s);
              SlicingMeterInstall (lInfo, slice);
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

  NS_ABORT_MSG ("OpenFlow error message: " << msgStr);
  return 0;
}

ofl_err
BackhaulController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = msg->stats->cookie;
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
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for inactive bearer teid " << rInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for bearer teid " << rInfo->GetTeidHex () <<
                   " with old priority " << prio);
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we are going to abort the program to avoid wrong results.
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

  NS_ABORT_MSG ("OpenFlow packet in message: " << msgStr);
  return 0;
}

void
BackhaulController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // For the switches on the backhaul network, install following rules:
  // -------------------------------------------------------------------------
  // Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  {
    // IP packets entering the switch from any port, except for those coming
    // from EPC elements connected to EPC ports (a high-priority match rule was
    // installed by NotifyEpcAttach function for this case).
    // Send the packet directly to the routing table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="    << INPUT_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << " goto:"     << ROUTE_TAB;
    DpctlExecute (swtch, cmd.str ());
  }
  {
    // Table miss entry.
    // Send the packet to the controller.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << INPUT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET
        << " apply:output=ctrl";
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // GTP-U packets entering the backhaul network. Classify the packet on the
  // corresponding logical slice based on the GTP-U TEID masked value.
  // Send the packet to the corresponding slice table.
  for (int s = 0; s < SliceId::ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      uint32_t sliceMasked = GetSvelteTeid (slice, 0, 0);

      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64"
          << ",table="      << CLASS_TAB
          << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET
          << " eth_type="   << IPV4_PROT_NUM
          << ",ip_proto="   << UDP_PROT_NUM
          << ",udp_src="    << GTPU_PORT
          << ",udp_dst="    << GTPU_PORT
          << ",gtpu_teid="  << (sliceMasked & TEID_SLICE_MASK)
          << "/"            << TEID_SLICE_MASK
          << " goto:"       << GetSliceTable (slice);
      DpctlExecute (swtch, cmd.str ());
    }

  // X2-C packets entering the backhaul network (identified by the UDP port
  // number as ns-3 currently offers no support for SCTP protocol). Set the
  // DSCP field for Expedited Forwarding.
  // Send the packet directly to the routing table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="                    << CLASS_TAB
        << ",flags="                    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="                 << IPV4_PROT_NUM
        << ",ip_proto="                 << UDP_PROT_NUM
        << ",udp_src="                  << X2C_PORT
        << ",udp_dst="                  << X2C_PORT
        << " apply:set_field=ip_dscp:"  << Ipv4Header::DSCP_EF
        << " goto:"                     << ROUTE_TAB;
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Slice tables (one for each slice) -- [from higher to lower priority]
  //
  // Entries will be installed here by TopologyRoutingInstall function.

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  // Entries will be installed here by NotifyTopologyBuilt function.
  // Entries will be installed here by the topology HandshakeSuccessful.
  {
    // Table miss entry.
    // Send the packet to the controller.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << ROUTE_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET
        << " apply:output=ctrl";
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Entries will be installed here by the topology HandshakeSuccessful.
  {
    // Table miss entry.
    // Send the packet to the output table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table="  << BANDW_TAB
        << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
        << " goto:"   << OUTPT_TAB;
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Output table -- [from higher to lower priority]
  //
  if (GetQosQueuesMode () == OpMode::ON)
    {
      // QoS output queues rules.
      for (auto const &it : m_queueByDscp)
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,prio=32"
              << ",table="        << OUTPT_TAB
              << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
              << " eth_type="     << IPV4_PROT_NUM
              << ",ip_dscp="      << static_cast<uint16_t> (it.first)
              << " write:queue="  << static_cast<uint32_t> (it.second);
          DpctlExecute (swtch, cmd.str ());
        }
    }
  {
    // Table miss entry. No instructions. This will trigger action set execute.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << OUTPT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET;
    DpctlExecute (swtch, cmd.str ());
  }
}

void
BackhaulController::SlicingExtraAdjust (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << lInfo << dir);

  NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::DYNA,
                 "Invalid inter-slice operation mode.");

  const LinkInfo::EwmaTerm longTerm = LinkInfo::LTERM;
  int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());

  // Check for the link bit rate statistics.
  int64_t linkRate = lInfo->GetLinkBitRate ();
  int64_t linkUsag = lInfo->GetUseBitRate (longTerm, dir);
  int64_t linkIdle = linkRate - linkUsag;
  int64_t linkGuar = linkRate * (1.0 - m_sliceLinkThs);

  if (linkIdle >= linkGuar)
    {
      // Link usage is OK. Iterate over slices in decreasing priority order,
      // assigning one new extra bit rate step for those slices that may
      // benefit from it.
      int maxSteps = linkIdle / stepRate;
      for (auto it = m_sliceCtrlPrio.rbegin ();
           it != m_sliceCtrlPrio.rend (); ++it)
        {
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdle = lInfo->GetIdlBitRate (longTerm, dir, slice);
          int64_t sliceOver = lInfo->GetOveBitRate (longTerm, dir, slice);
          int64_t sliceExtra = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Slice " << SliceIdStr (slice) <<
                        " direction " << LinkInfo::LinkDirStr (dir) <<
                        " extra bitrate " << sliceExtra <<
                        " over bitrate " << sliceOver <<
                        " idle bitrate " << sliceIdle);

          if (maxSteps > 0 && (sliceIdle < (stepRate / 2)))
            {
              // Increase the extra bit rate by one step.
              NS_LOG_DEBUG ("Increase extra bit rate.");
              bool success = lInfo->UpdateExtraBitRate (dir, slice, stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              SlicingMeterAdjust (lInfo, slice);
              maxSteps--;
            }
          else if ((sliceIdle > (stepRate * 2)) && (sliceExtra >= stepRate))
            {
              // Decrease one extra bit rate step from those slices that are
              // not using it to reduce unnecessary bit rate overbooking.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtraBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              SlicingMeterAdjust (lInfo, slice);
            }
        }
    }
  else
    {
      // Link usage is exceeding the safeguard threshold. Iterate over slices
      // in increasing priority order, removing some extra bit rate in use from
      // one (or more) slice to get the link usage below the safeguard
      // threshold again.
      int64_t getBack = linkGuar - linkIdle;

      for (auto it = m_sliceCtrlPrio.begin ();
           it != m_sliceCtrlPrio.end (); ++it)
        {
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdle = lInfo->GetIdlBitRate (longTerm, dir, slice);
          int64_t sliceOver = lInfo->GetOveBitRate (longTerm, dir, slice);
          int64_t sliceExtra = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Slice " << SliceIdStr (slice) <<
                        " direction " << LinkInfo::LinkDirStr (dir) <<
                        " extra bitrate " << sliceExtra <<
                        " over bitrate " << sliceOver <<
                        " idle bitrate " << sliceIdle);

          bool removed = false;
          while ((sliceExtra >= stepRate) && (getBack > 0))
            {
              removed = true;
              NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
              bool success = lInfo->UpdateExtraBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              sliceExtra -= stepRate;
              if (sliceIdle > stepRate)
                {
                  sliceIdle -= stepRate;
                }
              else
                {
                  getBack -= stepRate - sliceIdle;
                  sliceIdle = 0;
                }
            }

          if (removed)
            {
              SlicingMeterAdjust (lInfo, slice);
            }
          else if ((sliceIdle > (stepRate * 2)) && (sliceExtra >= stepRate))
            {
              // Decrease one extra bit rate step from those slices that are
              // not using it to reduce unnecessary bit rate overbooking.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtraBitRate (dir, slice, -stepRate);
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
      // Update the shared Non-GBR meter entry.
      slice = SliceId::ALL;
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

      int64_t meterBitRate = lInfo->GetMetBitRate (dir, slice);
      int64_t freeBitRate = lInfo->GetFreBitRate (dir, slice);
      uint64_t diffBitRate = std::abs (meterBitRate - freeBitRate);
      NS_LOG_DEBUG ("Slice " << SliceIdStr (slice) <<
                    " direction " << LinkInfo::LinkDirStr (dir) <<
                    " free bitrate " << freeBitRate <<
                    " diff bitrate " << diffBitRate <<
                    " step bitrate " << m_meterStep.GetBitRate ());
      if (diffBitRate >= m_meterStep.GetBitRate ())
        {
          uint32_t meterId = GetSvelteMeterId (slice, d);
          int64_t freeKbps = Bps2Kbps (freeBitRate);
          bool success = lInfo->SetMetBitRate (dir, slice, freeKbps * 1000);
          NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

          NS_LOG_INFO ("Update slice " << SliceIdStr (slice) <<
                       " direction " << LinkInfo::LinkDirStr (dir) <<
                       " meter ID " << GetUint32Hex (meterId) <<
                       " bitrate " << freeKbps << " Kbps");

          std::ostringstream cmd;
          cmd << "meter-mod cmd=mod"
              << ",flags="      << OFPMF_KBPS
              << ",meter="      << meterId
              << " drop:rate="  << freeKbps;
          DpctlExecute (lInfo->GetSwDpId (d), cmd.str ());
        }
    }
}

void
BackhaulController::SlicingMeterInstall (Ptr<LinkInfo> lInfo, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << slice);

  // Install slicing meters in both link directions.
  for (int d = 0; d <= LinkInfo::BWD; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);
      uint32_t meterId = GetSvelteMeterId (slice, d);
      int64_t freeKbps = Bps2Kbps (lInfo->GetFreBitRate (dir, slice));
      bool success = lInfo->SetMetBitRate (dir, slice, freeKbps * 1000);
      NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

      NS_LOG_INFO ("Created slice " << SliceIdStr (slice) <<
                   " direction " << LinkInfo::LinkDirStr (dir) <<
                   " meter ID " << GetUint32Hex (meterId) <<
                   " bitrate " << freeKbps << " Kbps");

      std::ostringstream cmd;
      cmd << "meter-mod cmd=add"
          << ",flags="      << OFPMF_KBPS
          << ",meter="      << meterId
          << " drop:rate="  << freeKbps;
      DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
    }
}

void
BackhaulController::SlicingTimeout (void)
{
  NS_LOG_FUNCTION (this);

  if (GetInterSliceMode () == SliceMode::DYNA)
    {
      // Adjust the extra bit rates in both directions for each backhaul link.
      for (auto &lInfo : LinkInfo::GetList ())
        {
          for (int d = 0; d <= LinkInfo::BWD; d++)
            {
              SlicingExtraAdjust (lInfo, static_cast<LinkInfo::LinkDir> (d));
            }
        }
    }

  // Schedule the next inter-slicing timeout operation.
  Simulator::Schedule (
    m_sliceTimeout, &BackhaulController::SlicingTimeout, this);
}

void
BackhaulController::StaticInitialize ()
{
  NS_LOG_FUNCTION_NOARGS ();

  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;

      // Populating the IP DSCP --> OpenFlow queue id mapping table.
      // DSCP_EF   --> OpenFlow queue 0 (priority)
      // DSCP_AF41 --> OpenFlow queue 1 (WRR)
      // DSCP_AF31 --> OpenFlow queue 2 (WRR)
      // DSCP_AF32 --> OpenFlow queue 2 (WRR)
      // DSCP_AF21 --> OpenFlow queue 3 (WRR)
      // DSCP_AF11 --> OpenFlow queue 4 (WRR)
      // DSCP_BE   --> OpenFlow queue 5 (WRR)
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 0));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF41, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF31, 2));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF32, 2));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF21, 3));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF11, 4));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DscpDefault, 5));
    }
}

} // namespace ns3
