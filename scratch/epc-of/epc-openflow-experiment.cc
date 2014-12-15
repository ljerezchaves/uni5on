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
#include <ns3/network-module.h>
#include <ns3/config-store-module.h>
#include <ns3/flow-monitor-module.h>
#include "internet-network.h"
#include "ring-openflow-network.h"
#include "lte-squared-grid-network.h"
#include "lte-applications.h"

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcExperiment");

using namespace ns3;

void
PrintCurrentTime ()
{
  std::cout << "Current simulation time: " 
            << Simulator::Now ().As (Time::S)
            << std::endl;
  Simulator::Schedule (Seconds (1) , &PrintCurrentTime);
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
EnableVerbose (bool verbose, bool progress)
{
  if (verbose) 
    {
      LogComponentEnable ("OpenFlowEpcExperiment", LOG_LEVEL_INFO);
      
      // Just for warnings and errors
      LogComponentEnable ("OFSwitch13NetDevice", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_WARN);
      LogComponentEnable ("OpenFlowEpcHelper", LOG_LEVEL_WARN);
      LogComponentEnable ("OpenFlowEpcNetwork", LOG_LEVEL_WARN);
      LogComponentEnable ("RingOpenFlowNetwork", LOG_LEVEL_WARN);

      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_WARN);
      LogComponentEnable ("EpcSdnController", LOG_LEVEL_ALL);
      LogComponentEnable ("RingController", LOG_LEVEL_ALL);
      
      LogComponentEnable ("VoipClient", LOG_LOGIC);
      LogComponentEnable ("OnOffUdpTraceClient", LOG_LOGIC);
    }

  if (progress)
    {
      // Enabling progress feedback
      Simulator::Schedule (Seconds (0), &PrintCurrentTime);
    }
}

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
  EnableVerbose (verbose, progress);

  
// ---------------------------------------------------- //
// Creating the scenario topology, setting up callbacks //
// ---------------------------------------------------- //

  // OpenFlow ring network (for EPC)
  Ptr<OpenFlowEpcNetwork> opfNetwork = CreateObject<RingOpenFlowNetwork> ();
  opfNetwork->SetAttribute ("NumSwitches", UintegerValue (nRing));
  opfNetwork->SetAttribute ("LinkDataRate", DataRateValue (DataRate ("1000Kb/s")));

  Ptr<EpcSdnController> controller = CreateObject<RingController> ();
  controller->SetOpenFlowNetwork (opfNetwork);
  opfNetwork->CreateTopology (controller);

  // LTE EPC core (with callbacks setup)
  Ptr<OpenFlowEpcHelper> epcHelper = CreateObject<OpenFlowEpcHelper> ();
  epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, opfNetwork));
  epcHelper->SetX2ConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, opfNetwork));
  epcHelper->SetAddBearerCallback (
      MakeCallback (&EpcSdnController::RequestNewDedicatedBearer, controller));
  epcHelper->SetCreateSessionRequestCallback (
      MakeCallback (&EpcSdnController::NotifyNewContextCreated, controller));
  
  // LTE radio access network
  Ptr<LteSquaredGridNetwork> lteNetwork = CreateObject<LteSquaredGridNetwork> ();
  lteNetwork->SetAttribute ("Enbs", UintegerValue (nEnbs));
  lteNetwork->SetAttribute ("Ues", UintegerValue (nUes));
  lteNetwork->CreateTopology (epcHelper);

  // Internet network
  Ptr<InternetNetwork> webNetwork = CreateObject<InternetNetwork> ();
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  Ptr<Node> webHost = webNetwork->CreateTopology (pgw);


// -------------------------------------------- //
// Creating applications for traffic generation //
// -------------------------------------------- //

  NodeContainer ueNodes = lteNetwork->GetUeNodes ();
  NetDeviceContainer ueDevices = lteNetwork->GetUeDevices ();
  Ptr<LteHelper> lteHelper = lteNetwork->GetLteHelper ();

  // ICMP ping over default Non-GBR EPS bearer (QCI 9)
  if (ping) 
    {
      SetPingTraffic (webHost, ueNodes);
    }

  // HTTP traffic over default Non-GBR EPS bearer (QCI 9) 
  if (http) 
    {
      SetHttpTraffic (webHost, ueNodes, ueDevices, lteHelper);
    }

  // VoIP traffic over dedicated GBR EPS bearer (QCI 1) 
  ApplicationContainer voipServers;
  if (voip) 
    {
      voipServers = SetVoipTraffic (webHost, ueNodes, ueDevices, lteHelper, 
                                    controller);
    }

  // Buffered video streaming over dedicated GBR EPS bearer (QCI 4)
  ApplicationContainer videoServers;
  if (video) 
    {
      videoServers = SetVideoTraffic (webHost, ueNodes, ueDevices, lteHelper, 
                                      controller);
    }

// --------------------------------- //
// Creating monitors and trace files //
// --------------------------------- //

  // Install FlowMonitor
  FlowMonitorHelper flowmonHelper;
  NodeContainer nodesFlowmon;
  nodesFlowmon.Add (webHost);
  nodesFlowmon.Add (ueNodes.Get (0));
  flowmonHelper.Install (nodesFlowmon);
  
  // Enable LTE and PCAP traces
  webNetwork->EnablePcap ("web");
  opfNetwork->EnableOpenFlowPcap ("openflow-channel");
  opfNetwork->EnableDataPcap ("ofn", true);
  epcHelper->EnablePcapS1u ("epc");
  // epcHelper->EnablePcapX2 ("epc");
  // lteNetwork->EnableTraces ();

  // Enabling library log
  if (liblog) 
    {
      opfNetwork->EnableDatapathLogs ();
    }

  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  
  // Flowmonitor statistcs
  flowmonHelper.SerializeToXmlFile ("FlowMonitorStats.xml", false, false);
 
  // Printing some other statistics
  DynamicCast<RingController> (controller)->PrintBlockRatioStatistics ();

  ApplicationContainer::Iterator it;
  
  for (it = voipServers.Begin (); it != voipServers.End (); it++)
    {
      Ptr<UdpServer> server = DynamicCast<UdpServer> (*it);
      std::cout << "For application " << server << ": " <<
                   server->GetReceived () << " pkts received, " <<
                   server->GetLost () << " pkts lost, " << 
                   server->GetAverageDelay ().ToInteger (Time::MS) << " ms avg delay, " << 
                   server->GetAverageJitter ().ToInteger (Time::MS) << " ms avg jitter." << std::endl;
    }

   for (it = videoServers.Begin (); it != videoServers.End (); it++)
    {
      Ptr<UdpServer> server = DynamicCast<UdpServer> (*it);
      std::cout << "For application " << server << ": " <<
                   server->GetReceived () << " pkts received, " <<
                   server->GetLost () << " pkts lost, " << 
                   server->GetAverageDelay ().ToInteger (Time::MS) << " ms avg delay, " << 
                   server->GetAverageJitter ().ToInteger (Time::MS) << " ms avg jitter." << std::endl;
    }



  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}
