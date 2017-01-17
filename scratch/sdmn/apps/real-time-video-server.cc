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

#include <ns3/seq-ts-header.h>
#include "real-time-video-server.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RealTimeVideoServer");
NS_OBJECT_ENSURE_REGISTERED (RealTimeVideoServer);

/**
 * \brief Default trace to send
 */
struct RealTimeVideoServer::TraceEntry
  RealTimeVideoServer::g_defaultEntries[] =
{
  {  0,  534, 'I'},
  { 40, 1542, 'P'},
  {120,  134, 'B'},
  { 80,  390, 'B'},
  {240,  765, 'P'},
  {160,  407, 'B'},
  {200,  504, 'B'},
  {360,  903, 'P'},
  {280,  421, 'B'},
  {320,  587, 'B'}
};

TypeId
RealTimeVideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RealTimeVideoServer")
    .SetParent<SdmnServerApp> ()
    .AddConstructor<RealTimeVideoServer> ()
    .AddAttribute ("MaxPacketSize",
                   "The maximum size [bytes] of a packet.",
                   UintegerValue (1400),
                   MakeUintegerAccessor (&RealTimeVideoServer::m_pktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (""),
                   MakeStringAccessor (&RealTimeVideoServer::SetTraceFile),
                   MakeStringChecker ())
  ;
  return tid;
}

RealTimeVideoServer::RealTimeVideoServer ()
  : m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

RealTimeVideoServer::~RealTimeVideoServer ()
{
  NS_LOG_FUNCTION (this);
}

void
RealTimeVideoServer::SetTraceFile (std::string traceFile)
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
RealTimeVideoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_entries.clear ();
  SdmnServerApp::DoDispose ();
}

void
RealTimeVideoServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), udpFactory);
      m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_socket->Connect (InetSocketAddress (m_clientAddress, m_clientPort));
      m_socket->ShutdownRecv ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
RealTimeVideoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->ShutdownSend ();
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
RealTimeVideoServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
  m_currentEntry = 0;

  // Chain up
  SdmnServerApp::NotifyStart ();

  // Start streaming
  NS_LOG_INFO ("Real-time video started.");
  SendStream ();
}

void
RealTimeVideoServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);

  // Chain up
  SdmnServerApp::NotifyForceStop ();

  // Stop streaming
  NS_LOG_INFO ("Real-time video stopped.");
}

void
RealTimeVideoServer::LoadTrace (std::string filename)
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
RealTimeVideoServer::LoadDefaultTrace (void)
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

void
RealTimeVideoServer::SendStream (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> packet;
  struct TraceEntry *entry = &m_entries[m_currentEntry];
  NS_LOG_DEBUG ("Real-time video frame " << entry->packetSize << " bytes");
  do
    {
      for (uint32_t i = 0; i < entry->packetSize / m_pktSize; i++)
        {
          SendPacket (m_pktSize);
        }
      uint16_t sizetosend = entry->packetSize % m_pktSize;
      SendPacket (sizetosend);

      m_currentEntry++;
      m_currentEntry %= m_entries.size ();
      entry = &m_entries[m_currentEntry];
    }
  while (entry->timeToSend == 0);

  // Schedulle next transmission
  m_sendEvent = Simulator::Schedule (MilliSeconds (entry->timeToSend),
                                     &RealTimeVideoServer::SendStream, this);
}

void
RealTimeVideoServer::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  // Create the packet and add the seq header without increasing packet size
  uint32_t packetSize = 0;
  if (size > 12)
    {
      packetSize = size - 12;
    }
  Ptr<Packet> packet = Create<Packet> (packetSize);

  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  // Send the packet
  int bytes = m_socket->Send (packet);
  if (bytes == (int)packet->GetSize ())
    {
      NS_LOG_DEBUG ("Real-time video TX " << bytes << " bytes " <<
                    "Sequence " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("Real-time video TX error.");
    }
}

} // Namespace ns3
