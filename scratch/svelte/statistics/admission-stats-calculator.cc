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
#include "admission-stats-calculator.h"
#include "../metadata/ring-info.h"
#include "../metadata/routing-info.h"
#include "../metadata/ue-info.h"
#include "../metadata/enb-info.h"
#include "../metadata/sgw-info.h"
#include "../metadata/pgw-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AdmissionStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (AdmissionStatsCalculator);

AdmissionStatsCalculator::AdmissionStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Clear slice metadata.
  memset (m_slices, 0, sizeof (SliceStats) * N_SLICES_ALL);

  // Connect this stats calculator to required trace sources.
  Config::ConnectWithoutContext (
    "/NodeList/*/ApplicationList/*/$ns3::SliceController/BearerRequest",
    MakeCallback (&AdmissionStatsCalculator::NotifyBearerRequest, this));
  Config::ConnectWithoutContext (
    "/NodeList/*/ApplicationList/*/$ns3::SliceController/BearerRelease",
    MakeCallback (&AdmissionStatsCalculator::NotifyBearerRelease, this));
}

AdmissionStatsCalculator::~AdmissionStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
AdmissionStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AdmissionStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<AdmissionStatsCalculator> ()
    .AddAttribute ("AdmStatsFilename",
                   "Filename for bearer admission and counter statistics.",
                   StringValue ("admission-counters"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_admFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("admission-requests"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_brqFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
AdmissionStatsCalculator::NotifyBearerRequest (Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  Ptr<const UeInfo> ueInfo = rInfo->GetUeInfo ();
  Ptr<const RingInfo> ringInfo = rInfo->GetObject<RingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ring information for this routing info.");

  // Update the slice stats.
  SliceStats &sliStats = m_slices [rInfo->GetSliceId ()];
  SliceStats &aggStats = m_slices [SliceId::ALL];

  sliStats.requests++;
  aggStats.requests++;
  if (rInfo->IsBlocked ())
    {
      sliStats.blocked++;
      aggStats.blocked++;
    }
  else
    {
      sliStats.accepted++;
      aggStats.accepted++;
      sliStats.activeBearers++;
      aggStats.activeBearers++;
      if (rInfo->IsAggregated ())
        {
          sliStats.aggregBearers++;
          aggStats.aggregBearers++;
          sliStats.aggregated++;
          aggStats.aggregated++;
        }
      else
        {
          sliStats.instalBearers++;
          aggStats.instalBearers++;
        }
    }

  // Save request stats into output file.
  *m_brqWrapper->GetStream ()
    << " " << setw (8) << Simulator::Now ().GetSeconds ()
    << *rInfo
    << *(rInfo->GetUeInfo ())
    << *(rInfo->GetUeInfo ()->GetEnbInfo ())
    << *(rInfo->GetUeInfo ()->GetSgwInfo ())
    << *(rInfo->GetUeInfo ()->GetPgwInfo ())
    << *ringInfo
    << std::endl;
}

void
AdmissionStatsCalculator::NotifyBearerRelease (Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  SliceStats &sliStats = m_slices [rInfo->GetSliceId ()];
  SliceStats &aggStats = m_slices [SliceId::ALL];
  NS_ASSERT_MSG (sliStats.activeBearers, "No active bearer here.");

  sliStats.releases++;
  aggStats.releases++;
  sliStats.activeBearers--;
  aggStats.activeBearers--;
  if (rInfo->IsAggregated ())
    {
      sliStats.aggregBearers--;
      aggStats.aggregBearers--;
    }
  else
    {
      sliStats.instalBearers--;
      aggStats.instalBearers--;
    }
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_brqWrapper = 0;
  for (int s = 0; s <= SliceId::ALL; s++)
    {
      m_slices [s].admWrapper = 0;
    }

  Object::DoDispose ();
}

void
AdmissionStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("AdmStatsFilename", StringValue (prefix + m_admFilename));
  SetAttribute ("BrqStatsFilename", StringValue (prefix + m_brqFilename));

  for (int s = 0; s <= SliceId::ALL; s++)
    {
      std::string sliceStr = SliceIdStr (static_cast<SliceId> (s));
      m_slices [s].admWrapper = Create<OutputStreamWrapper> (
          m_admFilename + "-" + sliceStr + ".log", std::ios::out);
      *m_slices [s].admWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "Time:s"
        << " " << setw (6) << "Relea"
        << " " << setw (6) << "Reque"
        << " " << setw (6) << "Accep"
        << " " << setw (6) << "Block"
        << " " << setw (6) << "Aggre"
        << " " << setw (6) << "#Actv"
        << " " << setw (6) << "#Inst"
        << " " << setw (6) << "#Aggr"
        << std::endl;
    }

  m_brqWrapper = Create<OutputStreamWrapper> (
      m_brqFilename + ".log", std::ios::out);
  *m_brqWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8) << "Time:s";
  RoutingInfo::PrintHeader (*m_brqWrapper->GetStream ());
  UeInfo::PrintHeader (*m_brqWrapper->GetStream ());
  EnbInfo::PrintHeader (*m_brqWrapper->GetStream ());
  SgwInfo::PrintHeader (*m_brqWrapper->GetStream ());
  PgwInfo::PrintHeader (*m_brqWrapper->GetStream ());
  RingInfo::PrintHeader (*m_brqWrapper->GetStream ());
  *m_brqWrapper->GetStream () << std::endl;

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time firstDump = timeValue.Get ();
  Simulator::Schedule (firstDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, firstDump);

  Object::NotifyConstructionCompleted ();
}

void
AdmissionStatsCalculator::DumpStatistics (Time nextDump)
{
  NS_LOG_FUNCTION (this);

  // Iterate over all slices dumping statistics.
  for (int s = 0; s <= SliceId::ALL; s++)
    {
      SliceStats &stats = m_slices [s];
      *stats.admWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (6) << stats.releases
        << " " << setw (6) << stats.requests
        << " " << setw (6) << stats.accepted
        << " " << setw (6) << stats.blocked
        << " " << setw (6) << stats.aggregated
        << " " << setw (6) << stats.activeBearers
        << " " << setw (6) << stats.instalBearers
        << " " << setw (6) << stats.aggregBearers
        << std::endl;
      ResetCounters (stats);
    }

  Simulator::Schedule (nextDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
AdmissionStatsCalculator::ResetCounters (SliceStats &stats)
{
  NS_LOG_FUNCTION (this);

  stats.releases   = 0;
  stats.requests   = 0;
  stats.accepted   = 0;
  stats.blocked    = 0;
  stats.aggregated = 0;
}

} // Namespace ns3