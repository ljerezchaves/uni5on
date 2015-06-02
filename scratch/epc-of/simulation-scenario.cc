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
    m_pgwHost (0)
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
}

TypeId 
SimulationScenario::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationScenario")
    .SetParent<Object> ()
    .AddConstructor<SimulationScenario> ()
    .AddAttribute ("AppStatsFilename",
                   "Filename for application QoS statistics.",
                   StringValue ("app_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_appStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC QoS statistics.",
                   StringValue ("epc_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_epcStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("PgwStatsFilename",
                   "Filename for packet gateway traffic statistics.",
                   StringValue ("pgw_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_pgwStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("SwtStatsFilename",
                   "FilName for flow table entries statistics.",
                   StringValue ("swt_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_swtStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("AdmStatsFilename",
                   "Filename for bearer admission control statistics.",
                   StringValue ("adm_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_admStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("WebStatsFilename",
                   "Filename for internet queue statistics.",
                   StringValue ("web_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_webStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwdStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("bwd_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_bwdStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("brq_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_brqStatsFilename),
                   MakeStringChecker ())
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

void
SimulationScenario::BuildRingTopology ()
{
  NS_LOG_FUNCTION (this);

  ParseTopology ();

  // OpenFlow EPC ring controller
  m_controller = CreateObject<RingController> ();
  m_controller->SetAttribute ("BwReserve", DoubleValue (0.2));
  Names::Add ("ctrlApp", m_controller);
  
  // OpenFlow EPC ring network (considering 20km fiber cable latency)
  m_opfNetwork = CreateObject<RingNetwork> ();
  m_opfNetwork->SetAttribute ("NumSwitches", UintegerValue (m_nSwitches));
  m_opfNetwork->SetAttribute ("LinkDelay", TimeValue (MicroSeconds (100)));
  m_opfNetwork->CreateTopology (m_controller, m_SwitchIdxPerEnb);
  
  // LTE EPC core (with callbacks setup)
  m_epcHelper = CreateObject<OpenFlowEpcHelper> ();
  m_epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
  m_epcHelper->SetX2ConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));
//  m_epcHelper->SetAddBearerCallback (
//      MakeCallback (&OpenFlowEpcController::RequestNewDedicatedBearer, m_controller));
  m_epcHelper->SetCreateSessionRequestCallback (
      MakeCallback (&OpenFlowEpcController::NotifyNewContextCreated, m_controller));
  
  // LTE radio access network
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteNetwork->SetAttribute ("Enbs", UintegerValue (m_nEnbs));
  m_lteHelper = m_lteNetwork->CreateTopology (m_epcHelper, m_UesPerEnb);

  // Internet network
  m_webNetwork = CreateObject<InternetNetwork> ();
  m_webHost = m_webNetwork->CreateTopology (m_epcHelper->GetPgwNode ());

  // Registering controller trace sinks
  m_opfNetwork->ConnectTraceSinks ();
 // TODO criar um objeto para gerenciar as saídas em texto, e passar como parametro pra que cada um se vire para ligar os sources nos sinks 
  // Saving simulation statistics 
  m_controller->TraceConnectWithoutContext ("AppStats", 
      MakeCallback (&SimulationScenario::ReportAppStats, this));
  
  m_opfNetwork->TraceConnectWithoutContext ("EpcStats", 
      MakeCallback (&SimulationScenario::ReportEpcStats, this));
  m_opfNetwork->TraceConnectWithoutContext ("PgwStats", 
      MakeCallback (&SimulationScenario::ReportPgwStats, this));
  m_opfNetwork->TraceConnectWithoutContext ("BwdStats", 
      MakeCallback (&SimulationScenario::ReportBwdStats, this));
  m_opfNetwork->TraceConnectWithoutContext ("SwtStats", 
      MakeCallback (&SimulationScenario::ReportSwtStats, this));
  
  m_controller->TraceConnectWithoutContext ("AdmStats", 
      MakeCallback (&SimulationScenario::ReportAdmStats, this));
  m_controller->TraceConnectWithoutContext ("BrqStats", 
      MakeCallback (&SimulationScenario::ReportBrqStats, this)); 
  
  m_webNetwork->TraceConnectWithoutContext ("WebStats",
      MakeCallback (&SimulationScenario::ReportWebStats, this));

  // Installing applications and traffic manager
  TrafficHelper tfcHelper (m_webHost, m_lteHelper, m_controller, m_opfNetwork);
  tfcHelper.Install (m_lteNetwork->GetUeNodes (), m_lteNetwork->GetUeDevices ());

  // Logs and traces
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
  ostringstream ss;
  ss << prefix << RngSeedManager::GetRun () << "-";
  m_commonPrefix = ss.str ();

  // Updating input filename with input prefix (without run number)
  m_topoFilename = prefix + m_topoFilename;

  // Updating output filenames with common prefix (with run number)
  m_appStatsFilename = m_commonPrefix + m_appStatsFilename;
  m_epcStatsFilename = m_commonPrefix + m_epcStatsFilename;
  m_pgwStatsFilename = m_commonPrefix + m_pgwStatsFilename;
  m_admStatsFilename = m_commonPrefix + m_admStatsFilename;
  m_webStatsFilename = m_commonPrefix + m_webStatsFilename;
  m_swtStatsFilename = m_commonPrefix + m_swtStatsFilename;
  m_bwdStatsFilename = m_commonPrefix + m_bwdStatsFilename;
  m_brqStatsFilename = m_commonPrefix + m_brqStatsFilename;
}


void 
SimulationScenario::ReportAppStats (std::string description, uint32_t teid,
                                    Ptr<const QosStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this << teid);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_appStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_appStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (12) << "Active (s)"
              << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"
              << setw (9)  << "Rx Pkts"
              << setw (12) << "Loss ratio"
              << setw (6)  << "Losts"
              << setw (10) << "Rx Bytes"
              << setw (8)  << "Throughput (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (m_appStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_appStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << description
          << setw (6)  << teid
          << setw (12) << stats->GetActiveTime ().GetSeconds ()
          << setw (12) << fixed << stats->GetRxDelay ().GetSeconds () * 1000
          << setw (12) << fixed << stats->GetRxJitter ().GetSeconds () * 1000
          << setw (9)  << fixed << stats->GetRxPackets ()
          << setw (12) << fixed << stats->GetLossRatio ()
          << setw (6)  << fixed << stats->GetLostPackets ()
          << setw (10) << fixed << stats->GetRxBytes ()
          << setw (8)  << fixed << (double)(stats->GetRxThroughput ().GetBitRate ()) / 1024
          << std::endl;
  outFile.close ();
}

void 
SimulationScenario::ReportEpcStats (std::string description, uint32_t teid,
                                    Ptr<const QosStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this << teid);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_epcStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_epcStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (12) << "Active (s)"
              << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"
              << setw (9)  << "Rx Pkts"
              << setw (12) << "Loss ratio"
              << setw (7)  << "Losts"
              << setw (7)  << "Meter"
              << setw (7)  << "Queue"
              << setw (10) << "Rx Bytes"
              << setw (8)  << "Throughput (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (m_epcStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_epcStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << description
          << setw (6)  << teid
          << setw (12) << stats->GetActiveTime ().GetSeconds ()
          << setw (12) << fixed << stats->GetRxDelay ().GetSeconds () * 1000
          << setw (12) << fixed << stats->GetRxJitter ().GetSeconds () * 1000
          << setw (9)  << fixed << stats->GetRxPackets ()
          << setw (12) << fixed << stats->GetLossRatio ()
          << setw (7)  << fixed << stats->GetLostPackets ()
          << setw (7)  << fixed << stats->GetMeterDrops ()
          << setw (7)  << fixed << stats->GetQueueDrops ()
          << setw (10) << fixed << stats->GetRxBytes ()
          << setw (8)  << fixed << (double)(stats->GetRxThroughput ().GetBitRate ()) / 1024
          << std::endl;
  outFile.close ();
}

void
SimulationScenario::ReportAdmStats (Ptr<const AdmissionStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_admStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_admStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (27) << "GBR"
              << setw (27) << "Non-GBR"
              << std::endl
              << setw (12) << " "
              << setw (9) << "Requests"
              << setw (9)  << "Blocks"
              << setw (9)  << "Ratio"
              << setw (9)  << "Requests"
              << setw (9)  << "Blocks"
              << setw (9)  << "Ratio"
              << std::endl;
    }
  else
    {
      outFile.open (m_admStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_admStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (9)  << stats->GetGbrRequests ()
          << setw (9)  << stats->GetGbrBlocked ()
          << setw (9)  << stats->GetGbrBlockRatio ()
          << setw (9)  << stats->GetNonGbrRequests ()
          << setw (9)  << stats->GetNonGbrBlocked ()
          << setw (9)  << stats->GetNonGbrBlockRatio ()
          << std::endl;
  outFile.close ();
}

void 
SimulationScenario::ReportPgwStats (DataRate downTraffic, DataRate upTraffic)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_pgwStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_pgwStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (17) << "Downlink (kbps)"
              << setw (14) << "Uplink (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (m_pgwStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_pgwStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << (double)downTraffic.GetBitRate () / 1024
          << setw (14) << (double)upTraffic.GetBitRate () / 1024
          << std::endl;
  outFile.close ();
}

void 
SimulationScenario::ReportSwtStats (std::vector<uint32_t> teid)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;
  size_t switches = teid.size ();

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_swtStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_swtStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (10)  << "Pgw"
              << setw (48) << "eNB switches"
              << std::endl
              << setw (12) << " "
              << setw (10) << " ";
      for (size_t i = 1; i < switches; i++)
        {
          outFile << setw (5) << i;
        }
      outFile << setw (12) << "Average" << std::endl;
    }
  else
    {
      outFile.open (m_swtStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_swtStatsFilename);
          return;
        }
    }

  double enbSum = 0;
  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ();
  
  std::vector<uint32_t>::iterator it = teid.begin ();
  outFile << setw (10) << *it;
  for (it++; it != teid.end (); it++)
    {
      outFile << setw (5) << *it;
      enbSum += *it;
    }
  
  outFile << setw (12) << (enbSum / (switches - 1)) << std::endl;
  outFile.close ();
}

