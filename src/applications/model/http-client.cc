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

#include "http-client.h"

NS_LOG_COMPONENT_DEFINE ("HttpClient");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HttpClient);

TypeId
HttpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpClient")
    .SetParent<EpcApplication> ()
    .AddConstructor<HttpClient> ()
    .AddAttribute ("ServerAddress",
                   "The server IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&HttpClient::m_serverAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("ServerPort",
                   "The server TCP port.",
                   UintegerValue (80),
                   MakeUintegerAccessor (&HttpClient::m_serverPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxReadingTime",
                   "The reading time threshold to stop application.",
                   TimeValue (Time::Max ()),
                   MakeTimeAccessor (&HttpClient::m_maxReadingTime),
                   MakeTimeChecker ())
    .AddAttribute ("MaxPages",
                   "The number of pages threshold to stop application.",
                   UintegerValue (std::numeric_limits<uint16_t>::max ()),
                   MakeUintegerAccessor (&HttpClient::m_maxPages),
                   MakeUintegerChecker<uint16_t> (1)) // At least 1 page
  ;
  return tid;
}

HttpClient::HttpClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_forceStop = EventId ();

  // Mu and Sigma data was taken from paper "An HTTP Web Traffic Model Based on
  // the Top One Million Visited Web Pages" by Rastin Pries et. al (Table II).
  m_readingTimeStream = CreateObject<LogNormalRandomVariable> ();
  m_readingTimeStream->SetAttribute ("Mu", DoubleValue (-0.495204));
  m_readingTimeStream->SetAttribute ("Sigma", DoubleValue (2.7731));

  // The above model provides a lot of reading times < 1sec, which is not soo
  // good for simulations in LTE EPC + SDN scenarios. So, we are incresing the
  // reading time by a some uniform random value in [0,10] secs.
  m_readingTimeAdjust = CreateObject<UniformRandomVariable> ();
  m_readingTimeAdjust->SetAttribute ("Min", DoubleValue (0.));
  m_readingTimeAdjust->SetAttribute ("Max", DoubleValue (10.));
}

HttpClient::~HttpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
HttpClient::SetServer (Ptr<HttpServer> server, Ipv4Address serverAddress,
                       uint16_t serverPort)
{
  m_serverApp = server;
  m_serverAddress = serverAddress;
  m_serverPort = serverPort;
}

Ptr<HttpServer>
HttpClient::GetServerApp ()
{
  return m_serverApp;
}

void
HttpClient::Start (void)
{
  ResetQosStats ();
  m_active = true;
  m_appStartTrace (this);

  if (!m_maxDurationTime.IsZero ())
    {
      m_forceStop = Simulator::Schedule (m_maxDurationTime,
                                         &HttpClient::CloseSocket, this);
    }
  OpenSocket ();
}

void
HttpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_serverApp = 0;
  m_socket = 0;
  m_readingTimeStream = 0;
  Simulator::Cancel (m_forceStop);
  EpcApplication::DoDispose ();
}

void
HttpClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);
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

  if (!m_socket)
    {
      NS_LOG_LOGIC ("Opening the TCP connection.");
      TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_socket->SetConnectCallback (
        MakeCallback (&HttpClient::ConnectionSucceeded, this),
        MakeCallback (&HttpClient::ConnectionFailed, this));
    }
}

void
HttpClient::CloseSocket ()
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
HttpClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_LOGIC ("Server accepted connection request!");
  socket->SetRecvCallback (MakeCallback (&HttpClient::HandleReceive, this));

  // Request the first main object
  m_pagesLoaded = 0;
  SendRequest (socket, "main/object");
}

void
HttpClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_ERROR ("Server did not accepted connection request!");
}

void
HttpClient::SendRequest (Ptr<Socket> socket, std::string url)
{
  NS_LOG_FUNCTION (this);

  // Setting request message
  HttpHeader httpHeader;
  httpHeader.SetRequest ();
  httpHeader.SetVersion ("HTTP/1.1");
  httpHeader.SetRequestMethod ("GET");
  httpHeader.SetRequestUrl (url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);
  socket->Send (packet);
}

void
HttpClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  static uint32_t    pendingBytes = 0;
  static uint32_t    pendingObjects = 0;
  static std::string contentType = "";
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

          // For main/objects, get the number of inline objects to load
          contentType = httpHeader.GetHeaderField ("ContentType");
          if (contentType == "main/object")
            {
              pendingObjects =
                atoi (httpHeader.GetHeaderField ("InlineObjects").c_str ());
            }
        }

      // Let's consume received data
      if (packet->GetSize () >= pendingBytes)
        {
          packet->RemoveAtStart (pendingBytes);
          pendingBytes = 0;
        }
      else
        {
          pendingBytes -= packet->GetSize ();
          packet->RemoveAtStart (packet->GetSize ());
        }

      if (!pendingBytes)
        {
          // This is the end of the HTTP message.
          NS_LOG_INFO ("HTTP " << contentType << " successfully received.");
          if (contentType == "main/object")
            {
              NS_LOG_INFO ("There are " << pendingObjects << " inline objects.");
            }
          else
            {
              pendingObjects--;
            }

          // When necessary, request inline objects
          if (pendingObjects)
            {
              NS_LOG_DEBUG ("Request for inline/object " << pendingObjects);
              SendRequest (socket, "inline/object");
            }
          else
            {
              NS_LOG_INFO ("HTTP page successfully received.");
              m_pagesLoaded++;
              SetReadingTime (socket);
            }
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || packet->GetSize ());
}

void
HttpClient::SetReadingTime (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  double randomSeconds = std::abs (m_readingTimeStream->GetValue ());
  double adjustSeconds = std::abs (m_readingTimeAdjust->GetValue ());
  Time readingTime = Seconds (randomSeconds + adjustSeconds);

  // Limiting reading time to 10000 seconds according to reference paper.
  if (readingTime > Seconds (10000))
    {
      readingTime = Seconds (10000);
    }

  // Stop application due to reading time threshold.
  if (readingTime > m_maxReadingTime)
    {
      CloseSocket ();
      return;
    }

  // Stop application due to max page threshold.
  if (m_pagesLoaded >= m_maxPages)
    {
      CloseSocket ();
      return;
    }

  NS_LOG_INFO ("Reading time: " << readingTime.As (Time::S));
  Simulator::Schedule (readingTime, &HttpClient::SendRequest,
                       this, socket, "main/object");
}

}

