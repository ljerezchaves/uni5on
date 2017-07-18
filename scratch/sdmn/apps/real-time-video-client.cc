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

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[LiveVid client teid " << GetTeid () << "] "; }

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
    //
    // For traffic length, we are considering a statistic that the majority of
    // YouTube brand videos are somewhere between 31 and 120 seconds long. So
    // we are using the average length of 1min 30sec, with 15sec stdev. See
    // http://tinyurl.com/q5xkwnn and http://tinyurl.com/klraxum for more
    // information on this topic.
    //
    .AddAttribute ("TrafficLength",
                   "A random variable used to pick the traffic length [s].",
                   StringValue (
                     "ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"),
                   MakePointerAccessor (&RealTimeVideoClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

RealTimeVideoClient::RealTimeVideoClient ()
  : m_stopEvent (EventId ())
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

  // Schedule the ForceStop method to stop traffic generation on server side
  // based on video length.
  Time sTime = Seconds (std::abs (m_lengthRng->GetValue ()));
  m_stopEvent = Simulator::Schedule (
      sTime, &RealTimeVideoClient::ForceStop, this);
  NS_LOG_INFO ("Set traffic length to " << sTime.GetSeconds () << "s.");

  // Chain up to reset statistics, notify server, and fire start trace source.
  SdmnClientApp::Start ();
}

void
RealTimeVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_lengthRng = 0;
  m_stopEvent.Cancel ();
  SdmnClientApp::DoDispose ();
}

void
RealTimeVideoClient::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  SetAttribute ("AppName", StringValue ("LiveVid"));
}

void
RealTimeVideoClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel (possible) pending stop event.
  m_stopEvent.Cancel ();

  // Chain up to notify server.
  SdmnClientApp::ForceStop ();

  // Notify the stopped application one second later.
  Simulator::Schedule (
    Seconds (1), &RealTimeVideoClient::NotifyStop, this, false);
}

void
RealTimeVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->ShutdownSend ();
  m_socket->SetRecvCallback (
    MakeCallback (&RealTimeVideoClient::ReadPacket, this));
}

void
RealTimeVideoClient::StopApplication ()
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
RealTimeVideoClient::ReadPacket (Ptr<Socket> socket)
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
