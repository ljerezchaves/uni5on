/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[Pilot server teid " << GetTeid () << "] "; }

#include <ns3/seq-ts-header.h>
#include "auto-pilot-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AutoPilotServer");
NS_OBJECT_ENSURE_REGISTERED (AutoPilotServer);

TypeId
AutoPilotServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AutoPilotServer")
    .SetParent<SdmnServerApp> ()
    .AddConstructor<AutoPilotServer> ()
    .AddAttribute ("PayloadSize",
                   "The payload size of packets (in bytes).",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&AutoPilotServer::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1024, 1024))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&AutoPilotServer::m_interval),
                   MakeTimeChecker (Seconds (1), Seconds (1)))
  ;
  return tid;
}

AutoPilotServer::AutoPilotServer ()
  : m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

AutoPilotServer::~AutoPilotServer ()
{
  NS_LOG_FUNCTION (this);
}

void
AutoPilotServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_sendEvent.Cancel ();
  SdmnServerApp::DoDispose ();
}

void
AutoPilotServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress (m_clientAddress, m_clientPort));
  m_socket->SetRecvCallback (MakeCallback (&AutoPilotServer::ReadPacket, this));
}

void
AutoPilotServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }
}

void
AutoPilotServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);

  // Chain up to reset statistics.
  SdmnServerApp::NotifyStart ();

  // Start traffic.
  m_sendEvent.Cancel ();
  m_sendEvent = Simulator::Schedule (
      m_interval, &AutoPilotServer::SendPacket, this);
}

void
AutoPilotServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Chain up just for log.
  SdmnServerApp::NotifyForceStop ();

  // Stop streaming.
  m_sendEvent.Cancel ();
}

void
AutoPilotServer::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = Create<Packet> (m_pktSize);

  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  int bytes = m_socket->Send (packet);
  if (bytes == (int)packet->GetSize ())
    {
      NS_LOG_DEBUG ("Server TX " << bytes << " bytes with " <<
                    "sequence number " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("Server TX error.");
    }

  // Schedule next packet transmission.
  m_sendEvent = Simulator::Schedule (
      m_interval, &AutoPilotServer::SendPacket, this);
}

void
AutoPilotServer::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket.
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  NotifyRx (packet->GetSize (), seqTs.GetTs ());
  NS_LOG_DEBUG ("Server RX " << packet->GetSize () << " bytes with " <<
                "sequence number " << seqTs.GetSeq ());
}

} // Namespace ns3
