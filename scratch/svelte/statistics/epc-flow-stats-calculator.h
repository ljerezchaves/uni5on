/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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

#ifndef EPC_FLOW_STATS_CALCULATOR_H
#define EPC_FLOW_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "flow-stats-calculator.h"

// Total number of drop reasons + 1 for aggregated metadata.
#define N_REASONS_ALL (static_cast<uint8_t> (DropReason::ALL) + 1)

namespace ns3 {

/**
 * \ingroup svelteStats
 * This class extends the FlowStatsCalculator to monitor basic QoS statistics
 * at link level in the OpenFlow EPC network, including packet drops.
 */
class EpcFlowStatsCalculator : public FlowStatsCalculator
{
public:
  /** Reason for packet drops at OpenFlow EPC network. */
  enum DropReason
  {
    PLOAD = 0,    //!< Switch pipeline capacity overloaded.
    METER = 1,    //!< EPC bearer MBR meter.
    SLICE = 2,    //!< OpenFlow EPC infrastructure slicing.
    QUEUE = 3,    //!< Network device queues.
    ALL   = 4     //!< ALL previous reasons.
  };

  EpcFlowStatsCalculator ();          //!< Default constructor.
  virtual ~EpcFlowStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  // Inherited from FlowStatsCalculator.
  void ResetCounters (void);

  /**
   * Update drop counters for a new dropped packet.
   * \param dpBytes The total number of bytes in this packet.
   * \param reason The drop reason.
   */
  void NotifyDrop (uint32_t dpBytes, DropReason reason);

  /**
   * Get drop statistics.
   * \param reason The drop reason.
   * \return The statistic value.
   */
  //\{
  uint32_t GetDpBytes   (DropReason reason) const;
  uint32_t GetDpPackets (DropReason reason) const;
  //\}

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  uint32_t  m_dpPackets [N_REASONS_ALL]; //!< Number of dropped packets.
  uint32_t  m_dpBytes [N_REASONS_ALL];   //!< Number of dropped bytes.
};

/**
 * Print the EPC QoS metadata on an output stream.
 * \param os The output stream.
 * \param stats The EpcFlowStatsCalculator object.
 * \returns The output stream.
 * \internal Keep this method consistent with the
 *           EpcFlowStatsCalculator::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os,
                            const EpcFlowStatsCalculator &stats);

} // namespace ns3
#endif /* EPC_FLOW_STATS_CALCULATOR_H */
