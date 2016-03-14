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

  /** Build the ring simulation topology. */
  void BuildRingTopology ();

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /** Enable/Disable ofsoftswitch13 library log. */
  void DatapathLogs ();

  /** Enable/Disable traces. */
  void EnableTraces ();

  Ptr<OpenFlowEpcNetwork>         m_opfNetwork;     //!< LTE EPC network
  Ptr<OpenFlowEpcHelper>          m_epcHelper;      //!< LTE EPC helper
  Ptr<LteHexGridNetwork>          m_lteNetwork;     //!< LTE radio network
  Ptr<LteHelper>                  m_lteHelper;      //!< LTE radio helper
  Ptr<InternetNetwork>            m_webNetwork;     //!< Internet network
  Ptr<Node>                       m_webHost;        //!< Internet server node
  bool                            m_pcapTrace;      //!< Enable PCAP traces
  Ptr<EpcS1uStatsCalculator>      m_epcS1uStats;    //!< EPC S1-U statistics
};

};  // namespace ns3
#endif // SIMULATION_SCENARIO_H
