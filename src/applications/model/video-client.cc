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

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "seq-ts-header.h"
#include "video-client.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoClient");
NS_OBJECT_ENSURE_REGISTERED (VideoClient);

/**
 * \brief Default trace to send
 */
struct VideoClient::TraceEntry VideoClient::g_defaultEntries[] = {
  { 0, 534, 'I'},
  { 40, 1542, 'P'},
  { 120, 134, 'B'},
  { 80, 390, 'B'},
  { 240, 765, 'P'},
  { 160, 407, 'B'},
  { 200, 504, 'B'},
  { 360, 903, 'P'},
  { 280, 421, 'B'},
  { 320, 587, 'B'}
};

TypeId
VideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoClient")
    .SetParent<Application> ()
    .AddConstructor<VideoClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&VideoClient::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VideoClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxPacketSize",
                   "The maximum size [bytes] of a packet.",
                   UintegerValue (1400),
                   MakeUintegerAccessor (&VideoClient::m_maxPacketSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (""),
                   MakeStringAccessor (&VideoClient::SetTraceFile),
                   MakeStringChecker ())
    .AddAttribute ("OnTime", 
                  "A random variable used to pick the 'ON' state duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VideoClient::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", 
                  "A random variable used to pick the 'OFF' state duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VideoClient::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
  ;
  return tid;
}

VideoClient::VideoClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_serverApp = 0;
  m_sendEvent = EventId ();
  m_maxPacketSize = 1480;
  m_connected = false;
}

VideoClient::~VideoClient ()
{
  NS_LOG_FUNCTION (this);
  m_entries.clear ();
}

void
VideoClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
VideoClient::SetTraceFile (std::string traceFile)
{
  NS_LOG_FUNCTION (this << traceFile);
  if (traceFile == "")
    {
      LoadDefaultTrace ();
    }
  else
    {
      LoadTrace (traceFile);
    }}

void
VideoClient::SetMaxPacketSize (uint16_t maxPacketSize)
{
  NS_LOG_FUNCTION (this << maxPacketSize);
  m_maxPacketSize = maxPacketSize;
}

void 
VideoClient::SetServerApp (Ptr<UdpServer> server)
{
  m_serverApp = server;
}

Ptr<UdpServer> 
VideoClient::GetServerApp ()
{
  return m_serverApp;
}

uint16_t 
VideoClient::GetMaxPacketSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_maxPacketSize;
}

void
VideoClient::ResetCounters ()
{
  m_sent = 0;
}

void
VideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_serverApp = 0;
  m_onTime = 0;
  m_offTime = 0;
  m_entries.clear ();
  Application::DoDispose ();
}

void
VideoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), udpFactory);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
          MakeCallback (&VideoClient::ConnectionSucceeded, this),
          MakeCallback (&VideoClient::ConnectionFailed, this));
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  ResetCounters ();
  CancelEvents ();
  ScheduleStartEvent ();
}

void
VideoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket = 0;
    }
}

void
VideoClient::LoadTrace (std::string filename)
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
  m_currentEntry = 0;
}

void
VideoClient::LoadDefaultTrace (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t prevTime = 0;
  for (uint32_t i = 0; i < (sizeof (g_defaultEntries) / sizeof (struct TraceEntry)); i++)
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
  m_currentEntry = 0;
}

void 
VideoClient::CancelEvents ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

void 
VideoClient::StartSending ()
{
  NS_LOG_FUNCTION (this);
  if (!m_startSendingCallback.IsNull ())
    {
      if (!m_startSendingCallback (this))
        {
          NS_LOG_WARN ("Application " << this << " has been blocked.");
          CancelEvents ();
          ScheduleStartEvent ();
          return;
        }
    }
  ScheduleStopEvent ();
  SendStream ();
}

void 
VideoClient::StopSending ()
{
  NS_LOG_FUNCTION (this);
  if (!m_stopSendingCallback.IsNull ())
    {
      m_stopSendingCallback (this);
    }
  CancelEvents ();
  ScheduleStartEvent ();
}

void 
VideoClient::ScheduleStartEvent ()
{  
  NS_LOG_FUNCTION (this);
  
  Time offInterval = Seconds (m_offTime->GetValue ());
  m_startStopEvent = Simulator::Schedule (offInterval, &VideoClient::StartSending, this);
  
  NS_LOG_LOGIC ("Video " << this << " will start in +" << offInterval.GetSeconds ());
}

void 
VideoClient::ScheduleStopEvent ()
{  
  NS_LOG_FUNCTION (this);
  
  Time onInterval = Seconds (m_onTime->GetValue ());
  m_startStopEvent = Simulator::Schedule (onInterval, &VideoClient::StopSending, this);
  
  NS_LOG_LOGIC ("Video " << this << " will stop in +" << onInterval.GetSeconds ());
}

void 
VideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void 
VideoClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void
VideoClient::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  uint32_t packetSize = 0;
  if (size > 12)
    {
      // Removing the SeqTsHeader size (12 bytes)
      packetSize = size - 12; 
    }
  
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);

  Ptr<Packet> p = Create<Packet> (packetSize);
  p->AddHeader (seqTs);

  if (m_socket->Send (p))
    {
      ++m_sent;
      NS_LOG_INFO ("Video TX "<< size << 
                   " bytes to " << m_peerAddress <<
                   ":" << m_peerPort <<
                   " Uid " << p->GetUid () <<
                   " Time " << (Simulator::Now ()).GetSeconds ());
    }
  else
    {
      NS_LOG_INFO ("Error sending Video " << size << 
                   " bytes to " << m_peerAddress);
    }
}

void
VideoClient::SendStream (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> p;
  struct TraceEntry *entry = &m_entries[m_currentEntry];
  do
    {
      for (uint32_t i = 0; i < entry->packetSize / m_maxPacketSize; i++)
        {
          SendPacket (m_maxPacketSize);
        }

      uint16_t sizetosend = entry->packetSize % m_maxPacketSize;
      SendPacket (sizetosend);

      m_currentEntry++;
      m_currentEntry %= m_entries.size ();
      entry = &m_entries[m_currentEntry];
    }
  while (entry->timeToSend == 0);

  m_sendEvent = Simulator::Schedule (MilliSeconds (entry->timeToSend), 
                                     &VideoClient::SendStream, this);
}



} // Namespace ns3
