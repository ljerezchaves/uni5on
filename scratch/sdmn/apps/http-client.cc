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
    .SetParent<SdmnApplication> ()
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
  : m_socket (0),
    m_serverPort (0),
    m_serverApp (0),
    m_pagesLoaded (0),
    m_forceStop (EventId ()),
    m_nextRequest (EventId ()),
    m_pendingBytes (0),
    m_rxPacket (0),
    m_pendingObjects (0)
{
  NS_LOG_FUNCTION (this);

  // Random variable parameters was taken from paper 'An HTTP Web Traffic Model
  // Based on the Top One Million Visited Web Pages' by Rastin Pries et. al
  // (Table II).
  m_readingTimeStream = CreateObject<LogNormalRandomVariable> ();
  m_readingTimeStream->SetAttribute ("Mu", DoubleValue (-0.495204));
  m_readingTimeStream->SetAttribute ("Sigma", DoubleValue (2.7731));

  // The above model provides a lot of reading times < 1sec, which is not soo
  // good for simulations in LTE EPC + SDN scenarios. So, we are incresing the
  // reading time by a some uniform random value in [0,10] secs.
  m_readingTimeAdjustStream = CreateObject<UniformRandomVariable> ();
  m_readingTimeAdjustStream->SetAttribute ("Min", DoubleValue (0.));
  m_readingTimeAdjustStream->SetAttribute ("Max", DoubleValue (10.));
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
  m_socket = 0;
  m_serverApp = 0;
  m_rxPacket = 0;
  m_readingTimeStream = 0;
  m_readingTimeAdjustStream = 0;
  Simulator::Cancel (m_forceStop);
  Simulator::Cancel (m_nextRequest);
  SdmnApplication::DoDispose ();
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

  // Preparing internal variables for new traffic cycle
  m_pendingBytes = 0;
  m_pendingObjects = 0;
  m_pagesLoaded = 0;
  m_rxPacket = 0;

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
  Simulator::Cancel (m_nextRequest);
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
HttpClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_LOGIC ("Server accepted connection request!");
  socket->SetRecvCallback (MakeCallback (&HttpClient::HandleReceive, this));

  // Request the first main/object
  SendRequest (socket, "main/object");
}

void
HttpClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Server did not accepted connection request!");
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
  NS_LOG_INFO ("Request for " << url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);
  socket->Send (packet);
}

void
HttpClient::HandleReceive (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (m_active, "Invalid active state");

  static std::string contentType = "";

  do
    {
      // Get more data from socket, if available
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

          // For main/objects, get the number of inline objects to load
          contentType = httpHeader.GetHeaderField ("ContentType");
          if (contentType == "main/object")
            {
              m_pendingObjects =
                atoi (httpHeader.GetHeaderField ("InlineObjects").c_str ());
            }
        }

      // Let's consume received data
      uint32_t consume = std::min (m_rxPacket->GetSize (), m_pendingBytes);
      m_rxPacket->RemoveAtStart (consume);
      m_pendingBytes -= consume;
      NS_LOG_DEBUG ("HTTP RX " << consume << " bytes");

      if (!m_pendingBytes)
        {
          // This is the end of the HTTP message.
          NS_LOG_INFO ("HTTP " << contentType << " successfully received.");
          NS_ASSERT (m_rxPacket->GetSize () == 0);

          if (contentType == "main/object")
            {
              NS_LOG_INFO ("There are inline objects: " << m_pendingObjects);
            }
          else
            {
              m_pendingObjects--;
            }

          // When necessary, request inline objects
          if (m_pendingObjects)
            {
              NS_LOG_DEBUG ("Request for inline/object " << m_pendingObjects);
              SendRequest (socket, "inline/object");
            }
          else
            {
              NS_LOG_INFO ("HTTP page successfully received.");
              m_pagesLoaded++;
              SetReadingTime (socket);
              break;
            }
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || m_rxPacket->GetSize ());
}

void
HttpClient::SetReadingTime (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  double randomSeconds = std::abs (m_readingTimeStream->GetValue ());
  double adjustSeconds = std::abs (m_readingTimeAdjustStream->GetValue ());
  Time readingTime = Seconds (randomSeconds + adjustSeconds);

  // Limiting reading time to 10000 seconds according to reference paper.
  if (readingTime > Seconds (10000))
    {
      readingTime = Seconds (10000);
    }

  // Stop application due to reading time threshold.
  if (readingTime > m_maxReadingTime)
    {
      NS_LOG_DEBUG ("Closing socket due to reading time threshold.");
      CloseSocket ();
      return;
    }

  // Stop application due to max page threshold.
  if (m_pagesLoaded >= m_maxPages)
    {
      NS_LOG_DEBUG ("Closing socket due to max page threshold.");
      CloseSocket ();
      return;
    }

  NS_LOG_INFO ("Reading time: " << readingTime.As (Time::S));
  m_nextRequest = Simulator::Schedule (readingTime, &HttpClient::SendRequest,
                                       this, socket, "main/object");
}

} // Namespace ns3
