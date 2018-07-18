/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *               2017 University of Campinas (Unicamp)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "svelte-mme.h"
#include "ue-info.h"
#include "enb-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteMme");
NS_OBJECT_ENSURE_REGISTERED (SvelteMme);

SvelteMme::SvelteMme ()
{
  NS_LOG_FUNCTION (this);

  m_s1apSapMme = new MemberEpcS1apSapMme<SvelteMme> (this);
  m_s11SapMme = new MemberEpcS11SapMme<SvelteMme> (this);
}

SvelteMme::~SvelteMme ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteMme::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteMme")
    .SetParent<Object> ()
    .AddConstructor<SvelteMme> ()
  ;
  return tid;
}

void
SvelteMme::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  delete (m_s1apSapMme);
  delete (m_s11SapMme);
}

EpcS1apSapMme*
SvelteMme::GetS1apSapMme (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1apSapMme;
}

EpcS11SapMme*
SvelteMme::GetS11SapMme (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s11SapMme;
}

// FIXME. Nós teremos vários S-GW para este MME. Precisamos de um jeito de
// gerenciar isso. Como essa informação depende do UE (um mesmo eNB pode estar
// atrelado à dois SGW diferentes), então o melhor é salvar isso dentro do
// UeInfo.
// void
// SvelteMme::SetS11SapSgw (EpcS11SapSgw *sap)
// {
//   NS_LOG_FUNCTION (this);
//
// //  m_s11SapSgw = sap;
// }

//
// S1-AP SAP MME forwarded methods
//
void
SvelteMme::DoInitialUeMessage (
  uint64_t mmeUeS1Id, uint16_t enbUeS1Id, uint64_t imsi, uint16_t ecgi)
{
  NS_LOG_FUNCTION (this << mmeUeS1Id << enbUeS1Id << imsi << ecgi);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  ueInfo->SetCellId (ecgi);
  ueInfo->SetEnbUeS1Id (enbUeS1Id); // TODO Verificar se isso nao causa erro.

  EpcS11SapSgw::CreateSessionRequestMessage msg;
  msg.imsi = imsi;
  msg.uli.gci = ecgi;

  std::list<UeInfo::BearerInfo>::const_iterator bit;
  for (bit = ueInfo->GetBearerListBegin ();
       bit != ueInfo->GetBearerListEnd ();
       ++bit)
    {
      EpcS11SapSgw::BearerContextToBeCreated bearerContext;
      bearerContext.epsBearerId     = bit->bearerId;
      bearerContext.bearerLevelQos  = bit->bearer;
      bearerContext.tft             = bit->tft;

      msg.bearerContextsToBeCreated.push_back (bearerContext);
    }

// FIXME Enviar a informação para o controlador do S-GW correto.
//  m_s11SapSgw->CreateSessionRequest (msg);
}

void
SvelteMme::DoInitialContextSetupResponse (
  uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
  std::list<EpcS1apSapMme::ErabSetupItem> erabList)
{
  NS_LOG_FUNCTION (this << mmeUeS1Id << enbUeS1Id);

  NS_ABORT_MSG ("Unimplemented method.");
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
SvelteMme::DoPathSwitchRequest (
  uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t gci,
  std::list<EpcS1apSapMme::ErabSwitchedInDownlinkItem> erabList)
{
  NS_LOG_FUNCTION (this << mmeUeS1Id << enbUeS1Id << gci);

  uint64_t imsi = mmeUeS1Id;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_LOG_INFO ("UE IMSI: " << imsi <<
               " old eNB: " << ueInfo->GetCellId () <<
               " new eNB: " << gci);

  ueInfo->SetCellId (gci);
  ueInfo->SetEnbUeS1Id (enbUeS1Id);

  EpcS11SapSgw::ModifyBearerRequestMessage msg;
  msg.teid = imsi;
  msg.uli.gci = gci;

// FIXME Enviar a informação para o controlador do S-GW correto.
//  m_s11SapSgw->ModifyBearerRequest (msg);
}

void
SvelteMme::DoErabReleaseIndication (
  uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
  std::list<EpcS1apSapMme::ErabToBeReleasedIndication> erabList)
{
  NS_LOG_FUNCTION (this << mmeUeS1Id << enbUeS1Id);

  uint64_t imsi = mmeUeS1Id;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  EpcS11SapSgw::DeleteBearerCommandMessage msg;
  msg.teid = imsi;

  std::list<EpcS1apSapMme::ErabToBeReleasedIndication>::iterator bit;
  for (bit = erabList.begin (); bit != erabList.end (); ++bit)
    {
      EpcS11SapSgw::BearerContextToBeRemoved bearerContext;
      bearerContext.epsBearerId = bit->erabId;
      msg.bearerContextsToBeRemoved.push_back (bearerContext);
    }

// FIXME Enviar a informação para o controlador do S-GW correto.
//  m_s11SapSgw->DeleteBearerCommand (msg);
}

//
// S11 SAP MME forwarded methods
//
void
SvelteMme::DoCreateSessionResponse (
  EpcS11SapMme::CreateSessionResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  uint64_t imsi = msg.teid;
  std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList;
  std::list<EpcS11SapMme::BearerContextCreated>::iterator bit;
  for (bit = msg.bearerContextsCreated.begin ();
       bit != msg.bearerContextsCreated.end ();
       ++bit)
    {
      EpcS1apSapEnb::ErabToBeSetupItem erab;
      erab.erabId = bit->epsBearerId;
      erab.erabLevelQosParameters = bit->bearerLevelQos;
      erab.transportLayerAddress = bit->sgwFteid.address;
      erab.sgwTeid = bit->sgwFteid.teid;
      erabToBeSetupList.push_back (erab);
    }

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  uint16_t cellId    = ueInfo->GetCellId ();
  uint16_t enbUeS1Id = ueInfo->GetEnbUeS1Id ();
  uint64_t mmeUeS1Id = ueInfo->GetMmeUeS1Id ();

  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  enbInfo->GetS1apSapEnb ()->InitialContextSetupRequest (
    mmeUeS1Id, enbUeS1Id, erabToBeSetupList);
}

void
SvelteMme::DoModifyBearerResponse (
  EpcS11SapMme::ModifyBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_ASSERT (msg.cause ==
             EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED);

  uint64_t imsi = msg.teid;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  uint64_t enbUeS1Id = ueInfo->GetEnbUeS1Id ();
  uint64_t mmeUeS1Id = ueInfo->GetMmeUeS1Id ();
  uint16_t cellId    = ueInfo->GetCellId ();
  std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem> erabList;

  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  enbInfo->GetS1apSapEnb ()->PathSwitchRequestAcknowledge (
    enbUeS1Id, mmeUeS1Id, cellId, erabList);
}

void
SvelteMme::DoDeleteBearerRequest (EpcS11SapMme::DeleteBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this);

  uint64_t imsi = msg.teid;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  EpcS11SapSgw::DeleteBearerResponseMessage res;
  res.teid = imsi;

  std::list<EpcS11SapMme::BearerContextRemoved>::iterator bit;
  for (bit = msg.bearerContextsRemoved.begin ();
       bit != msg.bearerContextsRemoved.end ();
       ++bit)
    {
      EpcS11SapSgw::BearerContextRemovedSgwPgw bearerContext;
      bearerContext.epsBearerId = bit->epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
      ueInfo->RemoveBearer (bearerContext.epsBearerId);
    }

// FIXME Enviar a informação para o controlador do S-GW correto.
//  m_s11SapSgw->DeleteBearerResponse (res);
}

} // namespace ns3
