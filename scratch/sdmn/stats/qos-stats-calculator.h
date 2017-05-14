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

#ifndef SDMN_QOS_STATS_CALCULATOR_H
#define SDMN_QOS_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

/**
 * \ingroup sdmn
 * \defgroup sdmnStats Statistics
 * Statistics calculators for monitorin the SDMN architecture.
 */
/**
 * \ingroup sdmnStats
 * This class monitors some basic QoS statistics in a network traffic flow. It
 * counts the number of transmitted/received bytes and packets, computes the
 * loss ratio, the average delay and the jitter. This class can be used to
 * monitor statistics at application and network level, but keep in mind that
 * it is not aware of duplicated of fragmented packets at lower levels.
 */
class QosStatsCalculator : public Object
{
public:
  QosStatsCalculator ();          //!< Default constructor.
  virtual ~QosStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Reset all internal counters.
   */
  void ResetCounters ();

  /**
   * Update TX counters for a new transmitted packet.
   * \param txBytes The total number of bytes in this packet.
   * \return The next TX sequence number to use.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counters for a new received packet.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  /**
   * Increase the meter dropped packet counter by one.
   */
  void NotifyLoadDrop ();

  /**
   * Increase the meter dropped packet counter by one.
   */
  void NotifyMeterDrop ();

  /**
   * Increase the queue dropped packet counter by one.
   */
  void NotifyQueueDrop ();

  /**
   * Get QoS statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime   (void) const;
  uint32_t  GetLostPackets  (void) const;
  double    GetLossRatio    (void) const;
  uint32_t  GetTxPackets    (void) const;
  uint32_t  GetTxBytes      (void) const;
  uint32_t  GetRxPackets    (void) const;
  uint32_t  GetRxBytes      (void) const;
  Time      GetRxDelay      (void) const;
  Time      GetRxJitter     (void) const;
  DataRate  GetRxThroughput (void) const;
  uint32_t  GetLoadDrops    (void) const;
  uint32_t  GetMeterDrops   (void) const;
  uint32_t  GetQueueDrops   (void) const;
  //\}

  /**
   * TracedCallback signature for QosStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (*QosStatsCallback)(Ptr<const QosStatsCalculator> stats);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  uint32_t           m_txPackets;        //!< Number of TX packets.
  uint32_t           m_txBytes;          //!< Number of TX bytes.
  uint32_t           m_rxPackets;        //!< Number of RX packets.
  uint32_t           m_rxBytes;          //!< Number of RX bytes.
  Time               m_firstTxTime;      //!< First TX time.
  Time               m_firstRxTime;      //!< First RX time.
  Time               m_lastRxTime;       //!< Last RX time.
  Time               m_lastTimestamp;    //!< Last timestamp.
  int64_t            m_jitter;           //!< Jitter estimation.
  Time               m_delaySum;         //!< Sum of packet delays.

  // Fields used by EPC network monitoring.
  uint32_t           m_loadDrop;         //!< Counter for drops by pipe load.
  uint32_t           m_meterDrop;        //!< Counter for drops by meter rules.
  uint32_t           m_queueDrop;        //!< Counter for drops by queues.
};

} // namespace ns3
#endif /* SDMN_QOS_STATS_CALCULATOR_H */
