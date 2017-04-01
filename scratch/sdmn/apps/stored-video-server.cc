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

#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[BuffVid server teid " << GetTeid () << "] "; }

#include "stored-video-server.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StoredVideoServer");
NS_OBJECT_ENSURE_REGISTERED (StoredVideoServer);

/**
 * \brief Default trace to send.
 */
struct StoredVideoServer::TraceEntry
StoredVideoServer::g_defaultEntries[] =
{
  {
    0,  534, 'I'
  },
  {
    40, 1542, 'P'
  },
  {
    120,  134, 'B'
  },
  {
    80,  390, 'B'
  },
  {
    240,  765, 'P'
  },
  {
    160,  407, 'B'
  },
  {
    200,  504, 'B'
  },
  {
    360,  903, 'P'
  },
  {
    280,  421, 'B'
  },
  {
    320,  587, 'B'
  }
};

TypeId
StoredVideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StoredVideoServer")
    .SetParent<SdmnServerApp> ()
    .AddConstructor<StoredVideoServer> ()
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
    .AddAttribute ("ChunkSize",
                   "The number of bytes on each chunk of the video.",
                   UintegerValue (128000),
                   MakeUintegerAccessor (&StoredVideoServer::m_chunkSize),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

StoredVideoServer::StoredVideoServer ()
  : m_connected (false),
    m_pendingBytes (0)
{
  NS_LOG_FUNCTION (this);
}

StoredVideoServer::~StoredVideoServer ()
{
  NS_LOG_FUNCTION (this);
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

  m_lengthRng = 0;
  m_entries.clear ();
  SdmnServerApp::DoDispose ();
}

void
StoredVideoServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Creating the listening TCP socket.");
  TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Listen ();
  m_socket->SetAcceptCallback (
    MakeCallback (&StoredVideoServer::NotifyConnectionRequest, this),
    MakeCallback (&StoredVideoServer::NotifyNewConnectionCreated, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&StoredVideoServer::NotifyNormalClose, this),
    MakeCallback (&StoredVideoServer::NotifyErrorClose, this));
}

void
StoredVideoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }
}

bool
StoredVideoServer::NotifyConnectionRequest (Ptr<Socket> socket,
                                            const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  uint16_t port = InetSocketAddress::ConvertFrom (address).GetPort ();
  NS_LOG_INFO ("Connection request received from " << ipAddr << ":" << port);

  return !m_connected;
}

void
StoredVideoServer::NotifyNewConnectionCreated (Ptr<Socket> socket,
                                               const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  uint16_t port = InetSocketAddress::ConvertFrom (address).GetPort ();
  NS_LOG_INFO ("Connection established with " << ipAddr << ":" << port);
  m_connected = true;
  m_pendingBytes = 0;

  socket->SetSendCallback (
    MakeCallback (&StoredVideoServer::SendData, this));
  socket->SetRecvCallback (
    MakeCallback (&StoredVideoServer::DataReceived, this));
}

void
StoredVideoServer::NotifyNormalClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("Connection successfully closed.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  m_connected = false;
  m_pendingBytes = 0;
}

void
StoredVideoServer::NotifyErrorClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_WARN ("Connection closed with errors.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  m_connected = false;
  m_pendingBytes = 0;
}

void
StoredVideoServer::DataReceived (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // This application expects to receive only
  // a single HTTP request message at a time.
  Ptr<Packet> packet = socket->Recv ();
  NotifyRx (packet->GetSize ());

  HttpHeader httpHeaderRequest;
  packet->RemoveHeader (httpHeaderRequest);
  NS_ASSERT_MSG (httpHeaderRequest.IsRequest (), "Invalid HTTP request.");
  NS_ASSERT_MSG (packet->GetSize () == 0, "Invalid RX data.");

  ProccessHttpRequest (socket, httpHeaderRequest);
}

void
StoredVideoServer::SendData (Ptr<Socket> socket, uint32_t available)
{
  NS_LOG_FUNCTION (this << socket << available);

  if (!m_pendingBytes)
    {
      NS_LOG_DEBUG ("No pending data to send.");
      return;
    }

  uint32_t pktSize = std::min (available, m_pendingBytes);
  Ptr<Packet> packet = Create<Packet> (pktSize);
  int bytes = socket->Send (packet);
  if (bytes > 0)
    {
      NS_LOG_DEBUG ("Server TX " << bytes << " bytes.");
      m_pendingBytes -= static_cast<uint32_t> (bytes);
    }
  else
    {
      NS_LOG_ERROR ("Server TX error.");
    }
}

void
StoredVideoServer::ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);

  // Check for requested URL.
  std::string url = header.GetRequestUrl ();
  NS_LOG_INFO ("Client requested " << url);
  if (url == "main/video")
    {
      // Set parameter values.
      m_pendingBytes = m_chunkSize;
      Time videoLength = Seconds (std::abs (m_lengthRng->GetValue ()));
      uint32_t numChunks = GetVideoChunks (videoLength) - 1;
      NS_LOG_INFO ("Video with " << numChunks << " chunks of " <<
                   m_chunkSize << " bytes each.");

      // Set the response message.
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", m_pendingBytes);
      httpHeaderOut.SetHeaderField ("ContentType", "main/video");
      httpHeaderOut.SetHeaderField ("InlineObjects", numChunks);

      Ptr<Packet> outPacket = Create<Packet> (0);
      outPacket->AddHeader (httpHeaderOut);

      NotifyTx (outPacket->GetSize () + m_pendingBytes);
      int bytes = socket->Send (outPacket);
      if (bytes != (int)outPacket->GetSize ())
        {
          NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
        }

      // Start sending the payload.
      SendData (socket, socket->GetTxAvailable ());
    }
  else if (url == "video/chunk")
    {
      m_pendingBytes = m_chunkSize;
      NS_LOG_DEBUG ("Video chunk size (bytes): " << m_pendingBytes);

      // Set the response message.
      HttpHeader httpHeaderOut;
      httpHeaderOut.SetResponse ();
      httpHeaderOut.SetVersion ("HTTP/1.1");
      httpHeaderOut.SetResponseStatusCode ("200");
      httpHeaderOut.SetResponsePhrase ("OK");
      httpHeaderOut.SetHeaderField ("ContentLength", m_pendingBytes);
      httpHeaderOut.SetHeaderField ("ContentType", "video/chunk");
      httpHeaderOut.SetHeaderField ("InlineObjects", 0);

      Ptr<Packet> outPacket = Create<Packet> (0);
      outPacket->AddHeader (httpHeaderOut);

      NotifyTx (outPacket->GetSize () + m_pendingBytes);
      int bytes = socket->Send (outPacket);
      if (bytes != (int)outPacket->GetSize ())
        {
          NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
        }

      // Start sending the payload.
      SendData (socket, socket->GetTxAvailable ());
    }
  else
    {
      NS_FATAL_ERROR ("Invalid URL requested.");
    }
}

void
StoredVideoServer::LoadTrace (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);

  uint32_t time, index, size, prevTime = 0;
  char frameType;
  TraceEntry entry;
  m_entries.clear ();

  std::ifstream ifTraceFile;
  ifTraceFile.open (filename.c_str (), std::ifstream::in);
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
StoredVideoServer::GetVideoChunks (Time length)
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
  return total / m_chunkSize;
}

} // Namespace ns3
