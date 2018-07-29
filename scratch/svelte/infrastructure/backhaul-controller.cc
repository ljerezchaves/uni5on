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
#include "../metadata/routing-info.h"
#include "backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulController");
NS_OBJECT_ENSURE_REGISTERED (BackhaulController);

// Initializing BackhaulController static members.
BackhaulController::QciDscpMap_t BackhaulController::m_qciDscpTable;
BackhaulController::DscpQueueMap_t BackhaulController::m_dscpQueueTable;
BackhaulController::DscpTosMap_t BackhaulController::m_dscpTosTable;

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
    .AddAttribute ("Slicing",
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

bool
BackhaulController::DedicatedBearerRelease (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

// FIXME ver integração com o Slicecontroller
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//
//  // This bearer must be active.
//  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
//  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
//
//  TopologyBitRateRelease (rInfo);
//  m_bearerReleaseTrace (rInfo); // FIXME Verificar momento exato no slice controller
//  NS_LOG_INFO ("Bearer released by controller.");
//
//  // Deactivate and remove the bearer.
//  rInfo->SetActive (false);
//  return BearerRemove (rInfo);
  return true;
}

bool
BackhaulController::DedicatedBearerRequest (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

// FIXME ver integração com slicecontroller
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//
//  // This bearer must be inactive as we are going to reuse its metadata.
//  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
//  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");
//
//  // Update the P-GW TFT index and the blocked flag.
//  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
//  rInfo->SetBlocked (false);
//
//  // Check for available resources on P-GW and backhaul network and then
//  // reserve the requested bandwidth (don't change the order!).
//  bool success = true;
//  success &= TopologyBearerRequest (rInfo);
//  success &= PgwBearerRequest (rInfo);
//  success &= TopologyBitRateReserve (rInfo);
//  m_bearerRequestTrace (rInfo); // FIXME Verificar momento exato no slice controller
//  if (!success)
//    {
//      NS_LOG_INFO ("Bearer request blocked by controller.");
//      return false;
//    }
//
//  // Every time the application starts using an (old) existing bearer, let's
//  // reinstall the rules on the switches, which will increase the bearer
//  // priority. Doing this, we avoid problems with old 'expiring' rules, and
//  // we can even use new routing paths when necessary.
//  NS_LOG_INFO ("Bearer request accepted by controller.");
//
//  // Activate and install the bearer.
//  rInfo->SetActive (true);
//  return BearerInstall (rInfo);
  return true;
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

uint16_t
BackhaulController::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchDevices.GetN ();
}

uint8_t
BackhaulController::Dscp2Tos (Ipv4Header::DscpType dscp)
{
  NS_LOG_FUNCTION_NOARGS ();

  DscpTosMap_t::const_iterator it;
  it = BackhaulController::m_dscpTosTable.find (dscp);
  if (it != BackhaulController::m_dscpTosTable.end ())
    {
      return it->second;
    }
  NS_ABORT_MSG ("No ToS mapped value for DSCP " << dscp);
}

Ipv4Header::DscpType
BackhaulController::Qci2Dscp (EpsBearer::Qci qci)
{
  NS_LOG_FUNCTION_NOARGS ();

  QciDscpMap_t::const_iterator it;
  it = BackhaulController::m_qciDscpTable.find (qci);
  if (it != BackhaulController::m_qciDscpTable.end ())
    {
      return it->second;
    }
  NS_ABORT_MSG ("No DSCP mapped value for QCI " << qci);
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

uint64_t
BackhaulController::GetDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid switch index.");
  return m_switchDevices.Get (idx)->GetDatapathId ();
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
      DscpQueueMap_t::iterator it;
      for (it = BackhaulController::m_dscpQueueTable.begin ();
           it != BackhaulController::m_dscpQueueTable.end (); ++it)
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,table=4,prio=16 eth_type=0x800,"
              << "ip_dscp=" << static_cast<uint16_t> (it->first)
              << " write:queue=" << static_cast<uint32_t> (it->second);
          DpctlExecute (swtch, cmd.str ());
        }
    }

  // Table miss entry. No instructions. This will trigger action set execute.
  DpctlExecute (swtch, "flow-mod cmd=add,table=4,prio=0");
}

