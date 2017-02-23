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

#include "stored-video-server.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StoredVideoServer");
NS_OBJECT_ENSURE_REGISTERED (StoredVideoServer);

/**
 * \brief Default trace to send
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

  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->SetAttribute ("SndBufSize", UintegerValue (16384));
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
StoredVideoServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  NS_LOG_INFO ("Connection request from " << ipAddr);
  return !m_connected;
}

void
StoredVideoServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  NS_LOG_INFO ("Connection successfully established with " << ipAddr);
  socket->SetSendCallback (MakeCallback (&StoredVideoServer::SendData, this));
  socket->SetRecvCallback (
    MakeCallback (&StoredVideoServer::ReceiveData, this));
  m_connected = true;
  m_pendingBytes = 0;
}

void
StoredVideoServer::HandlePeerClose (Ptr<Socket> socket)
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
StoredVideoServer::HandlePeerError (Ptr<Socket> socket)
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
StoredVideoServer::ReceiveData (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the HTTP GET message.
  HttpHeader httpHeader;
  Ptr<Packet> packet = socket->Recv ();
  NotifyRx (packet->GetSize ());
  packet->RemoveHeader (httpHeader);
  NS_ASSERT (packet->GetSize () == 0);

  ProccessHttpRequest (socket, httpHeader);
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
  int bytes = socket->Send (packet);
  if (bytes > 0)
    {
      NS_LOG_DEBUG ("Stored video server TX " << bytes << " bytes.");
      m_pendingBytes -= static_cast<uint32_t> (bytes);
    }
  else
    {
      NS_LOG_ERROR ("Stored video server TX error.");
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

      NotifyTx (outPacket->GetSize () + m_pendingBytes);
      int bytes = socket->Send (outPacket);
      if (bytes != (int)outPacket->GetSize ())
        {
          NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
        }

      // Start sending the stored video stream to the client.
      SendData (socket, socket->GetTxAvailable ());
    }
  else
    {
      NS_FATAL_ERROR ("Invalid request.");
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

} // Namespace ns3
