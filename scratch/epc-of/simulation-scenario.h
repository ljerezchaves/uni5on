/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#ifndef SIMULATION_SCENARIO_H
#define SIMULATION_SCENARIO_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/applications-module.h>
#include "internet-network.h"
#include "lte-hex-grid-network.h"
#include "ring-network.h"
#include "openflow-epc-controller.h"
#include <vector>

namespace ns3 {

/**
 * \defgroup epcof LTE EPC + OpenFlow
 * Network simulation scenario for LTE EPC + OpenFlow 1.3
 */
class SimulationScenario : public Object
{
public:
  SimulationScenario ();           //!< Default constructor
  virtual ~SimulationScenario ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** Build the ring simulation topology. */ 
  void BuildRingTopology ();

  /** Set the common prefix for input/output files. */ 
  void SetCommonPrefix (std::string prefix);

private:
  /**
   * Enable IPv4 ICMP ping application over default EPS bearer (QCI 9).
   * This QCI is typically used for the default bearer of a UE/PDN for non
   * privileged subscribers. 
   */
  void EnablePingTraffic ();
  
  /** 
   * Enable HTTP/TCP download traffic over dedicated Non-GBR EPS bearer (QCI 8). 
   * This QCI 8 could be used for a dedicated 'premium bearer' for any
   * subscriber, or could be used for the default bearer of a for 'premium
   * subscribers'.
   *
   * \internal This HTTP model is based on the distributions indicated in the
   * paper 'An HTTP Web Traffic Model Based on the Top One Million Visited Web
   * Pages' by Rastin Pries et. al. Each client will send a get request to the
   * server at port 80 and will get the page content back including inline
   * content. These requests repeats over time following appropriate
   * distribution.
   */
  void EnableHttpTraffic ();
  
  /** 
   * Enable VoIP/UDP bidirectional traffic over dedicated GBR EPS bearer (QCI 1). 
   * This QCI is typically associated with an operator controlled service.
   *
   * \internal This VoIP traffic simulates the G.729 codec (~8.5 kbps for
   * payload). Check http://goo.gl/iChPGQ for bandwidth calculation and
   * discussion. This code will install a bidirectional voip traffic between UE
   * and WebServer, using the VoipPeer application. 
   */
  void EnableVoipTraffic ();
  
  /**
   * Enable UDP downlink video streaming over dedicated GBR EPS bearer (QCI 4).
   * This QCI is typically associated with an operator controlled service.
   *
   * \internal This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf. This code
   * install an UdpTraceClient at the web server and an UdpServer application
   * at each client).
   */
  void EnableVideoTraffic ();

  /**
   * Save application statistics in file. 
   * \see ns3::OpenFlowEpcController::QosTracedCallback for parameters.
   */
  void ReportAppStats (std::string description, uint32_t teid, 
                       Ptr<const QosStatsCalculator> stats);

  /**
   * Save LTE EPC GTPU statistics in file. 
   * \see ns3::OpenFlowEpcController::QosTracedCallback for parameters.
   */
  void ReportEpcStats (std::string description, uint32_t teid, 
                       Ptr<const QosStatsCalculator> stats);

  /**
   * Save bearer admission control statistics in file. 
   * \see ns3::AdmissionStatsCalculator::AdmTracedCallback for parameters.
   */
  void ReportAdmStats (Ptr<const AdmissionStatsCalculator> stats);

  /**
   * Save packet gateway traffic statistics in file.
   * \see ns3::OpenFlowEpcController::PgwTracedCallback for parameters.
   */
  void ReportPgwStats (DataRate downTraffic, DataRate upTraffic);

  /**
   * Save flow table usage statistics in file.
   * \see ns3::OpenFlowEpcController::SwtTracedCallback for parameters.
   */
  void ReportSwtStats (std::vector<uint32_t> teid);

