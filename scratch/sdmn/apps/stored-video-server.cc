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
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StoredVideoServer");
NS_OBJECT_ENSURE_REGISTERED (StoredVideoServer);

/**
 * \brief Default trace to send
 */
struct StoredVideoServer::TraceEntry StoredVideoServer::g_defaultEntries[] =
{
  {  0,  534, 'I'}, { 40, 1542, 'P'}, {120,  134, 'B'}, { 80,  390, 'B'},
  {240,  765, 'P'}, {160,  407, 'B'}, {200,  504, 'B'}, {360,  903, 'P'},
  {280,  421, 'B'}, {320,  587, 'B'}
};

TypeId
StoredVideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StoredVideoServer")
    .SetParent<Application> ()
    .AddConstructor<StoredVideoServer> ()
    .AddAttribute ("LocalPort",
                   "Local TCP port listening for incoming connections.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&StoredVideoServer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (""),
                   MakeStringAccessor (&StoredVideoServer::SetTraceFile),
                   MakeStringChecker ())
    .AddAttribute ("VideoDuration",
                   "A random variable used to pick the video duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&StoredVideoServer::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

StoredVideoServer::StoredVideoServer ()
  : m_socket (0),
    m_localPort (0),
    m_connected (false),
    m_clientApp (0),
    m_pendingBytes (0),
    m_rxPacket (0)
{
  NS_LOG_FUNCTION (this);
}

StoredVideoServer::~StoredVideoServer ()
{
  NS_LOG_FUNCTION (this);
}

void
StoredVideoServer::SetClient (Ptr<StoredVideoClient> client)
{
  m_clientApp = client;
}

Ptr<StoredVideoClient>
StoredVideoServer::GetClientApp ()
{
  return m_clientApp;
}

void
StoredVideoServer::SetTraceFile (std::string traceFile)
{
  NS_LOG_FUNCTION (this << traceFile);
  if (traceFile == "")
    {
      LoadDefaultTrace ();
    }
  else
    {
      LoadTrace (traceFile);
    }
}

void
StoredVideoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_clientApp = 0;
  m_lengthRng = 0;
  m_rxPacket = 0;
  m_entries.clear ();
  Application::DoDispose ();
}

void
StoredVideoServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->SetAttribute ("SndBufSize", UintegerValue (4096));
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Listen ();
      m_socket->SetAcceptCallback (
        MakeCallback (&StoredVideoServer::HandleRequest, this),
        MakeCallback (&StoredVideoServer::HandleAccept, this));
      m_socket->SetCloseCallbacks (
        MakeCallback (&StoredVideoServer::HandlePeerClose, this),
        MakeCallback (&StoredVideoServer::HandlePeerError, this));
    }
}

void
StoredVideoServer::StopApplication ()
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
StoredVideoServer::ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT_MSG (header.IsRequest (), "Invalid HTTP request.");

  // Check for valid request.
  std::string url = header.GetRequestUrl ();
  NS_LOG_INFO ("Client requesting a " + url);
  if (url == "main/video")
    {
      // Set the random video length.
      Time videoLength = Seconds (std::abs (m_lengthRng->GetValue ()));
      m_pendingBytes = GetVideoBytes (videoLength);
      NS_LOG_INFO ("Stored video lenght " << videoLength.As (Time::S) <<
                   " with " << m_pendingBytes << " bytes");

      // Setting the HTTP response with number of bytes.
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", m_pendingBytes);
      httpHeaderOut.SetHeaderField ("ContentType", "main/video");
      Ptr<Packet> outPacket = Create<Packet> (0);
      outPacket->AddHeader (httpHeaderOut);
      socket->Send (outPacket);

      // Start sending the stored video stream to the client.
      SendStream (socket, socket->GetTxAvailable ());
    }
  else
    {
      NS_FATAL_ERROR ("Invalid request.");
    }
}

bool
StoredVideoServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Received request for connection from " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  return !m_connected;
}

void
StoredVideoServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_LOGIC ("Connection successfully established with client " <<
                InetSocketAddress::ConvertFrom (address).GetIpv4 ());
  socket->SetSendCallback (
    MakeCallback (&StoredVideoServer::SendStream, this));
  socket->SetRecvCallback (
    MakeCallback (&StoredVideoServer::HandleReceive, this));
  m_connected = true;
  m_pendingBytes = 0;
  m_rxPacket = 0;
}

void
StoredVideoServer::HandleReceive (Ptr<Socket> socket)
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
StoredVideoServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection closed.");
  socket->ShutdownSend ();
  m_connected = false;
}

void
StoredVideoServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Connection error.");
  socket->ShutdownSend ();
  m_connected = false;
}

void
StoredVideoServer::LoadTrace (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);
  uint32_t time, index, size, prevTime = 0;
  char frameType;
  TraceEntry entry;
  std::ifstream ifTraceFile;
  ifTraceFile.open (filename.c_str (), std::ifstream::in);
  m_entries.clear ();
  if (!ifTraceFile.good ())
    {
      NS_LOG_WARN ("Trace file not found. Loading default trace.");
      LoadDefaultTrace ();
    }
  while (ifTraceFile.good ())
    {
      ifTraceFile >> index >> frameType >> time >> size;
      if (frameType == 'B')
        {
          entry.timeToSend = 0;
        }
      else
        {
          entry.timeToSend = time - prevTime;
          prevTime = time;
        }
      entry.packetSize = size;
      entry.frameType = frameType;
      m_entries.push_back (entry);
    }
  ifTraceFile.close ();
}

void
StoredVideoServer::LoadDefaultTrace (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t prevTime = 0;
  for (uint32_t i = 0; i < (sizeof (g_defaultEntries) /
                            sizeof (struct TraceEntry)); i++)
    {
      struct TraceEntry entry = g_defaultEntries[i];
      if (entry.frameType == 'B')
        {
          entry.timeToSend = 0;
        }
      else
        {
          uint32_t tmp = entry.timeToSend;
          entry.timeToSend -= prevTime;
          prevTime = tmp;
        }
      m_entries.push_back (entry);
    }
}

uint32_t
StoredVideoServer::GetVideoBytes (Time length)
{
  uint32_t currentEntry = 0;
  Time elapsed = Seconds (0);
  struct TraceEntry *entry;
  uint32_t total = 0;
  while (elapsed < length)
    {
      entry = &m_entries[currentEntry];
      total += entry->packetSize;
      elapsed += MilliSeconds (entry->timeToSend);
      currentEntry++;
      currentEntry = currentEntry % m_entries.size ();
    }
  return total;
}

void
StoredVideoServer::SendStream (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this);

  if (!available || !m_connected || !m_pendingBytes)
    {
      return;
    }

  uint32_t pktSize = std::min (available, m_pendingBytes);
  Ptr<Packet> packet = Create<Packet> (pktSize);
  int bytesSent = socket->Send (packet);

  NS_LOG_DEBUG ("Stored video server TX " << bytesSent << " bytes");
  if (bytesSent > 0)
    {
      m_pendingBytes -= static_cast<uint32_t> (bytesSent);
    }
}

} // Namespace ns3
