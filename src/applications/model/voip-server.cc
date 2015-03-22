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
#include "voip-server.h"
#include "voip-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipServer");

NS_OBJECT_ENSURE_REGISTERED (VoipServer);

TypeId
VoipServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipServer")
    .SetParent<Application> ()
    .AddConstructor<VoipServer> ()
    .AddAttribute ("ClientAddress",
                   "The IPv4 destination address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&VoipServer::m_clientAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ClientPort", 
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipServer::m_clientPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LocalPort",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipServer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of packets (in bytes). "
                   "Choose between 40, 50 and 60 bytes.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&VoipServer::m_pktSize),
                   MakeUintegerChecker<uint32_t> (40, 120))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (0.06)),
                   MakeTimeAccessor (&VoipServer::m_interval),
                   MakeTimeChecker ())
    ;
  return tid;
}

VoipServer::VoipServer ()
  : m_pktSent (0),
    m_clientApp (0),
    m_txSocket (0),
    m_rxSocket (0),
    m_connected (false)
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_qosStats = Create<QosStatsCalculator> ();
}

VoipServer::~VoipServer ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipServer::SetClientAddress (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_clientAddress = ip;
  m_clientPort = port;
}

void
VoipServer::SetClientApp (Ptr<VoipClient> client)
{
  NS_LOG_FUNCTION (this << client);
  m_clientApp = client;
}

Ptr<VoipClient> 
VoipServer::GetClientApp ()
{
  return m_clientApp;
}

void 
VoipServer::ResetQosStats ()
{
  m_pktSent = 0;
  m_qosStats->ResetCounters ();
}

void 
VoipServer::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = Simulator::Schedule (m_interval, &VoipServer::SendPacket, this);
}

void 
VoipServer::StopSending ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();
}

Ptr<const QosStatsCalculator>
VoipServer::GetQosStats (void) const
{
  return m_qosStats;
}

void
VoipServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_rxSocket != 0)
    {
      m_rxSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  m_clientApp = 0;
  m_txSocket = 0;
  m_rxSocket = 0;
  m_qosStats = 0;
  Application::DoDispose ();
}

void
VoipServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  
  // Inbound side
  if (m_rxSocket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_rxSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_rxSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_rxSocket->SetRecvCallback (MakeCallback (&VoipServer::ReadPacket, this));
    }

  // Outbound side
  if (m_txSocket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_txSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_txSocket->Bind ();
      m_txSocket->Connect (InetSocketAddress (m_clientAddress, m_clientPort));
      m_txSocket->ShutdownRecv ();
      m_txSocket->SetConnectCallback (
          MakeCallback (&VoipServer::ConnectionSucceeded, this),
          MakeCallback (&VoipServer::ConnectionFailed, this));
      m_txSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  ResetQosStats ();
  CancelEvents ();
}

void
VoipServer::StopApplication ()
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
VoipServer::CancelEvents ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void 
VoipServer::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void 
VoipServer::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void
VoipServer::SendPacket ()
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
                   " bytes to " << m_clientAddress << 
                   ":" << m_clientPort << 
                   " Uid " << p->GetUid () << 
                   " Time " << (Simulator::Now ()).GetSeconds ());
    }
  else
    {
      NS_LOG_INFO ("Error sending VoIP " << m_pktSize << 
                   " bytes to " << m_clientAddress);
    }
  m_sendEvent = Simulator::Schedule (m_interval, &VoipServer::SendPacket, this);
}

void
VoipServer::ReadPacket (Ptr<Socket> socket)
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
