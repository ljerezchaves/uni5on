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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "uni5on-server.h"
#include "uni5on-client.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " server teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Uni5onServer");
NS_OBJECT_ENSURE_REGISTERED (Uni5onServer);

Uni5onServer::Uni5onServer ()
  : m_socket (0),
  m_clientApp (0),
  m_rxBytes (0),
  m_startTime (Time (0)),
  m_stopTime (Time (0))
{
  NS_LOG_FUNCTION (this);
}

Uni5onServer::~Uni5onServer ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
Uni5onServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Uni5onServer")
    .SetParent<Application> ()
    .AddConstructor<Uni5onServer> ()
    .AddAttribute ("ClientAddress", "The client socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&Uni5onServer::m_clientAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&Uni5onServer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

std::string
Uni5onServer::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetAppName () : std::string ();
}

bool
Uni5onServer::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsActive ();
}

bool
Uni5onServer::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->IsForceStop ();
}

std::string
Uni5onServer::GetTeidHex (void) const
{
  // No log to avoid infinite recursion.
  return m_clientApp ? m_clientApp->GetTeidHex () : "0x0";
}

Ptr<Uni5onClient>
Uni5onServer::GetClientApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_clientApp;
}

void
Uni5onServer::SetClient (Ptr<Uni5onClient> clientApp, Address clientAddress)
{
  NS_LOG_FUNCTION (this << clientApp << clientAddress);

  m_clientApp = clientApp;
  m_clientAddress = clientAddress;
}

DataRate
Uni5onServer::GetUlGoodput (void) const
{
  NS_LOG_FUNCTION (this);

  Time elapsed = (IsActive () ? Simulator::Now () : m_stopTime) - m_startTime;
  if (elapsed.IsZero ())
    {
      return DataRate (0);
    }
  else
    {
      return DataRate (m_rxBytes * 8 / elapsed.GetSeconds ());
    }
}

void
Uni5onServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  m_clientApp = 0;
  Application::DoDispose ();
}

void
Uni5onServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting server application.");

  // Reset rx byte counter and update start time.
  m_rxBytes = 0;
  m_startTime = Simulator::Now ();
  m_stopTime = Time (0);
}

void
Uni5onServer::NotifyStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Stopping server application.");

  // Update stop time.
  m_stopTime = Simulator::Now ();
}

void
Uni5onServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Forcing the server application to stop.");
}

void
Uni5onServer::NotifyRx (uint32_t bytes)
{
  NS_LOG_FUNCTION (this << bytes);

  m_rxBytes += bytes;
}

} // namespace ns3
