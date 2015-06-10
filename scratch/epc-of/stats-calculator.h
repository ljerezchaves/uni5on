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

#include "ns3/applications-module.h"
#include "ns3/nstime.h"
#include "routing-info.h"
#include "internet-network.h"
#include "ns3/queue.h"

namespace ns3 {
  
/**
 * \ingroup epcof
 * This class monitors bearer request statistics. It counts the number of
 * bearer requests, including those accepted or blocked by network. 
 */
class AdmissionStatsCalculator : public Object
{
public:
  AdmissionStatsCalculator ();  //!< Default constructor
  virtual ~AdmissionStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime       (void) const;
  uint32_t  GetNonGbrRequests   (void) const;
  uint32_t  GetNonGbrAccepted   (void) const;
  uint32_t  GetNonGbrBlocked    (void) const;
  double    GetNonGbrBlockRatio (void) const;
  uint32_t  GetGbrRequests      (void) const;
  uint32_t  GetGbrAccepted      (void) const;
  uint32_t  GetGbrBlocked       (void) const;
  double    GetGbrBlockRatio    (void) const;
  uint32_t  GetTotalRequests    (void) const;
  uint32_t  GetTotalAccepted    (void) const;
  uint32_t  GetTotalBlocked     (void) const;
  //\}

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

  /** 
   * TracedCallback signature for LTE EPC bearer request.
   * \param desc Traffic description.
   * \param teid GTP TEID.
   * \param accepted True for accepted bearer.
   * \param downRate Downlink requested data rate.
   * \param upRate Uplink requested data rate.
   * \param path Routing paths description.
   */
  typedef void (* BrqTracedCallback)(std::string desc, uint32_t teid, 
    bool accepted, DataRate downRate, DataRate upRate, std::string path);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /**
   * Notify new bearer request.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void NotifyRequest (bool accepted, Ptr<const RoutingInfo> rInfo);
  
  uint32_t  m_nonRequests;      //!< number of non-GBR requests
  uint32_t  m_nonAccepted;      //!< number of non-GBR accepted 
  uint32_t  m_nonBlocked;       //!< number of non-GBR blocked
  uint32_t  m_gbrRequests;      //!< Number of GBR requests
  uint32_t  m_gbrAccepted;      //!< Number of GBR accepted
  uint32_t  m_gbrBlocked;       //!< Number of GBR blocked
  Time      m_lastResetTime;    //!< Last reset time

  /** The LTE EPC Bearer request trace source, fired at NotifyRequest. */
  TracedCallback<std::string, uint32_t, bool, DataRate, DataRate, std::string> m_brqTrace;
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * This class monitors gateway bandwidth statistics. 
 */
class GatewayStatsCalculator : public Object
{
public:
  GatewayStatsCalculator ();  //!< Default constructor
  virtual ~GatewayStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime       (void) const;
  DataRate  GetDownDataRate     (void) const;
  DataRate  GetUpDataRate       (void) const;
  //\}

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /**
   * Notify gateway traffic.
   * \param context Context information.
   * \param packet The packet.
   */
  void NotifyTraffic (std::string context, Ptr<const Packet> packet);

  uint32_t  m_pgwDownBytes;   //!< Pgw traffic downlink bytes.
  uint32_t  m_pgwUpBytes;     //!< Pgw traffic uplink bytes.
  Time      m_lastResetTime;  //!< Last reset time
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * This class monitors EPC link bandwidth usage statistics. 
 */
class BandwidthStatsCalculator : public Object
{
public:
  BandwidthStatsCalculator ();  //!< Default constructor
  virtual ~BandwidthStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time  GetActiveTime       (void) const;
  //\}

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  Time  m_lastResetTime;  //!< Last reset time
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * This class monitors OpenFlow switch flow table statistics. 
 */
class SwitchRulesStatsCalculator : public Object
{
public:
  SwitchRulesStatsCalculator ();  //!< Default constructor
  virtual ~SwitchRulesStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time  GetActiveTime       (void) const;
  //\}

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  Time  m_lastResetTime;  //!< Last reset time
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * This class monitors Internet queues statistics. 
 */
class WebQueueStatsCalculator : public Object
{
public:
  WebQueueStatsCalculator ();  //!< Default constructor
  virtual ~WebQueueStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Ptr<const Queue> GetDownlinkQueue (void) const;
  Ptr<const Queue> GetUplinkQueue   (void) const;
  //\}

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  Ptr<Queue>  m_downQueue; //!< Internet downlink queue
  Ptr<Queue>  m_upQueue;   //!< Internet uplink queue
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * This class monitors OpenFlow EPC S1-U QoS statistics. 
 */
class EpcS1uStatsCalculator : public Object
{
public:
  EpcS1uStatsCalculator ();  //!< Default constructor
  virtual ~EpcS1uStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** 
   * TracedCallback signature for Epc QoS stats.
   * \param desc String describing this traffic.
   * \param teid GTP TEID.
   * \param stats The QoS statistics.
   */
  typedef void (* EpcTracedCallback)
    (std::string desc, uint32_t teid, Ptr<const QosStatsCalculator> stats);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

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
   * Trace sink fired when application traffic stops. Used to dump EPC traffic
   * statistics.
   * \param context Context information.
   * \param app The EpcApplication.
   */
  void DumpEpcStatistics (std::string context, Ptr<const EpcApplication> app);

  /**
   * Trace sink fired when application traffic starts. Used to reset EPC
   * traffic statistics.
   * \param context Context information.
   * \param app The EpcApplication.
   */ 
  void ResetEpcStatistics (std::string context, Ptr<const EpcApplication> app);
     
  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * \param teid The GTP tunnel id.
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);

  /** A pair of QosStatsCalculator, for downlink and uplink EPC statistics */
  typedef std::pair<Ptr<QosStatsCalculator>, Ptr<QosStatsCalculator> > QosStatsPair_t;
  
  /** A Map saving <GTP TEID / QoS stats pair > */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;
  TeidQosMap_t m_qosStats; //!< TEID QoS statistics

  /** The OpenFlow EPC QoS trace source, fired at DumpEpcStatistics. */
  TracedCallback<std::string, uint32_t, Ptr<const QosStatsCalculator> > m_epcTrace;
};


} // namespace ns3
#endif /* EPCOF_STATS_CALCULATOR_H */
