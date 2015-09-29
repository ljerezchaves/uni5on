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
                   "Local TCP port on which we listen for incoming connections.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

HttpServer::HttpServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_clientApp = 0;
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
  Application::DoDispose ();
}

void HttpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
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

void HttpServer::StopApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket = 0;
    }
}

bool
HttpServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Received request for connection from " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  return true;
}

void
HttpServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Connection successfully established with client " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  socket->SetRecvCallback (MakeCallback (&HttpServer::HandleReceive, this));
}

bool
HttpServer::ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (header.IsRequest (), "Invalid HTTP request.");

  // Get the TCP sending buffer size.
  uint32_t tcpBufSize = socket->GetTxAvailable ();

  std::string url = header.GetRequestUrl ();
  NS_LOG_INFO ("Client requesting a " + url);
  if (url == "main/object")
    {
      // Scale, Shape and Mean data was taken from paper "An HTTP Web Traffic Model Based on the
      // Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
      Ptr<WeibullRandomVariable> mainObjectSizeStream = CreateObject<WeibullRandomVariable> ();
      mainObjectSizeStream->SetAttribute ("Scale", DoubleValue (19104.9));
      mainObjectSizeStream->SetAttribute ("Shape", DoubleValue (0.771807));
      uint32_t mainObjectSize = mainObjectSizeStream->GetInteger ();

      Ptr<ExponentialRandomVariable> numOfInlineObjStream = CreateObject<ExponentialRandomVariable> ();
      numOfInlineObjStream->SetAttribute ("Mean", DoubleValue (31.9291));
      uint32_t numOfInlineObj = numOfInlineObjStream->GetInteger ();

      // Setting response
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", mainObjectSize);
      httpHeaderOut.SetHeaderField ("ContentType", "main/object");
      httpHeaderOut.SetHeaderField ("InlineObjects", numOfInlineObj);

      // Check if the buffer can store this packet size
      uint32_t headerSize = httpHeaderOut.GetSerializedSize ();
      if (mainObjectSize + headerSize > tcpBufSize)
        {
          mainObjectSize = tcpBufSize - headerSize;
          httpHeaderOut.SetHeaderField ("ContentLength", mainObjectSize);
        }
      NS_LOG_INFO ("HTTP main object size: " << mainObjectSize << " bytes. " <<
                   "Inline objects: " << numOfInlineObj);

      Ptr<Packet> packet = Create<Packet> (mainObjectSize);
      packet->AddHeader (httpHeaderOut);
      return socket->Send (packet);
    }
  else
    {
      // Mu and Sigma data was taken from paper "An HTTP Web Traffic Model Based on the
      // Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
      Ptr<LogNormalRandomVariable> logNormal = CreateObject<LogNormalRandomVariable> ();
      logNormal->SetAttribute ("Mu", DoubleValue (8.91365));
      logNormal->SetAttribute ("Sigma", DoubleValue (1.24816));

      uint32_t inlineObjectSize = logNormal->GetInteger ();

      // Setting response
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", inlineObjectSize);
      httpHeaderOut.SetHeaderField ("ContentType", "inline/object");
      httpHeaderOut.SetHeaderField ("InlineObjects", 0);

      // Check if the buffer can store this packet size
      uint32_t headerSize = httpHeaderOut.GetSerializedSize ();
      if (inlineObjectSize + headerSize > tcpBufSize)
        {
          inlineObjectSize = tcpBufSize - headerSize;
          httpHeaderOut.SetHeaderField ("ContentLength", inlineObjectSize);
        }
      NS_LOG_INFO ("HTTP inline object size: " << inlineObjectSize << " bytes.");

      Ptr<Packet> packet = Create<Packet> (inlineObjectSize);
      packet->AddHeader (httpHeaderOut);
      return socket->Send (packet);
    }
}

void
HttpServer::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  static Ptr<Packet> packet;

  do
    {
      // Get more data from socket, if available
      if (!packet || packet->GetSize () == 0)
        {
          packet = socket->Recv ();
        }
      else if (socket->GetRxAvailable ())
        {
          packet->AddAtEnd (socket->Recv ());
        }

      HttpHeader httpHeader;
      packet->RemoveHeader (httpHeader);
      ProccessHttpRequest (socket, httpHeader);

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || packet->GetSize ());
}

void
HttpServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection closed.");
}

void
HttpServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection error.");
}

} // Namespace ns3
