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
  m_forceStop = EventId ();
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
      m_forceStop = Simulator::Schedule (m_maxDurationTime,
                                         &StoredVideoClient::CloseSocket, this);
    }
  OpenSocket ();
}

void
StoredVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  Simulator::Cancel (m_forceStop);
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

  Simulator::Cancel (m_forceStop);
  if (m_socket != 0)
    {
      NS_LOG_LOGIC ("Closing the TCP connection.");
      m_socket->Close ();
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
  socket->SetRecvCallback (MakeCallback (&StoredVideoClient::HandleReceive, this));

  // Request the video
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
  httpHeader.SetVersion ("HTTP/1.1");
  httpHeader.SetRequest ();
  httpHeader.SetRequestMethod ("GET");
  httpHeader.SetRequestUrl (url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);
  socket->Send (packet);
}

void
StoredVideoClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  static uint32_t    pendingBytes = 0;
  static Ptr<Packet> packet;

  do
    {
      // Get more data from socket, if available
      if (!packet || packet->GetSize () == 0)
        {
          packet = socket->Recv ();
          m_qosStats->NotifyReceived (0, Simulator::Now (), packet->GetSize ());
        }
      else if (socket->GetRxAvailable ())
        {
          Ptr<Packet> pktTemp = socket->Recv ();
          packet->AddAtEnd (pktTemp);
          m_qosStats->NotifyReceived (0, Simulator::Now (), pktTemp->GetSize ());
        }

      if (!pendingBytes)
        {
          // No pending bytes. This is the start of a new HTTP message.
          HttpHeader httpHeader;
          packet->RemoveHeader (httpHeader);
          NS_ASSERT_MSG (httpHeader.GetResponseStatusCode () == "200",
                         "Invalid HTTP response message.");

          // Get the content length for this message
          pendingBytes =
            atoi (httpHeader.GetHeaderField ("ContentLength").c_str ());
        }

      // Let's consume received data
      uint32_t consume = std::min (packet->GetSize (), pendingBytes);
      packet->RemoveAtStart (consume);
      pendingBytes -= consume;
      NS_LOG_DEBUG ("Stored video RX " << consume << " bytes");

      if (!pendingBytes)
        {
          // This is the end of the HTTP message.
          NS_LOG_INFO ("Stored video successfully received.");
          CloseSocket ();
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || packet->GetSize ());
}

} // Namespace ns3
