/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#include "slice-controller.h"
#include "svelte-mme.h"
#include "../infrastructure/backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceController");
NS_OBJECT_ENSURE_REGISTERED (SliceController);

// Initializing SliceController static members.
// TODO

SliceController::SliceController ()
{
  NS_LOG_FUNCTION (this);

  // The S-GW side of S11 APs
  m_s11SapSgw = new MemberEpcS11SapSgw<SliceController> (this);

//  // FIXME Receber o MME como parâmetro no construtor
//  m_mme = CreateObject<SdranMme> ();
//  m_mme->SetS11SapSgw (m_s11SapSgw);
//  m_s11SapMme = m_mme->GetS11SapMme ();
}

SliceController::~SliceController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SliceController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SliceController")
    .SetParent<OFSwitch13Controller> ()
  ;
  return tid;
}

bool
SliceController::DedicatedBearerRelease (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

//   SgwRulesRemove (RoutingInfo::GetPointer (teid));
//   m_epcCtrlApp->DedicatedBearerRelease (bearer, teid);
//   FIXME Copiar EpcController::DedicatedBearerRelease
  return true;
}

bool
SliceController::DedicatedBearerRequest (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

//   FIXME EpcController::DedicatedBearerRequest
//   if (m_epcCtrlApp->DedicatedBearerRequest (bearer, teid))
//     {
//       return SgwRulesInstall (RoutingInfo::GetPointer (teid));
//     }
  return false;
}
//
// void
// SliceController::NotifyEnbAttach (uint16_t cellId, uint32_t sgwS1uPortNo)
// {
//   NS_LOG_FUNCTION (this << cellId << sgwS1uPortNo);
//
//   // Register this controller by cell ID for further usage.
//   RegisterController (Ptr<SliceController> (this), cellId);
//
//   // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
//   // Internet are sent to table 2, where rules will match the flow and set both
//   // TEID and P-GW address on tunnel metadata.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
//       << ",in_port=" << sgwS1uPortNo
//       << ",ip_dst=" << LteNetwork::m_sgiAddr
//       << "/" << LteNetwork::m_sgiMask.GetPrefixLength ()
//       << " goto:2";
//   DpctlSchedule (m_sgwDpId, cmd.str ());
// }
//
// void
// SliceController::NotifySgwAttach (
//   uint32_t sgwS5PortNo, Ptr<NetDevice> sgwS5Dev, uint32_t mtcGbrTeid,
//   uint32_t mtcNonTeid)
// {
//   NS_LOG_FUNCTION (this << sgwS5PortNo << sgwS5Dev << mtcGbrTeid <<
//                    mtcNonTeid);
//
//   m_sgwS5Addr = LteNetwork::GetIpv4Addr (sgwS5Dev);
//   m_sgwS5PortNo = sgwS5PortNo;
//
//   // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
//   // network are sent to table 1, where rules will match the flow and set both
//   // TEID and eNB address on tunnel metadata.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
//       << ",in_port=" << sgwS5PortNo
//       << ",ip_dst=" << LteNetwork::m_ueAddr
//       << "/" << LteNetwork::m_ueMask.GetPrefixLength ()
//       << " goto:1";
//   DpctlSchedule (m_sgwDpId, cmd.str ());
//
//   // The mtcGbrTeid != 0 or mtcNonTeid != 0 means that MTC traffic aggregation
//   // is enable. Install high-priority match rules on default table for
//   // aggregating traffic from all MTC UEs on the proper uplink S5 GTP tunnel.
//   if (mtcGbrTeid != 0)
//     {
//       // Print MTC aggregation TEID and P-GW IPv4 address into tunnel metadata.
//       Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (mtcGbrTeid);
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Instal OpenFlow MTC aggregation rule. We are using the DSCP field to
//       // distinguish GBR/Non-GBR packets. Packets inside the S-GW are not
//       // encapsulated, so the DSCP field is, in fact, the IP ToS set by the
//       // application socket.
//       uint8_t ipTos = SliceController::Dscp2Tos (rInfo->GetDscp ());
//       std::ostringstream cmd;
//       cmd << "flow-mod cmd=add,table=0,prio=65520 eth_type=0x800"
//           << ",ip_src=" << LteNetwork::m_mtcAddr
//           << "/" << LteNetwork::m_mtcMask.GetPrefixLength ()
//           << ",ip_dscp=" << (ipTos >> 2)
//           << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << m_sgwS5PortNo;
//       DpctlSchedule (m_sgwDpId, cmd.str ());
//     }
//   if (mtcNonTeid != 0)
//     {
//       // Print MTC aggregation TEID and P-GW IPv4 address into tunnel metadata.
//       Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (mtcNonTeid);
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Instal OpenFlow MTC aggregation rule. We are using the DSCP field to
//       // distinguish GBR/Non-GBR packets. Packets inside the S-GW are not
//       // encapsulated, so the DSCP field is, in fact, the IP ToS set by the
//       // application socket.
//       uint8_t ipTos = SliceController::Dscp2Tos (rInfo->GetDscp ());
//       std::ostringstream cmd;
//       cmd << "flow-mod cmd=add,table=0,prio=65520 eth_type=0x800"
//           << ",ip_src=" << LteNetwork::m_mtcAddr
//           << "/" << LteNetwork::m_mtcMask.GetPrefixLength ()
//           << ",ip_dscp=" << (ipTos >> 2)
//           << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << m_sgwS5PortNo;
//       DpctlSchedule (m_sgwDpId, cmd.str ());
//     }
// }

