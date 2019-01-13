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

#include "svelte-server.h"
#include "svelte-client.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " server teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteServer");
NS_OBJECT_ENSURE_REGISTERED (SvelteServer);

SvelteServer::SvelteServer ()
  : m_socket (0),
  m_clientApp (0),
  m_rxBytes (0),
  m_startTime (Time (0)),
  m_stopTime (Time (0))
{
  NS_LOG_FUNCTION (this);
}

SvelteServer::~SvelteServer ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteServer")
    .SetParent<Application> ()
    .AddConstructor<SvelteServer> ()
    .AddAttribute ("ClientAddress", "The client socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&SvelteServer::m_clientAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SvelteServer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

std::string
SvelteServer::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetAppName () : std::string ();
}

bool
SvelteServer::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsActive ();
}

bool
SvelteServer::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsForceStop ();
}

std::string
SvelteServer::GetTeidHex (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetTeidHex () : "0x0";
}

Ptr<SvelteClient>
SvelteServer::GetClientApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_clientApp;
}

void
SvelteServer::SetClient (Ptr<SvelteClient> clientApp, Address clientAddress)
{
  NS_LOG_FUNCTION (this << clientApp << clientAddress);

  m_clientApp = clientApp;
  m_clientAddress = clientAddress;
}

DataRate
SvelteServer::GetAppGoodput (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_stopTime != Time (0) && m_startTime != m_stopTime)
    {
      Time elapsed = m_stopTime - m_startTime;
      return DataRate (m_rxBytes * 8 / elapsed.GetSeconds ());
    }
  else
    {
      return DataRate (0);
    }
}

void
SvelteServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  m_clientApp = 0;
  Application::DoDispose ();
}

void
SvelteServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting server application.");

  // Reset rx byte counter and update start time.
  m_rxBytes = 0;
  m_startTime = Simulator::Now ();
  m_stopTime = Time (0);
}

void
SvelteServer::NotifyStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Stopping server application.");

  // Update stop time.
  m_stopTime = Simulator::Now ();
}

void
SvelteServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Forcing the server application to stop.");
}

void
SvelteServer::NotifyRx (uint32_t bytes)
{
  NS_LOG_FUNCTION (this << bytes);

  m_rxBytes += bytes;
}

} // namespace ns3
