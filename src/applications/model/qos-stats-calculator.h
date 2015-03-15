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

#ifndef QOS_STATS_CALCULATOR_H
#define QOS_STATS_CALCULATOR_H

#include "packet-loss-counter.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"

namespace ns3 {
  
/**
 * \ingroup applications
 *
 * This class monitors some basic QoS statistics in a network traffic flow. It
 * counts the number of received bytes and packets, estimate the number of lost
 * packets using a window-base approach, and computes the average delay and
 * jitter.
 */
class QosStatsCalculator
{
public:
  /**
   * Default constructor
   * \param bitmapSize The window size used for checking loss.
   */
  QosStatsCalculator (uint16_t bitmapSize);
  ~QosStatsCalculator (); //!< Default destructor
  
  /**
   * Returns the size of the window used for checking loss.
   * \return The window size.
   */
  uint16_t GetPacketWindowSize () const;

  /**
   * Set the size of the window used for checking loss.
   * \param The window size. This value should be a multiple of 8.
   */
  void SetPacketWindowSize (uint16_t size);
  
  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();
  
  /**
   * Update stats using information from a new received packet.
   * \param seqNum The sequence number for this packet.
   * \param timestamp The timestamp when this packet was sent.
   * \param rxBytes The total number of bytes in this packet.
   */
  void NotifyReceived (uint32_t seqNum, Time timestamp, uint32_t rxBytes);

  /**
   * Get QoS statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime   (void) const;
  uint32_t  GetLostPackets  (void) const;
  double    GetLossRatio    (void) const;
  uint32_t  GetRxPackets    (void) const;
  uint32_t  GetRxBytes      (void) const;
  Time      GetRxDelay      (void) const;
  Time      GetRxJitter     (void) const;
  DataRate  GetRxThroughput (void) const;
  //\}

private:
  PacketLossCounter m_lossCounter;      //!< Lost packet counter
  uint32_t          m_rxPackets;        //!< Number of received packets
  uint32_t          m_rxBytes;          //!< Number of RX bytes
  Time              m_previousRx;       //!< Previous Rx time
  Time              m_previousRxTx;     //!< Previous Rx or Tx time
  int64_t           m_jitter;           //!< Jitter estimation
  Time              m_delaySum;         //!< Sum of packet delays
  Time              m_lastResetTime;    //!< Last reset time
};

} // namespace ns3
#endif /* QOS_STATS_CALCULATOR_H */