OperationMode
SliceController::GetAggregationMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_aggregation;
}

OperationMode
SliceController::GetPgwAdaptiveMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftAdaptive;
}

void
SliceController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_mme = 0;
  delete (m_s11SapSgw);

  Object::DoDispose ();
}

ofl_err
SliceController::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Chain up for logging and abort.
  OFSwitch13Controller::HandleError (msg, swtch, xid);
  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
SliceController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  return 0; // FIXME remover
// FIXME Comentado apenas para remover a dependencia do rinfo
//  uint32_t teid = msg->stats->cookie;
//  uint16_t prio = msg->stats->priority;
//
//  char *msgStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
//  NS_LOG_DEBUG ("Flow removed: " << msgStr);
//  free (msgStr);
//
//  // Since handlers must free the message when everything is ok,
//  // let's remove it now, as we already got the necessary information.
//  ofl_msg_free_flow_removed (msg, true, 0);
//
//  // Check for existing routing information for this bearer.
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);
//
//  // When a flow is removed, check the following situations:
//  // 1) The application is stopped and the bearer must be inactive.
//  if (!rInfo->IsActive ())
//    {
//      NS_LOG_INFO ("Rule removed for inactive bearer teid " << teid);
//      return 0;
//    }
//
//  // 2) The application is running and the bearer is active, but the
//  // application has already been stopped since last rule installation. In this
//  // case, the bearer priority should have been increased to avoid conflicts.
//  if (rInfo->GetPriority () > prio)
//    {
//      NS_LOG_INFO ("Old rule removed for bearer teid " << teid);
//      return 0;
//    }
//
//  // 3) The application is running and the bearer is active. This is the
//  // critical situation. For some reason, the traffic absence lead to flow
//  // expiration, and we are going to abort the program to avoid wrong results.
//  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
//  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
SliceController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *msgStr = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_DEBUG ("Packet in match: " << msgStr);
  free (msgStr);

  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);
  return 0;
}

void
SliceController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Configure S-GW port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
  // network are sent to table 1, where rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  //
  // Entries will be installed here by NotifySgwAttach function.

  // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
  // Internet are sent to table 2, where rules will match the flow and set both
  // TEID and P-GW address on tunnel metadata.
  //
  // Entries will be installed here by NotifyEnbAttach function.

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- S-GW downlink forward table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.

  // -------------------------------------------------------------------------
  // Table 2 -- S-GW uplink forward table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
SliceController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

