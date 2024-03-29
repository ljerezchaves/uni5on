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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef TRAFFIC_STATS_CALCULATOR_H
#define TRAFFIC_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "flow-stats-calculator.h"
#include "../logical/epc-gtpu-tag.h"

namespace ns3 {

class RoutingInfo;
class Uni5onClient;

/**
 * \ingroup uni5on
 * \defgroup uni5onStats Statistics
 * Statistics calculators for monitoring the UNI5ON architecture.
 */

/**
 * \ingroup uni5onStats
 * This class monitors the network traffic at application L7 level and also at
 * L2 OpenFlow link level for traffic within the LTE backhaul.
 */
class TrafficStatsCalculator : public Object
{
public:
  TrafficStatsCalculator ();          //!< Default constructor.
  virtual ~TrafficStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Dump statistics into file.
   * Trace sink fired when application traffic stops.
   * \param context Context information.
   * \param app The client application.
   */
  void DumpStatistics (std::string context, Ptr<Uni5onClient> app);

  /**
   * Reset internal counters.
   * Trace sink fired when application traffic starts.
   * \param context Context information.
   * \param app The client application.
   */
  void ResetCounters (std::string context, Ptr<Uni5onClient> app);

  /**
   * Trace sink fired when a packet is dropped while exceeding pipeline load
   * capacity.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void OverloadDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when a packets is dropped by meter band.
   * \param context Context information.
   * \param packet The dropped packet.
   * \param meterId The meter ID that dropped the packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet,
                        uint32_t meterId);

  /**
   * Trace sink fired when a packet is dropped by OpenFlow port queues.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void QueueDropPacket (std::string context, Ptr<const Packet> packet);


  /**
   * Trace sink fired when an unmatched packets is dropped by a flow table.
   * \param context Context information.
   * \param packet The dropped packet.
   * \param tableId The flow table ID that dropped the packet.
   */
  void TableDropPacket (std::string context, Ptr<const Packet> packet,
                        uint8_t tableId);

  /**
   * Trace sink fired when a packet enters the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcInputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when a packet leaves the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcOutputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Classify the downlink packet as in the P-GWu TFT logical port.
   * \param packet The packet to classify.
   * \return The TEID for this packet.
   */
  uint32_t PgwTftClassify (Ptr<const Packet> packet);

  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * \param teid The GTP tunnel id.
   * \param dir The traffic direction.
   * \return The QoS information.
   */
  Ptr<FlowStatsCalculator> GetFlowStats (uint32_t teid, Direction dir);

  std::string               m_appFilename;  //!< AppStats filename.
  Ptr<OutputStreamWrapper>  m_appWrapper;   //!< AppStats file wrapper.
  std::string               m_epcFilename;  //!< EpcStats filename.
  Ptr<OutputStreamWrapper>  m_epcWrapper;   //!< EpcStats file wrapper.

  /** A pair of FlowStatsCalculator, one for each traffic direction. */
  struct FlowStatsPair
  {
    Ptr<FlowStatsCalculator> flowStats [N_DIRECTIONS];
  };

  /** A Map saving GTP TEID / EPC stats pair. */
  typedef std::map<uint32_t, FlowStatsPair> TeidFlowStatsMap_t;
  TeidFlowStatsMap_t        m_qosByTeid;    //!< TEID EPC statistics.
};

} // namespace ns3
#endif /* TRAFFIC_STATS_CALCULATOR_H */
