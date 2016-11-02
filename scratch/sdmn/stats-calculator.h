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

#ifndef EPCOF_STATS_CALCULATOR_H
#define EPCOF_STATS_CALCULATOR_H

#include <ns3/nstime.h>
#include <ns3/queue.h>
#include <ns3/ofswitch13-device-container.h>
#include <ns3/global-value.h> 
#include "apps/sdmn-application.h"

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
 * This class monitors OpenFlow EPC controller statistics.
 */
class ControllerStatsCalculator : public Object
{
public:
  ControllerStatsCalculator ();          //!< Default constructor
  virtual ~ControllerStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Dump regular statistics into file.
   * \param next The inverval before next dump.
   */
  void DumpStatistics (Time next);

  /**
   * Notify new bearer request.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void NotifyBearerRequest (bool accepted, Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Reset all internal counters.
   */
  void ResetCounters ();

  /**
   * Get the NonGBR block ratio.
   * \return The ratio.
   */
  double GetNonGbrBlockRatio (void) const;

  /**
   * Get the GBR block ratio.
   * \return The ratio.
   */
  double GetGbrBlockRatio (void) const;

  uint32_t    m_nonRequests;        //!< number of non-GBR requests
  uint32_t    m_nonAccepted;        //!< number of non-GBR accepted
  uint32_t    m_nonBlocked;         //!< number of non-GBR blocked
  uint32_t    m_gbrRequests;        //!< Number of GBR requests
  uint32_t    m_gbrAccepted;        //!< Number of GBR accepted
  uint32_t    m_gbrBlocked;         //!< Number of GBR blocked

  std::string m_admStatsFilename;   //!< AdmStats filename
  std::string m_brqStatsFilename;   //!< BrqStats filename

  Ptr<OutputStreamWrapper> m_admWrapper;  //!< AdmStats file wrapper
  Ptr<OutputStreamWrapper> m_brqWrapper;  //!< BrqStats file wrapper
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup sdmn
 * This class monitors OpenFlow EPC network statistics.
 */
class NetworkStatsCalculator : public Object
{
public:
  NetworkStatsCalculator ();  //!< Default constructor
  virtual ~NetworkStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Dump regular statistics into file.
   * \param next The inverval before next dump.
   */
  void DumpStatistics (Time next);

  /**
   * Notify this stats calculator of a new connection between two switches in
   * the OpenFlow network.
   * \param cInfo The connection information and metadata.
   */
  void NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this stats calculator that all connection between switches have
   * already been configure and the topology is finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Reset all internal counters.
   */
  void ResetCounters ();

  /**
   * Get the active time value since last reset.
   * \return The time value.
   */
  Time GetActiveTime (void) const;

  OFSwitch13DeviceContainer         m_devices;      //!< Switch devices
  std::vector<Ptr<ConnectionInfo> > m_connections;  //!< Switch connections

  Time m_lastResetTime;                 //!< Last reset time

  std::string m_regStatsFilename;       //!< RegStats filename
  std::string m_renStatsFilename;       //!< RenStats filename
  std::string m_bwbStatsFilename;       //!< BwbStats filename
  std::string m_bwgStatsFilename;       //!< BwgStats filename
  std::string m_bwnStatsFilename;       //!< BwnStats filename
  std::string m_swtStatsFilename;         //!< SwtStats filename

  Ptr<OutputStreamWrapper> m_regWrapper;  //!< RegStats file wrapper
  Ptr<OutputStreamWrapper> m_renWrapper;  //!< RenStats file wrapper
  Ptr<OutputStreamWrapper> m_bwbWrapper;  //!< BwbStats file wrapper
  Ptr<OutputStreamWrapper> m_bwgWrapper;  //!< BwgStats file wrapper
  Ptr<OutputStreamWrapper> m_bwnWrapper;  //!< BwnStats file wrapper
  Ptr<OutputStreamWrapper> m_swtWrapper;  //!< SwtStats file wrapper
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup sdmn
 * This class monitors link queues' statistics.
 */
class LinkQueuesStatsCalculator : public Object
{
public:
  LinkQueuesStatsCalculator ();  //!< Default constructor
  virtual ~LinkQueuesStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Dump regular statistics into file.
   * \param next The inverval before next dump.
   */
  void DumpStatistics (Time next);

