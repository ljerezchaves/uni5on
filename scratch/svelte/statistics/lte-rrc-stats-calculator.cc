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

#include <iomanip>
#include <iostream>
#include <ns3/mobility-model.h>
#include "lte-rrc-stats-calculator.h"
#include "../svelte-common.h"
#include "../metadata/enb-info.h"
#include "../metadata/ue-info.h"
#include "../metadata/sgw-info.h"
#include "../metadata/pgw-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteRrcStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (LteRrcStatsCalculator);

LteRrcStatsCalculator::LteRrcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/$ns3::MobilityModel/CourseChange",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeMobilityCourseChange, this));

  Config::Connect (
    "/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyHandoverStart, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyHandoverEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyHandoverStart, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyHandoverEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndError",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyHandoverEndError, this));

  Config::Connect (
    "/NodeList/*/DeviceList/*/LteEnbRrc/NewUeContext",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyEnbNewUeContext, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyConnectionEstablished, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionReconfiguration",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyConnectionReconfiguration, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyConnectionEstablished, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionReconfiguration",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyConnectionReconfiguration, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionTimeout",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeConnectionTimeout, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndOk",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeInitialCellSelectionEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndError",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeInitialCellSelectionEndError, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessSuccessful",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeRandomAccessSuccessful, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessError",
    MakeCallback (
      &LteRrcStatsCalculator::NotifyUeRandomAccessError, this));
}

