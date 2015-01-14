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

NS_LOG_COMPONENT_DEFINE ("Main");

using namespace ns3;

void ConfigureDefaults ();
void PrintCurrentTime ();
void EnableProgress ();
void EnableVerbose ();

int 
main (int argc, char *argv[])
{
  // Parsing Command Line parameters
  double   simTime = 30;
  uint32_t nEnbs = 4;
  uint32_t nUes = 1;
  uint16_t nRing = 5;
  bool     verbose = false;
  bool     liblog = false;
  bool     progress = true;
  bool     ping = false;
  bool     voip = false;
  bool     http = false;
  bool     video = false;

  CommandLine cmd;
  cmd.AddValue ("simTime",  "Simulation time (s)", simTime);
  cmd.AddValue ("nEnbs",    "Number of eNBs", nEnbs);
  cmd.AddValue ("nUes",     "Number of UEs per eNB", nUes);
  cmd.AddValue ("nRing",    "Number of switches in the ring", nRing);
  cmd.AddValue ("verbose",  "Enable verbose output", verbose);
  cmd.AddValue ("liblog",   "Enable ofsoftswitch log component", liblog);
  cmd.AddValue ("progress", "Enable simulation time progress", progress);
  cmd.AddValue ("ping",     "Enable ping traffic", ping);
  cmd.AddValue ("voip",     "Enable VoIP traffic", voip);
  cmd.AddValue ("http",     "Enable HTTP traffic", http);
  cmd.AddValue ("video",    "Enable video traffic", video);
  cmd.Parse (argc, argv);
  ConfigureDefaults ();
  
  if (progress) EnableProgress ();
  if (verbose)  EnableVerbose ();
  
  Ptr<SimulationScenario> scenario = 
      CreateObject<SimulationScenario> (nEnbs, nUes, nRing);
  
  if (liblog) scenario->EnableDatapathLogs ();
  
  // Application traffic Traffic
  if (ping)   scenario->EnablePingTraffic ();
  if (http)   scenario->EnableHttpTraffic ();
  if (voip)   scenario->EnableVoipTraffic ();
  if (video)  scenario->EnableVideoTraffic ();

  if (verbose) scenario->EnableTraces ();

  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();

  scenario->PrintStats ();

  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
ConfigureDefaults ()
{
  // Increasing SrsPeriodicity to allow more UEs per eNB.
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

  // Configuring dl and up channel and bandwidth (channel #7 bandwidth: 20Mhz)
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (2750));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (20750));

  // The defaul value for TCP MSS is 536, and there's no dynamic MTU discovery
  // implemented yet. We defined this value to 1420, considering 1500 bytes for
  // Ethernet payload and 80 bytes of headers (including GTP/UDP/IP tunnel).
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1420));

  // Enabling Checksum computations
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
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
EnableProgress ()
{
  Simulator::Schedule (Seconds (0), &PrintCurrentTime);
}

void
EnableVerbose ()
{
  LogComponentEnable ("Main", LOG_LEVEL_INFO);
  LogComponentEnable ("SimulationScenario", LOG_LEVEL_INFO);
  
  // Just for warnings and errors
  LogComponentEnable ("OFSwitch13NetDevice", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_WARN);
  LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_WARN);
  
  LogComponentEnable ("OpenFlowEpcHelper", LOG_LEVEL_WARN);
  LogComponentEnable ("OpenFlowEpcNetwork", LOG_LEVEL_WARN);
  LogComponentEnable ("OpenFlowEpcController", LOG_LEVEL_ALL);
  
  LogComponentEnable ("RingNetwork", LOG_LEVEL_WARN);
  LogComponentEnable ("RingController", LOG_LEVEL_ALL);

  LogComponentEnable ("VoipClient", LOG_LOGIC);
  LogComponentEnable ("OnOffUdpTraceClient", LOG_LOGIC);
}

