/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#include "sdran-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdranController");
NS_OBJECT_ENSURE_REGISTERED (SdranController);

SdranController::SdranController ()
  : m_s11SapMme (0)
{
  NS_LOG_FUNCTION (this);

  // The S-GW side of S11 AP
  m_s11SapSgw = new MemberEpcS11SapSgw<SdranController> (this);

  m_mme = SdmnMme::Get ();    // FIXME should be independent.
  m_mme->SetS11SapSgw (m_s11SapSgw);
  m_s11SapMme = m_mme->GetS11SapMme ();
}

SdranController::~SdranController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdranController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdranController")
    .SetParent<OFSwitch13Controller> ()
    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &SdranController::m_bearerRequestTrace),
                     "ns3::SdranController::BearerTracedCallback")
    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &SdranController::m_bearerReleaseTrace),
                     "ns3::SdranController::BearerTracedCallback")
    .AddTraceSource ("SessionCreated", "The session created trace source.",
                     MakeTraceSourceAccessor (
                       &SdranController::m_sessionCreatedTrace),
                     "ns3::SdranController::SessionCreatedTracedCallback")
  ;
  return tid;
}

bool
SdranController::RequestDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  // TODO
  return true;
}

bool
SdranController::ReleaseDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  // TODO
  return true;
}

void
SdranController::NotifySessionCreated (
  uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address pgwAddr,
  BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr << pgwAddr);

  // TODO
}

void
SdranController::NewS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
                              Ptr<NetDevice> gwDev, Ipv4Address gwIp)
{
  NS_LOG_FUNCTION (this << swtchDev << portNo << gwDev << gwIp);

  // TODO
}

void
SdranController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  delete (m_s11SapSgw);

  // Chain up.
  Object::DoDispose ();
}

void
SdranController::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  ObjectBase::NotifyConstructionCompleted ();
}

void
SdranController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // This function is called after a successfully handshake between controller
  // and switch. So, let's start configure the S-GW switch tables in accordance
  // to our methodology.

  // TODO: Implementar isso aqui.

//  // Configure the switch to buffer packets and send only the first 128 bytes
//  // to the controller.
//  DpctlExecute (swtch, "set-config miss=128");
//
//  // FIXME Find a better way to identify which nodes should or not scape here.
//  if (swtch->GetDpId () == m_pgwDpId)
//    {
//      // Don't install the following rules on the P-GW switch.
//      return;
//    }
//
//  // -------------------------------------------------------------------------
//  // Table 0 -- Input table -- [from higher to lower priority]
//  //
//  // Entries will be installed here by NewS5Attach function.
//
//  // GTP packets entering the switch from any port other then EPC ports.
//  // Send to Routing table.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=32 eth_type=0x800,"
//                "ip_proto=17,udp_src=2152,udp_dst=2152"
//                " goto:2");
//
//  // ARP request packets. Send to controller.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=16 eth_type=0x0806"
//                " apply:output=ctrl");
//
//  // Table miss entry. Send to controller.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");
//
//
//  // -------------------------------------------------------------------------
//  // Table 1 -- Classification table -- [from higher to lower priority]
//  //
//  // Entries will be installed here by TopologyInstallRouting function.
//
//  // Table miss entry. Send to controller.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");
//
//  // -------------------------------------------------------------------------
//  // Table 2 -- Routing table -- [from higher to lower priority]
//  //
//  // Entries will be installed here by NewS5Attach function.
//  // Entries will be installed here by NotifyTopologyBuilt function.
//
//  // GTP packets classified at previous table. Write the output group into
//  // action set based on metadata field. Send the packet to Coexistence QoS
//  // table.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
//                " meta=0x1"
//                " write:group=1 goto:3");
//  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
//                " meta=0x2"
//                " write:group=2 goto:3");
//
//  // Table miss entry. Send to controller.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=0 apply:output=ctrl");
//
//  // -------------------------------------------------------------------------
//  // Table 3 -- Coexistence QoS table -- [from higher to lower priority]
//  //
//  if (m_nonGbrCoexistence)
//    {
//      // Non-GBR packets indicated by DSCP field. Apply corresponding Non-GBR
//      // meter band. Send the packet to Output table.
//      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
//                    " eth_type=0x800,ip_dscp=0,meta=0x1"
//                    " meter:1 goto:4");
//      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
//                    " eth_type=0x800,ip_dscp=0,meta=0x2"
//                    " meter:2 goto:4");
//    }
//
//  // Table miss entry. Send the packet to Output table
//  DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=0 goto:4");
//
//  // -------------------------------------------------------------------------
//  // Table 4 -- Output table -- [from higher to lower priority]
//  //
//  if (m_voipQos)
//    {
//      int dscpVoip = SdranController::GetDscpValue (EpsBearer::GBR_CONV_VOICE);
//
//      // VoIP packets. Write the high-priority output queue #1.
//      std::ostringstream cmd;
//      cmd << "flow-mod cmd=add,table=4,prio=16"
//          << " eth_type=0x800,ip_dscp=" << dscpVoip
//          << " write:queue=1";
//      DpctlExecute (swtch, cmd.str ());
//    }
//
//  // Table miss entry. No instructions. This will trigger action set execute.
//  DpctlExecute (swtch, "flow-mod cmd=add,table=4,prio=0");
}