// bool
// BackhaulController::BearerInstall (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());
//
//   NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
//   rInfo->SetInstalled (false);
//
//   // Increasing the priority every time we (re)install routing rules.
//   rInfo->IncreasePriority ();
//
//   // Install the rules.
//   bool success = true;
//   success &= PgwRulesInstall (rInfo);
//   success &= TopologyRoutingInstall (rInfo);
//
//   rInfo->SetInstalled (success);
//   return success;
// }
//
// bool
// BackhaulController::BearerRemove (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());
//
//   NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");
//
//   // Remove the rules.
//   bool success = true;
//   success &= PgwRulesRemove (rInfo);
//   success &= TopologyRoutingRemove (rInfo);
//
//   rInfo->SetInstalled (!success);
//   return success;
// }

void
BackhaulController::StaticInitialize ()
{
  NS_LOG_FUNCTION_NOARGS ();

  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;

      // Populating the EPS QCI --> IP DSCP mapping table.
      // The following EPS QCI --> IP DSCP mapping was adapted from
      // https://ericlajoie.com/epcqos.html to meet our needs.
      //     GBR traffic: QCI 1, 2, 3 --> DSCP_EF
      //                  QCI 4       --> DSCP_AF41
      // Non-GBR traffic: QCI 5       --> DSCP_AF31
      //                  QCI 6, 7, 8 --> DSCP_AF11
      //                  QCI 9       --> DSCP_BE
      //
      // QCI 1: used by the HTC VoIP application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_CONV_VOICE,
                        Ipv4Header::DSCP_EF));

      // QCI 2: not in use.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_CONV_VIDEO,
                        Ipv4Header::DSCP_EF));

      // QCI 3: used by the MTC auto pilot application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_GAMING,
                        Ipv4Header::DSCP_EF));

      // QCI 4: used by the HTC live video application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_NON_CONV_VIDEO,
                        Ipv4Header::DSCP_AF41));

      // QCI 5: used by the MTC auto pilot application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_IMS,
                        Ipv4Header::DSCP_AF31));

      // QCI 6: used by the HTC buffered video application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_OPERATOR,
                        Ipv4Header::DSCP_AF11));

      // QCI 7: used by the HTC live video application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VOICE_VIDEO_GAMING,
                        Ipv4Header::DSCP_AF11));

      // QCI 8: used by the HTC HTTP application.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_PREMIUM,
                        Ipv4Header::DSCP_AF11));

      // QCI 9: used by default bearers and by aggregated traffic.
      BackhaulController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_DEFAULT,
                        Ipv4Header::DscpDefault)); // DSCP_BE


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


      // Populating the IP DSCP --> IP ToS mapping table.
      // This map is required here to ensure priority queueu compatibility
      // between the OpenFlow queues and the pfifo-fast queue discipline from
      // the traffic control module. We are mapping DSCP values to the IP ToS
      // byte that will be translated by the ns3::Socket::IpTos2Priority ()
      // method into the linux priority that is further used by the pfifo-fast
      // queue disc to select the priority queue.
      // See the ns3::Socket::IpTos2Priority for details.
      // DSCP_EF   --> ToS 0x10 --> priority 6 --> queue 0 (high priority).
      // DSCP_AF41 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
      // DSCP_AF31 --> ToS 0x18 --> priority 4 --> queue 1 (normal priority).
      // DSCP_AF11 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
      // DSCP_BE   --> ToS 0x08 --> priority 2 --> queue 2 (low priority).
      //
      // Mapping default and aggregated traffic to low priority queues.
      BackhaulController::m_dscpTosTable.insert (
        std::make_pair (Ipv4Header::DscpDefault, 0x08));

      // Mapping HTC VoIP and MTC auto pilot traffic to high priority queues.
      BackhaulController::m_dscpTosTable.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 0x10));

      // Mapping MTC Non-GBR traffic to normal priority queues.
      BackhaulController::m_dscpTosTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF31, 0x18));

      // Mapping other HTC traffics to normal priority queues.
      BackhaulController::m_dscpTosTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF41, 0x00));
      BackhaulController::m_dscpTosTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF11, 0x00));
    }
}

} // namespace ns3
