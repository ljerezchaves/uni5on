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

#ifndef SDMN_TRAFFIC_STATS_CALCULATOR_H
#define SDMN_TRAFFIC_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

class EpcController;
class SdmnClientApp;
class QosStatsCalculator;

/**
 * \ingroup sdmnStats
 * This class monitors the traffic QoS statistics at application L7 level for
 * end-to-end traffic, and also at IP network L3 level for traffic within the
 * LTE EPC.
 */
class TrafficStatsCalculator : public Object
{
public:
  TrafficStatsCalculator ();          //!< Default constructor.
  virtual ~TrafficStatsCalculator (); //!< Default destructor.

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
  void DumpStatistics (std::string context, Ptr<const SdmnClientApp> app);

  /**
   * Reset internal counters.
   * Trace sink fired when application traffic starts.
   * \param context Context information.
   * \param app The client application.
   */
  void ResetCounters (std::string context, Ptr<const SdmnClientApp> app);

  /**
   * Trace sink fired when a packets is dropped by meter band.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet);

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
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);

  /** A pair of QosStatsCalculator, for downlink and uplink EPC statistics. */
  typedef std::pair<Ptr<QosStatsCalculator>,
                    Ptr<QosStatsCalculator> > QosStatsPair_t;

  /** A Map saving <GTP TEID / QoS stats pair >. */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;

  TeidQosMap_t              m_qosStats;     //!< TEID QoS statistics
  std::string               m_appFilename;  //!< AppStats filename
  Ptr<OutputStreamWrapper>  m_appWrapper;   //!< AppStats file wrapper
  std::string               m_epcFilename;  //!< EpcStats filename
  Ptr<OutputStreamWrapper>  m_epcWrapper;   //!< EpcStats file wrapper
};

} // namespace ns3
#endif /* SDMN_TRAFFIC_STATS_CALCULATOR_H */
