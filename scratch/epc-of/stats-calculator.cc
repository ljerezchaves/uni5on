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

#include "stats-calculator.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsCalculator");

AdmissionStatsCalculator::AdmissionStatsCalculator ()
  : m_nonRequests (0),
    m_nonAccepted (0),
    m_nonBlocked (0),
    m_gbrRequests (0),
    m_gbrAccepted (0),
    m_gbrBlocked (0),
    m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);

  Config::ConnectWithoutContext (
    "/Names/MainController/BearerRequest",
    MakeCallback (&AdmissionStatsCalculator::BearerRequest, this));
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
    .AddAttribute ("DumpStatsTimeout",
                   "Periodic statistics dump interval.",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&AdmissionStatsCalculator::SetDumpTimeout),
                   MakeTimeChecker ())
    .AddTraceSource ("AdmStats",
                     "The cummulative bearer request trace source fired regularlly.",
                     MakeTraceSourceAccessor (&AdmissionStatsCalculator::m_admTrace),
                     "ns3::AdmissionStatsCalculator::AdmTracedCallback")
    .AddTraceSource ("BrqStats",
                     "The bearer request trace source fired for every request.",
                     MakeTraceSourceAccessor (&AdmissionStatsCalculator::m_brqTrace),
                     "ns3::BearerRequestStats::BrqTracedCallback")
  ;
  return tid;
}

void
AdmissionStatsCalculator::SetDumpTimeout (Time timeout)
{
  m_dumpTimeout = timeout;
  Simulator::Schedule (m_dumpTimeout,
    &AdmissionStatsCalculator::DumpStatistics, this);
}

void
AdmissionStatsCalculator::DumpStatistics ()
{
  m_admTrace (this);
  ResetCounters ();

  Simulator::Schedule (m_dumpTimeout, 
    &AdmissionStatsCalculator::DumpStatistics, this);
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void 
AdmissionStatsCalculator::BearerRequest (bool accepted, 
                                         Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << accepted << rInfo);
  
  // Update internal counter
  if (rInfo->IsGbr ())
    {
      m_gbrRequests++;
      if (accepted)
        {
          m_gbrAccepted++;
        }
      else
        { 
          m_gbrBlocked++;
        }
    }
  else
    {
      m_nonRequests++;
      if (accepted)
        {
          m_nonAccepted++;
        }
      else
        { 
          m_nonBlocked++;
        }
    }

  // Preparing bearer request stats for trace source
  Ptr<BearerRequestStats> reqStats = Create<BearerRequestStats> ();
  reqStats->m_teid = rInfo->GetTeid ();
  reqStats->m_accepted = accepted;
  reqStats->m_trafficDesc = "";
  reqStats->m_routingPaths = "Shortest paths";
  
  Ptr<const ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (reserveInfo)
    {
      reqStats->m_downDataRate = reserveInfo->GetDownDataRate ();
      reqStats->m_upDataRate = reserveInfo->GetUpDataRate ();
    }

  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo)
    {
      if (ringInfo->IsDownInv () && ringInfo->IsUpInv ())
        {
          reqStats->m_routingPaths = "Inverted paths";
        }
      else if (ringInfo->IsDownInv () || ringInfo->IsUpInv ())
        {
          reqStats->m_routingPaths = 
            ringInfo->IsDownInv () ? "Inverted down path" : "Inverted up path";
        }
    }
  // Fire trace source
  m_brqTrace (reqStats);
}

void
AdmissionStatsCalculator::ResetCounters ()
{
  m_nonRequests = 0;
  m_nonAccepted = 0;
  m_nonBlocked = 0;
  m_gbrRequests = 0;
  m_gbrAccepted = 0;
  m_gbrBlocked = 0;
  m_lastResetTime = Simulator::Now ();
}

Time      
AdmissionStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrRequests (void) const
{
  return m_nonRequests;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrAccepted (void) const
{
  return m_nonAccepted;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrBlocked (void) const
{
  return m_nonBlocked;
}

double
AdmissionStatsCalculator::GetNonGbrBlockRatio (void) const
{
  uint32_t req = GetNonGbrRequests ();
  return req ? (double)GetNonGbrBlocked () / req : 0; 
}

uint32_t
AdmissionStatsCalculator::GetGbrRequests (void) const
{
  return m_gbrRequests;
}

uint32_t
AdmissionStatsCalculator::GetGbrAccepted (void) const
{
  return m_gbrAccepted;
}

uint32_t
AdmissionStatsCalculator::GetGbrBlocked (void) const
{
  return m_gbrBlocked;
}

double
AdmissionStatsCalculator::GetGbrBlockRatio (void) const
{
  uint32_t req = GetGbrRequests ();
  return req ? (double)GetGbrBlocked () / req : 0; 
}

uint32_t
AdmissionStatsCalculator::GetTotalRequests (void) const
{
  return GetNonGbrRequests () + GetGbrRequests ();
}

uint32_t
AdmissionStatsCalculator::GetTotalAccepted (void) const
{
  return GetNonGbrAccepted () + GetGbrAccepted ();
}

uint32_t
AdmissionStatsCalculator::GetTotalBlocked (void) const
{
  return GetNonGbrBlocked () + GetGbrBlocked ();
}


// ------------------------------------------------------------------------ //
BearerRequestStats::BearerRequestStats ()
  : m_accepted (false)
{
  NS_LOG_FUNCTION (this);
}

BearerRequestStats::~BearerRequestStats ()
{
  NS_LOG_FUNCTION (this);
}

uint32_t  
BearerRequestStats::GetTeid (void) const
{
  return m_teid;
}

bool      
BearerRequestStats::IsAccepted (void) const
{
  return m_accepted;
}

DataRate  
BearerRequestStats::GetDownDataRate (void) const
{
  return m_downDataRate;
}

DataRate  
BearerRequestStats::GetUpDataRate (void) const
{
  return m_upDataRate;
}

std::string
BearerRequestStats::GetDescription (void) const
{
  return m_trafficDesc;
}

std::string
BearerRequestStats::GetRoutingPaths (void) const
{
  return m_routingPaths;
}

} // Namespace ns3
