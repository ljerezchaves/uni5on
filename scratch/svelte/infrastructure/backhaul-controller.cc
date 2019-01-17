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
    .AddAttribute ("BlockPolicy",
                   "Switch overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_blockPolicy),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("BlockThs",
                   "Switch overloaded block threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&BackhaulController::m_blockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
    .AddAttribute ("InterSliceMode",
                   "Inter-slice operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceMode::NONE),
                   MakeEnumAccessor (&BackhaulController::m_slicing),
                   MakeEnumChecker (SliceMode::NONE,
                                    SliceModeStr (SliceMode::NONE),
                                    SliceMode::SHAR,
                                    SliceModeStr (SliceMode::SHAR),
                                    SliceMode::STAT,
                                    SliceModeStr (SliceMode::STAT),
                                    SliceMode::DYNA,
                                    SliceModeStr (SliceMode::DYNA)))
    .AddAttribute ("MeterStep",
                   "Default meter bit rate adjustment step.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mbps")),
                   MakeDataRateAccessor (&BackhaulController::m_meterStep),
                   MakeDataRateChecker ())
    .AddAttribute ("PriorityQueues",
                   "Priority output queues mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_priorityQueues),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
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
BackhaulController::GetBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockPolicy;
}

double
BackhaulController::GetBlockThreshold (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockThs;
}

SliceMode
BackhaulController::GetInterSliceMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_slicing;
}

OpMode
BackhaulController::GetPriorityQueuesMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priorityQueues;
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
  return std::make_tuple (lInfo, dir, LinkInfo::Invert (dir));
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

