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
  ;
  return tid;
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void 
AdmissionStatsCalculator::NotifyRequest (bool accepted, 
                                         Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << accepted << rInfo);
  
  // Update internal counters
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

} // Namespace ns3
