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

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[" << GetAppName () << " server teid " << GetTeid () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnServerApp");
NS_OBJECT_ENSURE_REGISTERED (SdmnServerApp);

SdmnServerApp::SdmnServerApp ()
  : m_qosStats (CreateObject<QosStatsCalculator> ()),
  m_socket (0),
  m_clientApp (0)
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
                   "The client socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&SdmnServerApp::m_clientAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort",
                   "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SdmnServerApp::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

std::string
SdmnServerApp::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetAppName () : "";
}

bool
SdmnServerApp::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsActive ();
}

bool
SdmnServerApp::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsForceStop ();
}

uint32_t
SdmnServerApp::GetTeid (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetTeid () : 0;
}

Ptr<SdmnClientApp>
SdmnServerApp::GetClientApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_clientApp;
}

Ptr<const QosStatsCalculator>
SdmnServerApp::GetQosStats (void) const
{
  NS_LOG_FUNCTION (this);

  return m_qosStats;
}

void
SdmnServerApp::SetClient (Ptr<SdmnClientApp> clientApp, Address clientAddress)
{
  NS_LOG_FUNCTION (this << clientApp << clientAddress);

  m_clientApp = clientApp;
  m_clientAddress = clientAddress;
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

void
SdmnServerApp::NotifyStart ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting server application.");

  // Reset internal statistics.
  ResetQosStats ();
}

void
SdmnServerApp::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Forcing the server application to stop.");
}

uint32_t
SdmnServerApp::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->m_qosStats->NotifyTx (txBytes);
}

void
SdmnServerApp::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  m_qosStats->NotifyRx (rxBytes, timestamp);
}

void
SdmnServerApp::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);

  m_qosStats->ResetCounters ();
}

} // namespace ns3
