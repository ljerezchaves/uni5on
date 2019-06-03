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
  memset (m_slices, 0, sizeof (SliceMetadata) * N_SLICE_IDS_ALL);

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
  SliceMetadata &slData = m_slices [rInfo->GetSliceId ()];
  SliceMetadata &agData = m_slices [SliceId::ALL];

  slData.requests++;
  agData.requests++;
  if (rInfo->IsBlocked ())
    {
      slData.blocked++;
      agData.blocked++;
    }
  else
    {
      slData.accepted++;
      agData.accepted++;
      slData.activeBearers++;
      agData.activeBearers++;
      if (rInfo->IsAggregated ())
        {
          slData.aggregBearers++;
          agData.aggregBearers++;
          slData.aggregated++;
          agData.aggregated++;
        }
      else
        {
          slData.instalBearers++;
          agData.instalBearers++;
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

  SliceMetadata &slData = m_slices [rInfo->GetSliceId ()];
  SliceMetadata &agData = m_slices [SliceId::ALL];
  NS_ASSERT_MSG (slData.activeBearers, "No active bearer here.");

  slData.releases++;
  agData.releases++;
  slData.activeBearers--;
  agData.activeBearers--;
  if (rInfo->IsAggregated ())
    {
      slData.aggregBearers--;
      agData.aggregBearers--;
    }
  else
    {
      slData.instalBearers--;
      agData.instalBearers--;
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
      SliceMetadata &slData = m_slices [s];

      // Create the output file for this slice.
      slData.admWrapper = Create<OutputStreamWrapper> (
          m_admFilename + "-" + sliceStr + ".log", std::ios::out);

      // Print the header in output file.
      *slData.admWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec"
        << " " << setw (7) << "Release"
        << " " << setw (7) << "Request"
        << " " << setw (7) << "Accept"
        << " " << setw (7) << "Block"
        << " " << setw (7) << "Aggreg"
        << " " << setw (7) << "ActNum"
        << " " << setw (7) << "InsNum"
        << " " << setw (7) << "AggNum"
        << std::endl;
    }

  // Create the output file for bearer requests.
  m_brqWrapper = Create<OutputStreamWrapper> (
      m_brqFilename + ".log", std::ios::out);

  // Print the header in output file.
  *m_brqWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8) << "TimeSec";
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
      SliceMetadata &slData = m_slices [s];
      *slData.admWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (7) << slData.releases
        << " " << setw (7) << slData.requests
        << " " << setw (7) << slData.accepted
        << " " << setw (7) << slData.blocked
        << " " << setw (7) << slData.aggregated
        << " " << setw (7) << slData.activeBearers
        << " " << setw (7) << slData.instalBearers
        << " " << setw (7) << slData.aggregBearers
        << std::endl;
      ResetCounters (slData);
    }

  Simulator::Schedule (nextDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
AdmissionStatsCalculator::ResetCounters (SliceMetadata &slData)
{
  NS_LOG_FUNCTION (this);

  slData.releases   = 0;
  slData.requests   = 0;
  slData.accepted   = 0;
  slData.blocked    = 0;
  slData.aggregated = 0;
}

} // Namespace ns3
