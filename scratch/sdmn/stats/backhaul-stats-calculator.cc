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
  : m_lastUpdate (Simulator::Now ())
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
    .AddAttribute ("ShrStatsFilename",
                   "Filename for shared best-effort reservation statistics.",
                   StringValue ("backhaul-shared-res.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_shrFilename),
                   MakeStringChecker ())
    .AddAttribute ("StatsPrefix",
                   "Filename prefix for slice statistics.",
                   StringValue ("backhaul-"),
                   MakeStringAccessor (&BackhaulStatsCalculator::m_prefix),
                   MakeStringChecker ())
    .AddAttribute ("ResStatsSuffix",
                   "Filename suffix for slice reservation statistics.",
                   StringValue ("-res.log"),
                   MakeStringAccessor (&BackhaulStatsCalculator::m_resSuffix),
                   MakeStringChecker ())
    .AddAttribute ("ThpStatsSuffix",
                   "Filename suffix for slice throughput statistics.",
                   StringValue ("-thp.log"),
                   MakeStringAccessor (&BackhaulStatsCalculator::m_thpSuffix),
                   MakeStringChecker ())
    .AddAttribute ("UseStatsSuffix",
                   "Filename suffix for slice usage ratio statistics.",
                   StringValue ("-use.log"),
                   MakeStringAccessor (&BackhaulStatsCalculator::m_useSuffix),
                   MakeStringChecker ())
  ;
  return tid;
}

void
BackhaulStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_shrWrapper = 0;
  m_connections.clear ();
  for (int i = 0; i < Slice::ALL; i++)
    {
      m_slices [i].resWrapper = 0;
      m_slices [i].thpWrapper = 0;
      m_slices [i].useWrapper = 0;
      m_slices [i].fwdBytes.clear ();
      m_slices [i].bwdBytes.clear ();
    }
}

