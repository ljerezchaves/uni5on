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
#include "uni5on-udp-client.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[" << GetAppName ()                       \
            << " client teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Uni5onUdpClient");
NS_OBJECT_ENSURE_REGISTERED (Uni5onUdpClient);

TypeId
Uni5onUdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Uni5onUdpClient")
    .SetParent<Uni5onClient> ()
    .AddConstructor<Uni5onUdpClient> ()

    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("PktInterval",
                   "A random variable used to pick the packet "
                   "inter-arrival time [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&Uni5onUdpClient::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize",
                   "A random variable used to pick the packet size [bytes].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=100]"),
                   MakePointerAccessor (&Uni5onUdpClient::m_pktSizeRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

Uni5onUdpClient::Uni5onUdpClient ()
  : m_sendEvent (EventId ()),
  m_stopEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

Uni5onUdpClient::~Uni5onUdpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
Uni5onUdpClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Schedule the ForceStop method to stop traffic based on traffic length.
  Time stop = GetTrafficLength ();
  m_stopEvent = Simulator::Schedule (stop, &Uni5onUdpClient::ForceStop, this);
  NS_LOG_INFO ("Set traffic length to " << stop.GetSeconds () << "s.");

  // Chain up to reset statistics, notify server, and fire start trace source.
  Uni5onClient::Start ();

  // Start traffic.
  m_sendEvent.Cancel ();
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &Uni5onUdpClient::SendPacket,
                                     this, newSize);
}

void
Uni5onUdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();
  Uni5onClient::DoDispose ();
}

void
Uni5onUdpClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel (possible) pending stop event and stop the traffic.
  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();

  // Chain up to notify server.
  Uni5onClient::ForceStop ();

  // Notify the stopped application one second later.
  Simulator::Schedule (Seconds (1), &Uni5onUdpClient::NotifyStop, this, false);
}

void
Uni5onUdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_serverAddress));
  m_socket->SetRecvCallback (
    MakeCallback (&Uni5onUdpClient::ReadPacket, this));
}

void
Uni5onUdpClient::StopApplication ()
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
Uni5onUdpClient::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);
  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("Client TX packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("Client TX error.");
    }

  // Schedule next packet transmission.
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &Uni5onUdpClient::SendPacket,
                                     this, newSize);
}

void
Uni5onUdpClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  NotifyRx (packet->GetSize ());
  NS_LOG_DEBUG ("Client RX packet with " << packet->GetSize () << " bytes.");
}

} // Namespace ns3
