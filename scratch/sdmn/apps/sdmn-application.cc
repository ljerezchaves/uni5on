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

#include "sdmn-application.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnApplication");
NS_OBJECT_ENSURE_REGISTERED (SdmnApplication);

SdmnApplication::SdmnApplication()
  : m_qosStats (Create<QosStatsCalculator> ()),
    m_active (false),
    m_tft (0),
    m_teid (0)
{
  NS_LOG_FUNCTION (this);
}

SdmnApplication::~SdmnApplication()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdmnApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdmnApplication")
    .SetParent<Application> ()
    .AddConstructor<SdmnApplication> ()
    .AddAttribute ("MaxDurationTime",
                   "A hard duration time threshold.",
                   TimeValue (Time ()),
                   MakeTimeAccessor (&SdmnApplication::m_maxDurationTime),
                   MakeTimeChecker ())
    .AddAttribute ("AppName",
                   "The application name.",
                   StringValue ("NoName"),
                   MakeStringAccessor (&SdmnApplication::m_name),
                   MakeStringChecker ())

    .AddTraceSource ("AppStart",
                     "SdmnApplication start trace source.",
                     MakeTraceSourceAccessor (&SdmnApplication::m_appStartTrace),
                     "ns3::SdmnApplication::EpcAppTracedCallback")
    .AddTraceSource ("AppStop",
                     "SdmnApplication stop trace source.",
                     MakeTraceSourceAccessor (&SdmnApplication::m_appStopTrace),
                     "ns3::SdmnApplication::EpcAppTracedCallback")  
  ;
  return tid;
}

Ptr<const QosStatsCalculator>
SdmnApplication::GetQosStats (void) const
{
  return m_qosStats;
}

void
SdmnApplication::Start ()
{
  NS_LOG_FUNCTION (this);
}

bool
SdmnApplication::IsActive (void) const
{
  return m_active;
}

Ptr<EpcTft>
SdmnApplication::GetTft (void) const
{
  return m_tft;
}

EpsBearer
SdmnApplication::GetEpsBearer (void) const
{
  return m_bearer;
}

uint32_t
SdmnApplication::GetTeid (void) const
{
  return m_teid;
}

std::string 
SdmnApplication::GetAppName (void) const
{
  return m_name;
}

void
SdmnApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_qosStats = 0;
  m_tft = 0;
  Application::DoDispose ();
}

void
SdmnApplication::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);
  m_qosStats->ResetCounters ();
}

} // namespace ns3