ofl_err
SdranController::HandlePacketIn (
  ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *m = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_INFO ("Packet in match: " << m);
  free (m);

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);

  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");
  return 0;
}

ofl_err
SdranController::HandleFlowRemoved (
  ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  // uint8_t table = msg->stats->table_id;
  // uint32_t teid = msg->stats->cookie;
  // uint16_t prio = msg->stats->priority;

  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // Since handlers must free the message when everything is ok,
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // TODO: implementar logica para regras removidas do S-GW.
  NS_ABORT_MSG ("Should not get here :/");
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
SdranController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.imsi);

  uint16_t cellId = req.uli.gci;

  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (req.imsi);
  ueInfo->SetEnbAddress (enbInfo->GetEnbAddress ());

  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = req.imsi;

  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
  for (bit = req.bearerContextsToBeCreated.begin ();
       bit != req.bearerContextsToBeCreated.end ();
       ++bit)
    {
      // Check for available TEID.
      NS_ABORT_IF (m_teidCount == 0xFFFFFFFF);
      uint32_t teid = ++m_teidCount;

      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.teid = teid;
      bearerContext.sgwFteid.address = enbInfo->GetSgwAddress ();
      bearerContext.epsBearerId = bit->epsBearerId;
      bearerContext.bearerLevelQos = bit->bearerLevelQos;
      bearerContext.tft = bit->tft;
      res.bearerContextsCreated.push_back (bearerContext);
    }

  // Notify the controller of the new create session request accepted.
  NotifySessionCreated (req.imsi, cellId, enbInfo->GetEnbAddress (),
                        enbInfo->GetSgwAddress (), res.bearerContextsCreated);

  m_s11SapMme->CreateSessionResponse (res);
}

void
SdranController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;
  uint16_t cellId = req.uli.gci;

  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  ueInfo->SetEnbAddress (enbInfo->GetEnbAddress ());

  // No actual bearer modification: for now we just support the minimum needed
  // for path switch request (handover).
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = imsi;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  m_s11SapMme->ModifyBearerResponse (res);
}

void
SdranController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;

  EpcS11SapMme::DeleteBearerRequestMessage res;
  res.teid = imsi;

  std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit;
  for (bit = req.bearerContextsToBeRemoved.begin ();
       bit != req.bearerContextsToBeRemoved.end ();
       ++bit)
    {
      EpcS11SapMme::BearerContextRemoved bearerContext;
      bearerContext.epsBearerId = bit->epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
    }

  m_s11SapMme->DeleteBearerRequest (res);
}

void
SdranController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;

  // FIXME No need of ueInfo.
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  std::list<EpcS11SapSgw::BearerContextRemovedSgwPgw>::iterator bit;
  for (bit = req.bearerContextsRemoved.begin ();
       bit != req.bearerContextsRemoved.end ();
       ++bit)
    {
      // TODO Should remove entries from gateway?
    }
}

};  // namespace ns3
