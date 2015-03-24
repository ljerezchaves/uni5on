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
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "http-client.h"

NS_LOG_COMPONENT_DEFINE ("HttpClient");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HttpClient);

TypeId
HttpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpClient")
    .SetParent<Application> ()
    .AddConstructor<HttpClient> ()
    .AddAttribute ("RemoteAddress", "The destination Address of the outbound packets.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&HttpClient::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TcpTimeout", "The TCP connection timeout",
                   TimeValue (Seconds (4)),
                   MakeTimeAccessor (&HttpClient::m_tcpTimeout),
                   MakeTimeChecker ())
  ;
  return tid;
}

HttpClient::HttpClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_contentLength = 0;
  m_bytesReceived = 0;
  m_numOfInlineObjects = 0;
  m_inlineObjLoaded = 0;
  m_qosStats = Create<QosStatsCalculator> ();

  // Mu and Sigma data was taken from paper "An HTTP Web Traffic Model Based on
  // the Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
  m_readingTimeStream = CreateObject<LogNormalRandomVariable> ();
  m_readingTimeStream->SetAttribute ("Mu", DoubleValue (-0.495204));
  m_readingTimeStream->SetAttribute ("Sigma", DoubleValue (2.7731));

  // The above model provides a lot of reading times < 1sec, which is not soo
  // good for simulations in LTE EPC + SDN scenarios. So, we are incresing the
  // reading time by a some uniform random value in [1,10] secs.
  m_readingTimeAdjust = CreateObject<UniformRandomVariable> ();
  m_readingTimeAdjust->SetAttribute ("Min", DoubleValue (1.));
  m_readingTimeAdjust->SetAttribute ("Max", DoubleValue (10.));
}

HttpClient::~HttpClient ()
{
  NS_LOG_FUNCTION (this);
}

void 
HttpClient::SetServerApp (Ptr<HttpServer> server)
{
  m_serverApp = server;
}

Ptr<HttpServer> 
HttpClient::GetServerApp ()
{
  return m_serverApp;
}

void 
HttpClient::ResetQosStats ()
{
  m_qosStats->ResetCounters ();
}

Ptr<const QosStatsCalculator>
HttpClient::GetQosStats (void) const
{
  return m_qosStats;
}

void
HttpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
  m_serverApp = 0;
  m_socket = 0;
  m_qosStats = 0;
  m_readingTimeStream = 0;
}

void
HttpClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  ResetQosStats ();
  OpenSocket ();
}

void
HttpClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CloseSocket ();
}

void
HttpClient::OpenSocket ()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_startSendingCallback.IsNull ())
    {
      if (!m_startSendingCallback (this))
        {
          NS_LOG_WARN ("Http application (" << this << ") has been blocked." <<
                       "Retrying connection in 10 seconds.");
          Simulator::Schedule (Seconds (10), &HttpClient::OpenSocket, this);
          return;
        }
    }

  if (!m_socket)
    {
      NS_LOG_LOGIC ("Opening the TCP connection.");
      TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
      m_socket->SetConnectCallback (
          MakeCallback (&HttpClient::ConnectionSucceeded, this),
          MakeCallback (&HttpClient::ConnectionFailed, this));
    }
}

void
HttpClient::CloseSocket ()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_stopSendingCallback.IsNull ())
    {
      m_stopSendingCallback (this);
    }

  if (m_socket != 0)
    {
      NS_LOG_LOGIC ("Closing the TCP connection.");
      m_socket->Close ();
      m_socket = 0;
    }
}

void
HttpClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  
  m_clientAddress = socket->GetNode ()->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
  NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> Server accepted connection request!");
  socket->SetRecvCallback (MakeCallback (&HttpClient::HandleReceive, this));
  
  SendRequest (socket, "main/object");
}

void
HttpClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_ERROR ("HttpClient >> Server did not accepted connection request!");
}

void
HttpClient::SendRequest (Ptr<Socket> socket, string url)
{
  NS_LOG_FUNCTION (this);

  // Setting request message
  m_httpHeader.SetRequest (true);
  m_httpHeader.SetMethod ("GET");
  m_httpHeader.SetUrl (url);
  m_httpHeader.SetVersion ("HTTP/1.1");

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (m_httpHeader);
  NS_LOG_INFO ("HttpClient (" << m_clientAddress << ") >> Sending request for " << url << " to server (" << m_peerAddress << ").");
  socket->Send (packet);
}

