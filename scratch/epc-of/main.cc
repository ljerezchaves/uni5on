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

#include <ns3/core-module.h>
#include <ns3/config-store-module.h>
#include "simulation-scenario.h"
#include <iomanip>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

void PrintCurrentTime (uint32_t);
void EnableVerbose (bool);

//
// Prefixes used by input and output filenames.
//
static ns3::GlobalValue
g_inputPrefix ("InputPrefix", "Common prefix for output filenames.",
                ns3::StringValue (""),
                ns3::MakeStringChecker ());

static ns3::GlobalValue
g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                ns3::StringValue (""),
                ns3::MakeStringChecker ());

int
main (int argc, char *argv[])
{
  bool        verbose  = false;
  uint32_t    progress = 0;
  uint32_t    simTime  = 250;
  std::string prefix   = "";
  std::string cfgName  = "topology.txt";

  //
  // Parsing simulation arguments
  //
  CommandLine cmd;
  cmd.AddValue ("verbose",    "Enable verbose output.", verbose);
  cmd.AddValue ("progress",   "Simulation progress interval [s].", progress);
  cmd.AddValue ("simTime",    "Simulation stop time [s].", simTime);
  cmd.AddValue ("prefix",     "Common prefix for filenames.", prefix);
  cmd.AddValue ("cfgName",    "Configuration filename.", cfgName);
  cmd.AddValue ("pcap",       "ns3::SimulationScenario::PcapTrace");
  cmd.AddValue ("ascii",      "ns3::SimulationScenario::LteTrace");
  cmd.AddValue ("liblog",     "ns3::SimulationScenario::SwitchLogs");
  cmd.AddValue ("radioMap",   "ns3::LteHexGridNetwork::PrintRem");
  cmd.AddValue ("voip",       "ns3::TrafficHelper::VoipTraffic");
  cmd.AddValue ("gbrLiveVid", "ns3::TrafficHelper::GbrLiveVideoTraffic");
  cmd.AddValue ("buffVid",    "ns3::TrafficHelper::BufferedVideoTraffic");
  cmd.AddValue ("nonLiveVid", "ns3::TrafficHelper::NonGbrLiveVideoTraffic");
  cmd.AddValue ("http",       "ns3::TrafficHelper::HttpTraffic");
  cmd.AddValue ("fast",       "ns3::TrafficHelper::FastTraffic");
  cmd.AddValue ("strategy",   "ns3::RingController::Strategy");
  cmd.Parse (argc, argv);

  //
  // Updating input and output global prefixes
  //
  ostringstream inputPrefix, outputPrefix;
  inputPrefix << prefix;
  if (prefix != "")
    {
      char lastChar = *prefix.rbegin ();
      if (lastChar != '-')
        {
          inputPrefix << "-";
        }
    }
  outputPrefix << inputPrefix.str () << RngSeedManager::GetRun () << "-";
  Config::SetGlobal ("InputPrefix", StringValue (inputPrefix.str ()));
  Config::SetGlobal ("OutputPrefix", StringValue (outputPrefix.str ()));

  //
  // Reading the configuration file
  //
  std::string cfgFilename = inputPrefix.str () + cfgName;
  std::ifstream testFile (cfgFilename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (testFile.good (), "Invalid topology file.");
  testFile.close ();
  
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (cfgFilename));
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we must enable checksum computations. 
  //
  Config::SetGlobal ("ChecksumEnabled", BooleanValue (true));

  //
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns-3. To allow larger TCP packets, we defined
  // this value to 1400, based on 1500 bytes for Ethernet v2 MTU, and
  // considering 8 bytes for PPPoE header, 40 bytes for GTP/UDP/IP tunnel
  // headers, and 52 byter for default TCP/IP headers.
  //
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  //
  // Whenever possible, use the Full Duplex CSMA channel to improve throughput.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  //
  // Enable verbose output and progress report for debug purposes
  //
  PrintCurrentTime (progress);
  EnableVerbose (verbose);

  //
  // Create the scenario
  //
  NS_LOG_INFO ("Creating simulation scenario...");
  Ptr<SimulationScenario> scenario = CreateObject<SimulationScenario> ();
  scenario->BuildRingTopology ();

  //
  // Run the simulation
  //
  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
PrintCurrentTime (uint32_t interval)
{
  if (interval)
    {
      std::cout << "Current simulation time: "
                << Simulator::Now ().As (Time::S)
                << std::endl;
      Simulator::Schedule (Seconds (interval), &PrintCurrentTime, interval);
    }
}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogComponentEnable ("Main", LOG_INFO);
      LogComponentEnable ("SimulationScenario", LOG_LEVEL_INFO);
      LogComponentEnable ("StatsCalculator", LOG_LEVEL_WARN);

      LogComponentEnable ("OFSwitch13NetDevice", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_WARN);

      LogComponentEnable ("OpenFlowEpcHelper", LOG_LEVEL_WARN);
      LogComponentEnable ("OpenFlowEpcNetwork", LOG_LEVEL_ALL);
      LogComponentEnable ("RingNetwork", LOG_LEVEL_WARN);
      LogComponentEnable ("LteHexGridNetwork", LOG_LEVEL_WARN);
      LogComponentEnable ("LteHexGridEnbTopologyHelper", LOG_LEVEL_WARN);
      LogComponentEnable ("ConnectionInfo", LOG_LEVEL_WARN);
      LogComponentEnable ("RoutingInfo", LOG_LEVEL_WARN);
      
      LogComponentEnable ("OpenFlowEpcController", LOG_LEVEL_ALL);
      LogComponentEnable ("OpenFlowEpcController", LOG_PREFIX_TIME);
      LogComponentEnable ("RingController", LOG_LEVEL_ALL);
      LogComponentEnable ("RingController", LOG_PREFIX_TIME);

      LogComponentEnable ("HttpClient", LOG_LEVEL_WARN);
      LogComponentEnable ("HttpServer", LOG_LEVEL_WARN);
      LogComponentEnable ("VoipClient", LOG_LEVEL_WARN);
      LogComponentEnable ("VoipServer", LOG_LEVEL_WARN);
      LogComponentEnable ("StoredVideoClient", LOG_LEVEL_WARN);
      LogComponentEnable ("StoredVideoServer", LOG_LEVEL_WARN);
      LogComponentEnable ("RealTimeVideoClient", LOG_LEVEL_WARN);
      LogComponentEnable ("RealTimeVideoServer", LOG_LEVEL_WARN);
      LogComponentEnable ("TrafficManager", LOG_LEVEL_WARN);
      LogComponentEnable ("TrafficHelper", LOG_LEVEL_WARN);
      LogComponentEnable ("EpcApplication", LOG_LEVEL_WARN);
    }
}
