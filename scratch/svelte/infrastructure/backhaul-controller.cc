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

// Initializing BackhaulController static members.
BackhaulController::DscpQueueMap_t BackhaulController::m_dscpQueueTable;

BackhaulController::BackhaulController ()
{
  NS_LOG_FUNCTION (this);

  StaticInitialize ();
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
                   "Network slicing mechanism operation mode.",
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
BackhaulController::GetSlicingMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_slicing;
}

double
BackhaulController::GetSliceUsage (LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << slice);

  double sliceUsage = 0;
  uint16_t count = 0;
  for (auto const &lInfo : LinkInfo::GetList ())
    {
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

  OFSwitch13Controller::DoDispose ();
}

void
BackhaulController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  OFSwitch13Controller::NotifyConstructionCompleted ();
}

void
BackhaulController::BlockBearer (
  Ptr<RoutingInfo> rInfo, RoutingInfo::BlockReason reason) const
{
  NS_LOG_FUNCTION (this << rInfo << reason);

  NS_ASSERT_MSG (reason != RoutingInfo::NOTBLOCKED, "Invalid block reason.");
  rInfo->SetBlocked (true, reason);
}

Ptr<LinkInfo>
BackhaulController::GetLinkInfo (uint16_t idx1, uint16_t idx2) const
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  return LinkInfo::GetPointer (GetDpId (idx1), GetDpId (idx2));
}

void
BackhaulController::NotifyBearerCreated (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // Set the LinkSlice for this bearer.
  rInfo->SetLinkSlice (LinkSlice::DFT);
  if (GetSlicingMode () != OpMode::OFF)
    {
      // TODO
    }
}

void
BackhaulController::NotifyEpcAttach (
  Ptr<OFSwitch13Device> swDev, uint32_t portNo, Ptr<NetDevice> epcDev)
{
  NS_LOG_FUNCTION (this << swDev << portNo << epcDev);

  // Configure port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // GTP packets entering the ring network from any EPC port. Send to the
  // Classification table.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64,flags=0x0007"
        << " eth_type=0x800,ip_proto=17"
        << ",udp_src=" << GTPU_PORT
        << ",udp_dst=" << GTPU_PORT
        << ",in_port=" << portNo
        << " goto:1";
  DpctlSchedule (swDev->GetDatapathId (), cmdIn.str ());

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // GTP packets addressed to EPC elements connected to this switch over EPC
  // ports. Write the output port into action set. Send the packet directly to
  // Output table.
  Mac48Address epcMac = Mac48Address::ConvertFrom (epcDev->GetAddress ());
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=2,prio=256 eth_type=0x800"
         << ",eth_dst=" << epcMac
         << ",ip_dst=" << Ipv4AddressHelper::GetAddress (epcDev)
         << " write:output=" << portNo
         << " goto:4";
  DpctlSchedule (swDev->GetDatapathId (), cmdOut.str ());
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
  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed for inactive bearer teid " << teid);
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Old rule removed for bearer teid " << teid);
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
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.

  // GTP packets entering the switch from any port other then EPC ports.
  // Send to Routing table.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=32"
      << " eth_type=0x800,ip_proto=17"
      << ",udp_src=" << GTPU_PORT
      << ",udp_dst=" << GTPU_PORT
      << " goto:2";
  DpctlExecute (swtch, cmd.str ());

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- Classification table -- [from higher to lower priority]
  //
  // Entries will be installed here by TopologyRoutingInstall function.

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  // Entries will be installed here by NotifyTopologyBuilt function.

  // GTP packets classified at previous table. Write the output group into
  // action set based on metadata field. Send the packet to Slicing table.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x1"
                " write:group=1 goto:3");
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x2"
                " write:group=2 goto:3");

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 3 -- Slicing table -- [from higher to lower priority]
  //
  // FIXME Revisar o link slicing.
  if (GetSlicingMode () == OpMode::ON)
    {
      // FIXME This should be automatic depending on the number of slices.
      // When the network slicing operation mode is ON, the Non-GBR traffic of
      // each slice will be monitored independently. Here is how we are using
      // meter IDs:
      // DFT slice: meter ID 1 -> clockwise FWD direction
      //            meter ID 2 -> counterclockwise BWD direction
      // MTC slice: meter ID 3 -> clockwise FWD direction
      //            meter ID 4 -> counterclockwise BWD direction
      // In current implementation we don't have Non-GBR traffic on GBR slice,
      // so we don't need meters for this slice.

      // DFT Non-GBR packets are filtered by DSCP fields DSCP_AF11 and
      // DSCP_BE. Apply Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF11 (DSCP decimal 10)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x1,ip_dscp=10"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x2,ip_dscp=10"
                    " meter:2 goto:4");

      // DSCP_BE (DSCP decimal 0)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x1,ip_dscp=0"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x2,ip_dscp=0"
                    " meter:2 goto:4");

      // MTC Non-GBR packets are filtered by DSCP field DSCP_AF31.
      // Apply MTC Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF31 (DSCP decimal 26)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x1,ip_dscp=26"
                    " meter:3 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x2,ip_dscp=26"
                    " meter:4 goto:4");
    }
  else if (GetSlicingMode () == OpMode::AUTO)
    {
      // When the network slicing operation mode is AUTO, the Non-GBR traffic
      // of all slices will be monitored together. Here is how we are using
      // meter IDs:
      // Meter ID 1 -> clockwise FWD direction
      // Meter ID 2 -> counterclockwise BWD direction

      // Non-GBR packets are filtered by DSCP fields DSCP_AF31, DSCP_AF11, and
      // DSCP_BE. Apply Non-GBR meter band. Send the packet to Output table.
      //
      // DSCP_AF31 (DSCP decimal 26)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x1,ip_dscp=26"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=15"
                    " eth_type=0x800,meta=0x2,ip_dscp=26"
                    " meter:2 goto:4");

      // DSCP_AF11 (DSCP decimal 10)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x1,ip_dscp=10"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x2,ip_dscp=10"
                    " meter:2 goto:4");

      // DSCP_BE (DSCP decimal 0)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x1,ip_dscp=0"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x2,ip_dscp=0"
                    " meter:2 goto:4");
    }

  // Table miss entry. Send the packet to Output table
  DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=0 goto:4");

  // -------------------------------------------------------------------------
  // Table 4 -- Output table -- [from higher to lower priority]
  //
  if (GetPriorityQueuesMode () == OpMode::ON)
    {
      // Priority output queues rules.
      for (auto const &it : m_dscpQueueTable)
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,table=4,prio=16 eth_type=0x800,"
              << "ip_dscp=" << static_cast<uint16_t> (it.first)
              << " write:queue=" << static_cast<uint32_t> (it.second);
          DpctlExecute (swtch, cmd.str ());
        }
    }

  // Table miss entry. No instructions. This will trigger action set execute.
  DpctlExecute (swtch, "flow-mod cmd=add,table=4,prio=0");
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
      // DSCP_AF31 --> OpenFlow queue 1 (normal priority)
      // DSCP_AF11 --> OpenFlow queue 1 (normal priority)
      // DSCP_BE   --> OpenFlow queue 0 (low priority)
      //
      // Mapping default and aggregated traffic to low priority queues.
      BackhaulController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DscpDefault, 0));

      // Mapping HTC VoIP and MTC auto pilot traffic to high priority queues.
      BackhaulController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 2));

      // Mapping other traffics to normal priority queues.
      BackhaulController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF41, 1));
      BackhaulController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF31, 1));
      BackhaulController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF11, 1));
    }
}

} // namespace ns3
