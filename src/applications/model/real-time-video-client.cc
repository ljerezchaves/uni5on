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

#include "real-time-video-client.h"
#include "seq-ts-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RealTimeVideoClient");
NS_OBJECT_ENSURE_REGISTERED (RealTimeVideoClient);

TypeId
RealTimeVideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RealTimeVideoClient")
    .SetParent<Application> ()
    .AddConstructor<RealTimeVideoClient> ()
    .AddAttribute ("LocalPort",
                   "Local TCP port on which we listen for incoming connections.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&RealTimeVideoClient::m_port),
                   MakeUintegerChecker<uint16_t> ())

  ;
  return tid;
}

RealTimeVideoClient::RealTimeVideoClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_qosStats = Create<QosStatsCalculator> ();
}

RealTimeVideoClient::~RealTimeVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
RealTimeVideoClient::SetServer (Ptr<RealTimeVideoServer> server)
{
  m_serverApp = server;
}

Ptr<RealTimeVideoServer>
RealTimeVideoClient::GetServerApp ()
{
  return m_serverApp;
}

void
RealTimeVideoClient::ResetQosStats ()
{
  m_qosStats->ResetCounters ();
}

Ptr<const QosStatsCalculator>
RealTimeVideoClient::GetQosStats (void) const
{
  return m_qosStats;
}

void
RealTimeVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  m_qosStats = 0;
  Application::DoDispose ();
}

void
RealTimeVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),
                                                   m_port);
      m_socket->Bind (local);
    }
  m_socket->SetRecvCallback (MakeCallback (&RealTimeVideoClient::HandleRead, this));
  
  ResetQosStats ();
  m_serverApp->StartSending ();
}

void
RealTimeVideoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket = 0;
    }
}

void
RealTimeVideoClient::HandleRead (Ptr<Socket> socket)
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
          uint32_t currentSequenceNumber = seqTs.GetSeq ();
          if (InetSocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
            }
          else if (Inet6SocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
                           " Sequence Number: " << currentSequenceNumber <<
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
