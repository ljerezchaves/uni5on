/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <ns3/seq-ts-header.h>
#include "uni5on-udp-server.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " server teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Uni5onUdpServer");
NS_OBJECT_ENSURE_REGISTERED (Uni5onUdpServer);

TypeId
Uni5onUdpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Uni5onUdpServer")
    .SetParent<Uni5onServer> ()
    .AddConstructor<Uni5onUdpServer> ()

    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("PktInterval",
                   "A random variable used to pick the packet "
                   "inter-arrival time [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&Uni5onUdpServer::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize",
                   "A random variable used to pick the packet size [bytes].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=100]"),
                   MakePointerAccessor (&Uni5onUdpServer::m_pktSizeRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

Uni5onUdpServer::Uni5onUdpServer ()
  : m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

Uni5onUdpServer::~Uni5onUdpServer ()
{
  NS_LOG_FUNCTION (this);
}

void
Uni5onUdpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_sendEvent.Cancel ();
  Uni5onServer::DoDispose ();
}

void
Uni5onUdpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_clientAddress));
  m_socket->SetRecvCallback (
    MakeCallback (&Uni5onUdpServer::ReadPacket, this));
}

void
Uni5onUdpServer::StopApplication ()
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
Uni5onUdpServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);

  // Chain up to reset statistics.
  Uni5onServer::NotifyStart ();

  // Start traffic.
  m_sendEvent.Cancel ();
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &Uni5onUdpServer::SendPacket,
                                     this, newSize);
}

void
Uni5onUdpServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Chain up just for log.
  Uni5onServer::NotifyForceStop ();

  // Stop traffic.
  m_sendEvent.Cancel ();
}

void
Uni5onUdpServer::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);
  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("Server TX packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("Server TX error.");
    }

  // Schedule next packet transmission.
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &Uni5onUdpServer::SendPacket,
                                     this, newSize);
}

void
Uni5onUdpServer::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  NotifyRx (packet->GetSize ());
  NS_LOG_DEBUG ("Server RX packet with " << packet->GetSize () << " bytes.");
}

} // Namespace ns3
