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
#include "../info/ue-info.h"
#include "../info/routing-info.h"
#include "../info/ring-routing-info.h"
#include "../info/gbr-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AdmissionStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (AdmissionStatsCalculator);

AdmissionStatsCalculator::AdmissionStatsCalculator ()
  : m_nonRequests (0),
    m_nonAccepted (0),
    m_nonBlocked  (0),
    m_gbrRequests (0),
    m_gbrAccepted (0),
    m_gbrBlocked  (0),
    m_activeBearers (0)
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::ConnectWithoutContext (
    "/NodeList/*/ApplicationList/*/$ns3::EpcController/BearerRequest",
    MakeCallback (&AdmissionStatsCalculator::NotifyBearerRequest, this));
  Config::ConnectWithoutContext (
    "/NodeList/*/ApplicationList/*/$ns3::EpcController/BearerRelease",
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
                   StringValue ("admission-counters.log"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_admFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("admission-requests.log"),
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

  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  Ptr<const GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  NS_ASSERT_MSG (ringInfo, "No ring information for this routing info.");

  // Update internal counters.
  if (rInfo->IsGbr () && !rInfo->IsAggregated ())
    {
      m_gbrRequests++;
      rInfo->IsBlocked () ? m_gbrBlocked++ : m_gbrAccepted++;
    }
  else
    {
      m_nonRequests++;
      rInfo->IsBlocked () ? m_nonBlocked++ : m_nonAccepted++;
    }

  if (!rInfo->IsBlocked ())
    {
      m_activeBearers++;
    }

  // Preparing bearer request stats for trace source.
  double dwBitRate = 0.0, upBitRate = 0.0;
  if (gbrInfo)
    {
      dwBitRate = static_cast<double> (gbrInfo->GetDownBitRate ()) / 1000;
      upBitRate = static_cast<double> (gbrInfo->GetUpBitRate ()) / 1000;
    }

  // Save request stats into output file.
  *m_brqWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << right
  << " " << setw (6) << rInfo->GetTeid ()
  << " " << setw (4) << rInfo->GetQciInfo ()
  << " " << setw (6) << rInfo->IsGbr ()
  << " " << setw (5) << ueInfo->GetImsi ()
  << " " << setw (4) << ueInfo->GetCellId ()
  << " " << setw (6) << rInfo->IsDefault ()
  << " " << setw (6) << ringInfo->GetSgwSwDpId ()
  << " " << setw (6) << ringInfo->GetPgwSwDpId ()
  << " " << setw (6) << rInfo->GetPgwTftIdx ()
  << setprecision (2)
  << " " << setw (8) << dwBitRate
  << " " << setw (8) << upBitRate
  << setprecision (4)
  << " " << setw (6) << rInfo->IsAggregated ()
  << " " << setw (6) << rInfo->IsBlocked ()
  << " " << setw (9) << rInfo->GetBlockReasonStr ()
  << " " << setw (9) << ringInfo->GetPathStr ()
  << std::endl;
}

void
AdmissionStatsCalculator::NotifyBearerRelease (Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (m_activeBearers, "No active bearer here.");
  m_activeBearers--;
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_admWrapper = 0;
  m_brqWrapper = 0;
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

  m_admWrapper = Create<OutputStreamWrapper> (m_admFilename, std::ios::out);
  *m_admWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (8) << "Actives"
  << setw (9) << "Req:GBR"
  << setw (7) << "NonGBR"
  << setw (9) << "Blk:GBR"
  << setw (7) << "NonGBR"
  << std::endl;

  m_brqWrapper = Create<OutputStreamWrapper> (m_brqFilename, std::ios::out);
  *m_brqWrapper->GetStream ()
  << boolalpha << fixed << setprecision (4)
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (6)  << "TEID"
  << setw (5)  << "QCI"
  << setw (7)  << "IsGBR"
  << setw (6)  << "IMSI"
  << setw (5)  << "CGI"
  << setw (7)  << "IsDft"
  << setw (7)  << "SGWsw"
  << setw (7)  << "PGWsw"
  << setw (7)  << "TFTsw"
  << setw (9)  << "DwReq"
  << setw (9)  << "UpReq"
  << setw (7)  << "S5Agg"
  << setw (7)  << "Block"
  << setw (10) << "Reason"
  << setw (10) << "RingPath"
  << std::endl;

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

  *m_admWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << right
  << " " << setw (8) << m_activeBearers
  << " " << setw (8) << m_gbrRequests
  << " " << setw (6) << m_nonRequests
  << " " << setw (8) << m_gbrBlocked
  << " " << setw (6) << m_nonBlocked
  << std::endl;

  ResetCounters ();
  Simulator::Schedule (nextDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
AdmissionStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);

  m_nonRequests = 0;
  m_nonAccepted = 0;
  m_nonBlocked  = 0;
  m_gbrRequests = 0;
  m_gbrAccepted = 0;
  m_gbrBlocked  = 0;
}

} // Namespace ns3
