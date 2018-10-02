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
    .AddAttribute ("PriorityQueues",
                   "Priority output queues mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&BackhaulController::m_priorityQueues),
                   MakeEnumChecker (OpMode::OFF, "off",
                                    OpMode::ON,  "on"))
    .AddAttribute ("LinkSlicing",
                   "Link slicing mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::AUTO),
                   MakeEnumAccessor (&BackhaulController::m_slicing),
                   MakeEnumChecker (OpMode::OFF,  "off",
                                    OpMode::ON,   "on",
                                    OpMode::AUTO, "auto"))
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
BackhaulController::GetPriorityQueuesMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priorityQueues;
}

OpMode
BackhaulController::GetLinkSlicingMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_slicing;
}

double
BackhaulController::GetSliceUsage (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  double sliceUsage = 0;
  uint16_t count = 0;
  for (auto const &lInfo : LinkInfo::GetList ())
    {
      // We always have full-fuplex links in SVELTE backhaul.
      sliceUsage += lInfo->GetThpSliceRatio (LinkInfo::FWD, slice);
      sliceUsage += lInfo->GetThpSliceRatio (LinkInfo::BWD, slice);
      count += 2;
    }

  NS_ASSERT_MSG (count, "Invalid slice usage for empty topology.");
  return sliceUsage / static_cast<double> (count);
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

void
BackhaulController::BlockBearer (
  Ptr<RoutingInfo> rInfo, RoutingInfo::BlockReason reason) const
{
  NS_LOG_FUNCTION (this << rInfo << reason);

  NS_ASSERT_MSG (reason != RoutingInfo::NONE, "Invalid block reason.");
  rInfo->SetBlocked (true, reason);
}

Ptr<LinkInfo>
BackhaulController::GetLinkInfo (uint16_t idx1, uint16_t idx2) const
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  return LinkInfo::GetPointer (GetDpId (idx1), GetDpId (idx2));
}

