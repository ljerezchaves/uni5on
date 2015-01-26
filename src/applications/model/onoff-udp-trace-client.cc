/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
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
#include "onoff-udp-trace-client.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OnOffUdpTraceClient");
NS_OBJECT_ENSURE_REGISTERED (OnOffUdpTraceClient);

/**
 * \brief Default trace to send
 */
struct OnOffUdpTraceClient::TraceEntry OnOffUdpTraceClient::g_defaultEntries[] = {
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
OnOffUdpTraceClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OnOffUdpTraceClient")
    .SetParent<Application> ()
    .AddConstructor<OnOffUdpTraceClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&OnOffUdpTraceClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&OnOffUdpTraceClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxPacketSize",
                   "The maximum size of a packet (including the SeqTsHeader, 12 bytes).",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&OnOffUdpTraceClient::m_maxPacketSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from. By default, uses a hardcoded trace.",
                   StringValue (""),
                   MakeStringAccessor (&OnOffUdpTraceClient::SetTraceFile),
                   MakeStringChecker ())
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::NormalRandomVariable[Mean=30.0,Variance=5.0]"),
                   MakePointerAccessor (&OnOffUdpTraceClient::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=50.0]"),
                   MakePointerAccessor (&OnOffUdpTraceClient::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
  ;
  return tid;
}

OnOffUdpTraceClient::OnOffUdpTraceClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_maxPacketSize = 1400;
  m_connected = false;
}

OnOffUdpTraceClient::OnOffUdpTraceClient (Ipv4Address ip, uint16_t port,
                                char *traceFile)
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_peerAddress = ip;
  m_peerPort = port;
  m_currentEntry = 0;
  m_maxPacketSize = 1400;
  m_connected = false;
  if (traceFile != NULL)
    {
      SetTraceFile (traceFile);
    }
}

OnOffUdpTraceClient::~OnOffUdpTraceClient ()
{
  NS_LOG_FUNCTION (this);
  m_entries.clear ();
}

void
OnOffUdpTraceClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_entries.clear ();
  m_peerAddress = ip;
  m_peerPort = port;
}

void
OnOffUdpTraceClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_entries.clear ();
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
OnOffUdpTraceClient::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_entries.clear ();
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
OnOffUdpTraceClient::SetTraceFile (std::string traceFile)
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
OnOffUdpTraceClient::SetMaxPacketSize (uint16_t maxPacketSize)
{
  NS_LOG_FUNCTION (this << maxPacketSize);
  m_maxPacketSize = maxPacketSize;
}


uint16_t OnOffUdpTraceClient::GetMaxPacketSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_maxPacketSize;
}


void
OnOffUdpTraceClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
OnOffUdpTraceClient::LoadTrace (std::string filename)
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
OnOffUdpTraceClient::LoadDefaultTrace (void)
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
OnOffUdpTraceClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind ();
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6 ();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&OnOffUdpTraceClient::ConnectionSucceeded, this),
        MakeCallback (&OnOffUdpTraceClient::ConnectionFailed, this));
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  CancelEvents ();
  ScheduleStartEvent ();
}

void
OnOffUdpTraceClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();
  if (m_socket != 0)
    {
      m_socket->Close ();
    }
}

void 
OnOffUdpTraceClient::CancelEvents ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

void 
OnOffUdpTraceClient::StartSending ()
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
  Send ();
  ScheduleStopEvent ();
}

void 
OnOffUdpTraceClient::StopSending ()
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
OnOffUdpTraceClient::ScheduleStartEvent ()
{  
  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("Video " << this << " will start in +" << offInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (offInterval, &OnOffUdpTraceClient::StartSending, this);
}

void 
OnOffUdpTraceClient::ScheduleStopEvent ()
{  
  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION (this);

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("Video " << this << " will stop in +" << onInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (onInterval, &OnOffUdpTraceClient::StopSending, this);
}

void
OnOffUdpTraceClient::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  Ptr<Packet> p;
  uint32_t packetSize;
  if (size>12)
    {
      packetSize = size - 12; // 12 is the size of the SeqTsHeader
    }
  else
    {
      packetSize = 0;
    }
  p = Create<Packet> (packetSize);
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  p->AddHeader (seqTs);

  std::stringstream addressString;
  if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
    {
      addressString << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
    {
      addressString << Ipv6Address::ConvertFrom (m_peerAddress);
    }
  else
    {
      addressString << m_peerAddress;
    }

  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("Sent " << size << " bytes to "
                           << addressString.str ());
    }
  else
    {
      NS_LOG_INFO ("Error while sending " << size << " bytes to "
                                          << addressString.str ());
    }
}

void
OnOffUdpTraceClient::Send (void)
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
  m_sendEvent = Simulator::Schedule (MilliSeconds (entry->timeToSend), &OnOffUdpTraceClient::Send, this);
}

void 
OnOffUdpTraceClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void 
OnOffUdpTraceClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
