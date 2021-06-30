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

#include "base-client.h"
#include "base-server.h"
#include "../uni5on-common.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " client teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BaseClient");
NS_OBJECT_ENSURE_REGISTERED (BaseClient);

BaseClient::BaseClient ()
  : m_socket (0),
  m_serverApp (0),
  m_active (false),
  m_forceStop (EventId ()),
  m_forceStopFlag (false),
  m_rxBytes (0),
  m_startTime (Time (0)),
  m_stopTime (Time (0)),
  m_bearerId (1),   // This is the default BID.
  m_teid (0)
{
  NS_LOG_FUNCTION (this);
}

BaseClient::~BaseClient ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BaseClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BaseClient")
    .SetParent<Application> ()
    .AddConstructor<BaseClient> ()
    .AddAttribute ("AppName", "The application name.",
                   StringValue ("NoName"),
                   MakeStringAccessor (&BaseClient::m_name),
                   MakeStringChecker ())
    .AddAttribute ("MaxOnTime", "A hard duration time threshold.",
                   TimeValue (Time ()),
                   MakeTimeAccessor (&BaseClient::m_maxOnTime),
                   MakeTimeChecker ())
    .AddAttribute ("TrafficLength",
                   "A random variable used to pick the traffic length [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&BaseClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())

    .AddAttribute ("ServerAddress", "The server socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&BaseClient::m_serverAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&BaseClient::m_localPort),
                   MakeUintegerChecker<uint16_t> ())

    .AddTraceSource ("AppStart", "BaseClient start trace source.",
                     MakeTraceSourceAccessor (&BaseClient::m_appStartTrace),
                     "ns3::BaseClient::AppTracedCallback")
    .AddTraceSource ("AppStop", "BaseClient stop trace source.",
                     MakeTraceSourceAccessor (&BaseClient::m_appStopTrace),
                     "ns3::BaseClient::AppTracedCallback")
    .AddTraceSource ("AppError", "BaseClient error trace source.",
                     MakeTraceSourceAccessor (&BaseClient::m_appErrorTrace),
                     "ns3::BaseClient::AppTracedCallback")
  ;
  return tid;
}

std::string
BaseClient::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_name;
}

std::string
BaseClient::GetNameTeid (void) const
{
  // No log to avoid infinite recursion.
  std::ostringstream value;
  value << GetAppName () << " over bearer teid " << GetTeidHex ();
  return value.str ();
}

bool
BaseClient::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_active;
}

Time
BaseClient::GetMaxOnTime (void) const
{
  NS_LOG_FUNCTION (this);

  return m_maxOnTime;
}

bool
BaseClient::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  return m_forceStopFlag;
}

EpsBearer
BaseClient::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer;
}

uint8_t
BaseClient::GetEpsBearerId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearerId;
}

uint32_t
BaseClient::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_teid;
}

std::string
BaseClient::GetTeidHex (void) const
{
  // No log to avoid infinite recursion.
  return GetUint32Hex (m_teid);
}

Ptr<BaseServer>
BaseClient::GetServerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverApp;
}

void
BaseClient::SetEpsBearer (EpsBearer value)
{
  NS_LOG_FUNCTION (this);

  m_bearer = value;
}

void
BaseClient::SetEpsBearerId (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_bearerId = value;
}

void
BaseClient::SetTeid (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_teid = value;
}

void
BaseClient::SetServer (Ptr<BaseServer> serverApp, Address serverAddress)
{
  NS_LOG_FUNCTION (this << serverApp << serverAddress);

  m_serverApp = serverApp;
  m_serverAddress = serverAddress;
}

void
BaseClient::Start ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting client application.");

  // Set the active flag.
  NS_ASSERT_MSG (!IsActive (), "Can't start an already active application.");
  m_active = true;

  // Reset rx byte counter and update start time.
  m_rxBytes = 0;
  m_startTime = Simulator::Now ();
  m_stopTime = Time (0);

  // Schedule the force stop event.
  m_forceStopFlag = false;
  if (!m_maxOnTime.IsZero ())
    {
      m_forceStop =
        Simulator::Schedule (m_maxOnTime, &BaseClient::ForceStop, this);
    }

  // Notify the server and fire start trace source.
  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  m_serverApp->NotifyStart ();
  m_appStartTrace (this);
}

DataRate
BaseClient::GetDlGoodput (void) const
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

DataRate
BaseClient::GetUlGoodput (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverApp->GetUlGoodput ();
}

void
BaseClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_lengthRng = 0;
  m_socket = 0;
  m_serverApp = 0;
  m_forceStop.Cancel ();
  Application::DoDispose ();
}

void
BaseClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Forcing the client application to stop.");

  // Set the force stop flag.
  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  m_forceStopFlag = true;
  m_forceStop.Cancel ();

  // Notify the server.
  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  m_serverApp->NotifyForceStop ();
}

Time
BaseClient::GetTrafficLength ()
{
  NS_LOG_FUNCTION (this);

  return Seconds (std::abs (m_lengthRng->GetValue ()));
}

void
BaseClient::NotifyStop (bool withError)
{
  NS_LOG_FUNCTION (this << withError);
  NS_LOG_INFO ("Client application stopped.");

  // Set the active flag.
  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  m_active = false;
  m_forceStop.Cancel ();

  // Update stop time.
  m_stopTime = Simulator::Now ();

  // Notify the server.
  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  m_serverApp->NotifyStop ();

  // Fire the stop trace source.
  if (withError)
    {
      NS_LOG_ERROR ("Client application stopped with error.");
      m_appErrorTrace (this);
    }
  else
    {
      m_appStopTrace (this);
    }
}

void
BaseClient::NotifyRx (uint32_t bytes)
{
  NS_LOG_FUNCTION (this << bytes);

  m_rxBytes += bytes;
}

} // namespace ns3
