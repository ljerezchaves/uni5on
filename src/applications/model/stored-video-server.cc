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
struct StoredVideoServer::TraceEntry StoredVideoServer::g_defaultEntries[] = {
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
StoredVideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StoredVideoServer")
    .SetParent<Application> ()
    .AddConstructor<StoredVideoServer> ()
    .AddAttribute ("LocalPort",
                   "Local TCP port on which we listen for incoming connections.",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&StoredVideoServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TraceFilename",
                   "Name of file to load a trace from.",
                   StringValue (""),
                   MakeStringAccessor (&StoredVideoServer::SetTraceFile),
                   MakeStringChecker ())
    .AddAttribute ("VideoDuration",
                   "A random variable used to pick the video duration [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&StoredVideoServer::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

StoredVideoServer::StoredVideoServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_clientApp = 0;
  m_connected = false;
}

StoredVideoServer::~StoredVideoServer ()
{
  NS_LOG_FUNCTION (this);
  m_entries.clear ();
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
StoredVideoServer::SetClientApp (Ptr<StoredVideoClient> client)
{
  m_clientApp = client;
}

Ptr<StoredVideoClient>
StoredVideoServer::GetClientApp ()
{
  return m_clientApp;
}

void
StoredVideoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_clientApp = 0;
  m_lengthRng = 0;
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
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
      m_socket->Listen ();
      m_socket->SetAcceptCallback (
        MakeCallback (&StoredVideoServer::HandleRequest, this),
        MakeCallback (&StoredVideoServer::HandleAccept, this));
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
StoredVideoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket = 0;
    }
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
}

bool
StoredVideoServer::HandleRequest (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_INFO ("Request for connection from " <<
               InetSocketAddress::ConvertFrom (address).GetIpv4 () << " received.");
  return true;
}

void
StoredVideoServer::HandleAccept (Ptr<Socket> socket, const Address& address)
{
  NS_LOG_FUNCTION (this << socket << address);
  NS_LOG_INFO ("Connection with client (" <<
               InetSocketAddress::ConvertFrom (address).GetIpv4 () <<
               ") successfully established!");
  m_socket->SetSendCallback (MakeCallback (&StoredVideoServer::StartSending, this));

  m_connected = true;
  m_currentEntry = 0;
  m_lengthTime = Seconds (50 * std::abs (m_lengthRng->GetValue ()));
  m_elapsed = MilliSeconds (0);
  NS_LOG_DEBUG ("Video lenght: " << m_lengthTime.As (Time::S));

  // Start sending the stored video stream to the client.
  SendStream ();
}

void
StoredVideoServer::SendStream (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet;
  struct TraceEntry *entry;
  uint32_t toSend, available;
  int sent;
  do
    {
      entry = &m_entries[m_currentEntry];
      toSend = entry->packetSize;
      available = m_socket->GetTxAvailable ();
      if (available >= toSend)
        {
          packet = Create<Packet> (toSend);
          sent = m_socket->Send (packet);
          NS_LOG_DEBUG ("Video time: " << m_elapsed.As (Time::S) <<
                        " - " << sent << " bytes sent." << available);

          m_elapsed += MilliSeconds (entry->timeToSend);
          m_currentEntry++;
          m_currentEntry = m_currentEntry % m_entries.size ();
        }
      else
        {
          NS_LOG_DEBUG ("Buffer full! Wait...");
          break;
        }
    }
  while (m_elapsed < m_lengthTime);

}

void
StoredVideoServer::StartSending (Ptr<Socket> socket, uint32_t size)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    {
      // Only send new data if the connection has completed
      SendStream ();
    }
}

} // Namespace ns3
