/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[" << m_name << " client teid " << m_teid << "] "; }

#include "sdmn-client-app.h"
#include "sdmn-server-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnClientApp");
NS_OBJECT_ENSURE_REGISTERED (SdmnClientApp);

SdmnClientApp::SdmnClientApp ()
  : m_qosStats (CreateObject<QosStatsCalculator> ()),
    m_socket (0),
    m_serverApp (0),
    m_active (false),
    m_forceStop (EventId ()),
    m_forceStopFlag (false),
    m_tft (0),
    m_teid (0)
{
  NS_LOG_FUNCTION (this);
}

SdmnClientApp::~SdmnClientApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdmnClientApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdmnClientApp")
    .SetParent<Application> ()
    .AddConstructor<SdmnClientApp> ()
    .AddAttribute ("MaxOnTime",
                   "A hard duration time threshold.",
                   TimeValue (Time ()),
                   MakeTimeAccessor (&SdmnClientApp::m_maxOnTime),
                   MakeTimeChecker ())
    .AddAttribute ("AppName",
                   "The application name.",
                   StringValue ("NoName"),
                   MakeStringAccessor (&SdmnClientApp::m_name),
                   MakeStringChecker ())

    .AddAttribute ("ServerAddress",
                   "The server IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&SdmnClientApp::m_serverAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ServerPort",
                   "The server port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SdmnClientApp::m_serverPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LocalPort",
                   "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SdmnClientApp::m_localPort),
                   MakeUintegerChecker<uint16_t> ())

    .AddTraceSource ("AppStart",
                     "SdmnClientApp start trace source.",
                     MakeTraceSourceAccessor (&SdmnClientApp::m_appStartTrace),
                     "ns3::SdmnClientApp::EpcAppTracedCallback")
    .AddTraceSource ("AppStop",
                     "SdmnClientApp stop trace source.",
                     MakeTraceSourceAccessor (&SdmnClientApp::m_appStopTrace),
                     "ns3::SdmnClientApp::EpcAppTracedCallback")
  ;
  return tid;
}

std::string
SdmnClientApp::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_name;
}

bool
SdmnClientApp::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_active;
}

bool
SdmnClientApp::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  return m_forceStopFlag;
}

Ptr<EpcTft>
SdmnClientApp::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tft;
}

EpsBearer
SdmnClientApp::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer;
}

uint32_t
SdmnClientApp::GetTeid (void) const
{
  // No log to avoid infinite recursion.
  return m_teid;
}

Ptr<SdmnServerApp>
SdmnClientApp::GetServerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverApp;
}

Ptr<const QosStatsCalculator>
SdmnClientApp::GetQosStats (void) const
{
  NS_LOG_FUNCTION (this);

  return m_qosStats;
}

Ptr<const QosStatsCalculator>
SdmnClientApp::GetServerQosStats (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverApp->GetQosStats ();
}

void
SdmnClientApp::SetTft (Ptr<EpcTft> value)
{
  NS_LOG_FUNCTION (this << value);

  m_tft = value;
}

void
SdmnClientApp::SetEpsBearer (EpsBearer value)
{
  NS_LOG_FUNCTION (this);

  m_bearer = value;
}

void
SdmnClientApp::SetTeid (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_teid = value;
}

void
SdmnClientApp::SetServer (Ptr<SdmnServerApp> serverApp,
                          Ipv4Address serverAddress, uint16_t serverPort)
{
  NS_LOG_FUNCTION (this << serverApp << serverAddress << serverPort);

  m_serverApp = serverApp;
  m_serverAddress = serverAddress;
  m_serverPort = serverPort;
}

void
SdmnClientApp::Start ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsActive (), "Can't start an already active application.");
  NS_LOG_INFO ("Starting client application.");
  ResetQosStats ();
  m_active = true;
  m_forceStopFlag = false;
  if (!m_maxOnTime.IsZero ())
    {
      m_forceStop = Simulator::Schedule (
          m_maxOnTime, &SdmnClientApp::ForceStop, this);
    }
  m_serverApp->NotifyStart ();
  m_appStartTrace (this);
}

void
SdmnClientApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_qosStats = 0;
  m_tft = 0;
  m_socket = 0;
  m_serverApp = 0;
  m_forceStop.Cancel ();
  Application::DoDispose ();
}

void
SdmnClientApp::NotifyStop ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  NS_LOG_INFO ("Client application stopped.");
  m_forceStop.Cancel ();
  m_active = false;
  m_serverApp->NotifyStop ();
  m_appStopTrace (this);
}

void
SdmnClientApp::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  NS_LOG_INFO ("Forcing the client application to stop.");
  m_forceStop.Cancel ();
  m_forceStopFlag = true;
  m_serverApp->NotifyForceStop ();
}

uint32_t
SdmnClientApp::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  return m_serverApp->m_qosStats->NotifyTx (txBytes);
}

void
SdmnClientApp::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  m_qosStats->NotifyRx (rxBytes, timestamp);
}

void
SdmnClientApp::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);

  m_qosStats->ResetCounters ();
}

} // namespace ns3