void
BackhaulStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("ShrStatsFilename", StringValue (prefix + m_shrFilename));
  SetAttribute ("StatsPrefix", StringValue (prefix + m_prefix));

  m_shrWrapper = Create<OutputStreamWrapper> (m_shrFilename, std::ios::out);
  *m_shrWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_connections = ConnectionInfo::GetList ();
  ConnInfoList_t::const_iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      DpIdPair_t key = (*it)->GetSwitchDpIdPair ();

      *m_shrWrapper->GetStream ()
      << right << setw (4) << key.first  << "-"
      << left  << setw (4) << key.second << "   ";
    }
  *m_shrWrapper->GetStream () << left << std::endl;

  for (int i = 0; i < Slice::ALL; i++)
    {
      std::string statsPrefix = m_prefix + SliceStr (static_cast<Slice> (i));

      m_slices [i].resWrapper = Create<OutputStreamWrapper> (
          statsPrefix + m_resSuffix, std::ios::out);
      *m_slices [i].resWrapper->GetStream ()
      << left << fixed << setprecision (4)
      << setw (12) << "Time(s)";

      m_slices [i].thpWrapper = Create<OutputStreamWrapper> (
          statsPrefix + m_thpSuffix, std::ios::out);
      *m_slices [i].thpWrapper->GetStream ()
      << left << fixed << setprecision (4)
      << setw (12) << "Time(s)";

      m_slices [i].useWrapper = Create<OutputStreamWrapper> (
          statsPrefix + m_useSuffix, std::ios::out);
      *m_slices [i].useWrapper->GetStream ()
      << left << fixed << setprecision (4)
      << setw (12) << "Time(s)";

      for (it = m_connections.begin (); it != m_connections.end (); it++)
        {
          DpIdPair_t key = (*it)->GetSwitchDpIdPair ();

          *m_slices [i].resWrapper->GetStream ()
          << right << setw (4) << key.first  << "-"
          << left  << setw (4) << key.second << "   ";

          *m_slices [i].thpWrapper->GetStream ()
          << right << setw (10) << key.first  << "-"
          << left  << setw (10) << key.second << "   ";

          *m_slices [i].useWrapper->GetStream ()
          << right << setw (4) << key.first  << "-"
          << left  << setw (4) << key.second << "   ";

          m_slices [i].fwdBytes.push_back (0);
          m_slices [i].bwdBytes.push_back (0);
        }
      *m_slices [i].resWrapper->GetStream () << left << std::endl;
      *m_slices [i].thpWrapper->GetStream () << left << std::endl;
      *m_slices [i].useWrapper->GetStream () << left << std::endl;
    }

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

  double elapSecs = (Simulator::Now () - m_lastUpdate).GetSeconds ();

  *m_shrWrapper->GetStream ()
  << left << setprecision (4)
  << setw (12) << Simulator::Now ().GetSeconds ()
  << setprecision (2);

  uint32_t cIdx;
  Ptr<ConnectionInfo> cInfo;
  ConnInfoList_t::const_iterator it;
  for (cIdx = 0, it = m_connections.begin ();
       it != m_connections.end (); ++it, ++cIdx)
    {
      cInfo = *it;
      *m_shrWrapper->GetStream ()
      << right
      << setw (4) << cInfo->GetMeterLinkRatio (ConnectionInfo::FWD) << " "
      << setw (4) << cInfo->GetMeterLinkRatio (ConnectionInfo::BWD) << "   ";
    }
  *m_shrWrapper->GetStream () << std::endl;

  for (int i = 0; i < Slice::ALL; i++)
    {
      *m_slices [i].resWrapper->GetStream ()
      << left << setprecision (4)
      << setw (12) << Simulator::Now ().GetSeconds ()
      << setprecision (2);

      *m_slices [i].thpWrapper->GetStream ()
      << left << setprecision (4)
      << setw (12) << Simulator::Now ().GetSeconds ()
      << setprecision (2);

      *m_slices [i].useWrapper->GetStream ()
      << left << setprecision (4)
      << setw (12) << Simulator::Now ().GetSeconds ()
      << setprecision (2);

      for (cIdx = 0, it = m_connections.begin ();
           it != m_connections.end (); ++it, ++cIdx)
        {
          cInfo = *it;
          uint64_t fwdBytes = cInfo->GetTxBytes (
              ConnectionInfo::FWD, static_cast<Slice> (i));
          uint64_t bwdBytes = cInfo->GetTxBytes (
              ConnectionInfo::BWD, static_cast<Slice> (i));

          double fwdKbits = static_cast<double> (
              fwdBytes - m_slices [i].fwdBytes.at (cIdx)) * 8 / 1000;
          double bwdKbits = static_cast<double> (
              bwdBytes - m_slices [i].bwdBytes.at (cIdx)) * 8 / 1000;

          m_slices [i].fwdBytes.at (cIdx) = fwdBytes;
          m_slices [i].bwdBytes.at (cIdx) = bwdBytes;

          *m_slices [i].thpWrapper->GetStream ()
          << setfill ('0')
          << right
          << setw (10) << fwdKbits / elapSecs << " "
          << setw (10) << bwdKbits / elapSecs << "   ";

          *m_slices [i].resWrapper->GetStream ()
          << right
          << setw (4) << cInfo->GetResSliceRatio (
            ConnectionInfo::FWD, static_cast<Slice> (i)) << " "
          << setw (4) << cInfo->GetResSliceRatio (
            ConnectionInfo::BWD, static_cast<Slice> (i)) << "   ";

          *m_slices [i].useWrapper->GetStream ()
          << right
          << setw (4) << cInfo->GetEwmaSliceUsage (
            ConnectionInfo::FWD, static_cast<Slice> (i)) << " "
          << setw (4) << cInfo->GetEwmaSliceUsage (
            ConnectionInfo::BWD, static_cast<Slice> (i)) << "   ";
        }
      *m_slices [i].thpWrapper->GetStream () << setfill (' ') << std::endl;
      *m_slices [i].resWrapper->GetStream () << std::endl;
      *m_slices [i].useWrapper->GetStream () << std::endl;
    }

  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (nextDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, nextDump);
}

} // Namespace ns3
