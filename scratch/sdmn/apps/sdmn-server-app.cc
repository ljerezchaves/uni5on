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

#include "sdmn-server-app.h"
#include "sdmn-client-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnServerApp");
NS_OBJECT_ENSURE_REGISTERED (SdmnServerApp);

SdmnServerApp::SdmnServerApp ()
  : m_qosStats (Create<QosStatsCalculator> ()),
    m_socket (0),
    m_clientApp (0),
    m_active (false),
    m_forceStopFlag (false)
{
  NS_LOG_FUNCTION (this);
}

SdmnServerApp::~SdmnServerApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdmnServerApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdmnServerApp")
    .SetParent<Application> ()
    .AddConstructor<SdmnServerApp> ()
    .AddAttribute ("ClientAddress",
                   "The client IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&SdmnServerApp::m_clientAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ClientPort",
                   "The client port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SdmnServerApp::m_clientPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LocalPort",
                   "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SdmnServerApp::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

bool
SdmnServerApp::IsActive (void) const
{
  return m_active;
}

bool
SdmnServerApp::IsForceStop (void) const
{
  return m_forceStopFlag;
}

Ptr<const QosStatsCalculator>
SdmnServerApp::GetQosStats (void) const
{
  return m_qosStats;
}

void
SdmnServerApp::SetClient (Ptr<SdmnClientApp> clientApp,
                          Ipv4Address clientAddress, uint16_t clientPort)
{
  m_clientApp = clientApp;
  m_clientAddress = clientAddress;
  m_clientPort = clientPort;
}

Ptr<SdmnClientApp>
SdmnServerApp::GetClientApp ()
{
  return m_clientApp;
}

void
SdmnServerApp::NotifyStart ()
{
  NS_LOG_FUNCTION (this);
  ResetQosStats ();
  m_active = true;
  m_forceStopFlag = false;
}

void
SdmnServerApp::NotifyStop ()
{
  NS_LOG_FUNCTION (this);
  m_active = false;
}

void
SdmnServerApp::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);
  m_forceStopFlag = true;
}

void
SdmnServerApp::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);
  m_qosStats->ResetCounters ();
}

void
SdmnServerApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_qosStats = 0;
  m_socket = 0;
  m_clientApp = 0;
  Application::DoDispose ();
}

} // namespace ns3
