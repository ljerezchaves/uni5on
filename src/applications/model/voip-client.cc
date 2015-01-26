/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2014 University of Campinas (Unicamp)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

// #include "ns3/log.h"
// #include "ns3/ipv4-address.h"
// #include "ns3/nstime.h"
// #include "ns3/inet-socket-address.h"
// #include "ns3/socket.h"
// #include "ns3/simulator.h"
// #include "ns3/socket-factory.h"
// #include "ns3/packet.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

#include "voip-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipClient");

NS_OBJECT_ENSURE_REGISTERED (VoipClient);

TypeId
VoipClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipClient")
    .SetParent<Application> ()
    .AddConstructor<VoipClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&VoipClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", 
                   "The size of packets (in bytes). "
                   "Choose between 40, 50 and 60 bytes.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&VoipClient::m_size),
                   MakeUintegerChecker<uint32_t> (40, 60))
    .AddAttribute ("Interval",
                   "The time to wait between packets.",
                   TimeValue (Seconds (0.06)),
                   MakeTimeAccessor (&VoipClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::NormalRandomVariable[Mean=10.0,Variance=4.0]"),
                   MakePointerAccessor (&VoipClient::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=30.0]"),
                   MakePointerAccessor (&VoipClient::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("Stream",
                   "The stream number for RNG streams. -1 means \"allocate a stream automatically\".",
                   IntegerValue(-1),
                   MakeIntegerAccessor(&VoipClient::AssignStreams),
                   MakeIntegerChecker<int64_t>())
    ;
  return tid;
}

VoipClient::VoipClient ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

VoipClient::~VoipClient ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
VoipClient::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
VoipClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
VoipClient::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
}

void
VoipClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  Application::DoDispose ();
}

void
VoipClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind ();
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6 ();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&VoipClient::ConnectionSucceeded, this),
        MakeCallback (&VoipClient::ConnectionFailed, this));
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  CancelEvents ();
  ScheduleStartEvent ();
}

void
VoipClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();
  if (m_socket != 0)
    {
      m_socket->Close ();
    }
}

void 
VoipClient::CancelEvents ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

void 
VoipClient::StartSending ()
{
  NS_LOG_FUNCTION (this);
  if (!m_startSendingCallback.IsNull ())
    {
      if (!m_startSendingCallback (this))
        {
          NS_LOG_WARN ("Application " << this << " has been blocked.");
          CancelEvents ();
          ScheduleStartEvent ();
          return;
        }
    }
  m_sendEvent = Simulator::Schedule (m_interval, &VoipClient::SendPacket, this);
  ScheduleStopEvent ();
}

void 
VoipClient::StopSending ()
{
  NS_LOG_FUNCTION (this);
  if (!m_stopSendingCallback.IsNull ())
    {
      m_stopSendingCallback (this);
    }
  CancelEvents ();
  ScheduleStartEvent ();
}

void 
VoipClient::ScheduleStartEvent ()
{  
  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("VoIP " << this << " will start in +" << offInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (offInterval, &VoipClient::StartSending, this);
}

void 
VoipClient::ScheduleStopEvent ()
{  
  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("VoIP " << this << " will stop in +" << onInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (onInterval, &VoipClient::StopSending, this);
}

void
VoipClient::SendPacket ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);

  // Using compressed IP/UDP/RTP header. 
  // 38 bytes must be removed from payload.
  Ptr<Packet> p = Create<Packet> (m_size - (38));
  p->AddHeader (seqTs);
  m_totBytes += p->GetSize ();
 
  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if (m_socket->Send (p))
    {
      m_sent++;
      NS_LOG_INFO ("VoIP TX " << m_size << " bytes to "
                              << peerAddressStringStream.str () << " Uid: "
                              << p->GetUid () << " Time: "
                              << (Simulator::Now ()).GetSeconds ());
    }
  else
    {
      NS_LOG_INFO ("Error sending VoIP " << m_size << " bytes to "
                                         << peerAddressStringStream.str ());
    }
  m_sendEvent = Simulator::Schedule (m_interval, &VoipClient::SendPacket, this);
}

void 
VoipClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void 
VoipClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
