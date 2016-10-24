/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2015 University of Campinas (Unicamp)
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
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "http-server.h"

NS_LOG_COMPONENT_DEFINE ("HttpServer");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HttpServer);

TypeId
HttpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpServer")
    .SetParent<Application> ()
    .AddConstructor<HttpServer> ()
    .AddAttribute ("LocalPort",
                   "Local TCP port listening for incoming connections.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

HttpServer::HttpServer ()
  : m_socket (0),
    m_port (0),
    m_connected (false),
    m_clientApp (0),
    m_pendingBytes (0),
    m_rxPacket (0)
{
  NS_LOG_FUNCTION (this);

  // Random variable parameters was taken from paper 'An HTTP Web Traffic Model
  // Based on the Top One Million Visited Web Pages' by Rastin Pries et. al
  // (Table II).
  m_mainObjectSizeStream = CreateObject<WeibullRandomVariable> ();
  m_mainObjectSizeStream->SetAttribute ("Scale", DoubleValue (19104.9));
  m_mainObjectSizeStream->SetAttribute ("Shape", DoubleValue (0.771807));

  m_numOfInlineObjStream = CreateObject<ExponentialRandomVariable> ();
  m_numOfInlineObjStream->SetAttribute ("Mean", DoubleValue (31.9291));

  m_inlineObjectSizeStream = CreateObject<LogNormalRandomVariable> ();
  m_inlineObjectSizeStream->SetAttribute ("Mu", DoubleValue (8.91365));
  m_inlineObjectSizeStream->SetAttribute ("Sigma", DoubleValue (1.24816));
}

HttpServer::~HttpServer ()
{
  NS_LOG_FUNCTION (this);
}

void
HttpServer::SetClient (Ptr<HttpClient> client)
{
  m_clientApp = client;
}

Ptr<HttpClient>
HttpServer::GetClientApp ()
{
  return m_clientApp;
}

void
HttpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_clientApp = 0;
  m_rxPacket = 0;
  m_mainObjectSizeStream = 0;
  m_numOfInlineObjStream = 0;
  m_inlineObjectSizeStream = 0;
  Application::DoDispose ();
}

void
HttpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local =
        InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      m_socket->Listen ();
      m_socket->SetAcceptCallback (
        MakeCallback (&HttpServer::HandleRequest, this),
        MakeCallback (&HttpServer::HandleAccept, this));
      m_socket->SetCloseCallbacks (
        MakeCallback (&HttpServer::HandlePeerClose, this),
        MakeCallback (&HttpServer::HandlePeerError, this));
    }
}

void
HttpServer::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetAcceptCallback (
        MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
        MakeNullCallback<void, Ptr< Socket >, const Address &> ());
      m_socket->SetSendCallback (
        MakeNullCallback<void, Ptr< Socket >, uint32_t> ());
      m_socket->SetRecvCallback (
        MakeNullCallback<void, Ptr< Socket > > ());
      m_socket = 0;
    }
}

void
HttpServer::ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (header.IsRequest (), "Invalid HTTP request.");

  std::string url = header.GetRequestUrl ();
  NS_LOG_INFO ("Client requesting a " + url);
  if (url == "main/object")
    {
      // Setting random parameter values.
      m_pendingBytes = m_mainObjectSizeStream->GetInteger ();
      uint32_t numOfInlineObj = m_numOfInlineObjStream->GetInteger ();
      NS_LOG_INFO ("HTTP main object size (bytes): " << m_pendingBytes);
      NS_LOG_INFO ("Inline objects: " << numOfInlineObj);

      // Setting the HTTP response message.
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", m_pendingBytes);
      httpHeaderOut.SetHeaderField ("ContentType", "main/object");
      httpHeaderOut.SetHeaderField ("InlineObjects", numOfInlineObj);
      Ptr<Packet> outPacket = Create<Packet> (0);
      outPacket->AddHeader (httpHeaderOut);
      socket->Send (outPacket);

      // Start sending the HTTP object.
      SendObject (socket, socket->GetTxAvailable ());
    }
  else if (url == "inline/object")
    {
      // Setting random parameter values.
      m_pendingBytes = m_inlineObjectSizeStream->GetInteger ();
      NS_LOG_INFO ("HTTP inline object size (bytes): " << m_pendingBytes);

      // Setting the HTTP response message.
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", m_pendingBytes);
      httpHeaderOut.SetHeaderField ("ContentType", "inline/object");
      httpHeaderOut.SetHeaderField ("InlineObjects", 0);
      Ptr<Packet> outPacket = Create<Packet> (0);
      outPacket->AddHeader (httpHeaderOut);
      socket->Send (outPacket);

      // Start sending the HTTP object.
      SendObject (socket, socket->GetTxAvailable ());
    }
  else
    {
      NS_FATAL_ERROR ("Invalid request.");
    }
}

bool
HttpServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Received request for connection from " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  return !m_connected;
}

void
HttpServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Connection successfully established with client " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  socket->SetSendCallback (MakeCallback (&HttpServer::SendObject, this));
  socket->SetRecvCallback (MakeCallback (&HttpServer::HandleReceive, this));
  m_connected = true;
  m_pendingBytes = 0;
  m_rxPacket = 0;
}

void
HttpServer::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // This application expects to receive a single HTTP GET message at a time.
  HttpHeader httpHeader;
  m_rxPacket = socket->Recv ();
  m_rxPacket->RemoveHeader (httpHeader);
  NS_ASSERT (m_rxPacket->GetSize () == 0);

  ProccessHttpRequest (socket, httpHeader);
}

void
HttpServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection closed.");
  socket->ShutdownSend ();
  m_connected = false;
}

void
HttpServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection error.");
  socket->ShutdownSend ();
  m_connected = false;
}

void
HttpServer::SendObject (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this);

  if (!available || !m_connected || !m_pendingBytes)
    {
      return;
    }

  uint32_t pktSize = std::min (available, m_pendingBytes);
  Ptr<Packet> packet = Create<Packet> (pktSize);
  int bytesSent = socket->Send (packet);

  NS_LOG_DEBUG ("HTTP server TX " << bytesSent << " bytes");
  if (bytesSent > 0)
    {
      m_pendingBytes -= static_cast<uint32_t> (bytesSent);
    }
}

} // Namespace ns3