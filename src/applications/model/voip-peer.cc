/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2014 University of Campinas (Unicamp)
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
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "voip-peer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VoipPeer");

NS_OBJECT_ENSURE_REGISTERED (VoipPeer);

TypeId
VoipPeer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VoipPeer")
    .SetParent<Application> ()
    .AddConstructor<VoipPeer> ()
    .AddAttribute ("PeerAddress",
                   "The IPv4 destination address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&VoipPeer::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("PeerPort", 
                   "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipPeer::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("LocalPort",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&VoipPeer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of packets (in bytes). "
                   "Choose between 40, 50 and 60 bytes.",
                   UintegerValue (60),
                   MakeUintegerAccessor (&VoipPeer::m_pktSize),
                   MakeUintegerChecker<uint32_t> (40, 60))
    .AddAttribute ("Interval",
                   "The time to wait between consecutive packets.",
                   TimeValue (Seconds (0.06)),
                   MakeTimeAccessor (&VoipPeer::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("LossWindowSize",
                   "The size of the window used to compute the packet loss. " 
                   "This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&VoipPeer::GetPacketWindowSize,
                                         &VoipPeer::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
    .AddAttribute ("OnTime", 
                  "A RandomVariableStream used to pick the 'ON' state duration.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VoipPeer::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", 
                  "A RandomVariableStream used to pick the 'Off' state duration.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                   MakePointerAccessor (&VoipPeer::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("Stream",
                   "The stream number for RNG streams. "
                   "-1 means \"allocate a stream automatically\".",
                   IntegerValue(-1),
                   MakeIntegerAccessor(&VoipPeer::SetStreams),
                   MakeIntegerChecker<int64_t>())
    ;
  return tid;
}

VoipPeer::VoipPeer ()
  : m_peerApp (0),
    m_txSocket (0),
    m_rxSocket (0),
    m_connected (false),
    m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_startStopEvent = EventId ();
  m_lastStartTime = Time ();
  ResetCounters ();
}

VoipPeer::~VoipPeer ()
{
  NS_LOG_FUNCTION (this);
}

void
VoipPeer::SetPeerAddress (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
VoipPeer::SetPeerApp (Ptr<VoipPeer> peer)
{
  NS_LOG_FUNCTION (this << peer);
  m_peerApp = peer;
}

void
VoipPeer::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

void
VoipPeer::SetStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
}

uint16_t
VoipPeer::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

Ptr<VoipPeer> 
VoipPeer::GetPeerApp ()
{
  return m_peerApp;
}

void
VoipPeer::ResetCounters ()
{
  m_pktSent = 0;
  m_pktReceived = 0;
  m_txBytes = 0;
  m_rxBytes = 0;
  m_previousRx = Simulator::Now ();
  m_previousRxTx = Simulator::Now ();
  m_lastStartTime = Simulator::Now ();
  m_jitter = 0;
  m_delaySum = Time ();
  m_lossCounter.Reset ();
}

uint32_t  
VoipPeer::GetTxPackets (void) const
{
  return m_pktSent;
}

uint32_t  
VoipPeer::GetRxPackets (void) const
{
  return m_pktReceived;
}

uint32_t  
VoipPeer::GetTxBytes (void) const
{
  return m_txBytes;
}

uint32_t  
VoipPeer::GetRxBytes (void) const
{
  return m_rxBytes;
}

double
VoipPeer::GetLossRatio (void) const
{
  uint32_t lost = m_lossCounter.GetLost ();
  return ((double)lost) / (lost + GetRxPackets ());
}

Time      
VoipPeer::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastStartTime;
}

Time      
VoipPeer::GetDelay (void) const
{
  return m_pktReceived ? (m_delaySum / (int64_t)m_pktReceived) : m_delaySum;
}

Time      
VoipPeer::GetJitter (void) const
{
  return Time (m_jitter);
}

void
VoipPeer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  if (m_rxSocket != 0)
    {
      m_rxSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  m_peerApp = 0;
  m_txSocket = 0;
  m_rxSocket = 0;
  m_onTime = 0;
  m_offTime = 0;
  Application::DoDispose ();
}

void
VoipPeer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // Inbound side
  if (m_rxSocket == 0)
    {
      m_rxSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_rxSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_rxSocket->SetRecvCallback (MakeCallback (&VoipPeer::ReadPacket, this));
    }

  // Outbound side
  if (m_txSocket == 0)
    {
      m_txSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_txSocket->Bind ();
      m_txSocket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
      m_txSocket->ShutdownRecv ();
      m_txSocket->SetConnectCallback (
          MakeCallback (&VoipPeer::ConnectionSucceeded, this),
          MakeCallback (&VoipPeer::ConnectionFailed, this));
      m_txSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  CancelEvents ();
  ScheduleStartEvent ();
}

void
VoipPeer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  CancelEvents ();
  
  if (m_txSocket != 0)
    {
      m_txSocket->Close ();
      m_txSocket = 0;
    }

  // We won't stop de inbounding server side, so 
  // transient packets can get here even after stop.
}

void 
VoipPeer::CancelEvents ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

void 
VoipPeer::StartSending ()
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
  m_lastStartTime = Simulator::Now ();
  m_sendEvent = Simulator::Schedule (m_interval, &VoipPeer::SendPacket, this);
  ScheduleStopEvent ();
}

void 
VoipPeer::StopSending ()
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
VoipPeer::ScheduleStartEvent ()
{  
  NS_LOG_FUNCTION (this);
  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("VoIP " << this << " will start in +" << offInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (offInterval, &VoipPeer::StartSending, this);
}

void 
VoipPeer::ScheduleStopEvent ()
{  
  NS_LOG_FUNCTION (this);
  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("VoIP " << this << " will stop in +" << onInterval.GetSeconds ());
  m_startStopEvent = Simulator::Schedule (onInterval, &VoipPeer::StopSending, this);
}

void 
VoipPeer::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void 
VoipPeer::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void
VoipPeer::SendPacket ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  SeqTsHeader seqTs;
  seqTs.SetSeq (m_pktSent);

  // Using compressed IP/UDP/RTP header. 
  // 38 bytes must be removed from payload.
  Ptr<Packet> p = Create<Packet> (m_pktSize - (38));
  p->AddHeader (seqTs);
  m_txBytes += p->GetSize ();
 
  if (m_txSocket->Send (p))
    {
      m_pktSent++;
      NS_LOG_INFO ("VoIP TX " << m_pktSize <<
                   " bytes to " << m_peerAddress << 
                   ":" << m_peerPort << 
                   " Uid " << p->GetUid () << 
                   " Time " << (Simulator::Now ()).GetSeconds ());
    }
  else
    {
      NS_LOG_INFO ("Error sending VoIP " << m_pktSize << 
                   " bytes to " << m_peerAddress);
    }
  m_sendEvent = Simulator::Schedule (m_interval, &VoipPeer::SendPacket, this);
}

void
VoipPeer::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t seqNum = seqTs.GetSeq ();
          if (InetSocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << seqNum <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
            }

          // Updating counter and statistics
          // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
          Time delay = Simulator::Now () - seqTs.GetTs ();
          Time delta = (Simulator::Now () - m_previousRx) - (seqTs.GetTs () - m_previousRxTx);
          m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
          m_previousRx = Simulator::Now ();
          m_previousRxTx = seqTs.GetTs ();
          m_delaySum += delay;

          m_lossCounter.NotifyReceived (seqNum);
          m_pktReceived++;
          m_rxBytes += packet->GetSize ();
        }
    }
}

} // Namespace ns3
