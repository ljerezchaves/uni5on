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
#include "ring-open-flow-network.h"
#include "lte-squared-grid-network.h"
#include "lte-applications.h"
#include "epc-sdn-controller.h"

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

int 
main (int argc, char *argv[])
{
  // Simulation parameters
  uint32_t  nUes = 2;
  uint32_t  nEnbs = 2;
  uint16_t  nRing = 3;
  double    simTime = 10;
  bool      verbose = 0;
  bool      progress = 0;
  bool      video = 0;
  bool      voip = 0;
  bool      http = 0;
  bool      ping = 0;
  bool      dual = 0;
  uint32_t  dualFlows = 1;
  bool      dualDown = 1;
  bool      dualUp = 1;
  bool      dualUseUdp = 0;

  // Increasing SrsPeriodicity to allow more UEs per eNB.
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

  // Configuring dl and up transmission bandwidth in number of RBs
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (75));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (75));

  // The defaul value for TCP MSS is 536, and there's no dynamic MTU discovery
  // implemented yet. We defined this value to 1420, considering 1500 bytes for
  // Ethernet payload and 80 bytes of headers (including GTP/UDP/IP tunnel).
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1420));

  // Enabling Checksum computations
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  // Parsing Command Line parameter
  CommandLine cmd;
  cmd.AddValue ("verbose",  "Enable verbose output", verbose);
  cmd.AddValue ("progress", "Enable simulation time progress", progress);
  cmd.AddValue ("simTime",  "Simulation time (s)", simTime);
  cmd.AddValue ("nEnbs",    "Number of eNBs", nEnbs);
  cmd.AddValue ("nUes",     "Number of UEs per eNB", nUes);
  cmd.AddValue ("nRing",    "Number of switches in the ring", nRing);
  cmd.AddValue ("ping",     "Enable ping traffic", ping);
  cmd.AddValue ("voip",     "Enable VoIP traffic", voip);
  cmd.AddValue ("http",     "Enable HTTP traffic", http);
  cmd.AddValue ("video",    "Enable video traffic", video);
  cmd.AddValue ("dual",     "Enable Lena Dual Stripe traffic", dual);
  cmd.AddValue ("dualFlows",  "  dual config: flows per UE", dualFlows);
  cmd.AddValue ("dualUseUdp", "  dual config: UDP traffic (default TCP)", dualUseUdp);
  cmd.AddValue ("dualDown",   "  dual config: downlink traffic", dualDown);
  cmd.AddValue ("dualUp",     "  dual config: uplink traffic", dualUp);
  cmd.Parse (argc, argv);

  // Enabling log components
  if (progress)
    Simulator::Schedule (Seconds (0), &PrintCurrentTime);

  if (verbose) 
    {
      LogComponentEnable ("OpenFlowEpcExperiment", LOG_LEVEL_INFO);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      LogComponentEnable ("LearningController", LOG_LEVEL_ALL);
    }

  /*****************************************************************************
   * Creating the scenario topology, setting up callbacks
   */

  // OpenFlow network (for EPC)
  Ptr<OpenFlowEpcNetwork> opfNetwork = CreateObject<RingOpenFlowNetwork> ();
  Ptr<EpcSdnController> controller = CreateObject<EpcSdnController> ();
  opfNetwork->SetAttribute ("NumSwitches", UintegerValue (nRing));
  opfNetwork->SetAttribute ("LinkDataRate", DataRateValue (DataRate ("100Mb/s")));
  opfNetwork->CreateTopology (controller);
 
  // LTE EPC core (with callbacks setup)
  Ptr<OpenFlowEpcHelper> epcHelper = CreateObject<OpenFlowEpcHelper> ();
  epcHelper->SetS1uConnectCallback (MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, opfNetwork));
  epcHelper->SetX2ConnectCallback (MakeCallback (&OpenFlowEpcNetwork::AttachToX2, opfNetwork));
  epcHelper->SetAddBearerCallback (MakeCallback (&EpcSdnController::AddBearer, controller));
  
  // LTE radio access network
  Ptr<LteSquaredGridNetwork> lteNetwork = CreateObject<LteSquaredGridNetwork> ();
  lteNetwork->SetAttribute ("Enbs", UintegerValue (nEnbs));
  lteNetwork->SetAttribute ("Ues", UintegerValue (nUes));
  lteNetwork->CreateTopology (epcHelper);

  // Internet network
  Ptr<InternetNetwork> webNetwork = CreateObject<InternetNetwork> ();
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  Ptr<Node> webHost = webNetwork->CreateTopology (pgw);


  /*****************************************************************************
   * Creating applications for traffic generation
   */

  NodeContainer ueNodes = lteNetwork->GetUeNodes ();
  NetDeviceContainer ueDevices = lteNetwork->GetUeDevices ();
  Ptr<LteHelper> lteHelper = lteNetwork->GetLteHelper ();
  
  Ptr<UniformRandomVariable> appRngStart = CreateObject<UniformRandomVariable> ();
  appRngStart->SetAttribute ("Min", DoubleValue (0.5));
  appRngStart->SetAttribute ("Max", DoubleValue (1.0));

  // ICMP ping over default Non-GBR EPS bearer (QCI 9)
  if (ping) SetPingTraffic (webHost, ueNodes, appRngStart);

  // HTTP traffic over dedicated Non-GBR EPS bearer (QCI 8) 
  if (http) SetHttpTraffic (webHost, ueNodes, ueDevices, lteHelper, appRngStart);

  // VoIP traffic over dedicated GBR EPS bearer (QCI 1) 
  if (voip) SetVoipTraffic (webHost, ueNodes, ueDevices, lteHelper, appRngStart);

  // Buffered video streaming over dedicated GBR EPS bearer (QCI 4)
  if (video) SetVideoTraffic (webHost, ueNodes, ueDevices, lteHelper, appRngStart);

  // TCP/UDP Downlink/Uplink traffic over dedicated Non-GBR EPS beareres (QCI 8)
  if (dual) SetLenaDualStripeTraffic (webHost, ueNodes, ueDevices, lteHelper, appRngStart, 
                                      dualFlows, dualUseUdp, dualUp, dualDown);

  /*****************************************************************************
   * Creating monitors and trace files
   */

  // Install FlowMonitor
  FlowMonitorHelper flowmonHelper;
  NodeContainer nodesFlowmon;
  nodesFlowmon.Add (webHost);
  nodesFlowmon.Add (ueNodes.Get (0));
  flowmonHelper.Install (nodesFlowmon);

  // Enable LTE and PCAP traces
  webNetwork->EnablePcap ("web");
  // lteNetwork->EnableTraces ();
  opfNetwork->EnableOpenFlowPcap ("openflow-channel");
  opfNetwork->EnablePcap ("ofn");
  epcHelper->EnablePcapS1u ("epc");
  // epcHelper->EnablePcapX2 ("epc");

  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  
  // Flowmonitor statistcs
  flowmonHelper.SerializeToXmlFile ("FlowMonitorStats.xml", false, false);
  
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}
