/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "buffered-video-server.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " server teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BufferedVideoServer");
NS_OBJECT_ENSURE_REGISTERED (BufferedVideoServer);

TypeId
BufferedVideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BufferedVideoServer")
    .SetParent<Uni5onServer> ()
    .AddConstructor<BufferedVideoServer> ()
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (std::string ()),
                   MakeStringAccessor (&BufferedVideoServer::LoadTrace),
                   MakeStringChecker ())
  ;
  return tid;
}

BufferedVideoServer::BufferedVideoServer ()
  : m_connected (false),
  m_pendingBytes (0),
  m_chunkSize (128000)
{
  NS_LOG_FUNCTION (this);
}

BufferedVideoServer::~BufferedVideoServer ()
{
  NS_LOG_FUNCTION (this);
}

void
BufferedVideoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_entries.clear ();
  Uni5onServer::DoDispose ();
}

void
BufferedVideoServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_entries.empty (), "No trace file loaded.");
  NS_LOG_INFO ("Creating the listening TCP socket.");
  TypeId tcpFactory = TypeId::LookupByName ("ns3::TcpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), tcpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Listen ();
  m_socket->SetAcceptCallback (
    MakeCallback (&BufferedVideoServer::NotifyConnectionRequest, this),
    MakeCallback (&BufferedVideoServer::NotifyNewConnectionCreated, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&BufferedVideoServer::NotifyNormalClose, this),
    MakeCallback (&BufferedVideoServer::NotifyErrorClose, this));
}

void
BufferedVideoServer::StopApplication ()
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
BufferedVideoServer::NotifyConnectionRequest (Ptr<Socket> socket,
                                              const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  uint16_t port = InetSocketAddress::ConvertFrom (address).GetPort ();
  NS_LOG_INFO ("Connection request received from " << ipAddr << ":" << port);

  return !m_connected;
}

void
BufferedVideoServer::NotifyNewConnectionCreated (Ptr<Socket> socket,
                                                 const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);

  Ipv4Address ipAddr = InetSocketAddress::ConvertFrom (address).GetIpv4 ();
  uint16_t port = InetSocketAddress::ConvertFrom (address).GetPort ();
  NS_LOG_INFO ("Connection established with " << ipAddr << ":" << port);
  m_connected = true;
  m_pendingBytes = 0;

  socket->SetSendCallback (
    MakeCallback (&BufferedVideoServer::SendData, this));
  socket->SetRecvCallback (
    MakeCallback (&BufferedVideoServer::DataReceived, this));
}

void
BufferedVideoServer::NotifyNormalClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_INFO ("Connection successfully closed.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  m_connected = false;
  m_pendingBytes = 0;
}

void
BufferedVideoServer::NotifyErrorClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_LOG_WARN ("Connection closed with errors.");
  socket->ShutdownSend ();
  socket->ShutdownRecv ();
  m_connected = false;
  m_pendingBytes = 0;
}

void
BufferedVideoServer::DataReceived (Ptr<Socket> socket)
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
BufferedVideoServer::SendData (Ptr<Socket> socket, uint32_t available)
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
BufferedVideoServer::ProccessHttpRequest (Ptr<Socket> socket,
                                          HttpHeader header)
{
  NS_LOG_FUNCTION (this << socket);

  // Check for requested URL.
  std::string url = header.GetRequestUrl ();
  NS_LOG_DEBUG ("Client requested " << url);
  if (url == "main/video")
    {
      // Get traffic length from client request.
      Time videoLength = Time (header.GetHeaderField ("TrafficLength"));

      // Set parameter values.
      m_pendingBytes = m_chunkSize;
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

      int bytes = socket->Send (outPacket);
      if (bytes != static_cast<int> (outPacket->GetSize ()))
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

      int bytes = socket->Send (outPacket);
      if (bytes != static_cast<int> (outPacket->GetSize ()))
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
BufferedVideoServer::LoadTrace (std::string filename)
{
  NS_LOG_FUNCTION (this << filename);

  m_entries.clear ();
  if (filename.empty ())
    {
      return;
    }

  std::ifstream ifTraceFile;
  ifTraceFile.open (filename.c_str (), std::ifstream::in);
  NS_ABORT_MSG_IF (!ifTraceFile.good (), "Trace file not found.");

  uint32_t time, index, size, prevTime = 0;
  char frameType;
  TraceEntry entry;

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

uint32_t
BufferedVideoServer::GetVideoChunks (Time length)
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
