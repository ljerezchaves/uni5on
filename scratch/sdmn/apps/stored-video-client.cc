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
    .SetParent<SdmnClientApp> ()
    .AddConstructor<StoredVideoClient> ()
  ;
  return tid;
}

StoredVideoClient::StoredVideoClient ()
  : m_rxPacket (0),
    m_httpPacketSize (0),
    m_pendingBytes (0)
{
  NS_LOG_FUNCTION (this);
}

StoredVideoClient::~StoredVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
StoredVideoClient::Start (void)
{
  NS_LOG_FUNCTION (this);

  // Chain up to fire start trace
  SdmnClientApp::Start ();

  // Preparing internal variables for new traffic cycle
  m_httpPacketSize = 0;
  m_pendingBytes = 0;
  m_rxPacket = 0;

  // Open the TCP connection
  if (!m_socket)
    {
      NS_LOG_INFO ("Opening the TCP connection.");
      TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_socket->SetConnectCallback (
        MakeCallback (&StoredVideoClient::ConnectionSucceeded, this),
        MakeCallback (&StoredVideoClient::ConnectionFailed, this));
    }
}

void
StoredVideoClient::Stop ()
{
  NS_LOG_FUNCTION (this);

  // Close the TCP socket
  if (m_socket != 0)
    {
      NS_LOG_INFO ("Closing the TCP connection.");
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  // Chain up to fire stop trace
  SdmnClientApp::Stop ();
}

void
StoredVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_rxPacket = 0;
  SdmnClientApp::DoDispose ();
}

void
StoredVideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("Server accepted connection request!");
  socket->SetRecvCallback (
    MakeCallback (&StoredVideoClient::ReceiveData, this));

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
StoredVideoClient::ReceiveData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  do
    {
      // Get (more) data from socket, if available.
      if (!m_rxPacket || m_rxPacket->GetSize () == 0)
        {
          m_rxPacket = socket->Recv ();
        }
      else if (socket->GetRxAvailable ())
        {
          Ptr<Packet> pktTemp = socket->Recv ();
          m_rxPacket->AddAtEnd (pktTemp);
        }

      if (!m_pendingBytes)
        {
          // No pending bytes. This is the start of a new HTTP message.
          HttpHeader httpHeader;
          m_rxPacket->RemoveHeader (httpHeader);
          NS_ASSERT_MSG (httpHeader.GetResponseStatusCode () == "200",
                         "Invalid HTTP response message.");
          m_httpPacketSize = httpHeader.GetSerializedSize ();

          // Get the content length for this message
          m_pendingBytes = std::atoi (
              httpHeader.GetHeaderField ("ContentLength").c_str ());
          m_httpPacketSize += m_pendingBytes;
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
          NotifyRx (m_httpPacketSize);

          Stop ();
          break;
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || m_rxPacket->GetSize ());
}

void
StoredVideoClient::SendRequest (Ptr<Socket> socket, std::string url)
{
  NS_LOG_FUNCTION (this);

  // When the force stop flag is active, don't send new requests.
  if (IsForceStop ())
    {
      NS_LOG_WARN ("Can't send video request on force stop mode.");
      return;
    }

  // Setting request message
  HttpHeader httpHeader;
  httpHeader.SetRequest ();
  httpHeader.SetVersion ("HTTP/1.1");
  httpHeader.SetRequestMethod ("GET");
  httpHeader.SetRequestUrl (url);
  NS_LOG_INFO ("Request for " << url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);

  NotifyTx (packet->GetSize ());
  int bytes = socket->Send (packet);
  if (bytes != (int)packet->GetSize ())
    {
      NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
    }
}

} // Namespace ns3
