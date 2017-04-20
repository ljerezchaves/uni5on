/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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
#include "backhaul-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (BackhaulStatsCalculator);

BackhaulStatsCalculator::BackhaulStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
}

BackhaulStatsCalculator::~BackhaulStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BackhaulStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BackhaulStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<BackhaulStatsCalculator> ()
    .AddAttribute ("RegStatsFilename",
                   "Filename for GBR reservation statistics.",
                   StringValue ("backhaul-reserve-gbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_regFilename),
                   MakeStringChecker ())
    .AddAttribute ("RenStatsFilename",
                   "Filename for Non-GBR allowed bandwidth statistics.",
                   StringValue ("backhaul-reserve-nongbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_renFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwbStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("backhaul-throughput-all.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwbFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwgStatsFilename",
                   "Filename for GBR bandwidth statistics.",
                   StringValue ("backhaul-throughput-gbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwgFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwnStatsFilename",
                   "Filename for Non-GBR bandwidth statistics.",
                   StringValue ("backhaul-throughput-nongbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwnFilename),
                   MakeStringChecker ());
  return tid;
}

void
BackhaulStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_bwbWrapper = 0;
  m_bwgWrapper = 0;
  m_bwnWrapper = 0;
  m_regWrapper = 0;
  m_renWrapper = 0;
  m_connections.clear ();
}

void
BackhaulStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("RegStatsFilename", StringValue (prefix + m_regFilename));
  SetAttribute ("RenStatsFilename", StringValue (prefix + m_renFilename));
  SetAttribute ("BwbStatsFilename", StringValue (prefix + m_bwbFilename));
  SetAttribute ("BwgStatsFilename", StringValue (prefix + m_bwgFilename));
  SetAttribute ("BwnStatsFilename", StringValue (prefix + m_bwnFilename));

  m_bwbWrapper = Create<OutputStreamWrapper> (m_bwbFilename, std::ios::out);
  *m_bwbWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwgWrapper = Create<OutputStreamWrapper> (m_bwgFilename, std::ios::out);
  *m_bwgWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwnWrapper = Create<OutputStreamWrapper> (m_bwnFilename, std::ios::out);
  *m_bwnWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_regWrapper = Create<OutputStreamWrapper> (m_regFilename, std::ios::out);
  *m_regWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_renWrapper = Create<OutputStreamWrapper> (m_renFilename, std::ios::out);
  *m_renWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_connections = ConnectionInfo::GetList ();
  ConnInfoList_t::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      DpIdPair_t key = (*it)->GetSwitchDpIdPair ();

      *m_bwbWrapper->GetStream ()
      << right << setw (12) << key.first  << "-"
      << left  << setw (12) << key.second << "   ";

      *m_bwgWrapper->GetStream ()
      << right << setw (12) << key.first  << "-"
      << left  << setw (12) << key.second << "   ";

      *m_bwnWrapper->GetStream ()
      << right << setw (12) << key.first  << "-"
      << left  << setw (12) << key.second << "   ";

      *m_regWrapper->GetStream ()
      << left
      << right << setw (6) << key.first  << "-"
      << left  << setw (6) << key.second << "   ";

      *m_renWrapper->GetStream ()
      << left
      << right << setw (6) << key.first  << "-"
      << left  << setw (6) << key.second << "   ";
    }

  *m_bwbWrapper->GetStream () << left << std::endl;
  *m_bwgWrapper->GetStream () << left << std::endl;
  *m_bwnWrapper->GetStream () << left << std::endl;
  *m_regWrapper->GetStream () << left << std::endl;
  *m_renWrapper->GetStream () << left << std::endl;

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time firstDump = timeValue.Get ();
  Simulator::Schedule (firstDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, firstDump);

  Object::NotifyConstructionCompleted ();
}
void
BackhaulStatsCalculator::DumpStatistics (Time nextDump)
{
  NS_LOG_FUNCTION (this);

  *m_bwbWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_bwgWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_bwnWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_regWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_renWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  double interval = (Simulator::Now () - m_lastResetTime).GetSeconds ();
  ConnInfoList_t::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      Ptr<ConnectionInfo> cInfo = *it;
      uint64_t gbrFwdBytes = cInfo->GetGbrBytes (ConnectionInfo::FWD);
      uint64_t gbrBwdBytes = cInfo->GetGbrBytes (ConnectionInfo::BWD);
      uint64_t nonFwdBytes = cInfo->GetNonGbrBytes (ConnectionInfo::FWD);
      uint64_t nonBwdBytes = cInfo->GetNonGbrBytes (ConnectionInfo::BWD);

      double gbrFwdKbits = static_cast<double> (gbrFwdBytes * 8) / 1000;
      double gbrBwdKbits = static_cast<double> (gbrBwdBytes * 8) / 1000;
      double nonFwdKbits = static_cast<double> (nonFwdBytes * 8) / 1000;
      double nonBwdKbits = static_cast<double> (nonBwdBytes * 8) / 1000;

      *m_bwgWrapper->GetStream ()
      << right
      << setw (12) << gbrFwdKbits / interval << " "
      << setw (12) << gbrBwdKbits / interval << "   ";

      *m_bwnWrapper->GetStream ()
      << right
      << setw (12) << nonFwdKbits / interval << " "
      << setw (12) << nonBwdKbits / interval << "   ";

      *m_bwbWrapper->GetStream ()
      << right
      << setw (12) << (gbrFwdKbits + nonFwdKbits) / interval << " "
      << setw (12) << (gbrBwdKbits + nonBwdKbits) / interval << "   ";

      *m_regWrapper->GetStream ()
      << right
      << setw (6) << cInfo->GetGbrLinkRatio (ConnectionInfo::FWD) << " "
      << setw (6) << cInfo->GetGbrLinkRatio (ConnectionInfo::BWD) << "   ";

      *m_renWrapper->GetStream ()
      << right
      << setw (6) << cInfo->GetNonGbrLinkRatio (ConnectionInfo::FWD) << " "
      << setw (6) << cInfo->GetNonGbrLinkRatio (ConnectionInfo::BWD) << "   ";
    }

  *m_bwbWrapper->GetStream () << std::endl;
  *m_bwgWrapper->GetStream () << std::endl;
  *m_bwnWrapper->GetStream () << std::endl;
  *m_regWrapper->GetStream () << std::endl;
  *m_renWrapper->GetStream () << std::endl;

  ResetCounters ();
  Simulator::Schedule (nextDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
BackhaulStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);

  m_lastResetTime = Simulator::Now ();
  ConnInfoList_t::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      (*it)->ResetTxBytes ();
    }
}

} // Namespace ns3
