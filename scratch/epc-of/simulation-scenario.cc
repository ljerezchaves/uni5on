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

const std::string SimulationScenario::m_videoDir = "../movies/";

const std::string SimulationScenario::m_videoTrace [] = {
  "jurassic.data", "silence.data", "star-wars.data", "mr-bean.data",
  "first-contact.data", "from-dusk.data", "the-firm.data", "formula1.data",
  "soccer.data", "ard-news.data", "ard-talk.data", "ns3-talk.data",
  "office-cam.data"};

const uint64_t SimulationScenario::m_avgBitRate [] = {770000, 580000, 280000,
  580000, 330000, 680000, 310000, 840000, 1100000, 720000, 540000, 550000,
  400000};

const uint64_t SimulationScenario::m_maxBitRate [] = {3300000, 4400000,
  1900000, 3100000, 2500000, 3100000, 2100000, 2900000, 3600000, 3400000,
  3100000, 3400000, 2000000};

SimulationScenario::SimulationScenario ()
  : m_opfNetwork (0),
    m_controller (0),
    m_epcHelper (0),
    m_lteNetwork (0),
    m_webNetwork (0),
    m_lteHelper (0),
    m_webHost (0),
    m_pgwHost (0),
    m_rngStart (0)
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
  
  m_controller = 0;
  m_epcHelper = 0;
  m_lteNetwork = 0;
  m_webNetwork = 0;
  m_lteHelper = 0;
  m_webHost = 0;
  m_pgwHost = 0;
  m_opfNetwork = 0;
  m_rngStart = 0;
}

