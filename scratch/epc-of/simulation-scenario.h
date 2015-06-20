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
#include "stats-calculator.h"
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

  /**
   * Build the ring simulation topology.
   */
  void BuildRingTopology ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /**
   * Set the commom prefix for stats filenames.
   * \param prefix the prefix.
   */
  void SetCommonPrefix (std::string prefix);

  /**
   * Set the default statistics dump interval.
   * \param timeout The timeout value.
   */
  void SetDumpTimeout (Time timeout);

  /**
   * Dump regular statistics.
   */
  void DumpStatistics ();

  /**
   * Strip string value in topology description file.
   * \param value The value string.
   * \return The stripped value string.
   */
  std::string StripValue (std::string value);

  /**
   * Parse topology description file.
   * Topology file columns (indexes starts at 0):
   * eNB index | # of UEs at this eNB | OpenFlow switch index
   */
  bool ParseTopology ();

  /**
   * Enable/Disable ofsoftswitch13 library log.
   */
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

  std::string           m_inputPrefix;        //!< Common input prefix filenames
  std::string           m_outputPrefix;       //!< Common output prefix filenames
  std::string           m_topoFilename;       //!< Topology filename
  std::string           m_switchLog;          //!< Switches log level
  bool                  m_pcapTrace;          //!< Enable PCAP traces
  bool                  m_lteTrace;           //!< Enable LTE ASCII traces
  std::vector<uint32_t> m_UesPerEnb;          //!< Number of UEs per eNb
  std::vector<uint16_t> m_SwitchIdxPerEnb;    //!< Switch index per eNb

  Time                            m_dumpTimeout;    //!< Dump stats timeout.
  Ptr<AdmissionStatsCalculator>   m_admissionStats; // Admission statistics
  Ptr<GatewayStatsCalculator>     m_gatewayStats;   // Gateway statistics
  Ptr<BandwidthStatsCalculator>   m_bandwidthStats; // Bandwidth statistics
  Ptr<SwitchRulesStatsCalculator> m_switchStats;    // Switch rules statistics
  Ptr<WebQueueStatsCalculator>    m_internetStats;  // Web queues statistics
  Ptr<EpcS1uStatsCalculator>      m_epcS1uStats;    // EPC S1-U statistics
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