  /**
   * Set queue pointers for statistics dump.
   * \param downQueue The downlink tx queue at server node.
   * \param upQueue The uplink tx queue at client node.
   */
  void SetQueues (Ptr<Queue> downQueue, Ptr<Queue> upQueue);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Reset all internal counters.
   */
  void ResetCounters ();

  /**
   * Get the active time value since last reset.
   * \return The time value.
   */
  Time GetActiveTime (void) const;

  /**
   * Get the downlink bit rate.
   * \return The bit rate.
   */
  uint64_t GetDownBitRate (void) const;

  /**
   * Get the uplink bit rate.
   * \return The bit rate.
   */
  uint64_t GetUpBitRate (void) const;

  uint32_t    m_pgwDownBytes;   //!< Pgw traffic downlink bytes.
  uint32_t    m_pgwUpBytes;     //!< Pgw traffic uplink bytes.
  Ptr<Queue>  m_downQueue;      //!< Internet downlink queue
  Ptr<Queue>  m_upQueue;        //!< Internet uplink queue
  Time        m_lastResetTime;  //!< Last reset time

  std::string m_lnkStatsFilename;         //!< LinkStats filename
  Ptr<OutputStreamWrapper> m_lnkWrapper;  //!< LinkStats file wrapper
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup sdmn
 * This class monitors OpenFlow EPC S1-U QoS statistics.
 */
class EpcS1uStatsCalculator : public Object
{
public:
  /**
   * Complete constructor.
   * \param controller The OpenFlow EPC controller application.
   */
  EpcS1uStatsCalculator (Ptr<EpcController> controller);

  EpcS1uStatsCalculator ();  //!< Default constructor
  virtual ~EpcS1uStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set the controller pointer used by this stats calculator.
   * \param controller The controller pointer.
   */
  void SetController (Ptr<EpcController> controller);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Trace sink fired when packets are dropped by meter bands. The tag will be
   * read from packet, and EPC QoS stats updated.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when packets are dropped by OpenFlow port queues.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void QueueDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when packets enter the EPC. The packet will get tagged
   * for EPC QoS monitoring.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcInputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when packets leave the EPC. The tag will be read from
   * packet, and EPC QoS stats updated.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcOutputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when application traffic stops. Used to dump EPC and APP
   * traffic statistics.
   * \param context Context information.
   * \param app The SdmnApplication.
   */
  void DumpStatistics (std::string context, Ptr<const SdmnApplication> app);

  /**
   * Trace sink fired when application traffic starts. Used to reset EPC
   * traffic statistics.
   * \param context Context information.
   * \param app The SdmnApplication.
   */
  void ResetEpcStatistics (std::string context, Ptr<const SdmnApplication> app);

  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * \param teid The GTP tunnel id.
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);

  Ptr<const EpcController> m_controller;  //!< Network controller

  /** A pair of QosStatsCalculator, for downlink and uplink EPC statistics */
  typedef std::pair<Ptr<QosStatsCalculator>,
                    Ptr<QosStatsCalculator> > QosStatsPair_t;

  /** A Map saving <GTP TEID / QoS stats pair > */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;
  TeidQosMap_t m_qosStats; //!< TEID QoS statistics

  std::string m_appStatsFilename;       //!< AppStats filename
  std::string m_epcStatsFilename;       //!< EpcStats filename

  Ptr<OutputStreamWrapper> m_appWrapper;  //!< AppStats file wrapper
  Ptr<OutputStreamWrapper> m_epcWrapper;  //!< EpcStats file wrapper
};

} // namespace ns3
#endif /* EPCOF_STATS_CALCULATOR_H */