void
BackhaulController::NotifySlicesBuilt (ApplicationContainer &controllers)
{
  NS_LOG_FUNCTION (this);

  // Update the slice controller map with configured controller applications.
  ApplicationContainer::Iterator it;
  for (it = controllers.Begin (); it != controllers.End (); ++it)
    {
      Ptr<SliceController> ctrl = DynamicCast<SliceController> (*it);
      SliceId slice = ctrl->GetSliceId ();

      auto it = m_sliceCtrlById.find (slice);
      NS_ASSERT_MSG (it != m_sliceCtrlById.end (), "Invalid slice ID.");
      it->second = ctrl;
    }

  // Create the slice quota map for all slices (including those not configured
  // with null application pointer).
  for (auto const &it : m_sliceCtrlById)
    {
      SliceId sliceId = it.first;
      IntegerValue quotaValue (0);
      if (it.second)
        {
          it.second->GetAttribute ("Quota", quotaValue);
        }
      int quota = quotaValue.Get ();

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
  // Install inter-slicing meters.
  for (auto &lInfo : LinkInfo::GetList ())
    {
      SlicingMeterInstall (lInfo);
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
  if (GetPriorityQueuesMode () == OpMode::ON)
    {
      // Priority output queues rules.
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
BackhaulController::SlicingMeterAdjusted (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice);

  if (GetInterSliceMode () == SliceMode::NONE)
    {
      // Nothing to do when inter-slicing is disabled.
      return;
    }

  // Identify the meter ID based on inter-slice operation mode. When using the
  // static inter-slicing, the traffic of each slice will be independently
  // monitored by slicing meters, so we ignore the fake shared slice. When
  // using the Non-GBR shared inter-slicing, the Non-GBR traffic of all slices
  // are monitored together by the slicing meters, so we ignore individual
  // slices.
  if ((GetInterSliceMode () == SliceMode::STAT && slice != SliceId::ALL)
      || (GetInterSliceMode () == SliceMode::SHAR && slice == SliceId::ALL))
    {
      uint32_t meterId = GetSvelteMeterId (slice, static_cast<uint32_t> (dir));
      NS_LOG_INFO ("Updating slicing meter ID " << GetUint32Hex (meterId) <<
                   " for link info " << lInfo->GetSwDpId (0) <<
                   " to " << lInfo->GetSwDpId (1));

      // Update the proper slicing meter.
      int64_t kbps = Bps2Kbps (lInfo->GetFreeBitRate (dir, QosType::NON, slice));
      NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) << ": " <<
                    LinkInfo::LinkDirStr (dir) <<
                    " link set to " << kbps << " Kbps");
      // ---------------------------------------------------------------------
      // Meter table
      //
      std::ostringstream cmd;
      cmd << "meter-mod cmd=mod"
          << ",flags="      << OFPMF_KBPS
          << ",meter="      << meterId
          << " drop:rate="  << kbps;
      DpctlExecute (lInfo->GetSwDpId (dir), cmd.str ());
    }
}

void
BackhaulController::SlicingMeterInstall (Ptr<LinkInfo> lInfo)
{
  NS_LOG_FUNCTION (this << lInfo);

  bool success = true;
  switch (GetInterSliceMode ())
    {
    case SliceMode::NONE:
      {
        // Nothing to do when inter-slicing is disabled.
        return;
      }
    case SliceMode::SHAR:
      {
        // Install the shared Non-GBR meter entry for all slice
        // on each port direction (FWD and BWD).
        for (int d = 0; d <= LinkInfo::BWD; d++)
          {
            LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);
            uint32_t meterId = GetSvelteMeterId (SliceId::ALL, d);
            int64_t kbps = Bps2Kbps (
                lInfo->GetFreeBitRate (dir, QosType::NON, SliceId::ALL));
            success &= lInfo->SetMeterBitRate (dir, SliceId::ALL, kbps * 1000);

            NS_LOG_INFO ("Slicing meter ID " << GetUint32Hex (meterId) <<
                         " created for slice " << SliceIdStr (SliceId::ALL) <<
                         " in link info from " << lInfo->GetSwDpId (0) <<
                         " to " << lInfo->GetSwDpId (1) <<
                         " in " << LinkInfo::LinkDirStr (dir) <<
                         " direction set to " << kbps << " Kbps");

            std::ostringstream cmd;
            cmd << "meter-mod cmd=add"
                << ",flags="      << OFPMF_KBPS
                << ",meter="      << meterId
                << " drop:rate="  << kbps;
            DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
          }
        break;
      }
    case SliceMode::STAT:
    case SliceMode::DYNA:
      {
        // Install individual Non-GBR meter entries for each slice
        // on each port direction (FWD and BWD).
        for (int s = 0; s < SliceId::ALL; s++)
          {
            SliceId slice = static_cast<SliceId> (s);
            for (int d = 0; d <= LinkInfo::BWD; d++)
              {
                LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);
                uint32_t meterId = GetSvelteMeterId (slice, d);
                int64_t kbps = Bps2Kbps (
                    lInfo->GetFreeBitRate (dir, QosType::NON, slice));
                success &= lInfo->SetMeterBitRate (dir, slice, kbps * 1000);

                NS_LOG_INFO ("Slicing meter ID " << GetUint32Hex (meterId) <<
                             " created for slice " << SliceIdStr (slice) <<
                             " in link info from " << lInfo->GetSwDpId (0) <<
                             " to " << lInfo->GetSwDpId (1) <<
                             " in " << LinkInfo::LinkDirStr (dir) <<
                             " direction set to " << kbps << " Kbps");

                std::ostringstream cmd;
                cmd << "meter-mod cmd=add"
                    << ",flags="      << OFPMF_KBPS
                    << ",meter="      << meterId
                    << " drop:rate="  << kbps;
                DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
              }
          }
        break;
      }
    default:
      {
        NS_LOG_WARN ("Undefined inter-slicing operation mode.");
        break;
      }
    }
  NS_ASSERT_MSG (success, "Error when setting meter bit rates.");
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
      // DSCP_EF   --> OpenFlow queue 0 (high priority)
      // DSCP_AF41 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF32 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF31 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF21 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF11 --> OpenFlow queue 1 (normal priority)
      // DSCP_BE   --> OpenFlow queue 2 (low priority)
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 0));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF41, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF32, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF31, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF21, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_AF11, 1));
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DscpDefault, 2));
    }
}

} // namespace ns3
