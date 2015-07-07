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
    .SetParent<EpcApplication> ()
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
}

StoredVideoClient::~StoredVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
StoredVideoClient::SetServer (Ptr<StoredVideoServer> server,
                              Ipv4Address serverAddress, uint16_t serverPort)
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
StoredVideoClient::SetTraceFilename (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);

  NS_ASSERT_MSG (m_serverApp, "No server application found.");
  m_serverApp->SetAttribute ("TraceFilename", StringValue (filename));
}

void
StoredVideoClient::Start (void)
{
  ResetQosStats ();
  m_appStartTrace (this);
  OpenSocket ();
}

std::string 
StoredVideoClient::GetAppName (void) const
{
  return "StVd";
}

void
StoredVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  EpcApplication::DoDispose ();
}

void
StoredVideoClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
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

  // Fire stop trace source
  m_appStopTrace (this);
}

void
StoredVideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_LOGIC ("Server accepted connection request!");
  socket->SetRecvCallback (MakeCallback (&StoredVideoClient::HandleReceive, this));

  // Request the video
  m_bytesReceived = 0;
  NS_LOG_INFO ("Request for main/video");
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
  socket->Send (packet);
}

void
StoredVideoClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  uint32_t bytesReceived = packet->GetSize ();
  m_qosStats->NotifyReceived (0, Simulator::Now (), bytesReceived);
  
  HttpHeader httpHeaderIn;
  packet->PeekHeader (httpHeaderIn);
  std::string statusCode = httpHeaderIn.GetStatusCode ();

  if (statusCode == "200")
    {
      m_contentType = httpHeaderIn.GetHeaderField ("ContentType");
      m_contentLength = atoi (httpHeaderIn.GetHeaderField ("ContentLength").c_str ());
      m_bytesReceived = bytesReceived - httpHeaderIn.GetSerializedSize ();
    }
  else
    {
      m_bytesReceived += bytesReceived;
    }

  if (m_bytesReceived == m_contentLength)
    {
      m_contentLength = 0;
      NS_LOG_INFO ("Stored video successfully received.");
      CloseSocket ();
    }
}

} // Namespace ns3