  /**
   * Save internet queue statistics in file.
   * \see ns3::InternetNetwork::WebTracedCallback for parameters.
   */
  void ReportWebStats (Ptr<const Queue> downlink, Ptr<const Queue> uplink);

  /**
   * Save bandwidth usage in file.
   * \see ns3::OpenFlowEpcController::BwdTracedCallback for parameters.
   */
  void ReportBwdStats (std::vector<BandwidthStats_t> stats);

  /**
   * Save bearer request statistics in file.
   * \see ns3::BearerRequestStats::BrqTracedCallback for parameters.
   */
  void ReportBrqStats (Ptr<const BearerRequestStats> stats);

  /**
   * Parse topology description file.  
   * Topology file columns (indexes starts at 0):
   * eNB index | # of UEs at this eNB | OpenFlow switch index
   */
  bool ParseTopology ();

  /**
   * Get complete filename for video trace files;
   * \param idx The trace index.
   * \return Complete path.
   */
  static const std::string GetVideoFilename (uint8_t idx);

  /** Enable/Disable ofsoftswitch13 library log. */
  void DatapathLogs ();

  /** 
   * Enable/Disable PCAP and ASCII traces. 
   * \param prefix Common prefix for files.
   */
  void PcapAsciiTraces ();

  Ptr<OpenFlowEpcNetwork>    m_opfNetwork;    //!< LTE EPC network
  Ptr<OpenFlowEpcController> m_controller;    //!< OpenFLow controller
  Ptr<OpenFlowEpcHelper>     m_epcHelper;     //!< LTE EPC helper
  Ptr<LteHexGridNetwork>     m_lteNetwork;    //!< LTE radio network
  Ptr<InternetNetwork>       m_webNetwork;    //!< Internet network
  Ptr<LteHelper>             m_lteHelper;     //!< LTE radio helper
  Ptr<Node>                  m_webHost;       //!< Internet server node
  Ptr<Node>                  m_pgwHost;       //!< EPC Pgw gateway node
  
  NodeContainer         m_ueNodes;            //!< LTE UE nodes
  NetDeviceContainer    m_ueDevices;          //!< LTE UE devices
  ApplicationContainer  m_voipServers;        //!< Voip applications
  ApplicationContainer  m_videoServers;       //!< Video applications

  std::string           m_appStatsFilename;   //!< AppStats filename
  std::string           m_epcStatsFilename;   //!< EpcStats filename
  std::string           m_pgwStatsFilename;   //!< PgwStats filename
  std::string           m_swtStatsFilename;   //!< SwtStats filename
  std::string           m_admStatsFilename;   //!< AdmStats filename
  std::string           m_webStatsFilename;   //!< WebStats filename
  std::string           m_bwdStatsFilename;   //!< BwdStats filename
  std::string           m_brqStatsFilename;   //!< BrqStats filename
  std::string           m_topoFilename;       //!< Topology filename
  std::string           m_switchLog;          //!< Switches log level
  std::string           m_commonPrefix;       //!< Common prefix for filenames
  uint32_t              m_nEnbs;              //!< Number of eNBs
  uint16_t              m_nSwitches;          //!< Number of OpenFlow switches
  bool                  m_pcapTrace;          //!< Enable PCAP traces
  bool                  m_lteTrace;           //!< Enable LTE ASCII traces
  bool                  m_ping;               //!< Enable ping traffic
  bool                  m_voip;               //!< Enable VoIP traffic
  bool                  m_http;               //!< Enable http traffic
  bool                  m_video;              //!< Enable video traffic
  std::vector<uint32_t> m_UesPerEnb;          //!< Number of UEs per eNb
  std::vector<uint16_t> m_SwitchIdxPerEnb;    //!< Switch index per eNb

  static const std::string m_videoDir;        //!< Video trace directory
  static const std::string m_videoTrace [];   //!< Video trace filenames
  static const uint64_t    m_avgBitRate [];   //!< Video trace avg bitrate
  static const uint64_t    m_maxBitRate [];   //!< Video trace max bitrate
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
