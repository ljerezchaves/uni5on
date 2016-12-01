/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include <ns3/seq-ts-header.h>
#include "real-time-video-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RealTimeVideoClient");
NS_OBJECT_ENSURE_REGISTERED (RealTimeVideoClient);

TypeId
RealTimeVideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RealTimeVideoClient")
    .SetParent<SdmnClientApp> ()
    .AddConstructor<RealTimeVideoClient> ()
    .AddAttribute ("VideoDuration",
                   "A random variable used to pick the video duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&RealTimeVideoClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

RealTimeVideoClient::RealTimeVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

RealTimeVideoClient::~RealTimeVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
RealTimeVideoClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Schedule the stop event based on video lenght. It will schedule the
  // ForceStop method to stop traffic generation before firing the stop trace.
  Time stopTime = Seconds (std::abs (m_lengthRng->GetValue ()));
  m_stopEvent = Simulator::Schedule (
      stopTime, &RealTimeVideoClient::ForceStop, this);
  NS_LOG_INFO ("Real-time video lenght: " << stopTime.As (Time::S));

  // Chain up to fire start trace
  SdmnClientApp::Start ();
}

void
RealTimeVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_stopEvent);
  SdmnClientApp::DoDispose ();
}

void
RealTimeVideoClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_stopEvent);
  SdmnClientApp::ForceStop ();
}

void
RealTimeVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->ShutdownSend ();
      m_socket->SetRecvCallback (
        MakeCallback (&RealTimeVideoClient::ReadPacket, this));
    }
}

void
RealTimeVideoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
RealTimeVideoClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  m_qosStats->NotifyReceived (seqTs.GetSeq (), seqTs.GetTs (),
                              packet->GetSize ());
  NS_LOG_DEBUG ("Real-time video RX " << packet->GetSize () << " bytes. " <<
                "Sequence " << seqTs.GetSeq ());
}

} // Namespace ns3
