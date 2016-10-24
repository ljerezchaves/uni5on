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
    .SetParent<SdmnApplication> ()
    .AddConstructor<StoredVideoClient> ()
    .AddAttribute ("ServerAddress",
                   "The server IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (
                     &StoredVideoClient::m_serverAddress),
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
  : m_socket (0),
    m_serverPort (0),
    m_serverApp (0),
    m_forceStop (EventId ()),
    m_pendingBytes (0),
    m_rxPacket (0)
{
  NS_LOG_FUNCTION (this);
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
  m_active = true;
  m_appStartTrace (this);

  if (!m_maxDurationTime.IsZero ())
    {
      m_forceStop =
        Simulator::Schedule (m_maxDurationTime,
                             &StoredVideoClient::CloseSocket, this);
    }
  OpenSocket ();
}

void
StoredVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_rxPacket = 0;
  Simulator::Cancel (m_forceStop);
  SdmnApplication::DoDispose ();
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

  // Preparing internal variables for new traffic cycle
  m_pendingBytes = 0;
  m_rxPacket = 0;

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

  Simulator::Cancel (m_forceStop);
  if (m_socket != 0)
    {
      NS_LOG_LOGIC ("Closing the TCP connection.");
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr< Socket > > ());
      m_socket = 0;
    }

  // Fire stop trace source
  m_active = false;
  m_appStopTrace (this);
}

void
StoredVideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_LOGIC ("Server accepted connection request!");
  socket->SetRecvCallback (
    MakeCallback (&StoredVideoClient::HandleReceive, this));

  // Request the video object
  SendRequest (socket, "main/video");
}

void
StoredVideoClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Server did not accepted connection request!");
}

void
StoredVideoClient::SendRequest (Ptr<Socket> socket, std::string url)
{
  NS_LOG_FUNCTION (this);

  // Setting request message
  HttpHeader httpHeader;
  httpHeader.SetVersion ("HTTP/1.1");
  httpHeader.SetRequest ();
  httpHeader.SetRequestMethod ("GET");
  httpHeader.SetRequestUrl (url);
  NS_LOG_INFO ("Request for " << url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);
  socket->Send (packet);
}

void
StoredVideoClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (m_active, "Invalid active state");

  do
    {
      // Get (more) data from socket, if available
      if (!m_rxPacket || m_rxPacket->GetSize () == 0)
        {
          m_rxPacket = socket->Recv ();
          m_qosStats->NotifyReceived (0, Simulator::Now (),
                                      m_rxPacket->GetSize ());
        }
      else if (socket->GetRxAvailable ())
        {
          Ptr<Packet> pktTemp = socket->Recv ();
          m_rxPacket->AddAtEnd (pktTemp);
          m_qosStats->NotifyReceived (0, Simulator::Now (),
                                      pktTemp->GetSize ());
        }

      if (!m_pendingBytes)
        {
          // No pending bytes. This is the start of a new HTTP message.
          HttpHeader httpHeader;
          m_rxPacket->RemoveHeader (httpHeader);
          NS_ASSERT_MSG (httpHeader.GetResponseStatusCode () == "200",
                         "Invalid HTTP response message.");

          // Get the content length for this message
          m_pendingBytes =
            std::atoi (httpHeader.GetHeaderField ("ContentLength").c_str ());
        }

      // Let's consume received data
      uint32_t consume = std::min (m_rxPacket->GetSize (), m_pendingBytes);
      m_rxPacket->RemoveAtStart (consume);
      m_pendingBytes -= consume;
      NS_LOG_DEBUG ("Stored video RX " << consume << " bytes");

      if (!m_pendingBytes)
        {
          // This is the end of the HTTP message.
          NS_LOG_INFO ("Stored video successfully received.");
          NS_ASSERT (m_rxPacket->GetSize () == 0);
          CloseSocket ();
          break;
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || m_rxPacket->GetSize ());
}

} // Namespace ns3