Ptr<SliceController>
BackhaulController::GetSliceController (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  auto it = m_sliceCtrlById.find (slice);
  NS_ASSERT_MSG (it != m_sliceCtrlById.end (), "Slice controller not found.");
  return it->second;
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
    // GTP packets entering the ring network from any EPC port.
    // Send the packet to the classification table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,table=" << INPUT_TAB
        << ",prio=64,flags="
        << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
        << " eth_type=0x800,ip_proto=17"
        << ",udp_src=" << GTPU_PORT
        << ",udp_dst=" << GTPU_PORT
        << ",in_port=" << portNo
        << " goto:" << CLASS_TAB;
    DpctlSchedule (swDev->GetDatapathId (), cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  {
    // GTP packets addressed to EPC elements connected to this switch over EPC
    // ports. Write the output port into action set.
    // Send the packet directly to the output table.
    Mac48Address epcMac = Mac48Address::ConvertFrom (epcDev->GetAddress ());
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=256,table=" << ROUTE_TAB
        << " eth_type=0x800,eth_dst=" << epcMac
        << ",ip_dst=" << Ipv4AddressHelper::GetAddress (epcDev)
        << " write:output=" << portNo
        << " goto:" << OUTPT_TAB;
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
  LinkInfo::SliceQuotaMap_t quotas;
  for (auto const &it : m_sliceCtrlById)
    {
      UintegerValue quotaValue (0);
      if (it.second)
        {
          it.second->GetAttribute ("Quota", quotaValue);
        }
      std::pair<SliceId, uint16_t> entry (it.first, quotaValue.Get ());
      auto ret = quotas.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Error when creating quota map.");
    }

  // Iterate over links setting the initial quotas.
  for (auto const &lInfo : LinkInfo::GetList ())
    {
      bool success = true;
      success &= lInfo->SetSliceQuotas (LinkInfo::FWD, quotas);
      success &= lInfo->SetSliceQuotas (LinkInfo::BWD, quotas);
      NS_ASSERT_MSG (success, "Error when setting slice quotas.");
    }

  // ---------------------------------------------------------------------
  // Meter table
  //
  // Install slice meters only if the link slicing mechanism is enabled.
  if (GetLinkSlicingMode () != OpMode::OFF)
    {
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          SlicingMeterInstall (lInfo);
          lInfo->TraceConnectWithoutContext (
            "MeterAdjusted", MakeCallback (
              &BackhaulController::SlicingMeterAdjusted, this));
        }
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

  // Chain up for logging and abort.
  OFSwitch13Controller::HandleError (msg, swtch, xid);
  NS_ABORT_MSG ("Should not get here.");
}

ofl_err
BackhaulController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = msg->stats->cookie;
  uint16_t prio = msg->stats->priority;

  char *msgStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << msgStr);
  free (msgStr);

  // Since handlers must free the message when everything is ok,
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // Check for existing routing information for this bearer.
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "Routing metadata not found");

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed for inactive bearer teid " <<
                   rInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Old rule removed for bearer teid " <<
                   rInfo->GetTeidHex ());
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we are going to abort the program to avoid wrong results.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
BackhaulController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *msgStr = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_DEBUG ("Packet in match: " << msgStr);
  free (msgStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  NS_ABORT_MSG ("Should not get here.");
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
    // GTP packets entering the switch from any port other than EPC ports.
    // Send the packet to the routing table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32,table=" << INPUT_TAB
        << " eth_type=0x800,ip_proto=17"
        << ",udp_src=" << GTPU_PORT
        << ",udp_dst=" << GTPU_PORT
        << " goto:" << ROUTE_TAB;
    DpctlExecute (swtch, cmd.str ());
  }
  {
    // Table miss entry.
    // Send the packet to the controller.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0,table=" << INPUT_TAB
        << " apply:output=ctrl";
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // Entries will be installed here by TopologyRoutingInstall function.

  // -------------------------------------------------------------------------
  // Routing table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  // Entries will be installed here by NotifyTopologyBuilt function.
  {
    // FIXME Deveria mover isso pro ring-controller. São especificas da topo.
    // GTP packets classified at previous table. Write the output group into
    // action set based on metadata field.
    // Send the packet to the slicing table.
    std::ostringstream cmd1;
    cmd1 << "flow-mod cmd=add,prio=64,table=" << ROUTE_TAB
         << " meta=0x1"
         << " write:group=1 goto:" << SLICE_TAB;
    DpctlExecute (swtch, cmd1.str ());

    std::ostringstream cmd2;
    cmd2 << "flow-mod cmd=add,prio=64,table=" << ROUTE_TAB
         << " meta=0x2"
         << " write:group=2 goto:" << SLICE_TAB;
    DpctlExecute (swtch, cmd2.str ());
  }
  {
    // Table miss entry.
    // Send the packet to the controller.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0,table=" << ROUTE_TAB
        << " apply:output=ctrl";
    DpctlExecute (swtch, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Slicing table -- [from higher to lower priority]
  //
  // Entries will be installed here by the topology controller.
  {
    // Table miss entry.
    // Send the packet to the output table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0,table=" << SLICE_TAB
        << " goto:" << OUTPT_TAB;
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
          cmd << "flow-mod cmd=add,prio=16,table=" << OUTPT_TAB
              << " eth_type=0x800"
              << ",ip_dscp=" << static_cast<uint16_t> (it.first)
              << " write:queue=" << static_cast<uint32_t> (it.second);
          DpctlExecute (swtch, cmd.str ());
        }
    }

  {
    // Table miss entry. No instructions. This will trigger action set execute.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0,table=" << OUTPT_TAB;
    DpctlExecute (swtch, cmd.str ());
  }
}

void
BackhaulController::SlicingMeterAdjusted (
  Ptr<const LinkInfo> lInfo, LinkInfo::Direction dir, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice);

  NS_ASSERT_MSG (GetLinkSlicingMode () != OpMode::OFF, "Not supposed to "
                 "adjust slicing meters when network slicing mode is OFF.");

  // Identify the meter ID based on slicing operation mode. When the slicing
  // operation mode is ON, the traffic of each slice will be independently
  // monitored by slicing meters, so we ignore the fake shared slice. When the
  // slicing operation mode is AUTO, the traffic of all slices are monitored
  // together by the slicing meters, so we ignore individual slices.
  if ((GetLinkSlicingMode () == OpMode::ON && slice != SliceId::ALL)
      || (GetLinkSlicingMode () == OpMode::AUTO && slice == SliceId::ALL))
    {
      uint32_t meterId = GetSvelteMeterId (slice, static_cast<uint32_t> (dir));
      NS_LOG_INFO ("Updating slicing meter ID " << GetUint32Hex (meterId) <<
                   " for link info " << lInfo->GetSwDpId (0) <<
                   " to " << lInfo->GetSwDpId (1));

      // ---------------------------------------------------------------------
      // Meter table
      //
      // Update the proper slicing meter.
      uint64_t kbps = Bps2Kbps (lInfo->GetFreeBitRate (dir, slice));
      NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) << ": " <<
                    LinkInfo::DirectionStr (dir) <<
                    " link set to " << kbps << " Kbps");

      std::ostringstream cmd;
      cmd << "meter-mod cmd=mod"
          << ",flags=" << OFPMF_KBPS
          << ",meter=" << meterId
          << " drop:rate=" << kbps;
      DpctlExecute (lInfo->GetSwDpId (dir), cmd.str ());
    }
}

