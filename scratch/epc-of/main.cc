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
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

void PrintCurrentTime ();
void EnableVerbose ();

int 
main (int argc, char *argv[])
{
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns3. We defined this value to 1400,
  // considering 1500 bytes for Ethernet v2 MTU, 8 bytes for PPPoE, 40 bytes
  // for GTP/UDP/IP tunnel, and 52 byter for default TCP/IP headers. 
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  // Enabling checksum computations and packet metadata
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  PacketMetadata::Enable (); // FIXME Can be removed
  
  bool verbose = false;
  bool progress = false;
  bool lteRem = false;
  uint32_t simTime = 60;
  
  CommandLine cmd;
  cmd.AddValue ("verbose",    "Enable verbose output.", verbose);
  cmd.AddValue ("progress",   "Enable simulation time progress.", progress);
  cmd.AddValue ("simTime",    "Simulation time (seconds).", simTime);
  cmd.AddValue ("appStats",   "ns3::SimulationScenario::AppStatsFilename"); 
  cmd.AddValue ("epcStats",   "ns3::SimulationScenario::EpcStatsFilename"); 
  cmd.AddValue ("gbrStats",   "ns3::SimulationScenario::GbrStatsFilename"); 
  cmd.AddValue ("topoFile",   "ns3::SimulationScenario::TopoFilename");
  cmd.AddValue ("trace",      "ns3::SimulationScenario::Traces");
  cmd.AddValue ("liblog",     "ns3::SimulationScenario::SwitchLogs");
  cmd.AddValue ("ping",       "ns3::SimulationScenario::PingTraffic");
  cmd.AddValue ("http",       "ns3::SimulationScenario::HttpTraffic");
  cmd.AddValue ("voip",       "ns3::SimulationScenario::VoipTraffic");
  cmd.AddValue ("video",      "ns3::SimulationScenario::VideoTraffic");
  cmd.AddValue ("strategy",   "ns3::RingController::Strategy");
  cmd.AddValue ("ueFixed",    "ns3::LteHexGridNetwork::UeFixedPos");
  cmd.AddValue ("bandwidth",  "ns3::RingNetwork::LinkDataRate");
  cmd.AddValue ("radioMap",   "Generate LTE radio map", lteRem);
  cmd.Parse (argc, argv);
  
  if (progress) Simulator::Schedule (Seconds (0), &PrintCurrentTime);
  if (verbose)  EnableVerbose ();
 
  Ptr<SimulationScenario> scenario = CreateObject<SimulationScenario> ();
  scenario->BuildRingTopology ();
 
  if (lteRem)
    {
      // The channel number was mannualy set :/
      Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
      remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/11"));
      remHelper->SetAttribute ("OutputFile", StringValue ("lte-rem.out"));
      remHelper->SetAttribute ("XMin", DoubleValue (0));
      remHelper->SetAttribute ("XMax", DoubleValue (1500.0));
      remHelper->SetAttribute ("XRes", UintegerValue (500));
      remHelper->SetAttribute ("YMin", DoubleValue (0));
      remHelper->SetAttribute ("YMax", DoubleValue (1400.0));
      remHelper->SetAttribute ("YRes", UintegerValue (500));
      remHelper->SetAttribute ("Z", DoubleValue (1.5));
      remHelper->Install ();
    }

  // Run the simulation
  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
PrintCurrentTime ()
{
  std::cout << "Current simulation time: " 
            << Simulator::Now ().As (Time::S)
            << std::endl;
  Simulator::Schedule (Seconds (1) , &PrintCurrentTime);
}

void
EnableVerbose ()
{
  LogComponentEnable ("Main", LOG_LEVEL_ALL);
  LogComponentEnable ("SimulationScenario", LOG_LEVEL_INFO);
  
  LogComponentEnable ("OFSwitch13NetDevice", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_WARN);
  
  LogComponentEnable ("OpenFlowEpcHelper", LOG_LEVEL_WARN);
  LogComponentEnable ("OpenFlowEpcNetwork", LOG_LEVEL_WARN);
  LogComponentEnable ("RingNetwork", LOG_LEVEL_WARN);
  LogComponentEnable ("LteSquaredGridNetwork", LOG_LEVEL_WARN);
  
  LogComponentEnable ("RoutingInfo", LOG_LEVEL_ALL);
  LogComponentEnable ("RoutingInfo", LOG_PREFIX_TIME);
  LogComponentEnable ("OpenFlowEpcController", LOG_LEVEL_ALL);
  LogComponentEnable ("OpenFlowEpcController", LOG_PREFIX_TIME);
  LogComponentEnable ("RingController", LOG_LEVEL_ALL);
  LogComponentEnable ("RingController", LOG_PREFIX_TIME);
  
  LogComponentEnable ("HttpClient", LOG_LEVEL_WARN);
  LogComponentEnable ("HttpServer", LOG_LEVEL_WARN);
  LogComponentEnable ("UdpServer", LOG_LEVEL_WARN);
  LogComponentEnable ("VideoClient", LOG_LEVEL_WARN);
  LogComponentEnable ("VoipClient", LOG_LEVEL_WARN);
  LogComponentEnable ("VoipServer", LOG_LEVEL_WARN);
}

