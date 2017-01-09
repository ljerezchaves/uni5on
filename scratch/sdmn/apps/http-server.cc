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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "http-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HttpServer");
NS_OBJECT_ENSURE_REGISTERED (HttpServer);

TypeId
HttpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpServer")
    .SetParent<SdmnServerApp> ()
    .AddConstructor<HttpServer> ()
  ;
  return tid;
}

HttpServer::HttpServer ()
  : m_connected (false),
    m_pendingBytes (0)
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
HttpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_mainObjectSizeStream = 0;
  m_numOfInlineObjStream = 0;
  m_inlineObjectSizeStream = 0;
  SdmnServerApp::DoDispose ();
}

void
HttpServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->SetAttribute ("SndBufSize", UintegerValue (16384));
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
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
HttpServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetAcceptCallback (
        MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
        MakeNullCallback<void, Ptr<Socket>, const Address &> ());
      m_socket->SetSendCallback (
        MakeNullCallback<void, Ptr<Socket>, uint32_t> ());
      m_socket->SetRecvCallback (
        MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

bool
HttpServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  NS_LOG_INFO ("Connection request from " << ipAddr);
  return !m_connected;
}

void
HttpServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  NS_LOG_INFO ("Connection successfully established with " << ipAddr);
  socket->SetSendCallback (MakeCallback (&HttpServer::SendData, this));
  socket->SetRecvCallback (MakeCallback (&HttpServer::ReceiveData, this));
  m_connected = true;
  m_pendingBytes = 0;
}

void
HttpServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("Connection closed.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  socket->SetSendCallback (MakeNullCallback<void, Ptr<Socket>, uint32_t> ());
  m_connected = false;
}

void
HttpServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_ERROR ("Connection error.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  socket->SetSendCallback (MakeNullCallback<void, Ptr<Socket>, uint32_t> ());
  m_connected = false;
}

void
HttpServer::ReceiveData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the HTTP GET message.
  HttpHeader httpHeader;
  Ptr<Packet> packet = socket->Recv ();
  packet->RemoveHeader (httpHeader);
  NS_ASSERT (packet->GetSize () == 0);

  ProccessHttpRequest (socket, httpHeader);
}

void
HttpServer::SendData (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this << socket << available);

  if (!m_pendingBytes)
    {
      NS_LOG_DEBUG ("No pending data to send.");
      return;
    }

  if (IsForceStop ())
    {
      NS_LOG_DEBUG ("Can't send data on force stop mode.");
      return;
    }

  if (!m_connected)
    {
      NS_LOG_DEBUG ("Socket not connected.");
      return;
    }

  if (!available)
    {
      NS_LOG_DEBUG ("No TX buffer space available.");
      return;
    }

  uint32_t pktSize = std::min (available, m_pendingBytes);
  Ptr<Packet> packet = Create<Packet> (pktSize);
  int bytesSent = socket->Send (packet);
  if (bytesSent > 0)
    {
      NS_LOG_DEBUG ("HTTP server TX " << bytesSent << " bytes.");
      m_pendingBytes -= static_cast<uint32_t> (bytesSent);
    }
  else
    {
      NS_LOG_ERROR ("HTTP server TX error.");
    }
}

void
HttpServer::ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (header.IsRequest (), "Invalid HTTP request.");

  // Check for valid request.
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
      SendData (socket, socket->GetTxAvailable ());
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
      SendData (socket, socket->GetTxAvailable ());
    }
  else
    {
      NS_FATAL_ERROR ("Invalid request.");
    }
}

} // Namespace ns3
