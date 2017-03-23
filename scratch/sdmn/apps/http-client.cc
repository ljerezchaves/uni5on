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

#include "http-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HttpClient");
NS_OBJECT_ENSURE_REGISTERED (HttpClient);

TypeId
HttpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpClient")
    .SetParent<SdmnClientApp> ()
    .AddConstructor<HttpClient> ()
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
  : m_nextRequest (EventId ()),
    m_rxPacket (0),
    m_pagesLoaded (0),
    m_httpPacketSize (0),
    m_pendingBytes (0),
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
HttpClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Chain up to fire start trace
  SdmnClientApp::Start ();

  // Preparing internal variables for new traffic cycle
  m_httpPacketSize = 0;
  m_pendingBytes = 0;
  m_pendingObjects = 0;
  m_pagesLoaded = 0;
  m_rxPacket = 0;

  // Open the TCP connection
  if (!m_socket)
    {
      NS_LOG_INFO ("Opening the TCP connection for app " << GetAppName () <<
                   " with teid " << GetTeid ());
      TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Connect (InetSocketAddress (m_serverAddress, m_serverPort));
      m_socket->SetConnectCallback (
        MakeCallback (&HttpClient::ConnectionSucceeded, this),
        MakeCallback (&HttpClient::ConnectionFailed, this));
    }
}

void
HttpClient::Stop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel further requests
  Simulator::Cancel (m_nextRequest);

  // Close the TCP socket
  if (m_socket != 0)
    {
      NS_LOG_INFO ("Closing the TCP connection for app " << GetAppName () <<
                   " with teid " << GetTeid ());
      m_socket->ShutdownRecv ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  // Chain up to fire stop trace
  SdmnClientApp::Stop ();
}

void
HttpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_rxPacket = 0;
  m_readingTimeStream = 0;
  m_readingTimeAdjustStream = 0;
  Simulator::Cancel (m_nextRequest);
  SdmnClientApp::DoDispose ();
}

void
HttpClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("Server accepted connection request for app " <<
               GetAppName () << " with teid " << GetTeid ());
  socket->SetRecvCallback (MakeCallback (&HttpClient::ReceiveData, this));

  // Request the first main/object
  SendRequest (socket, "main/object");
}

void
HttpClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Server refused connection request!");
}

void
HttpClient::ReceiveData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  static std::string contentType = "";

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

          // For main/objects, get the number of inline objects to load
          contentType = httpHeader.GetHeaderField ("ContentType");
          if (contentType == "main/object")
            {
              m_pendingObjects = std::atoi (
                  httpHeader.GetHeaderField ("InlineObjects").c_str ());
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
          NS_LOG_DEBUG ("HTTP " << contentType << " successfully received.");
          NS_ASSERT (m_rxPacket->GetSize () == 0);
          NotifyRx (m_httpPacketSize);

          if (contentType == "main/object")
            {
              NS_LOG_DEBUG ("There are inline objects: " << m_pendingObjects);
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
              NS_LOG_INFO ("HTTP page successfully received by app " <<
                           GetAppName () << " with teid " << GetTeid ());
              m_pagesLoaded++;
              SetReadingTime (socket);
              break;
            }
        }

    } // Repeat until no more data available to process
  while (socket->GetRxAvailable () || m_rxPacket->GetSize ());
}

void
HttpClient::SendRequest (Ptr<Socket> socket, std::string url)
{
  NS_LOG_FUNCTION (this);

  // When the force stop flag is active, don't send new requests.
  if (IsForceStop ())
    {
      NS_LOG_WARN ("App " << GetAppName () << " with teid " << GetTeid () <<
                   " can't send request on force stop mode.");
      return;
    }

  // Setting request message
  HttpHeader httpHeader;
  httpHeader.SetRequest ();
  httpHeader.SetVersion ("HTTP/1.1");
  httpHeader.SetRequestMethod ("GET");
  httpHeader.SetRequestUrl (url);
  NS_LOG_DEBUG ("Request for " << url);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (httpHeader);

  NotifyTx (packet->GetSize ());
  int bytes = socket->Send (packet);
  if (bytes != (int)packet->GetSize ())
    {
      NS_LOG_ERROR ("Not all bytes were sent to socket of app " <<
                    GetAppName () << " with teid " << GetTeid ());
    }
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
      NS_LOG_INFO ("App " << GetAppName () << " with teid " << GetTeid () <<
                   " is closing the socket due to reading time threshold.");
      Stop ();
      return;
    }

  // Stop application due to max page threshold.
  if (m_pagesLoaded >= m_maxPages)
    {
      NS_LOG_INFO ("App " << GetAppName () << " with teid " << GetTeid () <<
                   " is closing the socket due to max page threshold.");
      Stop ();
      return;
    }

  NS_LOG_INFO ("App " << GetAppName () << " with teid " << GetTeid () <<
               " set the reading time to " << readingTime.As (Time::S));
  m_nextRequest = Simulator::Schedule (readingTime, &HttpClient::SendRequest,
                                       this, socket, "main/object");
}

} // Namespace ns3
