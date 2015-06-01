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
    .SetParent<EpcApplication> ()
    .AddConstructor<RealTimeVideoClient> ()
    .AddAttribute ("LocalPort",
                   "Local TCP port on which we listen for incoming connections.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&RealTimeVideoClient::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

RealTimeVideoClient::RealTimeVideoClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
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
RealTimeVideoClient::NofifyTrafficEnd (uint32_t pkts)
{
  NS_LOG_FUNCTION (this);

  if (!m_stopCb.IsNull ())
    {
      // Let's wait 1 sec for delayed packets before notifying stopped app.
      Simulator::Schedule (Seconds (1), &RealTimeVideoClient::m_stopCb, this, this);
    }
}

void 
RealTimeVideoClient::Start (void)
{
  NS_LOG_FUNCTION (this);

  ResetQosStats ();
  m_serverApp->StartSending ();
}

void
RealTimeVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  EpcApplication::DoDispose ();
}

void
RealTimeVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind (
        InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
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
      m_socket->Close ();
      m_socket = 0;
    }
}

void
RealTimeVideoClient::ReadPacket (Ptr<Socket> socket)
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
          NS_LOG_DEBUG ("Real-time video RX " << packet->GetSize () << " bytes");
          m_qosStats->NotifyReceived (seqTs.GetSeq (), seqTs.GetTs (), packet->GetSize ());
        }
    }
}

} // Namespace ns3
