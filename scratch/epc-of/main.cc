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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

void ForceDefaults ();
void PrintCurrentTime (uint32_t);
void EnableVerbose (bool);

int
main (int argc, char *argv[])
{
  bool     verbose  = false;
  uint32_t progress = 0;
  uint32_t simTime  = 250;

  // Parse input config arguments
  // ConfigStore inputConfig;
  // inputConfig->SetAttribute ("Mode", StringValue ("Load"));
  // inputConfig->SetAttribute ("FileFormat", StringValue ("RawText"));
  // inputConfig.ConfigureDefaults ();

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("verbose",    "Enable verbose output.", verbose);
  cmd.AddValue ("progress",   "Simulation progress interval [s].", progress);
  cmd.AddValue ("simTime",    "Simulation time [s].", simTime);
  cmd.AddValue ("topoFile",   "ns3::SimulationScenario::TopoFilename");
  cmd.AddValue ("prefix",     "ns3::SimulationScenario::CommonPrefix");
  cmd.AddValue ("pcap",       "ns3::SimulationScenario::PcapTrace");
  cmd.AddValue ("trace",      "ns3::SimulationScenario::LteTrace");
  cmd.AddValue ("radioMap",   "ns3::SimulationScenario::LteRem");
  cmd.AddValue ("liblog",     "ns3::SimulationScenario::SwitchLogs");
  cmd.AddValue ("voip",       "ns3::TrafficHelper::VoipTraffic");
  cmd.AddValue ("gbrLiveVid", "ns3::TrafficHelper::GbrLiveVideoTraffic");
  cmd.AddValue ("buffVid",    "ns3::TrafficHelper::BufferedVideoTraffic");
  cmd.AddValue ("nonLiveVid", "ns3::TrafficHelper::NonGbrLiveVideoTraffic");
  cmd.AddValue ("http",       "ns3::TrafficHelper::HttpTraffic");
  cmd.AddValue ("fast",       "ns3::TrafficHelper::FastTraffic");
  cmd.AddValue ("strategy",   "ns3::RingController::Strategy");
  cmd.AddValue ("bandwidth",  "ns3::RingNetwork::SwitchLinkDataRate");
  cmd.Parse (argc, argv);
  
  // Force (override) some default attributes
  ForceDefaults ();

  // Enable verbose and progress report output for debug
  PrintCurrentTime (progress);
  EnableVerbose (verbose);

  // Create the simulation scenario
  NS_LOG_INFO ("Creating simulation scenario...");
  Ptr<SimulationScenario> scenario = CreateObject<SimulationScenario> ();
  scenario->BuildRingTopology ();

  // Run the simulation
  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
ForceDefaults ()
{
  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we need to enable checksum computations (which are
  // disabled by default in ns-3).
  //
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

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
      LogComponentEnable ("ConnectionInfo", LOG_LEVEL_WARN);

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
      LogComponentEnable ("LteHexGridEnbTopologyHelper", LOG_LOGIC);

      LogComponentEnable ("RoutingInfo", LOG_LEVEL_ALL);
      LogComponentEnable ("RoutingInfo", LOG_PREFIX_TIME);
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
      LogComponentEnable ("TrafficManager", LOG_LEVEL_ALL);
      LogComponentEnable ("TrafficHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("EpcApplication", LOG_LEVEL_ALL);
    }
}
