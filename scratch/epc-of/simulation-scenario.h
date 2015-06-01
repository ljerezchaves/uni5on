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
#include "traffic-helper.h"
#include <vector>

using namespace std;

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
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
