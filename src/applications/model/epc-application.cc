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

#include "epc-application.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcApplication");
NS_OBJECT_ENSURE_REGISTERED (EpcApplication);

EpcApplication::EpcApplication()
  : m_qosStats (Create<QosStatsCalculator> ()),
    m_tft (0),
    m_teid (0)
{
  NS_LOG_FUNCTION (this);
}

EpcApplication::~EpcApplication()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcApplication")
    .SetParent<Application> ()
    .AddConstructor<EpcApplication> ()
    
    .AddTraceSource ("AppStart",
                     "EpcApplication start trace source.",
                     MakeTraceSourceAccessor (&EpcApplication::m_appStartTrace),
                     "ns3::EpcApplication::EpcAppTracedCallback")
    .AddTraceSource ("AppStop",
                     "EpcApplication stop trace source.",
                     MakeTraceSourceAccessor (&EpcApplication::m_appStopTrace),
                     "ns3::EpcApplication::EpcAppTracedCallback")  
  ;
  return tid;
}

Ptr<const QosStatsCalculator>
EpcApplication::GetQosStats (void) const
{
  return m_qosStats;
}

void
EpcApplication::Start ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<EpcTft>
EpcApplication::GetTft (void) const
{
  return m_tft;
}

EpsBearer
EpcApplication::GetEpsBearer (void) const
{
  return m_bearer;
}

uint32_t
EpcApplication::GetTeid (void) const
{
  return m_teid;
}

std::string
EpcApplication::GetDescription (void) const
{
  return GetAppName () + " [" + m_desc + "]";
}

std::string 
EpcApplication::GetAppName (void) const
{
  return "NoName";
}

void
EpcApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_qosStats = 0;
  m_tft = 0;
  Application::DoDispose ();
}

void
EpcApplication::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);
  m_qosStats->ResetCounters ();
}

} // namespace ns3
