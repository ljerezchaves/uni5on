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
#include "connection-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConnectionStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (ConnectionStatsCalculator);

ConnectionStatsCalculator::ConnectionStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndOk",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyInitialCellSelectionEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/InitialCellSelectionEndError",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyInitialCellSelectionEndError, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyConnectionEstablished, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionTimeout",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyConnectionTimeout, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/ConnectionReconfiguration",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyConnectionReconfiguration, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyHandoverStart, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyHandoverEndOk, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndError",
    MakeCallback (
      &ConnectionStatsCalculator::NotifyHandoverEndError, this));
}

ConnectionStatsCalculator::~ConnectionStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ConnectionStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<ConnectionStatsCalculator> ()
    .AddAttribute ("RrcStatsFilename",
                   "Filename for LTE UE RRC procedures statistics.",
                   StringValue ("connection-ue-rrc.log"),
                   MakeStringAccessor (
                     &ConnectionStatsCalculator::m_rrcFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
ConnectionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rrcWrapper = 0;
}

void
ConnectionStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("RrcStatsFilename", StringValue (prefix + m_rrcFilename));

  m_rrcWrapper = Create<OutputStreamWrapper> (m_rrcFilename, std::ios::out);
  *m_rrcWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (11) << "Time(s)"
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
ConnectionStatsCalculator::NotifyInitialCellSelectionEndOk (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Initial cell selection OK"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyInitialCellSelectionEndError (
  std::string context, uint64_t imsi, uint16_t cellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Initial cell selection error"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyConnectionEstablished (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection established"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyConnectionTimeout (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection timeout"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyConnectionReconfiguration (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Connection reconfiguration"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyHandoverStart (
  std::string context, uint64_t imsi, uint16_t srcCellId, uint16_t rnti,
  uint16_t dstCellId)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover start"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << srcCellId
  << " " << setw (5) << rnti
  << " " << setw (9) << dstCellId
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyHandoverEndOk (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover OK"
  << right
  << " " << setw (5) << imsi
  << " " << setw (4) << cellId
  << " " << setw (5) << rnti
  << std::endl;
}

void
ConnectionStatsCalculator::NotifyHandoverEndError (
  std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti)
{
  *m_rrcWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds ()
  << " " << setw (30) << "Handover error"
  << right
  << " " << setw (7) << imsi
  << " " << setw (7) << cellId
  << " " << setw (7) << rnti
  << std::endl;
}

} // Namespace ns3