//  FIXME O código abaixo veio do antigo EpcController
//  uint16_t cellId = msg.uli.gci;
//  uint64_t imsi = msg.imsi;
//
//  Ptr<SdranController> sdranCtrl = SdranController::GetPointer (cellId);
//  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
//  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
//
//  // Iterate over request message and create the response message.
//  EpcS11SapMme::CreateSessionResponseMessage res;
//  res.teid = imsi;
//
//  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
//  for (bit = msg.bearerContextsToBeCreated.begin ();
//       bit != msg.bearerContextsToBeCreated.end ();
//       ++bit)
//    {
//      NS_ABORT_IF (EpcController::m_teidCount > EpcController::m_teidEnd);
//
//      uint32_t teid = EpcController::m_teidCount++;
//      bool isDefault = res.bearerContextsCreated.empty ();
//
//      EpcS11SapMme::BearerContextCreated bearerContext;
//      bearerContext.sgwFteid.teid = teid;
//      bearerContext.sgwFteid.address = enbInfo->GetSgwS1uAddr ();
//      bearerContext.epsBearerId = bit->epsBearerId;
//      bearerContext.bearerLevelQos = bit->bearerLevelQos;
//      bearerContext.tft = bit->tft;
//      res.bearerContextsCreated.push_back (bearerContext);
//
//      // Add the TFT entry to the UeInfo (don't move this command from here).
//      ueInfo->AddTft (bit->tft, teid);
//
//      // Create the routing metadata for this bearer.
//      Ptr<RoutingInfo> rInfo = CreateObject<RoutingInfo> (
//          teid, bearerContext, imsi, isDefault, ueInfo->IsMtc ());
//      rInfo->SetPgwS5Addr (m_pgwS5Addr);
//      rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
//      rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
//      TopologyBearerCreated (rInfo);
//
//      // Set the network slice for this bearer.
//      if (GetSlicingMode () != OperationMode::OFF)
//        {
//          if (rInfo->IsMtc ())
//            {
//              rInfo->SetSlice (Slice::MTC);
//            }
//          else if (rInfo->IsGbr ())
//            {
//              rInfo->SetSlice (Slice::GBR);
//            }
//        }
//
//      // Check for the proper traffic aggregation mode for this bearer.
//      OperationMode mode;
//      mode = rInfo->IsMtc () ? GetMtcAggregMode () : GetHtcAggregMode ();
//      if (rInfo->IsHtc () && rInfo->IsDefault ())
//        {
//          // Never aggregates the default HTC bearer.
//          mode = OperationMode::OFF;
//        }
//
//      // Set the traffic aggregation operation mode.
//      Ptr<S5AggregationInfo> aggInfo = rInfo->GetObject<S5AggregationInfo> ();
//      aggInfo->SetOperationMode (mode);
//
//      if (rInfo->IsDefault ())
//        {
//          // Configure this default bearer.
//          rInfo->SetPriority (0x7F);
//          rInfo->SetTimeout (0);
//
//          // For logic consistence, let's check for available resources.
//          bool success = true;
//          success &= TopologyBearerRequest (rInfo);
//          success &= PgwBearerRequest (rInfo);
//          success &= TopologyBitRateReserve (rInfo);
//          NS_ASSERT_MSG (success, "Default bearer must be accepted.");
//          m_bearerRequestTrace (rInfo);
//
//          // Activate and install the bearer.
//          rInfo->SetActive (true);
//          bool installed = BearerInstall (rInfo);
//          NS_ASSERT_MSG (installed, "Default bearer must be installed.");
//        }
//      else
//        {
//          // Configure this dedicated bearer.
//          rInfo->SetPriority (0x1FFF);
//          rInfo->SetTimeout (m_flowTimeout);
//        }
//    }
//
//  // Fire trace source notifying the created session.
//  m_sessionCreatedTrace (imsi, cellId, res.bearerContextsCreated);
//
//   // Install S-GW rules for default bearer.
//   BearerContext_t defaultBearer = msg.bearerContextsCreated.front ();
//   NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
//   uint32_t teid = defaultBearer.sgwFteid.teid;
//   SgwRulesInstall (RoutingInfo::GetPointer (teid));
//
//   // Forward the response message to the MME.
//   m_s11SapMme->CreateSessionResponse (msg);
}

void
SliceController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

//   uint64_t imsi = msg.teid;
//
//   EpcS11SapMme::DeleteBearerRequestMessage res;
//   res.teid = imsi;
//
//   std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit;
//   for (bit = msg.bearerContextsToBeRemoved.begin ();
//        bit != msg.bearerContextsToBeRemoved.end ();
//        ++bit)
//     {
//       EpcS11SapMme::BearerContextRemoved bearerContext;
//       bearerContext.epsBearerId = bit->epsBearerId;
//       res.bearerContextsRemoved.push_back (bearerContext);
//     }
//
//   m_s11SapMme->DeleteBearerRequest (res);
}

void
SliceController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // Nothing to do here.
}

void
SliceController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // In current implementation, this Modify Bearer Request is triggered only by
  // X2 handover procedures. There is no actual bearer modification, for now we
  // just support the minimum needed for path switch request (handover). There
  // is no need to forward the request message to the P-GW.
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = msg.teid;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  // FIXME m_s11SapMme->ModifyBearerResponse (res);
}

