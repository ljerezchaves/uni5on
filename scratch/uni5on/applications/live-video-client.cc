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

#include <ns3/seq-ts-header.h>
#include "live-video-client.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[" << GetAppName ()                       \
            << " client teid " << GetTeidHex () << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LiveVideoClient");
NS_OBJECT_ENSURE_REGISTERED (LiveVideoClient);

TypeId
LiveVideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LiveVideoClient")
    .SetParent<BaseClient> ()
    .AddConstructor<LiveVideoClient> ()
    .AddAttribute ("MaxPayloadSize",
                   "The maximum payload size of packets [bytes].",
                   UintegerValue (1400),
                   MakeUintegerAccessor (&LiveVideoClient::m_pktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (std::string ()),
                   MakeStringAccessor (&LiveVideoClient::LoadTrace),
                   MakeStringChecker ())
  ;
  return tid;
}

LiveVideoClient::LiveVideoClient ()
  : m_sendEvent (EventId ()),
  m_stopEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

LiveVideoClient::~LiveVideoClient ()
{
  NS_LOG_FUNCTION (this);
}

void
LiveVideoClient::Start ()
{
  NS_LOG_FUNCTION (this);

  // Schedule the ForceStop method to stop traffic based on traffic length.
  Time stop = GetTrafficLength ();
  m_stopEvent = Simulator::Schedule (stop, &LiveVideoClient::ForceStop, this);
  NS_LOG_INFO ("Set traffic length to " << stop.GetSeconds () << "s.");

  // Chain up to reset statistics, notify server, and fire start trace source.
  BaseClient::Start ();

  // Start traffic.
  m_sendEvent.Cancel ();
  m_currentEntry = 0;
  SendStream ();
}

void
LiveVideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();
  m_entries.clear ();
  BaseClient::DoDispose ();
}

void
LiveVideoClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Cancel (possible) pending stop event and stop the traffic.
  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();

  // Chain up to notify server.
  BaseClient::ForceStop ();

  // Notify the stopped application one second later.
  Simulator::Schedule (Seconds (1), &LiveVideoClient::NotifyStop, this, false);
}

void
LiveVideoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_serverAddress));
  m_socket->SetRecvCallback (
    MakeCallback (&LiveVideoClient::ReadPacket, this));
}

void
LiveVideoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }
}

void
LiveVideoClient::LoadTrace (std::string filename)
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

void
LiveVideoClient::SendStream (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  if (m_entries.empty ())
    {
      NS_LOG_WARN ("No trace file defined.");
      return;
    }

  struct TraceEntry *entry = &m_entries[m_currentEntry];
  NS_LOG_DEBUG ("Frame no. " << m_currentEntry <<
                " with " << entry->packetSize << " bytes");
  do
    {
      for (uint32_t i = 0; i < entry->packetSize / m_pktSize; i++)
        {
          SendPacket (m_pktSize);
        }
      uint16_t sizeToSend = entry->packetSize % m_pktSize;
      SendPacket (sizeToSend);

      m_currentEntry++;
      m_currentEntry %= m_entries.size ();
      entry = &m_entries[m_currentEntry];
    }
  while (entry->timeToSend == 0);

  // Schedulle next transmission.
  m_sendEvent = Simulator::Schedule (MilliSeconds (entry->timeToSend),
                                     &LiveVideoClient::SendStream, this);
}

void
LiveVideoClient::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);
  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("Client TX packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("Client TX error.");
    }
}

void
LiveVideoClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  NotifyRx (packet->GetSize ());
  NS_LOG_DEBUG ("Client RX packet with " << packet->GetSize () << " bytes.");
}

} // Namespace ns3
