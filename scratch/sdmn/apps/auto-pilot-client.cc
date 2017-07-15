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
  { std::clog << "[Pilot client teid " << GetTeid () << "] "; }

#include <ns3/seq-ts-header.h>
#include "auto-pilot-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AutoPilotClient");
NS_OBJECT_ENSURE_REGISTERED (AutoPilotClient);

TypeId
AutoPilotClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AutoPilotClient")
    .SetParent<SdmnClientApp> ()
    .AddConstructor<AutoPilotClient> ()
    .AddAttribute ("PayloadSize",
                   "The payload size of packets (in bytes).",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&AutoPilotClient::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1024, 1024))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.025|Max=0.1]"),
                   MakePointerAccessor (&AutoPilotClient::m_intervalRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("TravelDuration",
                   "A random variable used to pick the travel duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&AutoPilotClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

AutoPilotClient::AutoPilotClient ()
  : m_sendEvent (EventId ()),
    m_stopEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

AutoPilotClient::~AutoPilotClient ()
{
  NS_LOG_FUNCTION (this);
}

void
AutoPilotClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Schedule the ForceStop method to stop traffic generation on both sides
  // based on call length.
  Time callLength = Seconds (std::abs (m_lengthRng->GetValue ()));
  m_stopEvent =
    Simulator::Schedule (callLength, &AutoPilotClient::ForceStop, this);
  NS_LOG_INFO ("Set call length to " << callLength.GetSeconds () << "s.");

  // Chain up to reset statistics, notify server, and fire start trace source.
  SdmnClientApp::Start ();

  // Start traffic.
  Time next = Seconds (m_intervalRng->GetValue ());
  m_sendEvent.Cancel ();
  m_sendEvent = Simulator::Schedule (next, &AutoPilotClient::SendPacket, this);
}

void
AutoPilotClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_lengthRng = 0;
  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();
  SdmnClientApp::DoDispose ();
}

void
AutoPilotClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel (possible) pending stop event and stop the traffic.
  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();

  // Chain up to notify server.
  SdmnClientApp::ForceStop ();

  // Notify the stopped application one second later.
  Simulator::Schedule (Seconds (1), &AutoPilotClient::NotifyStop, this, false);
}

void
AutoPilotClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
  m_socket->SetRecvCallback (MakeCallback (&AutoPilotClient::ReadPacket, this));
}

void
AutoPilotClient::StopApplication ()
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
AutoPilotClient::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = Create<Packet> (m_pktSize);

  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  int bytes = m_socket->Send (packet);
  if (bytes == (int)packet->GetSize ())
    {
      NS_LOG_DEBUG ("Client TX " << bytes << " bytes with " <<
                    "sequence number " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("Client TX error.");
    }

  // Schedule next packet transmission.
  Time next = Seconds (m_intervalRng->GetValue ());
  m_sendEvent = Simulator::Schedule (next, &AutoPilotClient::SendPacket, this);
}

void
AutoPilotClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket.
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  NotifyRx (packet->GetSize (), seqTs.GetTs ());
  NS_LOG_DEBUG ("Client RX " << packet->GetSize () << " bytes with " <<
                "sequence number " << seqTs.GetSeq ());
}

} // Namespace ns3