// bool
// SliceController::SgwRulesInstall (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//   NS_LOG_INFO ("Installing S-GW rules for bearer teid " << rInfo->GetTeid ());
//   Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
//   Ptr<const EnbInfo> enbInfo = EnbInfo::GetPointer (ueInfo->GetCellId ());
//
//   // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
//   std::string flagsStr ("0x0007");
//
//   // Print the cookie and buffer values in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Configure downlink.
//   if (rInfo->HasDownlinkTraffic ())
//     {
//       // Print downlink TEID and destination IPv4 address into tunnel metadata.
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (enbInfo->GetEnbS1uAddr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Build the dpctl command string.
//       std::ostringstream cmd, act;
//       cmd << "flow-mod cmd=add,table=1"
//           << ",flags=" << flagsStr
//           << ",cookie=" << cookieStr
//           << ",prio=" << rInfo->GetPriority ()
//           << ",idle=" << rInfo->GetTimeout ();
//
//       // Instruction: apply action: set tunnel ID, output port.
//       act << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << enbInfo->GetSgwS1uPortNo ();
//
//       // Install one downlink dedicated bearer rule for each packet filter.
//       Ptr<EpcTft> tft = rInfo->GetTft ();
//       for (uint8_t i = 0; i < tft->GetNFilters (); i++)
//         {
//           EpcTft::PacketFilter filter = tft->GetFilter (i);
//           if (filter.direction == EpcTft::UPLINK)
//             {
//               continue;
//             }
//
//           // Install rules for TCP traffic.
//           if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=6"
//                     << ",ip_dst=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_src=" << filter.remoteAddress
//                         << ",tcp_src=" << filter.remotePortStart;
//                 }
//
//               std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdTcpStr);
//             }
//
//           // Install rules for UDP traffic.
//           else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=17"
//                     << ",ip_dst=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_src=" << filter.remoteAddress
//                         << ",udp_src=" << filter.remotePortStart;
//                 }
//
//               std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdUdpStr);
//             }
//         }
//     }
//
//   // Configure uplink.
//   if (rInfo->HasUplinkTraffic () && !rInfo->IsAggregated ())
//     {
//       // Print uplink TEID and destination IPv4 address into tunnel metadata.
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Build the dpctl command string.
//       std::ostringstream cmd, act;
//       cmd << "flow-mod cmd=add,table=2"
//           << ",flags=" << flagsStr
//           << ",cookie=" << cookieStr
//           << ",prio=" << rInfo->GetPriority ()
//           << ",idle=" << rInfo->GetTimeout ();
//
//       // Check for meter entry.
//       Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//       if (meterInfo && meterInfo->HasUp ())
//         {
//           if (!meterInfo->IsUpInstalled ())
//             {
//               // Install the per-flow meter entry.
//               DpctlExecute (m_sgwDpId, meterInfo->GetUpAddCmd ());
//               meterInfo->SetUpInstalled (true);
//             }
//
//           // Instruction: meter.
//           act << " meter:" << rInfo->GetTeid ();
//         }
//
//       // Instruction: apply action: set tunnel ID, output port.
//       act << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << m_sgwS5PortNo;
//
//       // Install one uplink dedicated bearer rule for each packet filter.
//       Ptr<EpcTft> tft = rInfo->GetTft ();
//       for (uint8_t i = 0; i < tft->GetNFilters (); i++)
//         {
//           EpcTft::PacketFilter filter = tft->GetFilter (i);
//           if (filter.direction == EpcTft::DOWNLINK)
//             {
//               continue;
//             }
//
//           // Install rules for TCP traffic.
//           if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=6"
//                     << ",ip_src=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_dst=" << filter.remoteAddress
//                         << ",tcp_dst=" << filter.remotePortStart;
//                 }
//
//               std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdTcpStr);
//             }
//
//           // Install rules for UDP traffic.
//           else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=17"
//                     << ",ip_src=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_dst=" << filter.remoteAddress
//                         << ",udp_dst=" << filter.remotePortStart;
//                 }
//
//               std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdUdpStr);
//             }
//         }
//     }
//   return true;
// }
//
// bool
// SliceController::SgwRulesRemove (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//   NS_LOG_INFO ("Removing S-GW rules for bearer teid " << rInfo->GetTeid ());
//
//   // Print the cookie value in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Remove flow entries for this TEID.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=del,"
//       << ",cookie=" << cookieStr
//       << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
//   DpctlExecute (m_sgwDpId, cmd.str ());
//
//   // Remove meter entry for this TEID.
//   Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//   if (meterInfo && meterInfo->IsUpInstalled ())
//     {
//       DpctlExecute (m_sgwDpId, meterInfo->GetDelCmd ());
//       meterInfo->SetUpInstalled (false);
//     }
//   return true;
// }
} // namespace ns3
