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

#include "stored-video-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StoredVideoClient");
NS_OBJECT_ENSURE_REGISTERED (StoredVideoClient);

TypeId
StoredVideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StoredVideoClient")
    .SetParent<Application> ()
    .AddConstructor<StoredVideoClient> ()
    .AddAttribute ("ServerAddress",
                   "The server IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&StoredVideoClient::m_serverAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ServerPort",
                   "The server TCP port.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&StoredVideoClient::m_serverPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

StoredVideoClient::StoredVideoClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_qosStats = Create<QosStatsCalculator> ();
}

StoredVideoClient::~StoredVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
StoredVideoClient::SetServerApp (Ptr<StoredVideoServer> server,
                                 Ipv4Address serverAddress,
                                 uint16_t serverPort)
{
  m_serverApp = server;
  m_serverAddress = serverAddress;
  m_serverPort = serverPort;
}

Ptr<StoredVideoServer>
StoredVideoClient::GetServerApp ()
{
  return m_serverApp;
}

void
StoredVideoClient::ResetQosStats ()
{
  m_qosStats->ResetCounters ();
}

Ptr<const QosStatsCalculator>
StoredVideoClient::GetQosStats (void) const
{
  return m_qosStats;
}

void
StoredVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  m_qosStats = 0;
  Application::DoDispose ();
}

void
StoredVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  ResetQosStats ();
  OpenSocket ();
}

void
StoredVideoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CloseSocket ();
}

void
StoredVideoClient::OpenSocket ()
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      NS_LOG_LOGIC ("Opening the TCP connection.");
      TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_socket->SetConnectCallback (
        MakeCallback (&StoredVideoClient::ConnectionSucceeded, this),
        MakeCallback (&StoredVideoClient::ConnectionFailed, this));
    }
}

void
StoredVideoClient::CloseSocket ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      NS_LOG_LOGIC ("Closing the TCP connection.");
      m_socket->Close ();
      m_socket = 0;
    }
}

void
StoredVideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_DEBUG ("Server accepted connection request!");
  socket->SetRecvCallback (MakeCallback (&StoredVideoClient::HandleReceive, this));

  SendRequest (socket, "main/video");
}

void
StoredVideoClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_ERROR ("Server did not accepted connection request!");
}

void
StoredVideoClient::SendRequest (Ptr<Socket> socket, std::string url)
{
  NS_LOG_FUNCTION (this);

  // Setting request message
  HttpHeader httpHeader;
  httpHeader.SetRequest (true);
  httpHeader.SetMethod ("GET");
  httpHeader.SetUrl (url);
  httpHeader.SetVersion ("HTTP/1.1");

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);
  NS_LOG_INFO ("Request for " << url);
  socket->Send (packet);

  m_bytesReceived = 0;
}

void
StoredVideoClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  uint32_t bytesReceived = packet->GetSize ();
  m_qosStats->NotifyReceived (0, Simulator::Now (), bytesReceived);
  NS_LOG_DEBUG (bytesReceived << " bytes received from server.");

  HttpHeader httpHeaderIn;
  packet->PeekHeader (httpHeaderIn);
  std::string statusCode = httpHeaderIn.GetStatusCode ();

  if (statusCode == "200")
    {
      m_contentType = httpHeaderIn.GetHeaderField ("ContentType");
      m_contentLength = atoi (httpHeaderIn.GetHeaderField ("ContentLength").c_str ());
      m_bytesReceived = bytesReceived - httpHeaderIn.GetSerializedSize ();
      NS_LOG_DEBUG ("Video size is " << m_contentLength << " bytes.");
    }
  else
    {
      m_bytesReceived += bytesReceived;
    }

  if (m_bytesReceived == m_contentLength)
    {
      NS_LOG_INFO ("main/video successfully received.");
      NS_LOG_DEBUG (socket->GetRxAvailable () << "bytes available.");
      CloseSocket ();
    }
}

} // Namespace ns3