TypeId 
SimulationScenario::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationScenario")
    .SetParent<Object> ()
    .AddConstructor<SimulationScenario> ()
    .AddAttribute ("AppStatsFilename",
                   "Filename for application QoS statistcis.",
                   StringValue ("app_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_appStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC QoS statistics.",
                   StringValue ("epc_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_epcStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("PgwStatsFilename",
                   "Filename for packet gateway traffic statistcs.",
                   StringValue ("pgw_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_pgwStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("SwtStatsFilename",
                   "FilName for flow table entries statistics.",
                   StringValue ("swt_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_swtStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("GbrStatsFilename",
                   "Filename for bearers resquest/block statistics.",
                   StringValue ("gbr_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_gbrStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("WebStatsFilename",
                   "Filename for internet queue statistics.",
                   StringValue ("web_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_webStatsFilename),
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
                   "Number of OpenFlow switches in network toplogy.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SimulationScenario::m_nSwitches),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PcapTrace",
                   "Enable/Disable simulation pcap traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_pcapTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("LteTrace",
                   "Enable/Disable simulation LTE ascii traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_lteTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("SwitchLogs",
                   "Set the ofsoftswitch log level.",
                   StringValue ("none"),
                   MakeStringAccessor (&SimulationScenario::m_switchLog),
                   MakeStringChecker ())
    .AddAttribute ("PingTraffic",
                   "Enable/Disable ping traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_ping),
                   MakeBooleanChecker ())
    .AddAttribute ("HttpTraffic",
                   "Enable/Disable http traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SimulationScenario::m_http),
                   MakeBooleanChecker ())
    .AddAttribute ("VoipTraffic",
                   "Enable/Disable voip traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SimulationScenario::m_voip),
                   MakeBooleanChecker ())
    .AddAttribute ("VideoTraffic",
                   "Enable/Disable video traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SimulationScenario::m_video),
                   MakeBooleanChecker ())
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
  m_epcHelper->SetAddBearerCallback (
      MakeCallback (&OpenFlowEpcController::RequestNewDedicatedBearer, m_controller));
  m_epcHelper->SetCreateSessionRequestCallback (
      MakeCallback (&OpenFlowEpcController::NotifyNewContextCreated, m_controller));
  
  // LTE radio access network
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteNetwork->SetAttribute ("Enbs", UintegerValue (m_nEnbs));
  m_lteHelper = m_lteNetwork->CreateTopology (m_epcHelper, m_UesPerEnb);

  // Internet network
  m_webNetwork = CreateObject<InternetNetwork> ();
  m_webHost = m_webNetwork->CreateTopology (m_epcHelper->GetPgwNode ());

  // UE Nodes and UE devices
  m_ueNodes = m_lteNetwork->GetUeNodes ();
  m_ueDevices = m_lteNetwork->GetUeDevices ();

  // Application random start time
  m_rngStart = CreateObject<UniformRandomVariable> ();
  m_rngStart->SetAttribute ("Min", DoubleValue (2.));
  m_rngStart->SetAttribute ("Max", DoubleValue (5.));

  // Registering controller trace sinks
  m_controller->ConnectTraceSinks ();
  
  // Saving simulation statistics 
  m_controller->TraceConnectWithoutContext ("AppStats", 
      MakeCallback (&SimulationScenario::ReportAppStats, this));
  m_controller->TraceConnectWithoutContext ("EpcStats", 
      MakeCallback (&SimulationScenario::ReportEpcStats, this));
  m_controller->TraceConnectWithoutContext ("PgwStats", 
      MakeCallback (&SimulationScenario::ReportPgwStats, this));
  m_controller->TraceConnectWithoutContext ("GbrStats", 
      MakeCallback (&SimulationScenario::ReportGbrStats, this));
  m_controller->TraceConnectWithoutContext ("SwtStats", 
      MakeCallback (&SimulationScenario::ReportSwtStats, this));
  m_webNetwork->TraceConnectWithoutContext ("WebStats",
      MakeCallback (&SimulationScenario::ReportWebStats, this));

  // Application traffic
  if (m_ping) EnablePingTraffic ();
  if (m_http) EnableHttpTraffic ();
  if (m_voip) EnableVoipTraffic ();
  if (m_video) EnableVideoTraffic ();

  // Logs and traces
  DatapathLogs ();
  PcapAsciiTraces ();
}

void 
SimulationScenario::SetCommonPrefix (std::string prefix)
{
  static bool prefixSet = false;

  if (prefixSet || prefix == "") return;
  
  prefixSet = true;
  m_commonPrefix = prefix;
  char lastChar = *prefix.rbegin (); 
  if (lastChar != '-')
    {
      m_commonPrefix += "-";
    }
  m_appStatsFilename = m_commonPrefix + m_appStatsFilename;
  m_epcStatsFilename = m_commonPrefix + m_epcStatsFilename;
  m_pgwStatsFilename = m_commonPrefix + m_pgwStatsFilename;
  m_gbrStatsFilename = m_commonPrefix + m_gbrStatsFilename;
  m_webStatsFilename = m_commonPrefix + m_webStatsFilename;
  m_swtStatsFilename = m_commonPrefix + m_swtStatsFilename;
  m_topoFilename     = m_commonPrefix + m_topoFilename;
}

void 
SimulationScenario::EnablePingTraffic ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Ipv4> dstIpv4 = m_webHost->GetObject<Ipv4> ();
  Ipv4Address dstAddr = dstIpv4->GetAddress (1,0).GetLocal ();
  V4PingHelper ping = V4PingHelper (dstAddr);
  ApplicationContainer clientApps = ping.Install (m_ueNodes);
  clientApps.Start (Seconds (m_rngStart->GetValue ()));
}

void
SimulationScenario::EnableHttpTraffic ()
{
  NS_LOG_FUNCTION (this);
  
  static uint16_t httpPort = 80;

  Ptr<Ipv4> serverIpv4 = m_webHost->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer httpApps;

  HttpHelper httpHelper;
  httpHelper.SetClientAttribute ("Direction", EnumValue (Application::BIDIRECTIONAL));
  httpHelper.SetServerAttribute ("Direction", EnumValue (Application::BIDIRECTIONAL));
  httpHelper.SetClientAttribute ("TcpTimeout", TimeValue (Seconds (9))); 
  httpHelper.SetServerAttribute ("StartTime", TimeValue (Seconds (0)));
  // The HttpClient TcpTimeout was selected based on HTTP traffic model and
  // dedicated bearer idle timeout. Every time the TCP socket is closed, HTTP
  // client application notify the controller, and traffic statistics are
  // printed.
  
  for (uint32_t u = 0; u < m_ueNodes.GetN (); u++, httpPort++)
    {
      Ptr<Node> client = m_ueNodes.Get (u);
      Ptr<NetDevice> clientDev = m_ueDevices.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();

      // Bidirectional HTTP traffic.
      // NOTE: The HttpClient is the one that requests pages to HttpServer. 
      // The client is installed into UE, and the server into m_webHost.
      Ptr<HttpClient> clientApp = httpHelper.Install (client, m_webHost, 
          serverAddr, httpPort);
      clientApp->AggregateObject (tft);
      clientApp->SetStartTime (Seconds (m_rngStart->GetValue ()));
      httpApps.Add (clientApp);

      // TFT Packet filter
      EpcTft::PacketFilter filter;
      filter.direction = EpcTft::BIDIRECTIONAL;
      filter.remoteAddress = serverAddr;
      filter.remoteMask = serverMask;
      filter.localAddress = clientAddr;
      filter.localMask = clientMask;
      filter.remotePortStart = httpPort;
      filter.remotePortEnd = httpPort;
      tft->Add (filter);

      // Dedicated Non-GBR EPS bearer (QCI 8)
      GbrQosInformation qos;
      qos.mbrDl = 524288;     // Up to 512 Kbps downlink
      qos.mbrUl = 131072;     // Up to 128 Kbps uplink 
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, qos);
      m_lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }

  // Setting up app start callback to controller
  ApplicationContainer::Iterator i;
  for (i = httpApps.Begin (); i != httpApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&OpenFlowEpcController::NotifyAppStart, m_controller),
          MakeCallback (&OpenFlowEpcController::NotifyAppStop, m_controller));
    }
}

void
SimulationScenario::EnableVoipTraffic ()
{
  NS_LOG_FUNCTION (this);
 
  static uint16_t voipPort = 16000;
  uint16_t pktSize = 68;            // Lower values don't trigger meter band
  double pktInterval = 0.06;
 
  Ptr<Ipv4> serverIpv4 = m_webHost->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer voipApps;
  VoipHelper voipHelper;
  voipHelper.SetClientAttribute ("Direction", EnumValue (Application::BIDIRECTIONAL));
  voipHelper.SetServerAttribute ("Direction", EnumValue (Application::BIDIRECTIONAL));
  voipHelper.SetClientAttribute ("PacketSize", UintegerValue (pktSize));
  voipHelper.SetServerAttribute ("PacketSize", UintegerValue (pktSize));
  voipHelper.SetClientAttribute ("Interval", TimeValue (Seconds (pktInterval)));
  voipHelper.SetServerAttribute ("Interval", TimeValue (Seconds (pktInterval)));
  voipHelper.SetServerAttribute ("StartTime", TimeValue (Seconds (0)));

  // ON/OFF pattern for VoIP applications (Poisson process)
  // Random average time between calls (1 to 5 minutes) [Exponential mean]
  Ptr<UniformRandomVariable> meanTime = CreateObject<UniformRandomVariable> ();
  meanTime->SetAttribute ("Min", DoubleValue (60));
  meanTime->SetAttribute ("Max", DoubleValue (300));
  Ptr<ExponentialRandomVariable> offTime = CreateObject<ExponentialRandomVariable> ();
  offTime->SetAttribute ("Mean", DoubleValue (meanTime->GetValue ()));
  
  voipHelper.SetClientAttribute ("OnTime", 
      StringValue ("ns3::NormalRandomVariable[Mean=100.0|Variance=900.0]"));
  voipHelper.SetClientAttribute ("OffTime", PointerValue (offTime));
 
  for (uint32_t u = 0; u < m_ueNodes.GetN (); u++, voipPort++)
    {
      Ptr<Node> client = m_ueNodes.Get (u);
      Ptr<NetDevice> clientDev = m_ueDevices.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();

      // Bidirectional VoIP traffic.
      // NOTE: clientApp is the one at UE, providing the uplink traffic. 
      // Linking only this app to the callbacks for start/stop notifications.
      voipHelper.SetClientAttribute ("Stream", IntegerValue (u));
      Ptr<VoipClient> clientApp = voipHelper.Install (client, m_webHost, 
          clientAddr, serverAddr, voipPort, voipPort);
      clientApp->AggregateObject (tft);
      clientApp->SetStartTime (Seconds (m_rngStart->GetValue ()));
      voipApps.Add (clientApp);
  
      // TFT downlink packet filter
      EpcTft::PacketFilter filterDown;
      filterDown.direction = EpcTft::DOWNLINK;
      filterDown.remoteAddress = serverAddr;
      filterDown.remoteMask = serverMask;
      filterDown.localAddress = clientAddr;
      filterDown.localMask = clientMask;
      filterDown.localPortStart = voipPort;
      filterDown.localPortEnd = voipPort;
      tft->Add (filterDown);

      // TFT uplink packet filter
      EpcTft::PacketFilter filterUp;
      filterUp.direction = EpcTft::UPLINK;
      filterUp.remoteAddress = serverAddr;
      filterUp.remoteMask = serverMask;
      filterUp.localAddress = clientAddr;
      filterUp.localMask = clientMask;
      filterUp.remotePortStart = voipPort;
      filterUp.remotePortEnd = voipPort;
      tft->Add (filterUp);
 
      // Dedicated GBR EPS bearer (QCI 1)
      GbrQosInformation qos;
      // 2 bytes from compressed UDP/IP/RTP + 58 bytes from GTPU/UDP/IP/ETH
      qos.gbrDl = 8 * (pktSize + 2 + 58) / pktInterval;
      qos.gbrUl = qos.gbrDl;
      // No meter rules for VoIP traffic
      // qos.mbrDl = 1.1 * qos.gbrDl; // (10 % more)
      // qos.mbrUl = qos.mbrDl;
      EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);
      m_lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }

  // Setting up app start callback to controller
  ApplicationContainer::Iterator i;
  for (i = voipApps.Begin (); i != voipApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&OpenFlowEpcController::NotifyAppStart, m_controller),
          MakeCallback (&OpenFlowEpcController::NotifyAppStop, m_controller));
    }
}

