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

// Initializing SdranController static members.
SdranController::CellIdCtrlMap_t SdranController::m_cellIdCtrlMap;

SdranController::SdranController ()
  : m_epcCtrlApp (0)
{
  NS_LOG_FUNCTION (this);

  // The S-GW side of S11 AP
  m_s11SapSgw = new MemberEpcS11SapSgw<SdranController> (this);

  m_mme = CreateObject<SdmnMme> ();
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
    .AddConstructor<SdranController> ()
  ;
  return tid;
}

bool
SdranController::RequestDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  // TODO
  return m_epcCtrlApp->RequestDedicatedBearer (bearer, teid);
}

bool
SdranController::ReleaseDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  // TODO
  return m_epcCtrlApp->ReleaseDedicatedBearer (bearer, teid);
}

void
SdranController::NotifySessionCreated (
  uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address sgwAddr,
  BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr << sgwAddr);

  // TODO ??? Acho que nao precisa fazer mais nada.. se realmente for so isso,
  // levar essa linha la pra baixo, na funcao chamada pelo mme.
  m_epcCtrlApp->NotifySessionCreated (imsi, cellId, m_sgwS5Addr, bearerList);
}

void
SdranController::NotifySgwAttach (
  uint32_t sgwS5PortNum, Ptr<NetDevice> sgwS5Dev)
{
  NS_LOG_FUNCTION (this << sgwS5PortNum << sgwS5Dev);

  m_sgwS5Addr = EpcController::GetIpAddressForDevice (sgwS5Dev);
  // TODO Install forwarding rules on S-GW?
}

void
SdranController::NotifyEnbAttach (
  uint16_t cellId, uint32_t sgwS1uPortNum)
{
  NS_LOG_FUNCTION (this << sgwS1uPortNum);

  // Register this controller by cell ID for further usage.
  RegisterController (Ptr<SdranController> (this), cellId);

  // TODO
}

void
SdranController::SetEpcController (Ptr<EpcController> epcCtrlApp)
{
  NS_LOG_FUNCTION (this << epcCtrlApp);

  m_epcCtrlApp = epcCtrlApp;
}

EpcS1apSapMme*
SdranController::GetS1apSapMme (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mme->GetS1apSapMme ();
}

void
SdranController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_epcCtrlApp = 0;
  m_mme = 0;
  delete (m_s11SapSgw);

  // Chain up.
  Object::DoDispose ();
}

void
SdranController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // This function is called after a successfully handshake between the SDRAN
  // controller and the S-GW user plane. 
  // TODO
}

ofl_err
SdranController::HandlePacketIn (
  ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *m = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_INFO ("Packet in match: " << m);
  free (m);
  
  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

ofl_err
SdranController::HandleFlowRemoved (
  ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

Ptr<SdranController>
SdranController::GetPointer (uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<SdranController> ctrl = 0;
  CellIdCtrlMap_t::iterator ret;
  ret = SdranController::m_cellIdCtrlMap.find (cellId);
  if (ret != SdranController::m_cellIdCtrlMap.end ())
    {
      ctrl = ret->second;
    }
  return ctrl;
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
      uint32_t teid = EpcController::GetNextTeid ();
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

  NS_LOG_DEBUG ("Nothing to do here. Done.");
}

void
SdranController::RegisterController (Ptr<SdranController> ctrl,
                                     uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Saving map by cell ID.
  std::pair<CellIdCtrlMap_t::iterator, bool> ret;
  std::pair<uint16_t, Ptr<SdranController> > entry (cellId, ctrl);
  ret = SdranController::m_cellIdCtrlMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Can't register SDRAN controller by cell ID.");
    }
}

};  // namespace ns3