void
BackhaulController::SlicingMeterInstall (Ptr<const LinkInfo> lInfo)
{
  NS_LOG_FUNCTION (this << lInfo);

  if (GetLinkSlicingMode () == OpMode::ON)
    {
      // Install meter rules for each slice.
      for (int s = 0; s < SliceId::ALL; s++)
        {
          SliceId slice = static_cast<SliceId> (s);
          for (int d = 0; d <= LinkInfo::BWD; d++)
            {
              LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);
              uint32_t meterId = GetSvelteMeterId (slice, d);
              NS_LOG_INFO ("Creating slicing meter ID " <<
                           GetUint32Hex (meterId) <<
                           " for link info " << lInfo->GetSwDpId (0) <<
                           " to " << lInfo->GetSwDpId (1));

              uint64_t kbps = Bps2Kbps (lInfo->GetFreeBitRate (dir, slice));
              NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) <<
                            ": " << LinkInfo::DirectionStr (dir) <<
                            " link set to " << kbps << " Kbps");

              std::ostringstream cmd;
              cmd << "meter-mod cmd=add,flags=" << OFPMF_KBPS
                  << ",meter=" << meterId
                  << " drop:rate=" << kbps;
              DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
            }
        }
    }
  else if (GetLinkSlicingMode () == OpMode::AUTO)
    {
      // Install meter rules shared among slices.
      SliceId slice = SliceId::ALL;
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          LinkInfo::Direction dir = static_cast<LinkInfo::Direction> (d);
          uint32_t meterId = GetSvelteMeterId (slice, d);
          NS_LOG_INFO ("Creating slicing meter ID " <<
                       GetUint32Hex (meterId) <<
                       " for link info " << lInfo->GetSwDpId (0) <<
                       " to " << lInfo->GetSwDpId (1));

          uint64_t kbps = Bps2Kbps (lInfo->GetFreeBitRate (dir, slice));
          NS_LOG_DEBUG ("Link slice " << SliceIdStr (slice) <<
                        ": " << LinkInfo::DirectionStr (dir) <<
                        " link set to " << kbps << " Kbps");

          std::ostringstream cmd;
          cmd << "meter-mod cmd=add,flags=" << OFPMF_KBPS
              << ",meter=" << meterId
              << " drop:rate=" << kbps;
          DpctlSchedule (lInfo->GetSwDpId (d), cmd.str ());
        }
    }
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
      // DSCP_EF   --> OpenFlow queue 2 (high priority)
      // DSCP_AF41 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF32 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF31 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF21 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF11 --> OpenFlow queue 1 (normal priority)
      // DSCP_BE   --> OpenFlow queue 0 (low priority)
      //
      // Mapping default and aggregated traffic to low priority queues.
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DscpDefault, 0));

      // Mapping HTC VoIP and MTC auto pilot traffic to high priority queues.
      BackhaulController::m_queueByDscp.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 2));

      // Mapping other traffics to normal priority queues.
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
    }
}

} // namespace ns3
