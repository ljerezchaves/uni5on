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

#ifndef TRAFFIC_STATS_CALCULATOR_H
#define TRAFFIC_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "../applications/app-stats-calculator.h"
#include "../logical/epc-gtpu-tag.h"

// Total number of drop reasons + 1 for aggregated metadata.
#define N_REASONS_ALL (static_cast<uint8_t> (DropReason::ALL) + 1)

namespace ns3 {

class RoutingInfo;
class SvelteClientApp;
class UeInfo;

/**
 * \ingroup svelte
 * \defgroup svelteStats Statistics
 * Statistics calculators for monitoring the SVELTE architecture.
 */

/**
 * \ingroup svelteStats
 * This class extends the AppStatsCalculator to monitor basic QoS statistics at
 * link level in the OpenFlow EPC network, including packet drops.
 */
class EpcStatsCalculator : public AppStatsCalculator
{
public:
  /** Reason for packet drops at OpenFlow EPC network. */
  enum DropReason
  {
    SWTCH = 0,    //!< Switch pipeline capacity overloaded.
    METER = 1,    //!< EPC bearer MBR meter.
    SLICE = 2,    //!< OpenFlow EPC infrastructure slicing.
    QUEUE = 3,    //!< Network device queues.
    ALL   = 4     //!< ALL previous reasons.
  };

  EpcStatsCalculator ();          //!< Default constructor.
  virtual ~EpcStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  // Inherited from AppStatsCalculator.
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
   * \return The header string.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::string PrintHeader (void);

  /**
   * TracedCallback signature for EpcStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (*EpcStatsCallback)(Ptr<const EpcStatsCalculator> stats);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  uint32_t  m_dpPackets [N_REASONS_ALL]; //!< Number of dropped packets.
  uint32_t  m_dpBytes [N_REASONS_ALL];   //!< Number of dropped bytes.
};


/**
 * \ingroup svelteStats
 * This class monitors the traffic QoS statistics at application L7 level for
 * end-to-end traffic, and also at IP network L3 level for traffic within the
 * LTE EPC.
 */
class TrafficStatsCalculator : public Object
{
public:
  /** Traffic direction. */
  enum Direction
  {
    DLINK = 0,  //!< Downlink traffic.
    ULINK = 1   //!< Uplink traffic.
  };

  TrafficStatsCalculator ();          //!< Default constructor.
  virtual ~TrafficStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Identify the traffic direction based on the GTPU packet tag.
   * \param gtpuTag The GTPU packet tag.
   * \return The traffic direction.
   */
  Direction GetDirection (EpcGtpuTag &gtpuTag) const;

  /**
   * Get the string representing the given direction.
   * \param dir The link direction.
   * \return The link direction string.
   */
  static std::string DirectionStr (Direction dir);

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
  void DumpStatistics (std::string context, Ptr<SvelteClientApp> app);

  /**
   * Reset internal counters.
   * Trace sink fired when application traffic starts.
   * \param context Context information.
   * \param app The client application.
   */
  void ResetCounters (std::string context, Ptr<SvelteClientApp> app);

  /**
   * Trace sink fired when a packet is dropped while exceeding pipeline load
   * capacity.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void LoadDropPacket (std::string context, Ptr<const Packet> packet);

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
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * \param teid The GTP tunnel id.
   * \param dir The traffic direction.
   * \return The QoS information.
   */
  Ptr<EpcStatsCalculator> GetEpcStats (uint32_t teid, Direction dir);

  /**
   * Get the header for common statistics metrics.
   * \return The commom formatted string.
   */
  std::string GetHeader (void);

  /**
   * Get the common statistics metrics.
   * \param app The SVELTE client application.
   * \param rInfo The routing information for this traffic.
   * \param stats The QoS statistics.
   * \param direction The traffic direction (down/up).
   * \return The commom formatted string.
   */
  std::string GetStats (
    Ptr<const SvelteClientApp> app, Ptr<const AppStatsCalculator> stats,
    Ptr<const RoutingInfo> rInfo, std::string direction);

  std::string               m_appFilename;  //!< AppStats filename.
  Ptr<OutputStreamWrapper>  m_appWrapper;   //!< AppStats file wrapper.
  std::string               m_epcFilename;  //!< EpcStats filename.
  Ptr<OutputStreamWrapper>  m_epcWrapper;   //!< EpcStats file wrapper.

  /** A pair of EpcStatsCalculator, for downlink and uplink traffic. */
  struct EpcStatsPair
  {
    Ptr<EpcStatsCalculator> stats [2];
  };

  /** A Map saving GTP TEID / EPC stats pair. */
  typedef std::map<uint32_t, EpcStatsPair> TeidEpcStatsMap_t;
  TeidEpcStatsMap_t         m_qosByTeid;    //!< TEID EPC statistics.
};

/**
 * Print the EPC QoS metadata on an output stream.
 * \param os The output stream.
 * \param stats The EpcStatsCalculator object.
 * \returns The output stream.
 * \internal Keep this method consistent with the
 *           EpcStatsCalculator::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const EpcStatsCalculator &stats);

} // namespace ns3
#endif /* TRAFFIC_STATS_CALCULATOR_H */
