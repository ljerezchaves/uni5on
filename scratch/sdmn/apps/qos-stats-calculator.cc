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

#include "qos-stats-calculator.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosStatsCalculator");

QosStatsCalculator::QosStatsCalculator ()
  : m_lossCounter (0),
    m_windowSize (32),
    m_rxPackets (0),
    m_rxBytes (0),
    m_firstRxTime (Simulator::Now ()),
    m_lastRxTime (Simulator::Now ()),
    m_lastTimestamp (Simulator::Now ()),
    m_jitter (0),
    m_delaySum (Time ()),
    m_seqNum (0),
    m_meterDrop (0),
    m_queueDrop (0)
{
  NS_LOG_FUNCTION (this);
  m_lossCounter = new PacketLossCounter (m_windowSize);
}

QosStatsCalculator::~QosStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
  delete m_lossCounter;
}

uint16_t
QosStatsCalculator::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_windowSize;
}

void
QosStatsCalculator::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_windowSize = size;
  m_lossCounter->SetBitMapSize (m_windowSize);
}

void
QosStatsCalculator::ResetCounters ()
{
  delete m_lossCounter;
  m_lossCounter = new PacketLossCounter (m_windowSize);

  m_rxPackets = 0;
  m_rxBytes = 0;
  m_firstRxTime = Simulator::Now ();
  m_lastRxTime = Simulator::Now ();
  m_lastTimestamp = Simulator::Now ();
  m_jitter = 0;
  m_delaySum = Time ();

  m_seqNum = 0;
  m_meterDrop = 0;
  m_queueDrop = 0;
}

uint32_t
QosStatsCalculator::GetNextSeqNum ()
{
  return m_seqNum++;
}

void
QosStatsCalculator::NotifyReceived (uint32_t seqNum, Time timestamp,
                                    uint32_t rxBytes)
{
  // Check for the first packet received
  Time now = Simulator::Now ();
  if (m_rxPackets == 0)
    {
      m_firstRxTime = now;
    }

  // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
  Time delta = (now - m_lastRxTime) - (timestamp - m_lastTimestamp);
  m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
  m_lastRxTime = now;
  m_lastTimestamp = timestamp;

  // Updating delay, byte and packet counter
  Time delay = now - timestamp;
  m_delaySum += delay;
  m_rxPackets++;
  m_rxBytes += rxBytes;

  // Notify packet loss counter
  m_lossCounter->NotifyReceived (seqNum);
}

void
QosStatsCalculator::NotifyMeterDrop ()
{
  m_meterDrop++;
}

void
QosStatsCalculator::NotifyQueueDrop ()
{
  m_queueDrop++;
}

Time
QosStatsCalculator::GetActiveTime (void) const
{
  if (m_rxPackets > 1)
    {
      return m_lastRxTime - m_firstRxTime;
    }
  else
    {
      return Time ();
    }
}

uint32_t
QosStatsCalculator::GetLostPackets (void) const
{
  // Workaround for lost packets not yet identified
  // by the PacketLossCounter packet window.
  uint32_t lostPkts = m_lossCounter->GetLost ();
  uint32_t dropPkts = m_meterDrop + m_queueDrop;
  return lostPkts < dropPkts ? dropPkts : lostPkts;
}

double
QosStatsCalculator::GetLossRatio (void) const
{
  uint32_t lostPkts = GetLostPackets ();
  uint32_t txPkts = lostPkts + GetRxPackets ();
  return txPkts ? (double)lostPkts / txPkts : 0.;
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
  return m_rxPackets > 1 ? (m_delaySum / (int64_t)m_rxPackets) : m_delaySum;
}

Time
QosStatsCalculator::GetRxJitter (void) const
{
  return Time (m_jitter);
}

DataRate
QosStatsCalculator::GetRxThroughput (void) const
{
  return m_rxPackets > 1 ?
         DataRate (GetRxBytes () * 8 / GetActiveTime ().GetSeconds ()) :
         DataRate (0);
}

uint32_t
QosStatsCalculator::GetMeterDrops (void) const
{
  return m_meterDrop;
}

uint32_t
QosStatsCalculator::GetQueueDrops (void) const
{
  return m_queueDrop;
}

} // Namespace ns3
