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

#include <iomanip>
#include <iostream>
#include "flow-stats-calculator.h"
#include "../svelte-common.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (FlowStatsCalculator);

FlowStatsCalculator::FlowStatsCalculator ()
  : m_txPackets (0),
  m_txBytes (0),
  m_rxPackets (0),
  m_rxBytes (0),
  m_firstTxTime (Simulator::Now ()),
  m_firstRxTime (Simulator::Now ()),
  m_lastRxTime (Simulator::Now ()),
  m_lastTimestamp (Simulator::Now ()),
  m_jitter (0),
  m_delaySum (Time ())
{
  NS_LOG_FUNCTION (this);
}

FlowStatsCalculator::~FlowStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
FlowStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<FlowStatsCalculator> ()
  ;
  return tid;
}

void
FlowStatsCalculator::ResetCounters (void)
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
}

uint32_t
FlowStatsCalculator::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  // Check for the first TX packet.
  if (!m_txPackets)
    {
      m_firstTxTime = Simulator::Now ();
    }

  m_txPackets++;
  m_txBytes += txBytes;
  return m_txPackets;
}

void
FlowStatsCalculator::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  Time now = Simulator::Now ();

  // Check for the first RX packet.
  if (!m_rxPackets)
    {
      m_firstRxTime = now;
    }

  m_rxPackets++;
  m_rxBytes += rxBytes;

  // Updating the jitter estimation.
  // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
  Time delta = (now - m_lastRxTime) - (timestamp - m_lastTimestamp);
  m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
  m_lastRxTime = now;
  m_lastTimestamp = timestamp;

  // Updating the delay sum.
  m_delaySum += (now - timestamp);
}

Time
FlowStatsCalculator::GetActiveTime (void) const
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
FlowStatsCalculator::GetLostPackets (void) const
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
FlowStatsCalculator::GetLossRatio (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetLostPackets ())
    {
      return static_cast<double> (GetLostPackets ()) / GetTxPackets ();
    }
  else
    {
      return 0.0;
    }
}

uint32_t
FlowStatsCalculator::GetTxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txPackets;
}

uint32_t
FlowStatsCalculator::GetTxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txBytes;
}

uint32_t
FlowStatsCalculator::GetRxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxPackets;
}

uint32_t
FlowStatsCalculator::GetRxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxBytes;
}

Time
FlowStatsCalculator::GetRxDelay (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetRxPackets ())
    {
      return m_delaySum / static_cast<int64_t> (GetRxPackets ());
    }
  else
    {
      return m_delaySum;
    }
}

Time
FlowStatsCalculator::GetRxJitter (void) const
{
  NS_LOG_FUNCTION (this);

  return Time (m_jitter);
}

DataRate
FlowStatsCalculator::GetRxThroughput (void) const
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

std::ostream &
FlowStatsCalculator::PrintHeader (std::ostream &os)
{
  os << " " << setw (8) << "ActvSec"
     << " " << setw (7) << "DlyMsec"
     << " " << setw (7) << "JitMsec"
     << " " << setw (7) << "TxPkts"
     << " " << setw (7) << "RxPkts"
     << " " << setw (7) << "LossRat"
     << " " << setw (8) << "RxBytes"
     << " " << setw (9) << "ThpKbps";
  return os;
}

void
FlowStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

std::ostream & operator << (std::ostream &os, const FlowStatsCalculator &stats)
{
  os << " " << setw (8) << stats.GetActiveTime ().GetSeconds ()
     << " " << setw (7) << stats.GetRxDelay ().GetSeconds () * 1000
     << " " << setw (7) << stats.GetRxJitter ().GetSeconds () * 1000
     << " " << setw (7) << stats.GetTxPackets ()
     << " " << setw (7) << stats.GetRxPackets ()
     << " " << setw (7) << stats.GetLossRatio () * 100
     << " " << setw (8) << stats.GetRxBytes ()
     << " " << setw (9) << Bps2Kbps (stats.GetRxThroughput ().GetBitRate ());
  return os;
}

} // Namespace ns3
