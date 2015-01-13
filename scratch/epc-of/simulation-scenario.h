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
   * \param nUes The number of UEs per eNB.
   * \param nRing The number of OpenFlow switches in the EPC ring network.
   */
  SimulationScenario (uint32_t nEnbs, uint32_t nUes, uint32_t nRing);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

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
   * Enable VoIP/UDP bidiretional traffic over dedicated GBR EPS bearer (QCI 1). 
   * This QCI is typically associated with an operator controlled service.
   *
   * \internal This VoIP traffic simulates the G.729 codec (~8.5 kbps for
   * payload). Check http://goo.gl/iChPGQ for bandwidth calculation and
   * discussion. This code will install a bidirectional voip traffic between UE
   * and WebServer (in fact, in install a VoipClient and an UdpServer
   * application at each node). 
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
   */
  void EnableDatapathLogs ();

  /**
   * Enable LTE and PCAP traces.
   */
  void EnableTraces ();

  /**
   * Print block ratio and application statistics.
   */
  void PrintStats ();

private:
  Ptr<OpenFlowEpcNetwork> m_opfNetwork;         //!< LTE EPC network
  Ptr<OpenFlowEpcController> m_controller;      //!< OpenFLow controller
  Ptr<OpenFlowEpcHelper> m_epcHelper;           //!< LTE EPC helper
  Ptr<LteSquaredGridNetwork> m_lteNetwork;      //!< LTE radio network
  Ptr<InternetNetwork> m_webNetwork;            //!< Internet network
  Ptr<LteHelper> m_lteHelper;                   //!< LTE radio helper
  Ptr<Node> m_webHost;                          //!< Internet server node
    
  NodeContainer m_ueNodes;                      //!< LTE UE nodes
  NetDeviceContainer m_ueDevices;               //!< LTE UE devices

  ApplicationContainer m_voipServers;           //!< Voip applications
  ApplicationContainer m_videoServers;          //!< Video applications

  static const std::string m_videoTrace [];     //!< Video trace filenames
  static const uint64_t m_avgBitRate [];        //!< Video trace avg bitrate
  static const uint64_t m_maxBitRate [];        //!< Video trace max bitrate
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
