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
{
  NS_LOG_FUNCTION (this);

  // Clear slice metadata.
  memset (m_slices, 0, sizeof (SliceStats) * N_SLICES_ALL);
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
    .AddAttribute ("LinStatsFilename",
                   "Filename for backhaul link statistics.",
                   StringValue ("backhaul-link"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_linFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
BackhaulStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  for (int s = 0; s <= SliceId::ALL; s++)
    {
      m_slices [s].linWrapper = 0;
    }
  Object::DoDispose ();
}

void
BackhaulStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("LinStatsFilename", StringValue (prefix + m_linFilename));

  for (int s = 0; s <= SliceId::ALL; s++)
    {
      std::string sliceStr = SliceIdStr (static_cast<SliceId> (s));
      SliceStats &stats = m_slices [s];

      // Create the output file for this slice.
      stats.linWrapper = Create<OutputStreamWrapper> (
          m_linFilename + "-" + sliceStr + ".log", std::ios::out);

      // Print the header in output file.
      *stats.linWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec";
      LinkInfo::PrintHeader (*stats.linWrapper->GetStream ());
      *stats.linWrapper->GetStream () << std::endl;
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

  // For each network slice, iterate over all links dumping statistics.
  for (int s = 0; s <= SliceId::ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      SliceStats &stats = m_slices [s];

      for (auto const &lInfo : LinkInfo::GetList ())
        {
          *stats.linWrapper->GetStream ()
            << " " << setw (8) << Simulator::Now ().GetSeconds ();
          lInfo->PrintSliceValues (*stats.linWrapper->GetStream (), slice);
          *stats.linWrapper->GetStream () << std::endl;
        }
      *stats.linWrapper->GetStream () << std::endl;
    }

  Simulator::Schedule (nextDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, nextDump);
}

} // Namespace ns3
