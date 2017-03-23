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

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[Voip server - teid " << GetTeid () << "] "; }

#include <ns3/seq-ts-header.h>
#include "voip-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipServer");
NS_OBJECT_ENSURE_REGISTERED (VoipServer);

TypeId
VoipServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipServer")
    .SetParent<SdmnServerApp> ()
    .AddConstructor<VoipServer> ()
    .AddAttribute ("PayloadSize",
                   "The payload size of packets (in bytes).",
                   UintegerValue (20),
                   MakeUintegerAccessor (&VoipServer::m_pktSize),
                   MakeUintegerChecker<uint32_t> (10, 60))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (0.02)),
                   MakeTimeAccessor (&VoipServer::m_interval),
                   MakeTimeChecker ())
  ;
  return tid;
}

VoipServer::VoipServer ()
  : m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

VoipServer::~VoipServer ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
  SdmnServerApp::DoDispose ();
}

void
VoipServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), udpFactory);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Connect (InetSocketAddress (m_clientAddress, m_clientPort));
      m_socket->SetRecvCallback (MakeCallback (&VoipServer::ReadPacket, this));
    }
}

void
VoipServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      m_socket->ShutdownSend ();
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;

    }
}

void
VoipServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);

  // Chain up
  SdmnServerApp::NotifyStart ();

  // Start traffic
  Simulator::Cancel (m_sendEvent);
  m_sendEvent = Simulator::Schedule (
      m_interval, &VoipServer::SendPacket, this);
}

void
VoipServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Chain up
  SdmnServerApp::NotifyForceStop ();

  // Stop traffic
  Simulator::Cancel (m_sendEvent);
}

void
VoipServer::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  // Create the packet and add seq header
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  // Send the packet
  int bytes = m_socket->Send (packet);
  if (bytes == (int)packet->GetSize ())
    {
      NS_LOG_DEBUG ("VoIP TX " << bytes << " bytes " <<
                    "Sequence " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("VoIP TX error.");
    }

  // Schedule next packet transmission
  m_sendEvent = Simulator::Schedule (
      m_interval, &VoipServer::SendPacket, this);
}

void
VoipServer::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  NotifyRx (packet->GetSize (), seqTs.GetTs ());
  NS_LOG_DEBUG ("VoIP RX " << packet->GetSize () << " bytes. " <<
                "Sequence " << seqTs.GetSeq ());
}

} // Namespace ns3
