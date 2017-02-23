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
                   StringValue ("bearer-counters.log"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_admFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("bearer-requests.log"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_brqFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
AdmissionStatsCalculator::NotifyBearerRequest (bool accepted,
                                               Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << accepted << rInfo);

  // Update internal counters
  if (rInfo->IsGbr ())
    {
      m_gbrRequests++;
      accepted ? m_gbrAccepted++ : m_gbrBlocked++;
    }
  else
    {
      m_nonRequests++;
      accepted ? m_nonAccepted++ : m_nonBlocked++;
    }

  if (accepted)
    {
      m_activeBearers++;
    }

  // Preparing bearer request stats for trace source
  uint64_t downBitRate = 0, upBitRate = 0;
  Ptr<const GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo)
    {
      downBitRate = gbrInfo->GetDownBitRate ();
      upBitRate = gbrInfo->GetUpBitRate ();
    }

  std::string path = "No information";
  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo && accepted)
    {
      // For ring routing, print detailed routing path description
      if (ringInfo->IsDefaultPath ())
        {
          path = "Shortest";
        }
      else
        {
          path = "Inverted";
        }
    }

  // Save request stats into output file
  *m_brqWrapper->GetStream ()
  << left
  << setw (9)  << Simulator::Now ().GetSeconds ()           << " "
  << right
  << setw (4)  << rInfo->GetQciInfo ()                      << " "
  << setw (6)  << rInfo->IsGbr ()                           << " "
  << setw (9)  << rInfo->IsDefault ()                       << " "
  << setw (7)  << rInfo->GetImsi ()                         << " "
  << setw (7)  << rInfo->GetCellId ()                       << " "
  << setw (6)  << rInfo->GetEnbSwDpId ()                    << " "
  << setw (6)  << rInfo->GetPgwSwDpId ()                    << " "
  << setw (6)  << rInfo->GetTeid ()                         << " "
  << setw (9)  << accepted                                  << " "
  << setw (11) << static_cast<double> (downBitRate) / 1000  << " "
  << setw (11) << static_cast<double> (upBitRate) / 1000    << "  "
  << left
  << setw (15) << path
  << std::endl;
}

void
AdmissionStatsCalculator::NotifyBearerRelease (bool success,
                                               Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << success << rInfo);

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
  << setw (11) << "Time(s)"
  << right
  << setw (14) << "GbrReqs"
  << setw (14) << "GbrBlocks"
  << setw (14) << "NonGbrReqs"
  << setw (14) << "NonGbrBlocks"
  << setw (14) << "ActiveBearers"
  << std::endl;

  m_brqWrapper = Create<OutputStreamWrapper> (m_brqFilename, std::ios::out);
  *m_brqWrapper->GetStream ()
  << boolalpha << fixed << setprecision (4)
  << left
  << setw (10) << "Time(s)"
  << right
  << setw (4)  << "QCI"
  << setw (7)  << "IsGBR"
  << setw (10) << "IsDefault"
  << setw (8)  << "UeImsi"
  << setw (8)  << "CellId"
  << setw (7)  << "SgwSw"
  << setw (7)  << "PgwSw"
  << setw (7)  << "TEID"
  << setw (10) << "Accepted"
  << setw (12) << "Down(kbps)"
  << setw (12) << "Up(kbps)"
  << left      << "  "
  << setw (12) << "RoutingPath"
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
  << setw (11) << Simulator::Now ().GetSeconds () << " "
  << right
  << setw (13) << m_gbrRequests                   << " "
  << setw (13) << m_gbrBlocked                    << " "
  << setw (13) << m_nonRequests                   << " "
  << setw (13) << m_nonBlocked                    << " "
  << setw (13) << m_activeBearers
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
