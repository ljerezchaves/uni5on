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
    .AddAttribute ("CommonPrefix",
                   "Common prefix for input and output filenames.",
                   StringValue (""),
                   MakeStringAccessor (&SimulationScenario::SetCommonPrefix),
                   MakeStringChecker ())
    .AddAttribute ("DumpStatsTimeout",
                   "Periodic statistics dump interval.",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&SimulationScenario::SetDumpTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("Enbs",
                   "Number of eNBs in network topology.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SimulationScenario::SetEnbs),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Switches",
                   "Number of OpenFlow switches in network topology.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&SimulationScenario::SetSwitches),
                   MakeUintegerChecker<uint16_t> ())
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
  m_opfNetwork->CreateTopology (m_controller, m_SwitchIdxPerEnb);
 
  // 6) Set up OpenFlowEpcHelper S1U and X2 connection callbacks (network
  // topology must be already created).
  m_epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
  m_epcHelper->SetX2ConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));
  
  // 7) Create LTE radio access network and build topology
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteHelper = m_lteNetwork->CreateTopology (m_epcHelper, m_UesPerEnb);

  // 8) Create Internet network and build topology
  m_webNetwork = CreateObject<InternetNetwork> ();
  Names::Add ("InternetNetwork", m_webNetwork);
  m_webHost = m_webNetwork->CreateTopology (m_epcHelper->GetPgwNode ());

  // 9) Install applications and traffic manager
  TrafficHelper tfcHelper (m_webHost, m_lteHelper, m_controller);
  tfcHelper.Install (m_lteNetwork->GetUeNodes (), m_lteNetwork->GetUeDevices ());

  // 10) Set up output ofsoftswitch13 logs and ns-3 traces
  DatapathLogs ();
  PcapAsciiTraces ();

  // 11) Creating remaining stats calculator for output dump
  m_admissionStats  = CreateObject<AdmissionStatsCalculator> ();
  m_gatewayStats    = CreateObject<GatewayStatsCalculator> ();
  m_internetStats   = CreateObject<WebQueueStatsCalculator> ();
  m_epcS1uStats     = CreateObject<EpcS1uStatsCalculator> ();
}

void
SimulationScenario::SetSwitches (uint16_t value)
{
  m_nSwitches = value;
  Config::SetDefault ("ns3::RingNetwork::NumSwitches", UintegerValue (m_nSwitches));
}

void
SimulationScenario::SetEnbs (uint16_t value)
{
  m_nEnbs = value;
  Config::SetDefault ("ns3::LteHexGridNetwork::Enbs", UintegerValue (m_nEnbs));
}

void 
SimulationScenario::SetCommonPrefix (std::string prefix)
{
  // Parsing common prefix
  if (prefix != "")
    {
      char lastChar = *prefix.rbegin (); 
      if (lastChar != '-')
        {
          prefix += "-";
        }
    }
  m_inputPrefix = prefix;
  
  ostringstream ss;
  ss << prefix << RngSeedManager::GetRun () << "-";
  m_outputPrefix = ss.str ();

  Config::SetDefault ("ns3::AdmissionStatsCalculator::AdmStatsFilename", 
                      StringValue (m_outputPrefix + "adm_stats.txt"));
  Config::SetDefault ("ns3::AdmissionStatsCalculator::BrqStatsFilename", 
                      StringValue (m_outputPrefix + "brq_stats.txt"));
  Config::SetDefault ("ns3::EpcS1uStatsCalculator::AppStatsFilename", 
                      StringValue (m_outputPrefix + "app_stats.txt"));
  Config::SetDefault ("ns3::EpcS1uStatsCalculator::EpcStatsFilename", 
                      StringValue (m_outputPrefix + "epc_stats.txt"));
  Config::SetDefault ("ns3::WebQueueStatsCalculator::WebStatsFilename", 
                      StringValue (m_outputPrefix + "web_stats.txt"));
  Config::SetDefault ("ns3::GatewayStatsCalculator::PgwStatsFilename", 
                      StringValue (m_outputPrefix + "pgw_stats.txt"));
  Config::SetDefault ("ns3::SwitchRulesStatsCalculator::SwtStatsFilename", 
                      StringValue (m_outputPrefix + "swt_stats.txt"));
  Config::SetDefault ("ns3::BandwidthStatsCalculator::BwdStatsFilename", 
                      StringValue (m_outputPrefix + "bwd_stats.txt"));
}

void
SimulationScenario::SetDumpTimeout (Time timeout)
{
  m_dumpTimeout = timeout;
  Simulator::Schedule (m_dumpTimeout, &SimulationScenario::DumpStatistics, this);
}

void
SimulationScenario::DumpStatistics ()
{
  m_admissionStats->DumpStatistics ();
  m_internetStats->DumpStatistics ();
  m_gatewayStats->DumpStatistics ();
  m_switchStats->DumpStatistics ();
  m_bandwidthStats->DumpStatistics ();

  Simulator::Schedule (m_dumpTimeout, &SimulationScenario::DumpStatistics, this);
}

bool
SimulationScenario::ParseTopology ()
{
  NS_LOG_INFO ("Parsing topology...");
 
  std::string name = m_inputPrefix + m_topoFilename;
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

  if (m_pcapTrace)
    {
      m_webNetwork->EnablePcap (m_outputPrefix + "internet");
      m_opfNetwork->EnableOpenFlowPcap (m_outputPrefix + "ofchannel");
      m_opfNetwork->EnableDataPcap (m_outputPrefix + "ofnetwork", true);
      m_epcHelper->EnablePcapS1u (m_outputPrefix + "lte-epc");
      m_epcHelper->EnablePcapX2 (m_outputPrefix + "lte-epc");
    }
  if (m_lteTrace)
    {
      m_lteNetwork->EnableTraces (m_outputPrefix);
    }
}

};  // namespace ns3
