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

#ifndef SDMN_STATS_CALCULATOR_H
#define SDMN_STATS_CALCULATOR_H

#include <ns3/nstime.h>
#include <ns3/ofswitch13-device-container.h>
#include <ns3/global-value.h>
#include "apps/sdmn-client-app.h"

namespace ns3 {

static GlobalValue g_dumpTimeout =
  GlobalValue ("DumpStatsTimeout", "Periodic statistics dump interval.",
               TimeValue (Seconds (10)),
               ns3::MakeTimeChecker ());

class RoutingInfo;
class ConnectionInfo;
class EpcController;

/**
 * \ingroup sdmn
 * This class monitors the SDN EPC bearer admission control and dump bearer
 * requests and blocking statistics.
 */
class AdmissionStatsCalculator : public Object
{
public:
  AdmissionStatsCalculator ();          //!< Default constructor.
  virtual ~AdmissionStatsCalculator (); //!< Default destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Notify a new bearer request.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void NotifyBearerRequest (bool accepted, Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  /**
   * Reset internal counters.
   */
  void ResetCounters ();

  uint32_t                  m_nonRequests;  //!< Number of non-GBR requests
  uint32_t                  m_nonAccepted;  //!< Number of non-GBR accepted
  uint32_t                  m_nonBlocked;   //!< Number of non-GBR blocked
  uint32_t                  m_gbrRequests;  //!< Number of GBR requests
  uint32_t                  m_gbrAccepted;  //!< Number of GBR accepted
  uint32_t                  m_gbrBlocked;   //!< Number of GBR blocked
  std::string               m_admFilename;  //!< AdmStats filename
  Ptr<OutputStreamWrapper>  m_admWrapper;   //!< AdmStats file wrapper
  std::string               m_brqFilename;  //!< BrqStats filename
  Ptr<OutputStreamWrapper>  m_brqWrapper;   //!< BrqStats file wrapper
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup sdmn
 * This class monitors the backhaul OpenFlow network and dump bandwidth usage
 * and resource reservation statistics on links between OpenFlow switches.
 */
class BackhaulStatsCalculator : public Object
{
public:
  BackhaulStatsCalculator ();          //!< Default constructor.
  virtual ~BackhaulStatsCalculator (); //!< Default destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Notify this stats calculator of a new connection between two switches in
   * the OpenFlow backhaul network.
   * \param cInfo The connection information and metadata.
   */
  void NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this stats calculator that all connection between OpenFlow switches
   * have already been configure and the backhaul topology is done.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  /**
   * Reset internal counters.
   */
  void ResetCounters ();

  /** A Vector of connection information objects. */
  typedef std::vector<Ptr<ConnectionInfo> > ConnInfoList_t;

  ConnInfoList_t            m_connections;    //!< Switch connections
  Time                      m_lastResetTime;  //!< Last reset time
  std::string               m_regFilename;    //!< RegStats filename
  Ptr<OutputStreamWrapper>  m_regWrapper;     //!< RegStats file wrapper
  std::string               m_renFilename;    //!< RenStats filename
  Ptr<OutputStreamWrapper>  m_renWrapper;     //!< RenStats file wrapper
  std::string               m_bwbFilename;    //!< BwbStats filename
  Ptr<OutputStreamWrapper>  m_bwbWrapper;     //!< BwbStats file wrapper
  std::string               m_bwgFilename;    //!< BwgStats filename
  Ptr<OutputStreamWrapper>  m_bwgWrapper;     //!< BwgStats file wrapper
  std::string               m_bwnFilename;    //!< BwnStats filename
  Ptr<OutputStreamWrapper>  m_bwnWrapper;     //!< BwnStats file wrapper
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup sdmn
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
   * Complete constructor.
   * \param controller The OpenFlow EPC controller application.
   */
  TrafficStatsCalculator (Ptr<EpcController> controller);

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
  Ptr<const EpcController>  m_controller;   //!< Network controller
  std::string               m_appFilename;  //!< AppStats filename
  Ptr<OutputStreamWrapper>  m_appWrapper;   //!< AppStats file wrapper
  std::string               m_epcFilename;  //!< EpcStats filename
  Ptr<OutputStreamWrapper>  m_epcWrapper;   //!< EpcStats file wrapper
};

} // namespace ns3
#endif /* SDMN_STATS_CALCULATOR_H */
