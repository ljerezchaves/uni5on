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

#ifndef FLOW_STATS_CALCULATOR_H
#define FLOW_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

// Total number of drop reasons + 1 for aggregated metadata.
#define N_REASONS_ALL (static_cast<uint8_t> (DropReason::ALL) + 1)

namespace ns3 {

/**
 * \ingroup svelteStats
 * This class monitors basic QoS statistics at link level in the OpenFlow
 * backhaul network. This class monitors some basic QoS statistics of a traffic
 * flow. It counts the number of transmitted, received and dropped bytes and
 * packets. It computes the loss ratio, the average delay, and the jitter.
 */
class FlowStatsCalculator : public Object
{
public:
  /** Reason for packet drops at OpenFlow backhaul network. */
  enum DropReason
  {
    PLOAD = 0,    //!< Switch pipeline capacity overloaded.
    METER = 1,    //!< EPC bearer MBR meter.
    SLICE = 2,    //!< OpenFlow EPC infrastructure slicing.
    QUEUE = 3,    //!< Network device queues.
    ALL   = 4     //!< ALL previous reasons.
  };

  FlowStatsCalculator ();          //!< Default constructor.
  virtual ~FlowStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Reset all internal counters.
   */
  void ResetCounters (void);

  /**
   * Update TX counters for a new transmitted packet.
   * \param txBytes The total number of bytes in this packet.
   * \return The counter of TX packets.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counters for a new received packet.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  /**
   * Update drop counters for a new dropped packet.
   * \param dpBytes The total number of bytes in this packet.
   * \param reason The drop reason.
   */
  void NotifyDrop (uint32_t dpBytes, DropReason reason);

  /**
   * Get QoS statistics.
   * \param reason The drop reason.
   * \return The statistic value.
   */
  //\{
  uint64_t  GetDpBytes      (DropReason reason) const;
  uint64_t  GetDpPackets    (DropReason reason) const;
  Time      GetActiveTime   (void) const;
  uint64_t  GetLostPackets  (void) const;
  double    GetLossRatio    (void) const;
  uint64_t  GetTxPackets    (void) const;
  uint64_t  GetTxBytes      (void) const;
  uint64_t  GetRxPackets    (void) const;
  uint64_t  GetRxBytes      (void) const;
  Time      GetRxDelay      (void) const;
  Time      GetRxJitter     (void) const;
  DataRate  GetRxThroughput (void) const;
  //\}

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * TracedCallback signature for FlowStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (*FlowStatsCallback)(Ptr<const FlowStatsCalculator> stats);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  uint64_t  m_dpPackets [N_REASONS_ALL];  //!< Number of dropped packets.
  uint64_t  m_dpBytes [N_REASONS_ALL];    //!< Number of dropped bytes.
  uint64_t  m_txPackets;                  //!< Number of TX packets.
  uint64_t  m_txBytes;                    //!< Number of TX bytes.
  uint64_t  m_rxPackets;                  //!< Number of RX packets.
  uint64_t  m_rxBytes;                    //!< Number of RX bytes.
  Time      m_firstTxTime;                //!< First TX time.
  Time      m_firstRxTime;                //!< First RX time.
  Time      m_lastRxTime;                 //!< Last RX time.
  Time      m_lastTimestamp;              //!< Last timestamp.
  int64_t   m_jitter;                     //!< Jitter estimation.
  Time      m_delaySum;                   //!< Sum of packet delays.
};

/**
 * Print the traffic flow QoS metadata on an output stream.
 * \param os The output stream.
 * \param stats The FlowStatsCalculator object.
 * \returns The output stream.
 * \internal Keep this method consistent with the
 *           FlowStatsCalculator::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const FlowStatsCalculator &stats);

} // namespace ns3
#endif /* FLOW_STATS_CALCULATOR_H */
