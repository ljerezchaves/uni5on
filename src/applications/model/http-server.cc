/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
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
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "ns3/tcp-newreno.h"
#include "ns3/drop-tail-queue.h"
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
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
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
HttpServer::SetClientApp (Ptr<HttpClient> client)
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
    }
}

void HttpServer::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  if (m_socket != 0)
    {
      m_socket->Close ();
    }
}

bool
HttpServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_INFO ("HttpServer >> Request for connection from " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 () << " received.");
  return true;
}

void
HttpServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_INFO ("HttpServer >> Connection with Client (" <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 () <<
                ") successfully established!");
  socket->SetRecvCallback (MakeCallback (&HttpServer::HandleReceive, this));
}

void
HttpServer::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  HttpHeader httpHeaderIn;
  Ptr<Packet> packet = socket->Recv ();

  // Getting TCP Sending Buffer Size.
  Ptr<TcpNewReno> tcp = CreateObject<TcpNewReno> ();
  NS_ASSERT (tcp != 0);
  UintegerValue bufSizeValue;
  tcp->GetAttribute ("SndBufSize", bufSizeValue);
  uint32_t tcpBufSize = bufSizeValue.Get ();

  packet->PeekHeader (httpHeaderIn);

  string url = httpHeaderIn.GetUrl ();

  NS_LOG_INFO ("HttpServer >> Client requesting a " + url);

  if (url == "main/object")
    {
      //Scale, Shape and Mean data was taken from paper "An HTTP Web Traffic Model Based on the
      //Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
      Ptr<WeibullRandomVariable> mainObjectSizeStream = CreateObject<WeibullRandomVariable> ();
      mainObjectSizeStream->SetAttribute ("Scale", DoubleValue (19104.9));
      mainObjectSizeStream->SetAttribute ("Shape", DoubleValue (0.771807));
      uint32_t mainObjectSize = mainObjectSizeStream->GetInteger ();

      Ptr<ExponentialRandomVariable> numOfInlineObjStream = CreateObject<ExponentialRandomVariable> ();
      numOfInlineObjStream->SetAttribute ("Mean", DoubleValue (31.9291));
      uint32_t numOfInlineObj = numOfInlineObjStream->GetInteger ();

      //Setting response
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetRequest (false);
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetStatusCode ("200");
      httpHeaderOut.SetPhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", mainObjectSize);
      httpHeaderOut.SetHeaderField ("ContentType", "main/object");
      httpHeaderOut.SetHeaderField ("NumOfInlineObjects", numOfInlineObj);

      //Verifying if the buffer can store this packet size
      if (mainObjectSize > tcpBufSize)
        {
          mainObjectSize = tcpBufSize - httpHeaderOut.GetSerializedSize ();
          httpHeaderOut.SetHeaderField ("ContentLength", mainObjectSize);
        }

      Ptr<Packet> p = Create<Packet> (mainObjectSize);
      p->AddHeader (httpHeaderOut);

      NS_LOG_INFO ("HttpServer >> Sending response to client. Main Object Size ("
                   << mainObjectSize << " bytes). NumOfInlineObjects ("
                   << numOfInlineObj << ").");
    }
  else
    {
      //Mu and Sigma data was taken from paper "An HTTP Web Traffic Model Based on the
      //Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
      Ptr<LogNormalRandomVariable> logNormal = CreateObject<LogNormalRandomVariable> ();
      logNormal->SetAttribute ("Mu", DoubleValue (8.91365));
      logNormal->SetAttribute ("Sigma", DoubleValue (1.24816));

      uint32_t inlineObjectSize = logNormal->GetInteger ();

      //Setting response
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetRequest (false);
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetStatusCode ("200");
      httpHeaderOut.SetPhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", inlineObjectSize);
      httpHeaderOut.SetHeaderField ("ContentType", "inline/object");
      httpHeaderOut.SetHeaderField ("NumOfInlineObjects", 0);

      //Verifying if the buffer can store this packet size
      if (inlineObjectSize > tcpBufSize)
        {
          inlineObjectSize = tcpBufSize - httpHeaderOut.GetSerializedSize ();
          httpHeaderOut.SetHeaderField ("ContentLength", inlineObjectSize);
        }

      Ptr<Packet> p = Create<Packet> (inlineObjectSize);
      p->AddHeader (httpHeaderOut);

      NS_LOG_INFO ("HttpServer >> Sending response to client. Inline Objectsize ("
                   << inlineObjectSize << " bytes).");
    }
}

}

