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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (QosStatsCalculator);

QosStatsCalculator::QosStatsCalculator ()
  : m_txPackets (0),
    m_txBytes (0),
    m_rxPackets (0),
    m_rxBytes (0),
    m_firstTxTime (Simulator::Now ()),
    m_firstRxTime (Simulator::Now ()),
    m_lastRxTime (Simulator::Now ()),
    m_lastTimestamp (Simulator::Now ()),
    m_jitter (0),
    m_delaySum (Time ()),
    m_loadDrop (0),
    m_meterDrop (0),
    m_queueDrop (0)
{
  NS_LOG_FUNCTION (this);
}

QosStatsCalculator::~QosStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
QosStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QosStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<QosStatsCalculator> ()
  ;
  return tid;
}

void
QosStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);

  m_txPackets = 0;
  m_txBytes = 0;
  m_rxPackets = 0;
  m_rxBytes = 0;
  m_firstTxTime = Simulator::Now ();
  m_firstRxTime = Simulator::Now ();
  m_lastRxTime = Simulator::Now ();
  m_lastTimestamp = Simulator::Now ();
  m_jitter = 0;
  m_delaySum = Time ();

  m_loadDrop = 0;
  m_meterDrop = 0;
  m_queueDrop = 0;
}

uint32_t
QosStatsCalculator::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  m_txPackets++;
  m_txBytes += txBytes;

  // Check for the first TX packet
  if (m_txPackets == 1)
    {
      m_firstTxTime = Simulator::Now ();
    }

  return m_txPackets - 1;
}

void
QosStatsCalculator::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  m_rxPackets++;
  m_rxBytes += rxBytes;
  Time now = Simulator::Now ();

  // Check for the first RX packet
  if (m_rxPackets == 1)
    {
      m_firstRxTime = now;
    }

  // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
  Time delta = (now - m_lastRxTime) - (timestamp - m_lastTimestamp);
  m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
  m_lastRxTime = now;
  m_lastTimestamp = timestamp;

  // Updating delay sum
  m_delaySum += (now - timestamp);
}

void
QosStatsCalculator::NotifyLoadDrop ()
{
  NS_LOG_FUNCTION (this);

  m_loadDrop++;
}

void
QosStatsCalculator::NotifyMeterDrop ()
{
  NS_LOG_FUNCTION (this);

  m_meterDrop++;
}

void
QosStatsCalculator::NotifyQueueDrop ()
{
  NS_LOG_FUNCTION (this);

  m_queueDrop++;
}

Time
QosStatsCalculator::GetActiveTime (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetRxPackets ())
    {
      return m_lastRxTime - m_firstTxTime;
    }
  else
    {
      return Time ();
    }
}

uint32_t
QosStatsCalculator::GetLostPackets (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetTxPackets () && (GetTxPackets () >= GetRxPackets ()))
    {
      return GetTxPackets () - GetRxPackets ();
    }
  else
    {
      return 0;
    }
}

double
QosStatsCalculator::GetLossRatio (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetLostPackets ())
    {
      return (double)GetLostPackets () / GetTxPackets ();
    }
  else
    {
      return 0.0;
    }
}

uint32_t
QosStatsCalculator::GetTxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txPackets;
}

uint32_t
QosStatsCalculator::GetTxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txBytes;
}

uint32_t
QosStatsCalculator::GetRxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxPackets;
}

uint32_t
QosStatsCalculator::GetRxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxBytes;
}

Time
QosStatsCalculator::GetRxDelay (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetRxPackets ())
    {
      return m_delaySum / (int64_t)GetRxPackets ();
    }
  else
    {
      return m_delaySum;
    }
}

Time
QosStatsCalculator::GetRxJitter (void) const
{
  NS_LOG_FUNCTION (this);

  return Time (m_jitter);
}

DataRate
QosStatsCalculator::GetRxThroughput (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetRxPackets ())
    {
      return DataRate (GetRxBytes () * 8 / GetActiveTime ().GetSeconds ());
    }
  else
    {
      return DataRate (0);
    }
}

uint32_t
QosStatsCalculator::GetLoadDrops (void) const
{
  NS_LOG_FUNCTION (this);

  return m_loadDrop;
}

uint32_t
QosStatsCalculator::GetMeterDrops (void) const
{
  NS_LOG_FUNCTION (this);

  return m_meterDrop;
}

uint32_t
QosStatsCalculator::GetQueueDrops (void) const
{
  NS_LOG_FUNCTION (this);

  return m_queueDrop;
}

void
QosStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

} // Namespace ns3
