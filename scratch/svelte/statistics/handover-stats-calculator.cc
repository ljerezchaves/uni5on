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
#include "handover-stats-calculator.h"
#include "../svelte-common.h"
#include "../metadata/enb-info.h"
#include "../metadata/ue-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HandoverStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (HandoverStatsCalculator);

HandoverStatsCalculator::HandoverStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
    MakeCallback (
      &HandoverStatsCalculator::NotifyConnectionEstablished, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionTimeout",
    MakeCallback (
      &HandoverStatsCalculator::NotifyConnectionTimeout, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionReconfiguration",
    MakeCallback (
      &HandoverStatsCalculator::NotifyConnectionReconfiguration, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
    MakeCallback (
      &HandoverStatsCalculator::NotifyHandoverStart, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
    MakeCallback (
      &HandoverStatsCalculator::NotifyHandoverEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndError",
    MakeCallback (
      &HandoverStatsCalculator::NotifyHandoverEndError, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndOk",
    MakeCallback (
      &HandoverStatsCalculator::NotifyInitialCellSelectionEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndError",
    MakeCallback (
      &HandoverStatsCalculator::NotifyInitialCellSelectionEndError, this));
  Config::Connect (
    "/NodeList/*/$ns3::MobilityModel/CourseChange",
    MakeCallback (
      &HandoverStatsCalculator::NotifyMobilityCourseChange, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessSuccessful",
    MakeCallback (
      &HandoverStatsCalculator::NotifyRandomAccessSuccessful, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/RandomAccessError",
    MakeCallback (
      &HandoverStatsCalculator::NotifyRandomAccessError, this));
}

HandoverStatsCalculator::~HandoverStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
HandoverStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HandoverStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<HandoverStatsCalculator> ()
    .AddAttribute ("MobStatsFilename",
                   "Filename for LTE UE mobility model statistics.",
                   StringValue ("handover-mobility"),
                   MakeStringAccessor (
                     &HandoverStatsCalculator::m_mobFilename),
                   MakeStringChecker ())
    .AddAttribute ("RrcStatsFilename",
                   "Filename for LTE UE RRC procedures statistics.",
                   StringValue ("handover-connection"),
                   MakeStringAccessor (
                     &HandoverStatsCalculator::m_rrcFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
HandoverStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_mobWrapper = 0;
  m_rrcWrapper = 0;
  Object::DoDispose ();
}

void
HandoverStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("MobStatsFilename", StringValue (prefix + m_mobFilename));
  SetAttribute ("RrcStatsFilename", StringValue (prefix + m_rrcFilename));

  m_mobWrapper = Create<OutputStreamWrapper> (
      m_mobFilename + ".log", std::ios::out);
  *m_mobWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "Time:s"
    << " " << setw (8)  << "NodeId"
    << " " << setw (9)  << "NodeName"
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
    << " " << setw (32) << "UE-RRC-event";
  UeInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  EnbInfo::PrintHeader (*m_rrcWrapper->GetStream ());
  *m_rrcWrapper->GetStream ()
    << " " << setw (5)  << "RNTI"
    << " " << setw (9)  << "TargetCGI"
    << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
HandoverStatsCalculator::NotifyConnectionEstablished (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "connection-established"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyConnectionReconfiguration (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "connection-reconfiguration"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyConnectionTimeout (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "connection-timeout"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverStart (
  std::string context, uint64_t imsi, uint16_t srcCellId, uint16_t rnti,
  uint16_t dstCellId)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "handover-start"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (srcCellId)
    << " " << setw (5) << rnti
    << " " << setw (9) << dstCellId
    << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverEndOk (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "handover-end-ok"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverEndError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "handover-end-error"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyInitialCellSelectionEndOk (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "initial-cell-selection-end-ok"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << std::endl;
}

void
HandoverStatsCalculator::NotifyInitialCellSelectionEndError (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "initial-cell-selection-end-error"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << std::endl;
}

void
HandoverStatsCalculator::NotifyMobilityCourseChange (
  std::string context, Ptr<const MobilityModel> mobility)
{
  Ptr<Node> node = mobility->GetObject<Node> ();
  Vector position = mobility->GetPosition ();
  Vector velocity = mobility->GetVelocity ();

  *m_mobWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (8)  << node->GetId ()
    << " " << setw (9)  << Names::FindName (node)
    << " " << setw (9)  << position.x
    << " " << setw (9)  << position.y
    << " " << setw (9)  << position.z
    << " " << setw (9)  << velocity.x
    << " " << setw (9)  << velocity.y
    << " " << setw (9)  << velocity.z
    << std::endl;
}

void
HandoverStatsCalculator::NotifyRandomAccessSuccessful (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "random-access-successfull"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

void
HandoverStatsCalculator::NotifyRandomAccessError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (32) << "random-access-error"
    << *UeInfo::GetPointer (imsi)
    << *EnbInfo::GetPointer (cellId)
    << " " << setw (5) << rnti
    << std::endl;
}

} // Namespace ns3
