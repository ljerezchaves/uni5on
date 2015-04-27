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

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "voip-client.h"
#include "voip-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipClient");

NS_OBJECT_ENSURE_REGISTERED (VoipClient);

TypeId
VoipClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipClient")
    .SetParent<Application> ()
    .AddConstructor<VoipClient> ()
    .AddAttribute ("ServerAddress",
                   "The IPv4 destination address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&VoipClient::m_serverAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ServerPort", 
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipClient::m_serverPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LocalPort",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipClient::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of packets (in bytes). "
                   "Choose between 40, 50 and 60 bytes.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&VoipClient::m_pktSize),
                   MakeUintegerChecker<uint32_t> (40, 120))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (0.06)),
                   MakeTimeAccessor (&VoipClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("OnTime",
                  "A RandomVariableStream used to pick the 'ON' state duration.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VoipClient::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", 
                  "A RandomVariableStream used to pick the 'Off' state duration.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VoipClient::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("Stream",
                   "The stream number for RNG streams. "
                   "-1 means \"allocate a stream automatically\".",
                   IntegerValue(-1),
                   MakeIntegerAccessor(&VoipClient::SetStreams),
                   MakeIntegerChecker<int64_t>())
    ;
  return tid;
}

VoipClient::VoipClient ()
  : m_pktSent (0),
    m_serverApp (0),
    m_txSocket (0),
    m_rxSocket (0),
    m_connected (false)
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_startStopEvent = EventId ();
  m_qosStats = Create<QosStatsCalculator> ();
}

VoipClient::~VoipClient ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipClient::SetServerAddress (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_serverAddress = ip;
  m_serverPort = port;
}

void
VoipClient::SetServerApp (Ptr<VoipServer> server)
{
  NS_LOG_FUNCTION (this << server);
  m_serverApp = server;
}

void
VoipClient::SetStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
}

Ptr<VoipServer> 
VoipClient::GetServerApp ()
{
  return m_serverApp;
}

void 
VoipClient::ResetQosStats ()
{
  m_pktSent = 0;
  m_qosStats->ResetCounters ();
}

Ptr<const QosStatsCalculator>
VoipClient::GetQosStats (void) const
{
  return m_qosStats;
}

void
VoipClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_rxSocket != 0)
    {
      m_rxSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  m_serverApp = 0;
  m_txSocket = 0;
  m_rxSocket = 0;
  m_qosStats = 0;
  m_onTime = 0;
  m_offTime = 0;
  Application::DoDispose ();
}

void
VoipClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  
  // Inbound side
  if (m_rxSocket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_rxSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_rxSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_rxSocket->SetRecvCallback (MakeCallback (&VoipClient::ReadPacket, this));
    }

  // Outbound side
  if (m_txSocket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_txSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_txSocket->Bind ();
      m_txSocket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_txSocket->ShutdownRecv ();
      m_txSocket->SetConnectCallback (
          MakeCallback (&VoipClient::ConnectionSucceeded, this),
          MakeCallback (&VoipClient::ConnectionFailed, this));
      m_txSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  ResetQosStats ();
  CancelEvents ();
  ScheduleStartEvent ();
}

void
VoipClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();
  
  if (m_txSocket != 0)
    {
      m_txSocket->Close ();
      m_txSocket = 0;
    }
  
  if (m_rxSocket == 0)
    {
      m_rxSocket->Close ();
      m_rxSocket = 0;
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
  m_serverApp->StartSending ();
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
  m_serverApp->StopSending ();
  CancelEvents ();
  ScheduleStartEvent ();
}

void 
VoipClient::ScheduleStartEvent ()
{  
  NS_LOG_FUNCTION (this);
  
  // Wait at least 2 seconds before (re)starting the application
  Time offInterval = Seconds (2 + std::abs (m_offTime->GetValue ()));
  m_startStopEvent = Simulator::Schedule (offInterval, &VoipClient::StartSending, this);
  
  NS_LOG_LOGIC ("VoIP " << this << " will start in +" << offInterval.GetSeconds ());
}

void 
VoipClient::ScheduleStopEvent ()
{  
  NS_LOG_FUNCTION (this);
  
  Time onInterval = Seconds (std::abs (m_onTime->GetValue ()));
  m_startStopEvent = Simulator::Schedule (onInterval, &VoipClient::StopSending, this);
  
  NS_LOG_LOGIC ("VoIP " << this << " will stop in +" << onInterval.GetSeconds ());
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

void
VoipClient::SendPacket ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  SeqTsHeader seqTs;
  seqTs.SetSeq (m_pktSent);

  // Using compressed IP/UDP/RTP header. 
  // 38 bytes must be removed from payload.
  Ptr<Packet> p = Create<Packet> (m_pktSize - (38));
  p->AddHeader (seqTs);
 
  if (m_txSocket->Send (p))
    {
      m_pktSent++;
      NS_LOG_INFO ("VoIP TX " << m_pktSize <<
                   " bytes to " << m_serverAddress << 
                   ":" << m_serverPort << 
                   " Uid " << p->GetUid () << 
                   " Time " << (Simulator::Now ()).GetSeconds ());
    }
  else
    {
      NS_LOG_INFO ("Error sending VoIP " << m_pktSize << 
                   " bytes to " << m_serverAddress);
    }
  m_sendEvent = Simulator::Schedule (m_interval, &VoipClient::SendPacket, this);
}

void
VoipClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t seqNum = seqTs.GetSeq ();
          if (InetSocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << seqNum <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
            }

          m_qosStats->NotifyReceived (seqTs.GetSeq (), seqTs.GetTs (), packet->GetSize ());
        }
    }
}

} // Namespace ns3
