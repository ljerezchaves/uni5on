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
    m_webNetwork (0),
    m_lteHelper (0),
    m_webHost (0),
    m_pgwHost (0),
    m_logger (0)
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
  m_pgwHost = 0;
  m_logger = 0;
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
    .AddAttribute ("CommonPrefix",
                   "Common prefix for input and output filenames.",
                   StringValue (""),
                   MakeStringAccessor (&SimulationScenario::SetCommonPrefix),
                   MakeStringChecker ())
    .AddAttribute ("Enbs",
                   "Number of eNBs in network topology.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SimulationScenario::m_nEnbs),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Switches",
                   "Number of OpenFlow switches in network topology.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SimulationScenario::m_nSwitches),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PcapTrace",
                   "Enable/Disable simulation PCAP traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_pcapTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("LteTrace",
                   "Enable/Disable simulation LTE ASCII traces.",
                   BooleanValue (true),
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

// NOTE: Don't change object names. The logger mechanism uses theses specific
// names to connect to trace sources.
void
SimulationScenario::BuildRingTopology ()
{
  NS_LOG_FUNCTION (this);

  ParseTopology ();

  // OpenFlow EPC ring controller
  m_controller = CreateObject<RingController> ();
  Names::Add ("MainController", m_controller);
  
  // OpenFlow EPC ring network (considering 20km fiber cable latency)
  m_opfNetwork = CreateObject<RingNetwork> ();
  Names::Add ("OpenFlowNetwork", m_opfNetwork);
  m_opfNetwork->SetAttribute ("NumSwitches", UintegerValue (m_nSwitches));
  m_opfNetwork->CreateTopology (m_controller, m_SwitchIdxPerEnb);
  
  // LTE EPC core (with callbacks and trace sink setup)
  m_epcHelper = CreateObject<OpenFlowEpcHelper> ();
  m_epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
  m_epcHelper->SetX2ConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));
  Config::ConnectWithoutContext (
    "/Names/SgwPgwApplication/ContextCreated",
    MakeCallback (&OpenFlowEpcController::NotifyContextCreated, m_controller));
  
  // LTE radio access network
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteNetwork->SetAttribute ("Enbs", UintegerValue (m_nEnbs));
  m_lteHelper = m_lteNetwork->CreateTopology (m_epcHelper, m_UesPerEnb);

  // Internet network
  m_webNetwork = CreateObject<InternetNetwork> ();
  Names::Add ("InternetNetwork", m_webNetwork);
  m_webHost = m_webNetwork->CreateTopology (m_epcHelper->GetPgwNode ());

  // Applications and traffic manager
  TrafficHelper tfcHelper (m_webHost, m_lteHelper, m_controller);
  tfcHelper.Install (m_lteNetwork->GetUeNodes (), m_lteNetwork->GetUeDevices ());

  // Output logger. Must be created after scenario configuration, so it can
  // connect to traces sources successfully.
  m_logger = CreateObject<OutputLogger> ();
  m_logger->SetCommonPrefix (m_commonPrefix);

  // OpenFlow network trace sinks. Must be called after scenario configuration,
  // so it can connect to trace sources successfully.
  // m_opfNetwork->ConnectTraceSinks ();

  // ofsoftswitch13 log and ns-3 traces
  DatapathLogs ();
  PcapAsciiTraces ();
}

void 
SimulationScenario::SetCommonPrefix (std::string prefix)
{
  static bool prefixSet = false;

  if (prefixSet) return;

  prefixSet = true;
  char lastChar = *prefix.rbegin (); 
  if (lastChar != '-')
    {
      prefix += "-";
    }
  m_commonPrefix = prefix;
}

bool
SimulationScenario::ParseTopology ()
{
  NS_LOG_INFO ("Parsing topology...");
 
  std::string name = m_commonPrefix + m_topoFilename;
  std::ifstream file;
  file.open (name.c_str ());
  if (!file.is_open ())
    {
      NS_FATAL_ERROR ("Topology file not found.");
    }

  std::istringstream lineBuffer;
  std::string line;
  uint32_t enb, ues, swtch;
  uint32_t idx = 0;

  // At first we expect the number of eNBs and switches in network
  std::string attr;
  uint32_t value;
  uint8_t attrOk = 0;
  while (getline (file, line))
    {
      if (line.empty () || line.at (0) == '#') continue;
      
      lineBuffer.clear ();
      lineBuffer.str (line);
      lineBuffer >> attr;
      lineBuffer >> value;
      if (attr == "Enbs" || attr == "Switches")
        {
          NS_LOG_DEBUG (attr << " " << value);
          SetAttribute (attr, UintegerValue (value));
          attrOk++;
          if (attrOk == 2) break;
        }
    }
  NS_ASSERT_MSG (attrOk == 2, "Missing attributes in topology file.");

  // Then we expect the distribution of UEs per eNBs and switch indexes
  while (getline (file, line))
    {
      if (line.empty () || line.at (0) == '#') continue;

      lineBuffer.clear ();
      lineBuffer.str (line);
      lineBuffer >> enb;
      lineBuffer >> ues;
      lineBuffer >> swtch;

      NS_LOG_DEBUG (enb << " " << ues << " " << swtch);
      NS_ASSERT_MSG (idx == enb, "Invalid eNB idx order in topology file.");
      NS_ASSERT_MSG (swtch < m_nSwitches, "Invalid switch idx in topology file.");
      
      m_UesPerEnb.push_back (ues);
      m_SwitchIdxPerEnb.push_back (swtch);
      idx++;
    }
  NS_ASSERT_MSG (idx == m_nEnbs, "Missing information in topology file.");
  
  return true;  
}

void
SimulationScenario::DatapathLogs ()
{
  NS_LOG_FUNCTION (this);
  m_opfNetwork->EnableDatapathLogs (m_switchLog);
}

void
SimulationScenario::PcapAsciiTraces ()
{
  NS_LOG_FUNCTION (this);

  // Including the simulation run number in commom prefix
  ostringstream ss;
  ss << m_commonPrefix << RngSeedManager::GetRun () << "-";
  std::string completePrefix = ss.str ();

  if (m_pcapTrace)
    {
      m_webNetwork->EnablePcap (completePrefix + "internet");
      m_opfNetwork->EnableOpenFlowPcap (completePrefix + "ofchannel");
      m_opfNetwork->EnableDataPcap (completePrefix + "ofnetwork", true);
      m_epcHelper->EnablePcapS1u (completePrefix + "lte-epc");
      m_epcHelper->EnablePcapX2 (completePrefix + "lte-epc");
    }
  if (m_lteTrace)
    {
      m_lteNetwork->EnableTraces (completePrefix);
    }
}

};  // namespace ns3
