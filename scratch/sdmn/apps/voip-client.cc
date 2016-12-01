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

#include <ns3/seq-ts-header.h>
#include "voip-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipClient");
NS_OBJECT_ENSURE_REGISTERED (VoipClient);

TypeId
VoipClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipClient")
    .SetParent<SdmnClientApp> ()
    .AddConstructor<VoipClient> ()
    .AddAttribute ("PayloadSize",
                   "The payload size of packets (in bytes).",
                   UintegerValue (20),
                   MakeUintegerAccessor (&VoipClient::m_pktSize),
                   MakeUintegerChecker<uint32_t> (10, 60))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (0.02)),
                   MakeTimeAccessor (&VoipClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("CallDuration",
                   "A random variable used to pick the call duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&VoipClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

VoipClient::VoipClient ()
  : m_sendEvent (EventId ()),
    m_stopEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

VoipClient::~VoipClient ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Schedule the stop event based on call lenght. It will schedule the
  // ForceStop method to stop traffic generation before firing the stop trace.
  Time stopTime = Seconds (std::abs (m_lengthRng->GetValue ()));
  m_stopEvent = Simulator::Schedule (stopTime, &VoipClient::ForceStop, this);
  NS_LOG_INFO ("VoIP call lenght: " << stopTime.As (Time::S));

  // Chain up to fire start trace
  SdmnClientApp::Start ();

  // Start traffic
  m_pktSent = 0;
  Simulator::Cancel (m_sendEvent);
  m_sendEvent = Simulator::Schedule (
      m_interval, &VoipClient::SendPacket, this);
  NS_LOG_INFO ("VoIP client started.");
}

void
VoipClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_lengthRng = 0;
  Simulator::Cancel (m_stopEvent);
  Simulator::Cancel (m_sendEvent);
  SdmnClientApp::DoDispose ();
}

void
VoipClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), udpFactory);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_socket->SetRecvCallback (MakeCallback (&VoipClient::ReadPacket, this));
    }
}

void
VoipClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->ShutdownSend ();
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
VoipClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel events and stop traffic
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_stopEvent);

  // Chain up
  SdmnClientApp::ForceStop ();
  NS_LOG_INFO ("VoIP client stopped.");
}

void
VoipClient::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  // Create the packet and add seq header
  Ptr<Packet> packet = Create<Packet> (m_pktSize);
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_pktSent++);
  packet->AddHeader (seqTs);

  // Send the packet
  int bytesSent = m_socket->Send (packet);
  if (bytesSent > 0)
    {
      NS_LOG_DEBUG ("VoIP TX " << packet->GetSize () << " bytes. " <<
                    "Sequence " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("VoIP TX error.");
    }

  // Schedule next packet transmission
  m_sendEvent = Simulator::Schedule (
      m_interval, &VoipClient::SendPacket, this);
}

void
VoipClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  m_qosStats->NotifyReceived (seqTs.GetSeq (), seqTs.GetTs (),
                              packet->GetSize ());
  NS_LOG_DEBUG ("VoIP RX " << packet->GetSize () << " bytes. " <<
                "Sequence " << seqTs.GetSeq ());
}

} // Namespace ns3
