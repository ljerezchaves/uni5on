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

// #include "ns3/ipv4-address.h"
// #include "ns3/nstime.h"
// #include "ns3/inet-socket-address.h"
// #include "ns3/inet6-socket-address.h"
// #include "ns3/socket.h"
// #include "ns3/socket-factory.h"
// #include "ns3/packet.h"
// #include "ns3/uinteger.h"
// #include "packet-loss-counter.h"
// 
// #include "seq-ts-header.h"
// #include "udp-server.h"
#include "qos-stats-calculator.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosStatsCalculator");

QosStatsCalculator::QosStatsCalculator (uint16_t bitmapSize)
  : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this << bitmapSize);

  SetPacketWindowSize (bitmapSize);
  ResetCounters ();
}

QosStatsCalculator::~QosStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
QosStatsCalculator::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
QosStatsCalculator::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

void
QosStatsCalculator::ResetCounters ()
{
  m_rxPackets = 0;
  m_rxBytes = 0;
  m_jitter = 0;
  m_delaySum = Time ();
  m_previousRx = Simulator::Now ();
  m_previousRxTx = Simulator::Now ();
  m_lastResetTime = Simulator::Now ();
  m_lossCounter.Reset ();
}

void
QosStatsCalculator::NotifyReceived (uint32_t seqNum, Time timestamp, 
                                    uint32_t rxBytes)
{
  // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
  Time delta = (Simulator::Now () - m_previousRx) - (timestamp - m_previousRxTx);
  m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
  m_previousRx = Simulator::Now ();
  m_previousRxTx = timestamp;

  // Updating delay, byte and packet counter
  Time delay = Simulator::Now () - timestamp;
  m_delaySum += delay;
  m_rxPackets++;
  m_rxBytes += rxBytes;  
  
  // Notify packet loss counter
  m_lossCounter.NotifyReceived (seqNum);
}

Time      
QosStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

uint32_t
QosStatsCalculator::GetLostPackets (void) const
{
  return m_lossCounter.GetLost ();
}

double
QosStatsCalculator::GetLossRatio (void) const
{
  uint32_t lost = GetLostPackets ();
  return ((double)lost) / (lost + GetRxPackets ());
}

uint32_t  
QosStatsCalculator::GetRxPackets (void) const
{
  return m_rxPackets;
}

uint32_t  
QosStatsCalculator::GetRxBytes (void) const
{
  return m_rxBytes;
}

Time      
QosStatsCalculator::GetRxDelay (void) const
{
  return m_rxPackets ? (m_delaySum / (int64_t)m_rxPackets) : m_delaySum;
}

Time      
QosStatsCalculator::GetRxJitter (void) const
{
  return Time (m_jitter);
}

DataRate 
QosStatsCalculator::GetRxThroughput (void) const
{
  return DataRate (GetRxBytes () * 8 / GetActiveTime ().GetSeconds ());
}

} // Namespace ns3
