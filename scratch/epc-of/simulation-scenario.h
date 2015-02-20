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
#include "lte-squared-grid-network.h"
#include "ring-network.h"
#include <vector>

namespace ns3 {

/**
 * Network simulation scenario for LTE EPC + OpenFlow 1.3
 */
class SimulationScenario : public Object
{
public:
  SimulationScenario ();           //!< Default constructor
  virtual ~SimulationScenario ();  //!< Dummy destructor, see DoDipose

  /**
   * Creates a simulation scenario based on user preferences from command line.
   * \param nEnbs The number of eNBs in the LTE network.
   * \param nRing The number of OpenFlow switches in the EPC ring network.
   * \param eNbUes Indicates, for each eNB, the number of UEs to create.
   * \param eNbSwt Indicates, for each eNB, the OpenFlow switch index to use.
   */
  SimulationScenario (std::string filename, uint32_t nEnbs, uint32_t nUes, uint32_t nRing);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

//protected:
  // Inherited from Object
  void NotifyConstructionCompleted ();

  /**
   * Parse topology description file.  
   * Topology file columns (indexes starts at 0):
   * eNB index | # of UEs at this eNB | OpenFlow switch index
   */
  bool ParseTopology (std::string filename, uint32_t nEnbs, uint32_t nUes, uint16_t nRing);

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
   * Enable ofsoftswitch13 library log.
   * \param level string representing library logging level.
   */
  void EnableDatapathLogs (std::string level = "all");

  /**
   * Enable LTE and PCAP traces.
   */
  void EnableTraces ();

  /**
   * Save application statistics in file. 
   * \see ns3::OpenFlowEpcController::AppQosTracedCallback for parameters.
   */
  void ReportAppStats (std::string description, uint32_t teid,
      Time duration, double lossRatio, Time delay, 
      Time jitter, uint32_t bytes, DataRate goodput);

  /**
   * Save block ratio statistics in file. 
   * \see ns3::OpenFlowEpcController::GbrBlockTracedCallback for parameters.
   */
  void ReportBlockRatio (uint32_t requests, uint32_t blocks, double ratio);


private:
  /**
   * Get complete filename for video trace files;
   * \param idx The trace index.
   * \return Complete path.
   */
  static const std::string GetVideoFilename (uint8_t idx);

  Ptr<OpenFlowEpcNetwork> m_opfNetwork;       //!< LTE EPC network
  Ptr<OpenFlowEpcController> m_controller;    //!< OpenFLow controller
  Ptr<OpenFlowEpcHelper> m_epcHelper;         //!< LTE EPC helper
  Ptr<LteSquaredGridNetwork> m_lteNetwork;    //!< LTE radio network
  Ptr<InternetNetwork> m_webNetwork;          //!< Internet network
  Ptr<LteHelper> m_lteHelper;                 //!< LTE radio helper
  Ptr<Node> m_webHost;                        //!< Internet server node
  Ptr<UniformRandomVariable> m_rngStart;      //!< Random app start time
  
  NodeContainer         m_ueNodes;            //!< LTE UE nodes
  NetDeviceContainer    m_ueDevices;          //!< LTE UE devices
  ApplicationContainer  m_voipServers;        //!< Voip applications
  ApplicationContainer  m_videoServers;       //!< Video applications

  bool                  m_statsFirstWrite;    //!< First write to appStatsFile
  std::string           m_statsFilename;      //!< AppStats filename
  std::string           m_topoFilename;       //!< Topology filename
  std::string           m_liblog;             //!< Switches log level
  uint32_t              m_nEnbs;              //!< Number of eNBs
  uint32_t              m_nUes;               //!< Number of UEs per eNB
  uint16_t              m_nRing;              //!< Number of OpenFlow switches
  bool                  m_traces;             //!< Enable pcap traces
  bool                  m_ping;               //!< Enable ping traffic
  bool                  m_voip;               //!< Enable voip traffic
  bool                  m_http;               //!< Enable http traffic
  bool                  m_video;              //!< Enable video traffic
  std::vector<uint32_t> m_eNbUes;             // Number of UEs per eNb
  std::vector<uint16_t> m_eNbSwt;             // Switch index per eNb

  static const std::string m_videoDir;        //!< Video trace directory
  static const std::string m_videoTrace [];   //!< Video trace filenames
  static const uint64_t m_avgBitRate [];      //!< Video trace avg bitrate
  static const uint64_t m_maxBitRate [];      //!< Video trace max bitrate
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