void
HttpClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  uint32_t bytesReceived = packet->GetSize ();
 
  // Update QoS statistics
  m_qosStats->NotifyReceived (0, Simulator::Now (), bytesReceived);

  HttpHeader httpHeaderIn;
  packet->PeekHeader (httpHeaderIn);
  string statusCode = httpHeaderIn.GetStatusCode ();
  
  if (statusCode == "200")
    {
      m_contentType = httpHeaderIn.GetHeaderField ("ContentType");
      m_contentLength = atoi (httpHeaderIn.GetHeaderField ("ContentLength").c_str ());

      m_bytesReceived = bytesReceived - httpHeaderIn.GetSerializedSize ();

      if (m_contentType == "main/object")
        {
          m_numOfInlineObjects = atoi (httpHeaderIn.GetHeaderField ("NumOfInlineObjects").c_str ());
        }

      if (m_bytesReceived == m_contentLength)
        {
          NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << ": " << m_bytesReceived << " bytes of " << m_contentLength << " received.");
          m_contentLength = 0;

          if (m_contentType == "main/object")
            {
              NS_LOG_INFO ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << " successfully received. There are " << m_numOfInlineObjects << " inline objects to request.");
              NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> Requesting inline/object 1...");
              m_inlineObjLoaded = 0;
              SendRequest (socket, "inline/object");
            }
          else
            {
              m_inlineObjLoaded++;
              if (m_inlineObjLoaded < m_numOfInlineObjects)
                {
                  NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> Requesting inline/object " << m_inlineObjLoaded + 1 << "...");
                  SendRequest (socket, "inline/object");
                }
              else
                {
                  SetReadingTime (socket);
                }
            }
        }
      else
        {
          NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << ": " << m_bytesReceived << " bytes of " << m_contentLength << " received.");
        }
    }
  else
    {
      m_bytesReceived += bytesReceived;

      if (m_bytesReceived == m_contentLength)
        {
          NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << ": " << m_bytesReceived << " bytes of " << m_contentLength << " received.");
          m_contentLength = 0;

          if (m_contentType == "main/object")
            {
              NS_LOG_INFO ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << " successfully received. There are " << m_numOfInlineObjects << " inline objects to request.");
              NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> Requesting inline/object 1...");
              m_inlineObjLoaded = 0;
              SendRequest (socket, "inline/object");
            }
          else
            {
              m_inlineObjLoaded++;
              if (m_inlineObjLoaded < m_numOfInlineObjects)
                {
                  NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << " " << m_inlineObjLoaded << " of " << m_numOfInlineObjects << " successfully received.\nHttpClient >> Requesting inline/object " << m_inlineObjLoaded + 1);
                  SendRequest (socket, "inline/object");
                }
              else
                {
                  NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << " " << m_inlineObjLoaded << " of " << m_numOfInlineObjects << " successfully received.");
                  SetReadingTime (socket);
                }
            }
        }
      else
        {
          NS_LOG_DEBUG ("HttpClient (" << m_clientAddress << ") >> " << m_contentType << ": " << m_bytesReceived << " bytes of " << m_contentLength << " received.");
        }
    }
}

void
HttpClient::SetReadingTime (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  
  double randomSeconds = m_readingTimeStream->GetValue ();
  double adjustSeconds = m_readingTimeAdjust->GetValue ();
  Time readingTime = Seconds (randomSeconds + adjustSeconds);
  if (readingTime > Seconds (10000))
    {
      //Limiting reading time to 10000 seconds according to reference paper.
      readingTime = Seconds (10000);
    }

  if (readingTime > (m_tcpTimeout + Seconds (1)))
    {
      Simulator::Schedule (m_tcpTimeout, &HttpClient::CloseSocket, this);
      Simulator::Schedule (readingTime, &HttpClient::OpenSocket, this);
    }
  else
    {
      Simulator::Schedule (readingTime, &HttpClient::SendRequest, this, socket, "main/object");
    }
  
  NS_LOG_INFO ("HttpClient (" << m_clientAddress << ") >> Reading time: " << readingTime.As (Time::S));
}

}