void
SimulationScenario::EnableVideoTraffic ()
{
  NS_LOG_FUNCTION (this);
 
  static uint16_t videoPort = 20000;

  Ptr<Ipv4> serverIpv4 = m_webHost->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer videoApps;
  VideoHelper videoHelper;
  videoHelper.SetClientAttribute ("Direction", EnumValue (Application::DOWNLINK));
  videoHelper.SetServerAttribute ("Direction", EnumValue (Application::DOWNLINK));
  videoHelper.SetClientAttribute ("MaxPacketSize", UintegerValue (1400));
  videoHelper.SetServerAttribute ("StartTime", TimeValue (Seconds (0)));

  // ON/OFF pattern for VoIP applications (Poisson process)
  // Average time between videos (1 minute) [Exponential mean]
  videoHelper.SetClientAttribute ("OnTime", 
      StringValue ("ns3::NormalRandomVariable[Mean=75.0|Variance=2025.0]"));
  videoHelper.SetClientAttribute ("OffTime", 
      StringValue ("ns3::ExponentialRandomVariable[Mean=60.0]"));

  // Video random selection
  Ptr<UniformRandomVariable> rngVideo = CreateObject<UniformRandomVariable> ();
  rngVideo->SetAttribute ("Min", DoubleValue (0));
  rngVideo->SetAttribute ("Max", DoubleValue (12));
  
  for (uint32_t u = 0; u < m_ueNodes.GetN (); u++, videoPort++)
    {
      Ptr<Node> client = m_ueNodes.Get (u);
      Ptr<NetDevice> clientDev = m_ueDevices.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();
 
      // Downlink video traffic.
      // NOTE: The clientApp is the one that sends traffic to the UdpServer.
      // The clientApp is installed into m_webHost and the server into UE, 
      // providing a downlink video traffic.
      int videoIdx = rngVideo->GetInteger ();
      videoHelper.SetClientAttribute ("TraceFilename",
          StringValue (GetVideoFilename (videoIdx)));
      Ptr<VideoClient> clientApp  = videoHelper.Install (m_webHost, client,
          clientAddr, videoPort);
      clientApp->AggregateObject (tft);
      clientApp->SetStartTime (Seconds (m_rngStart->GetValue ()));
      videoApps.Add (clientApp);

      // TFT downlink packet filter
      EpcTft::PacketFilter filter;
      filter.direction = EpcTft::DOWNLINK;
      filter.remoteAddress = serverAddr;
      filter.remoteMask = serverMask;
      filter.localAddress = clientAddr;
      filter.localMask = clientMask;
      filter.localPortStart = videoPort;
      filter.localPortEnd = videoPort;
      tft->Add (filter);
 
      // Dedicated GBR EPS bearer (QCI 4).
      GbrQosInformation qos;
      qos.gbrDl = 1.5 * m_avgBitRate [videoIdx];  // Avg + 50%
      // Set the meter in average between gbr and maxBitRate
      qos.mbrDl = (qos.gbrDl + m_maxBitRate [videoIdx]) / 2;
      EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);
      m_lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }

  // Setting up app start callback to controller
  ApplicationContainer::Iterator i;
  for (i = videoApps.Begin (); i != videoApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&OpenFlowEpcController::NotifyAppStart, m_controller),
          MakeCallback (&OpenFlowEpcController::NotifyAppStop, m_controller));
    }
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
SimulationScenario::ReportGbrStats (uint32_t requests, uint32_t blocks, double ratio)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (m_gbrStatsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_gbrStatsFilename);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (9) << "Requests" 
              << setw (9) << "Blocked"
              << setw (9) << "Ratio"
              << std::endl;
    }
  else
    {
      outFile.open (m_gbrStatsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_gbrStatsFilename);
          return;
        }
    }

  outFile << left
          << setw (9) << requests
          << setw (9) << blocks
          << setw (9) << ratio 
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
              << setw (5)  << "Pgw"
              << setw (48) << "eNB switches"
              << std::endl
              << setw (12) << " ";
      for (size_t i = 0; i < switches; i++)
        {
          outFile << setw (5) << i;
        }
      outFile << setw (12) << "Avg (eNBs)" << std::endl;
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
  outFile << setw (5) << *it;
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

  // Then we expect the distribuiton of UEs per eNBs and switch indexes
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

const std::string 
SimulationScenario::GetVideoFilename (uint8_t idx)
{
  return m_videoDir + m_videoTrace [idx];
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
      m_lteNetwork->EnableTraces ();
    }
}

};  // namespace ns3
