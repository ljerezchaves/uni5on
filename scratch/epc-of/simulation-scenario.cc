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

#include "simulation-scenario.h"
#include <iomanip>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulationScenario");
NS_OBJECT_ENSURE_REGISTERED (SimulationScenario);

SimulationScenario::SimulationScenario ()
  : m_opfNetwork (0),
    m_controller (0),
    m_epcHelper (0),
    m_lteNetwork (0),
    m_lteHelper (0),
    m_webNetwork (0),
    m_webHost (0)
{
  NS_LOG_FUNCTION (this);
}

SimulationScenario::~SimulationScenario ()
{
  NS_LOG_FUNCTION (this);
}

void
SimulationScenario::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_opfNetwork = 0;
  m_controller = 0;
  m_epcHelper = 0;
  m_lteNetwork = 0;
  m_webNetwork = 0;
  m_lteHelper = 0;
  m_webHost = 0;

  m_admissionStats = 0;
  m_gatewayStats = 0;
  m_bandwidthStats = 0;
  m_switchStats = 0;
  m_internetStats = 0;
  m_epcS1uStats = 0;
}

TypeId
SimulationScenario::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationScenario")
    .SetParent<Object> ()
    .AddConstructor<SimulationScenario> ()
    .AddAttribute ("TopoFilename",
                   "Filename for scenario topology description.",
                   StringValue ("topology.txt"),
                   MakeStringAccessor (&SimulationScenario::m_topoFilename),
                   MakeStringChecker ())
    .AddAttribute ("PcapTrace",
                   "Enable/Disable simulation PCAP traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_pcapTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("LteTrace",
                   "Enable/Disable simulation LTE ASCII traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_lteTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("SwitchLogs",
                   "Set the ofsoftswitch log level.",
                   StringValue ("none"),
                   MakeStringAccessor (&SimulationScenario::m_switchLog),
                   MakeStringChecker ())
  ;
  return tid;
}

// Observe the following order when creating the simulation scenario objects.
// Don't change object names or the trace connections won't work.
void
SimulationScenario::BuildRingTopology ()
{
  NS_LOG_FUNCTION (this);

  ParseTopology ();

  // 1) Create OpenFlowEpcNetwork object and name it OpenFlowNetwork.
  m_opfNetwork = CreateObject<RingNetwork> ();
  Names::Add ("OpenFlowNetwork", m_opfNetwork);

  // 2) Create OpenFlowEpcHelper object and name it OpenFlowEpcHelper.
  m_epcHelper = CreateObject<OpenFlowEpcHelper> ();
  Names::Add ("OpenFlowEpcHelper", m_epcHelper);

  // 3) Create the OpenFlowEpcController object and name it MainController (the
  // controller constructor will connect to OpenFlowEpcNetwork and
  // SgwPgwApplication trace sources).
  m_controller = CreateObject<RingController> ();
  Names::Add ("MainController", m_controller);

  // 4) Create the BandwidthStatsCalculator and SwitchRulesStatsCalculator
  // objects. They must be created after OpenFlowNetwork object but before
  // topology creation, as they will connect to OpenFlowNetwork trace sources
  // to monitor switches and connections.
  m_bandwidthStats = CreateObject<BandwidthStatsCalculator> ();
  m_switchStats = CreateObject<SwitchRulesStatsCalculator> ();

  // 5) Build network topology calling OpenFlowEpcNetwork::CreateTopology ().
  m_opfNetwork->CreateTopology (m_controller);

  // 6) Set up OpenFlowEpcHelper S1U and X2 connection callbacks (network
  // topology must be already created).
  m_epcHelper->SetS1uConnectCallback (
    MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
  m_epcHelper->SetX2ConnectCallback (
    MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));

  // 7) Create LTE radio access network and build topology
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteHelper = m_lteNetwork->CreateTopology (m_epcHelper);

  // 8) Create Internet network and build topology
  m_webNetwork = CreateObject<InternetNetwork> ();
  Names::Add ("InternetNetwork", m_webNetwork);
  m_webHost = m_webNetwork->CreateTopology (m_epcHelper->GetPgwNode ());

  // 9) Install applications and traffic manager
  Ptr<TrafficHelper> tfcHelper =
    CreateObject<TrafficHelper> (m_webHost, m_lteHelper, m_controller);
  tfcHelper->Install (m_lteNetwork->GetUeNodes (),
                      m_lteNetwork->GetUeDevices ());

  // 10) Set up output ofsoftswitch13 logs and ns-3 traces
  DatapathLogs ();
  EnableTraces ();

  // 11) Creating remaining stats calculator for output dump
  m_admissionStats = CreateObject<AdmissionStatsCalculator> ();
  m_gatewayStats = CreateObject<GatewayStatsCalculator> ();
  m_internetStats = CreateObject<WebQueueStatsCalculator> ();
  m_epcS1uStats = CreateObject<EpcS1uStatsCalculator> ();
}

std::string
SimulationScenario::StripValue (std::string value)
{
  std::string::size_type start = value.find ("\"");
  std::string::size_type end = value.find ("\"", 1);
  NS_ASSERT (start == 0);
  NS_ASSERT (end == value.size () - 1);
  return value.substr (start + 1, end - start - 1);
}

bool
SimulationScenario::ParseTopology ()
{
  NS_LOG_INFO ("Parsing topology...");

  StringValue stringValue;
  GlobalValue::GetValueByName ("InputPrefix", stringValue);
  std::string inputPrefix = stringValue.Get ();

  std::string name = inputPrefix + m_topoFilename;
  std::ifstream file;
  file.open (name.c_str (), std::ios::out);
  if (!file.is_open ())
    {
      NS_FATAL_ERROR ("Topology file not found.");
    }

  std::istringstream lineBuffer;
  std::string line, command;

  while (getline (file, line))
    {
      if (line.empty () || line.at (0) == '#')
        {
          continue;
        }

      lineBuffer.clear ();
      lineBuffer.str (line);
      lineBuffer >> command;
      if (command == "set")
        {
          std::string attrName, attrValue;
          lineBuffer >> attrName;
          lineBuffer >> attrValue;
          NS_LOG_DEBUG ("Setting attribute " << attrName <<
                        " with " << attrValue);
          Config::SetDefault (attrName, StringValue (StripValue (attrValue)));
        }
    }
  return true;
}

void
SimulationScenario::DatapathLogs ()
{
  NS_LOG_FUNCTION (this);
  m_opfNetwork->EnableDatapathLogs (m_switchLog);
}

void
SimulationScenario::EnableTraces ()
{
  NS_LOG_FUNCTION (this);

  if (m_pcapTrace)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();

      m_webNetwork->EnablePcap (prefix + "internet");
      m_opfNetwork->EnableOpenFlowPcap (prefix + "ofchannel");
      m_opfNetwork->EnableDataPcap (prefix + "ofnetwork", true);
      m_epcHelper->EnablePcapS1u (prefix + "lte-epc");
      m_epcHelper->EnablePcapX2 (prefix + "lte-epc");
    }
  if (m_lteTrace)
    {
      m_lteNetwork->EnableTraces ();
    }
}

};  // namespace ns3
