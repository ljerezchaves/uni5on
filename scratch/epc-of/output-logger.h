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

#ifndef OUTPUT_LOGGER_H
#define OUTPUT_LOGGER_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include "openflow-epc-network.h"

namespace ns3 {

// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Output logger to monitor, save and dump simulation statistics into text
 * files.
 */
class OutputLogger : public Object
{
public:
  OutputLogger ();          //!< Default constructor
  virtual ~OutputLogger (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  
  /** 
   * Set the common prefix for output files, including the simulation run no.
   * \param prefix The prefix.
   */ 
  void SetCommonPrefix (std::string prefix);
  
  /**
   * Set the default statistics dump interval.
   * \param timeout The timeout value.
   */
  void SetDumpTimeout (Time timeout);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /**
   * Dump regular statistics.
   */
  void DumpStatistics ();
  
  /**
   * Trace sink fired at every new bearer request to OpenFlow controller.
   * \param context Context information.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void BearerRequest (std::string context, bool accepted, 
                      Ptr<const RoutingInfo> rInfo);

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
   * Trace sink fired when packets traverse the EPC packet gateway from/to the
   * Internet to/from the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void PgwTraffic (std::string context, Ptr<const Packet> packet);

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
   * Concatenate the commom prefix and the filename.
   * \param name The filename.
   * \return The complete filename.
   */
  std::string GetCompleteName (std::string name);
   
  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * \param teid The GTP tunnel id.
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);


// ------------------------------------------------------------------------ //
  /**
   * Save application statistics in file. 
   */
  void ReportAppStats (std::string description, uint32_t teid, 
                       Ptr<const QosStatsCalculator> stats);

  /**
   * Save LTE EPC statistics in file. 
   */
  void ReportEpcStats (std::string description, uint32_t teid, 
                       Ptr<const QosStatsCalculator> stats);

  /**
   * Save bearer admission control statistics in file. 
   */
  void ReportAdmStats (Ptr<const AdmissionStatsCalculator> stats);

  /**
   * Save packet gateway traffic statistics in file.
   */
  void ReportPgwStats (Ptr<const GatewayStatsCalculator> stats);

  /**
   * Save flow table usage statistics in file.
   */
  void ReportSwtStats (std::vector<uint32_t> teid);

  /**
   * Save internet queue statistics in file.
   */
  void ReportWebStats (Ptr<const Queue> downlink, Ptr<const Queue> uplink);

  /**
   * Save bandwidth usage in file.
   */
  void ReportBwdStats (std::vector<BandwidthStats_t> stats);

  /**
   * Save bearer request statistics in file.
   * \param desc Traffic description.
   * \param teid GTP TEID.
   * \param accepted True for accepted bearer.
   * \param downRate Downlink requested data rate.
   * \param upRate Uplink requested data rate.
   * \param path Routing paths description.
   */
  void ReportBrqStats (std::string desc, uint32_t teid, bool accepted,
                       DataRate downRate, DataRate upRate, std::string path);
// ------------------------------------------------------------------------ //

  Time        m_dumpTimeout;        //!< Dump stats timeout.
  std::string m_commonPrefix;       //!< Common prefix for filenames
  std::string m_appStatsFilename;   //!< AppStats filename
  std::string m_epcStatsFilename;   //!< EpcStats filename
  std::string m_pgwStatsFilename;   //!< PgwStats filename
  std::string m_swtStatsFilename;   //!< SwtStats filename
  std::string m_admStatsFilename;   //!< AdmStats filename
  std::string m_webStatsFilename;   //!< WebStats filename
  std::string m_bwdStatsFilename;   //!< BwdStats filename
  std::string m_brqStatsFilename;   //!< BrqStats filename

  Ptr<AdmissionStatsCalculator> m_admissionStats; // Admission statistics
  Ptr<GatewayStatsCalculator>   m_gatewayStats;   // Gateway statistics

  /** A pair of QosStatsCalculator, for downlink and uplink EPC statistics */
  typedef std::pair<Ptr<QosStatsCalculator>, Ptr<QosStatsCalculator> > QosStatsPair_t;
  
  /** A Map saving <GTP TEID / QoS stats pair > */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;
  TeidQosMap_t m_qosStats; //!< TEID QoS statistics
};

};  // namespace ns3
#endif // OUTPUT_LOGGER_H