void
SimulationScenario::ReportWebStats (Ptr<const Queue> downlink, 
                                    Ptr<const Queue> uplink)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_webStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_webStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s) " 
              << setw (48) << "Downlink"
              << setw (48) << "Uplink"
              << std::endl
              << setw (12) << " " 
              << setw (12) << "Pkts"
              << setw (12) << "Bytes"
              << setw (12) << "Pkts drop"
              << setw (12) << "Bytes drop"
              << setw (12) << "Pkts"
              << setw (12) << "Bytes"
              << setw (12) << "Pkts drop"
              << setw (12) << "Bytes drop"
              << std::endl;
    }
  else
    {
      outFile.open (m_webStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_webStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (12) << downlink->GetTotalReceivedPackets ()
          << setw (12) << downlink->GetTotalReceivedBytes ()
          << setw (12) << downlink->GetTotalDroppedPackets ()
          << setw (12) << downlink->GetTotalDroppedBytes ()
          << setw (12) << uplink->GetTotalReceivedPackets ()
          << setw (12) << uplink->GetTotalReceivedBytes ()
          << setw (12) << uplink->GetTotalDroppedPackets ()
          << setw (12) << uplink->GetTotalDroppedBytes ()
          << std::endl;
  outFile.close ();
}

void
SimulationScenario::ReportBwdStats (std::vector<BandwidthStats_t> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_bwdStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_bwdStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)";
      std::vector<BandwidthStats_t>::iterator it;
      for (it = stats.begin (); it != stats.end (); it++)        
        {
          BandwidthStats_t entry = *it;
          SwitchPair_t pair = entry.first;
          outFile << setw (1) << pair.first
                  << "-" 
                  << setw (7) << pair.second;
        }
      outFile << std::endl;
    }
  else
    {
      outFile.open (m_bwdStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_bwdStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ();
  std::vector<BandwidthStats_t>::iterator it;
  for (it = stats.begin (); it != stats.end (); it++)        
    {
      BandwidthStats_t entry = *it;
      outFile << setw (5) << fixed << entry.second << " ";
    }
  outFile << std::endl;
  outFile.close ();
}

void
SimulationScenario::ReportBrqStats (Ptr<const BearerRequestStats> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_brqStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_brqStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (10) << "Accepted?"
              << setw (12) << "Down (kbps)"
              << setw (10) << "Up (kbps)"
              << setw (40) << "Routing paths"
              << std::endl;
    }
  else
    {
      outFile.open (m_brqStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_brqStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << stats->GetDescription ()
          << setw (6)  << stats->GetTeid ()
          << setw (10) << (stats->IsAccepted () ? "yes" : "no")
          << setw (12) << (double)(stats->GetDownDataRate ().GetBitRate ()) / 1024
          << setw (10) << (double)(stats->GetUpDataRate ().GetBitRate ()) / 1024
          << setw (40) << stats->GetRoutingPaths ()
          << std::endl;
  outFile.close ();
}

bool
SimulationScenario::ParseTopology ()
{
  NS_LOG_INFO ("Parsing topology...");
 
  std::ifstream file;
  file.open (m_topoFilename.c_str ());
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
      m_webNetwork->EnablePcap (m_commonPrefix + "internet");
      m_opfNetwork->EnableOpenFlowPcap (m_commonPrefix + "ofchannel");
      m_opfNetwork->EnableDataPcap (m_commonPrefix + "ofnetwork", true);
      m_epcHelper->EnablePcapS1u (m_commonPrefix + "lte-epc");
      m_epcHelper->EnablePcapX2 (m_commonPrefix + "lte-epc");
    }
  if (m_lteTrace)
    {
      m_lteNetwork->EnableTraces (m_commonPrefix);
    }
}

};  // namespace ns3
