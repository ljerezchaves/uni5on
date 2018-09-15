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
#include "app-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AppStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (AppStatsCalculator);

AppStatsCalculator::AppStatsCalculator ()
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

AppStatsCalculator::~AppStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
AppStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AppStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<AppStatsCalculator> ()
  ;
  return tid;
}

void
AppStatsCalculator::ResetCounters (void)
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
AppStatsCalculator::NotifyTx (uint32_t txBytes)
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
AppStatsCalculator::NotifyRx (uint32_t rxBytes, Time timestamp)
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

Time
AppStatsCalculator::GetActiveTime (void) const
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
AppStatsCalculator::GetLostPackets (void) const
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
AppStatsCalculator::GetLossRatio (void) const
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
AppStatsCalculator::GetTxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txPackets;
}

uint32_t
AppStatsCalculator::GetTxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txBytes;
}

uint32_t
AppStatsCalculator::GetRxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxPackets;
}

uint32_t
AppStatsCalculator::GetRxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxBytes;
}

Time
AppStatsCalculator::GetRxDelay (void) const
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
AppStatsCalculator::GetRxJitter (void) const
{
  NS_LOG_FUNCTION (this);

  return Time (m_jitter);
}

DataRate
AppStatsCalculator::GetRxThroughput (void) const
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

std::string
AppStatsCalculator::PrintHeader (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  // FIXME
  std::ostringstream str;
  str << setw (11) << "Active(s)"
      << setw (11) << "Delay(ms)"
      << setw (12) << "Jitter(ms)"
      << setw (8)  << "TxPkts"
      << setw (8)  << "RxPkts"
      << setw (9)  << "Loss(%)"
      << setw (10) << "RxBytes"
      << setw (12) << "Thp(Kbps)";
  return str.str ();
}

void
AppStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

std::ostream & operator << (std::ostream &os, const AppStatsCalculator &stats)
{
  // FIXME
  os << setw (11) << stats.GetActiveTime ().GetSeconds ()
     << setw (11) << stats.GetRxDelay ().GetSeconds () * 1000
     << setw (12) << stats.GetRxJitter ().GetSeconds () * 1000
     << setw (8)  << stats.GetTxPackets ()
     << setw (8)  << stats.GetRxPackets ()
     << setw (9)  << stats.GetLossRatio () * 100
     << setw (10) << stats.GetRxBytes ()
     << setw (12) << static_cast<double> (stats.GetRxThroughput ().GetBitRate ()) / 1000;
  return os;
}

} // Namespace ns3