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

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HandoverStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (HandoverStatsCalculator);

HandoverStatsCalculator::HandoverStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndOk",
    MakeCallback (
      &HandoverStatsCalculator::NotifyInitialCellSelectionEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndError",
    MakeCallback (
      &HandoverStatsCalculator::NotifyInitialCellSelectionEndError, this));
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
    "/NodeList/*/$ns3::MobilityModel/CourseChange",
    MakeCallback (
      &HandoverStatsCalculator::NotifyMobilityCourseChange, this));
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
                   StringValue ("handover-mobility.log"),
                   MakeStringAccessor (
                     &HandoverStatsCalculator::m_mobFilename),
                   MakeStringChecker ())
    .AddAttribute ("RrcStatsFilename",
                   "Filename for LTE UE RRC procedures statistics.",
                   StringValue ("handover-connection.log"),
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

  m_mobWrapper = Create<OutputStreamWrapper> (m_mobFilename, std::ios::out);
  *m_mobWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (8) << "NodeId"
  << setw (10) << "NodeName"
  << setw (10) << "PosX"
  << setw (10) << "PosY"
  << setw (10) << "PosZ"
  << setw (10) << "VelX"
  << setw (10) << "VelY"
  << setw (10) << "VelZ"
  << std::endl;

  m_rrcWrapper = Create<OutputStreamWrapper> (m_rrcFilename, std::ios::out);
  *m_rrcWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (12) << "Time(s)"
  << setw (30) << "UE RRC event"
  << right
  << setw (6)  << "IMSI"
  << setw (5)  << "CGI"
  << setw (6)  << "RNTI"
  << setw (10) << "TargetCGI"
  << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
HandoverStatsCalculator::NotifyInitialCellSelectionEndOk (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Initial cell selection OK"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << std::endl;
}

void
HandoverStatsCalculator::NotifyInitialCellSelectionEndError (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Initial cell selection error"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << std::endl;
}

void
HandoverStatsCalculator::NotifyConnectionEstablished (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection established"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
HandoverStatsCalculator::NotifyConnectionTimeout (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection timeout"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
HandoverStatsCalculator::NotifyConnectionReconfiguration (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection reconfiguration"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverStart (
  std::string context, uint64_t imsi, uint16_t srcCellId, uint16_t rnti,
  uint16_t dstCellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover start"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << srcCellId
  << " " << setw (5) << rnti
  << " " << setw (9) << dstCellId
  << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverEndOk (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover OK"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
HandoverStatsCalculator::NotifyHandoverEndError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover error"
  << right
  << " " << setw (7) << imsi
  << " " << setw (7) << cellId
  << " " << setw (7) << rnti
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
  << left << setprecision (4)
  << setw (11) << Simulator::Now ().GetSeconds ()
  << right << setprecision (2)
  << " " << setw (8) << node->GetId ()
  << " " << setw (9)  << Names::FindName (node)
  << " " << setw (9)  << position.x
  << " " << setw (9)  << position.y
  << " " << setw (9)  << position.z
  << " " << setw (9)  << velocity.x
  << " " << setw (9)  << velocity.y
  << " " << setw (9)  << velocity.z
  << std::endl;
}

} // Namespace ns3