LteRrcStatsCalculator::~LteRrcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LteRrcStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteRrcStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<LteRrcStatsCalculator> ()
    .AddAttribute ("HvoStatsFilename",
                   "Filename for LTE UE handover statistics.",
                   StringValue ("rrc-handover"),
                   MakeStringAccessor (
                     &LteRrcStatsCalculator::m_hvoFilename),
                   MakeStringChecker ())
    .AddAttribute ("MobStatsFilename",
                   "Filename for LTE UE mobility statistics.",
                   StringValue ("ue-mobility"),
                   MakeStringAccessor (
                     &LteRrcStatsCalculator::m_mobFilename),
                   MakeStringChecker ())
    .AddAttribute ("RrcStatsFilename",
                   "Filename for LTE UE RRC procedures statistics.",
                   StringValue ("rrc-procedures"),
                   MakeStringAccessor (
                     &LteRrcStatsCalculator::m_rrcFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
LteRrcStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_hvoWrapper = 0;
  m_mobWrapper = 0;
  m_rrcWrapper = 0;
  Object::DoDispose ();
}

void
LteRrcStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("HvoStatsFilename", StringValue (prefix + m_hvoFilename));
  SetAttribute ("MobStatsFilename", StringValue (prefix + m_mobFilename));
  SetAttribute ("RrcStatsFilename", StringValue (prefix + m_rrcFilename));

  m_hvoWrapper = Create<OutputStreamWrapper> (
      m_hvoFilename + ".log", std::ios::out);
  *m_hvoWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "Time:s"
    << " " << setw (5)  << "Node"
    << " " << setw (7)  << "Event"
    << " " << setw (5)  << "RNTI";
  UeInfo::PrintHeader (*m_hvoWrapper->GetStream ());
  *m_hvoWrapper->GetStream ()
    << " " << setw (9)  << "SrcEnbId"
    << " " << setw (9)  << "SrcEnbSw"
    << " " << setw (9)  << "DstEnbId"
    << " " << setw (9)  << "DstEnbSw";
  SgwInfo::PrintHeader (*m_hvoWrapper->GetStream ());
  PgwInfo::PrintHeader (*m_hvoWrapper->GetStream ());
  *m_hvoWrapper->GetStream () << std::endl;

  m_mobWrapper = Create<OutputStreamWrapper> (
      m_mobFilename + ".log", std::ios::out);
  *m_mobWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "Time:s"
    << " " << setw (8)  << "NodeId"
    << " " << setw (11) << "NodeName"
    << " " << setw (9)  << "PosX"
    << " " << setw (9)  << "PosY"
    << " " << setw (9)  << "PosZ"
    << " " << setw (9)  << "VelX"
    << " " << setw (9)  << "VelY"
    << " " << setw (9)  << "VelZ"
    << std::endl;

  m_rrcWrapper = Create<OutputStreamWrapper> (
      m_rrcFilename + ".log", std::ios::out);
  *m_rrcWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "Time:s"
    << " " << setw (5)  << "Node"
    << " " << setw (10) << "RrcEvent"
    << " " << setw (5)  << "RNTI";
  UeInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  EnbInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  SgwInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
LteRrcStatsCalculator::NotifyUeMobilityCourseChange (
  std::string context, Ptr<const MobilityModel> mobility)
{
  NS_LOG_FUNCTION (this << context << mobility);

  Ptr<Node> node = mobility->GetObject<Node> ();
  Vector position = mobility->GetPosition ();
  Vector velocity = mobility->GetVelocity ();

  *m_mobWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (8)  << node->GetId ()
    << " " << setw (11) << Names::FindName (node)
    << " " << setw (9)  << position.x
    << " " << setw (9)  << position.y
    << " " << setw (9)  << position.z
    << " " << setw (9)  << velocity.x
    << " " << setw (9)  << velocity.y
    << " " << setw (9)  << velocity.z
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyHandoverEndError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_ASSERT_MSG (ueInfo, "Invalid UE info.");
  NS_ASSERT_MSG (ueInfo->GetSgwInfo (), "Invalid S-GW info.");
  NS_ASSERT_MSG (ueInfo->GetPgwInfo (), "Invalid P-GW info.");

  Ptr<EnbInfo> srcEnbInfo = EnbInfo::GetPointer (cellId);
  NS_ASSERT_MSG (srcEnbInfo, "Invalid eNB info.");

  std::string node = "UE";
  if (context.find ("LteEnbRrc") != std::string::npos)
    {
      node = "eNB";
    }

  *m_hvoWrapper->GetStream ()
    << " " << setw (8) << Simulator::Now ().GetSeconds ()
    << " " << setw (5) << node
    << " " << setw (7) << "EndErr"
    << " " << setw (5) << rnti
    << *ueInfo
    << " " << setw (9) << srcEnbInfo->GetCellId ()
    << " " << setw (9) << srcEnbInfo->GetInfraSwIdx ()
    << " " << setw (9) << "-"
    << " " << setw (9) << "-"
    << *ueInfo->GetSgwInfo ()
    << *ueInfo->GetPgwInfo ()
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyHandoverEndOk (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_ASSERT_MSG (ueInfo, "Invalid UE info.");
  NS_ASSERT_MSG (ueInfo->GetSgwInfo (), "Invalid S-GW info.");
  NS_ASSERT_MSG (ueInfo->GetPgwInfo (), "Invalid P-GW info.");

  Ptr<EnbInfo> dstEnbInfo = EnbInfo::GetPointer (cellId);
  NS_ASSERT_MSG (dstEnbInfo, "Invalid eNB info.");

  std::string node = "UE";
  if (context.find ("LteEnbRrc") != std::string::npos)
    {
      node = "eNB";
      NS_ASSERT_MSG (ueInfo->GetEnbInfo ()->GetCellId () == cellId,
                     "Inconsistente eNB info.");
    }

  *m_hvoWrapper->GetStream ()
    << " " << setw (8) << Simulator::Now ().GetSeconds ()
    << " " << setw (5) << node
    << " " << setw (7) << "EndOk"
    << " " << setw (5) << rnti
    << *ueInfo
    << " " << setw (9) << "-"
    << " " << setw (9) << "-"
    << " " << setw (9) << dstEnbInfo->GetCellId ()
    << " " << setw (9) << dstEnbInfo->GetInfraSwIdx ()
    << *ueInfo->GetSgwInfo ()
    << *ueInfo->GetPgwInfo ()
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyHandoverStart (
  std::string context, uint64_t imsi, uint16_t srcCellId, uint16_t rnti,
  uint16_t dstCellId)
{
  NS_LOG_FUNCTION (this << context << imsi << srcCellId << rnti << dstCellId);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_ASSERT_MSG (ueInfo, "Invalid UE info.");
  NS_ASSERT_MSG (ueInfo->GetSgwInfo (), "Invalid S-GW info.");
  NS_ASSERT_MSG (ueInfo->GetPgwInfo (), "Invalid P-GW info.");
  NS_ASSERT_MSG (ueInfo->GetEnbInfo ()->GetCellId () == srcCellId,
                 "Inconsistente eNB info.");

  Ptr<EnbInfo> srcEnbInfo = EnbInfo::GetPointer (srcCellId);
  Ptr<EnbInfo> dstEnbInfo = EnbInfo::GetPointer (dstCellId);
  NS_ASSERT_MSG (srcEnbInfo && dstEnbInfo, "Invalid eNB info.");

  std::string node = "UE";
  if (context.find ("LteEnbRrc") != std::string::npos)
    {
      node = "eNB";
    }

  *m_hvoWrapper->GetStream ()
    << " " << setw (8) << Simulator::Now ().GetSeconds ()
    << " " << setw (5) << node
    << " " << setw (7) << "Start"
    << " " << setw (5) << rnti
    << *ueInfo
    << " " << setw (9) << srcEnbInfo->GetCellId ()
    << " " << setw (9) << srcEnbInfo->GetInfraSwIdx ()
    << " " << setw (9) << dstEnbInfo->GetCellId ()
    << " " << setw (9) << dstEnbInfo->GetInfraSwIdx ()
    << *ueInfo->GetSgwInfo ()
    << *ueInfo->GetPgwInfo ()
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyEnbNewUeContext (
  std::string context, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << cellId << rnti);

  NS_ASSERT_MSG (cellId && rnti, "Invalid CellId or RNTI.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "eNB"
    << " " << setw (10) << "NewUeCtx"
    << " " << setw (5)  << rnti;
  UeInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream ()
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

void
LteRrcStatsCalculator::NotifyConnectionEstablished (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_ASSERT_MSG (ueInfo, "Invalid UE info.");
  NS_ASSERT_MSG (ueInfo->GetEnbInfo (), "Invalid eNB info.");
  NS_ASSERT_MSG (ueInfo->GetSgwInfo (), "Invalid S-GW info.");
  NS_ASSERT_MSG (ueInfo->GetPgwInfo (), "Invalid P-GW info.");
  NS_ASSERT_MSG (ueInfo->GetEnbInfo ()->GetCellId () == cellId,
                 "Inconsistente eNB info.");

  std::string node = "UE";
  if (context.find ("LteEnbRrc") != std::string::npos)
    {
      node = "eNB";
    }

  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << node
    << " " << setw (10) << "CnnEstab"
    << " " << setw (5)  << rnti
    << *ueInfo
    << *ueInfo->GetEnbInfo ()
    << *ueInfo->GetSgwInfo ()
    << *ueInfo->GetPgwInfo ()
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyConnectionReconfiguration (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  NS_ASSERT_MSG (ueInfo, "Invalid UE info.");
  NS_ASSERT_MSG (ueInfo->GetEnbInfo (), "Invalid eNB info.");
  NS_ASSERT_MSG (ueInfo->GetSgwInfo (), "Invalid S-GW info.");
  NS_ASSERT_MSG (ueInfo->GetPgwInfo (), "Invalid P-GW info.");
  NS_ASSERT_MSG (ueInfo->GetEnbInfo ()->GetCellId () == cellId,
                 "Inconsistente eNB info.");

  std::string node = "UE";
  if (context.find ("LteEnbRrc") != std::string::npos)
    {
      node = "eNB";
    }

  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << node
    << " " << setw (10) << "CnnReconf"
    << " " << setw (5)  << rnti
    << *ueInfo
    << *ueInfo->GetEnbInfo ()
    << *ueInfo->GetSgwInfo ()
    << *ueInfo->GetPgwInfo ()
    << std::endl;
}

void
LteRrcStatsCalculator::NotifyUeConnectionTimeout (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  NS_ASSERT_MSG (imsi && cellId, "Invalid IMSI or CellId.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "UE"
    << " " << setw (10) << "CnnTmout"
    << " " << setw (5)  << rnti
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

void
LteRrcStatsCalculator::NotifyUeInitialCellSelectionEndError (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId);

  NS_ASSERT_MSG (imsi && cellId, "Invalid IMSI or CellId.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "UE"
    << " " << setw (10) << "CellSelErr"
    << " " << setw (5)  << "-"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

void
LteRrcStatsCalculator::NotifyUeInitialCellSelectionEndOk (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId);

  NS_ASSERT_MSG (imsi && cellId, "Invalid IMSI or CellId.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "UE"
    << " " << setw (10) << "CellSelOk"
    << " " << setw (5)  << "-"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

void
LteRrcStatsCalculator::NotifyUeRandomAccessError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  NS_ASSERT_MSG (imsi && cellId, "Invalid IMSI or CellId.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "UE"
    << " " << setw (10) << "RndAcsErr"
    << " " << setw (5)  << rnti
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

void
LteRrcStatsCalculator::NotifyUeRandomAccessSuccessful (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  NS_LOG_FUNCTION (this << context << imsi << cellId << rnti);

  NS_ASSERT_MSG (imsi && cellId, "Invalid IMSI or CellId.");
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (5)  << "UE"
    << " " << setw (10) << "RndAcsOk"
    << " " << setw (5)  << rnti
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId);
  SgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  PgwInfo::PrintNull (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream () << std::endl;
}

} // Namespace ns3
