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
    m_rngStart (0),
    m_statsFirstWrite (true)
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
  m_opfNetwork = 0;
  m_rngStart = 0;
}

TypeId 
SimulationScenario::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationScenario")
    .SetParent<Object> ()
    .AddConstructor<SimulationScenario> ()
    .AddAttribute ("StatsFilename",
                   "Name of the file where the app statistics will be saved.",
                   StringValue ("app_stats.txt"),
                   MakeStringAccessor (&SimulationScenario::m_statsFilename),
                   MakeStringChecker ())
    .AddAttribute ("TopoFilename",
                   "Name of the file with topology description.",
                   StringValue ("../scratch/epc-of/default.txt"),
                   MakeStringAccessor (&SimulationScenario::m_topoFilename),
                   MakeStringChecker ())
    .AddAttribute ("neNBs",
                   "Number of eNBs in the topology.",
                   UintegerValue (4),
                   MakeUintegerAccessor (&SimulationScenario::m_nEnbs),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("nUEs",
                   "Number of UEs per eNB (used only when no topology file present.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SimulationScenario::m_nUes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("nSwitches",
                   "Number of OpenFlow switches in the toplogy.",
                   UintegerValue (5),
                   MakeUintegerAccessor (&SimulationScenario::m_nRing),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PcapTraces",
                   "Enable/Disable simulation pcap traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_traces),
                   MakeBooleanChecker ())
    .AddAttribute ("SwitchLogs",
                   "Enable/Disable ofsoftswitch logs.",
                   StringValue ("none"),
                   MakeStringAccessor (&SimulationScenario::m_liblog),
                   MakeStringChecker ())
    .AddAttribute ("PingTraffic",
                   "Enable/Disable ping traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_ping),
                   MakeBooleanChecker ())
    .AddAttribute ("HttpTraffic",
                   "Enable/Disable http traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_http),
                   MakeBooleanChecker ())
    .AddAttribute ("VoipTraffic",
                   "Enable/Disable voip traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_voip),
                   MakeBooleanChecker ())
    .AddAttribute ("VideoTraffic",
                   "Enable/Disable video traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_video),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
SimulationScenario::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);
  Object::NotifyConstructionCompleted ();

  ParseTopology ();

  // OpenFlow ring network (for EPC)
  m_opfNetwork = CreateObject<RingNetwork> ();
  m_controller = CreateObject<RingController> ();
  Names::Add ("ctrlApp", m_controller);
  
  m_controller->SetAttribute ("OFNetwork", PointerValue (m_opfNetwork));
  m_controller->SetAttribute ("Strategy", EnumValue (RingController::BAND));
  m_controller->SetAttribute ("BwReserve", DoubleValue (0.9));
  
  m_opfNetwork->SetAttribute ("Controller", PointerValue (m_controller));
  m_opfNetwork->SetAttribute ("NumSwitches", UintegerValue (m_nRing));
  m_opfNetwork->SetAttribute ("LinkDataRate", DataRateValue (DataRate ("10Mb/s")));
  m_opfNetwork->CreateTopology (m_eNbSwt);
  
  // LTE EPC core (with callbacks setup)
  m_epcHelper = CreateObject<OpenFlowEpcHelper> ();
  m_epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
//  m_epcHelper->SetX2ConnectCallback (
//      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));
  m_epcHelper->SetAddBearerCallback (
      MakeCallback (&OpenFlowEpcController::RequestNewDedicatedBearer, m_controller));
  m_epcHelper->SetCreateSessionRequestCallback (
      MakeCallback (&OpenFlowEpcController::NotifyNewContextCreated, m_controller));
  
  // LTE radio access network
  m_lteNetwork = CreateObject<LteSquaredGridNetwork> ();
  m_lteNetwork->SetAttribute ("RoomLength", DoubleValue (100.0));
  m_lteNetwork->SetAttribute ("Enbs", UintegerValue (m_nEnbs));
  m_lteNetwork->CreateTopology (m_epcHelper, m_eNbUes);
  m_lteHelper = m_lteNetwork->GetLteHelper ();

  // Internet network
  m_webNetwork = CreateObject<InternetNetwork> ();
  Ptr<Node> pgw = m_epcHelper->GetPgwNode ();
  m_webHost = m_webNetwork->CreateTopology (pgw);

  // UE Nodes and UE devices
  m_ueNodes = m_lteNetwork->GetUeNodes ();
  m_ueDevices = m_lteNetwork->GetUeDevices ();

  // Application random start time
  m_rngStart = CreateObject<UniformRandomVariable> ();
  m_rngStart->SetAttribute ("Min", DoubleValue (0.));
  m_rngStart->SetAttribute ("Max", DoubleValue (5.));

  // Saving controller and application statistics 
  Config::ConnectWithoutContext ("/Names/ctrlApp/AppStats", 
      MakeCallback (&SimulationScenario::ReportAppStats, this));
  Config::ConnectWithoutContext ("/Names/ctrlApp/GbrBlock", 
      MakeCallback (&SimulationScenario::ReportBlockRatio, this));

  // Application traffic
  if (m_ping)   EnablePingTraffic ();
  if (m_http)   EnableHttpTraffic ();
  if (m_voip)   EnableVoipTraffic ();
  if (m_video)  EnableVideoTraffic ();

  // Logs and traces
  DatapathLogs ();
  PcapTraces ();
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

  ApplicationContainer serverApps, clientApps;
  ApplicationContainer httpApps;

  HttpHelper httpHelper;
  httpHelper.SetServerAttribute ("Direction", 
      EnumValue (Application::BIDIRECTIONAL));
  httpHelper.SetClientAttribute ("Direction", 
      EnumValue (Application::BIDIRECTIONAL));
  httpHelper.SetServerAttribute ("StartTime", TimeValue (Seconds (0)));
  httpHelper.SetClientAttribute ("TcpTimeout", TimeValue (Seconds (4))); 
  // The HTTP client/server TCP timeout was selected based on HTTP traffic
  // model and dedicated bearer idle timeout. Every time the TCP socket is
  // closed, HTTP client application notify the controller, and traffic
  // statistics are printed.
  
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

      // HTTP client / server
      ApplicationContainer apps = httpHelper.Install (client, m_webHost, 
           serverAddr, httpPort);
      Ptr<HttpClient> clientApp = DynamicCast<HttpClient> (apps.Get (0));
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
      qos.mbrDl = 262144;     // 256 Kbps downlink
      qos.mbrUl = 65536;      //  64 Kbps uplink
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
  voipHelper.SetAttribute ("Direction", EnumValue (Application::BIDIRECTIONAL));

  // ON/OFF pattern for VoIP applications (Poisson process)
  voipHelper.SetAttribute ("OnTime", 
      StringValue ("ns3::NormalRandomVariable[Mean=5.0,Variance=2.0]"));
  voipHelper.SetAttribute ("OffTime", 
      StringValue ("ns3::ExponentialRandomVariable[Mean=15.0]"));
  voipHelper.SetAttribute ("PacketSize", UintegerValue (pktSize));
  voipHelper.SetAttribute ("Interval", TimeValue (Seconds (pktInterval)));
 
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

      // Bidirectional VoIP traffic
      voipHelper.SetAttribute ("Stream", IntegerValue (u));
      ApplicationContainer apps = voipHelper.Install (client, m_webHost, 
          clientAddr, serverAddr, voipPort, voipPort);
      apps.Get (0)->AggregateObject (tft);
      apps.Get (1)->AggregateObject (tft);
      apps.Start (Seconds (m_rngStart->GetValue ()));
      voipApps.Add (apps);
  
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
      qos.gbrDl = 8 * (pktSize + 2 + 58) / pktInterval; // ~ 17.0 Kbps
      qos.mbrDl = 1.1 * qos.gbrDl;                      // ~ 18.7 Kbps (10 % more)
      qos.gbrUl = qos.gbrDl;
      qos.mbrUl = qos.mbrDl;
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
  videoHelper.SetClientAttribute ("MaxPacketSize", UintegerValue (1400));
  videoHelper.SetServerAttribute ("Direction", EnumValue (Application::DOWNLINK));
  videoHelper.SetServerAttribute ("StartTime", TimeValue (Seconds (0)));

  // ON/OFF pattern for VoIP applications (Poisson process)
  videoHelper.SetClientAttribute ("OnTime", 
      StringValue ("ns3::NormalRandomVariable[Mean=5.0,Variance=2.0]"));
  videoHelper.SetClientAttribute ("OffTime", 
      StringValue ("ns3::ExponentialRandomVariable[Mean=15.0]"));

  // Video random selection
  Ptr<UniformRandomVariable> rngVideo = CreateObject<UniformRandomVariable> ();
  rngVideo->SetAttribute ("Min", DoubleValue (0.));
  rngVideo->SetAttribute ("Max", DoubleValue (12.));
  
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
 
      // Downlink video traffic 
      int videoIdx = rngVideo->GetInteger ();
      videoHelper.SetClientAttribute ("TraceFilename",
          StringValue (GetVideoFilename (videoIdx)));
      ApplicationContainer apps = videoHelper.Install (m_webHost, client,
          clientAddr, videoPort);
      Ptr<VideoClient> clientApp = DynamicCast<VideoClient> (apps.Get (0));
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
      qos.gbrDl = 1.4 * m_avgBitRate [videoIdx];  // Avg + stdev (~806 Kbps)
      qos.mbrDl = 1.1 * qos.gbrDl;                // 10 % more
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
SimulationScenario::DatapathLogs ()
{
  NS_LOG_FUNCTION (this);
  m_opfNetwork->EnableDatapathLogs (m_liblog);
}

void
SimulationScenario::PcapTraces ()
{
  if (!m_traces) return;
  NS_LOG_FUNCTION (this);
 
  m_webNetwork->EnablePcap ("web");
  m_opfNetwork->EnableOpenFlowPcap ("openflow-channel");
  m_opfNetwork->EnableDataPcap ("ofn", true);
  m_epcHelper->EnablePcapS1u ("epc");
//  m_epcHelper->EnablePcapX2 ("epc");
//  m_lteNetwork->EnableTraces ();
}

void 
SimulationScenario::ReportAppStats (std::string description, uint32_t teid,
      Time duration, double lossRatio, Time delay, 
      Time jitter, uint32_t bytes, DataRate goodput)
{
  NS_LOG_FUNCTION (this << teid);
 
  std::ofstream outFile;
  if ( m_statsFirstWrite == true )
    {
      outFile.open (m_statsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_statsFilename);
          return;
        }
      m_statsFirstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"     << setw (17) << "Description" 
              << setw (6)  << "TEID"         << setw (12) << "Active (s)"
              << setw (12) << "Loss ratio"   << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"  << setw (10) << "RX bytes"
              << setw (8)  << "Goodput";
      outFile << std::endl;
    }
  else
    {
      outFile.open (m_statsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_statsFilename);
          return;
        }
    }

  outFile << left;
  outFile << setw (12) << Simulator::Now ().GetSeconds ();
  outFile << setw (17) << description;
  outFile << setw (6)  << teid;
  outFile << setw (12) << duration.GetSeconds ();
  outFile << setw (12) << lossRatio;
  outFile << setw (12) << delay.GetSeconds () * 1000;
  outFile << setw (12) << jitter.GetSeconds () * 1000;
  outFile << setw (10) << bytes;
  outFile << setw (8)  << goodput << std::endl;
  outFile.close ();
}

void
SimulationScenario::ReportBlockRatio (uint32_t requests, uint32_t blocks, double ratio)
{
  std::ofstream outFile;
  if ( m_statsFirstWrite == true )
    {
      outFile.open (m_statsFilename.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_statsFilename);
          return;
        }
      m_statsFirstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"     << setw (17) << "Description" 
              << setw (6)  << "TEID"         << setw (12) << "Active (s)"
              << setw (12) << "Loss ratio"   << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"  << setw (10) << "RX bytes"
              << setw (8)  << "Goodput";
      outFile << std::endl;
    }
  else
    {
      outFile.open (m_statsFilename.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << m_statsFilename);
          return;
        }
    }

 outFile << "------------------------------------"        << std::endl
         << "Number of GBR bearers request: " << requests << std::endl
         << "Number of GBR bearers blocked: " << blocks   << std::endl
         << "Block ratio: "                   << ratio    << std::endl;

 outFile.close ();
}

bool
SimulationScenario::ParseTopology ()
{
  NS_LOG_INFO ("Parsing topology...");
 
  std::ifstream file;
  file.open (m_topoFilename.c_str ());
  if (!file.is_open () )
    {
      NS_LOG_ERROR ("Topology file not open. Manually populating values.");
      
      // Set the same m_nUes for all eNBs and attach each eNBs
      // to switchs indexes 1 through m_nRing - 1, in turns.
      for (uint32_t idx = 0; idx < m_nEnbs; idx++)
        {
          m_eNbUes.push_back (m_nUes);
          m_eNbSwt.push_back (1 + (idx % (m_nRing - 1)));
        }
      return true;
    }

  std::istringstream lineBuffer;
  std::string line;
  uint32_t enb, ues, swtch, idx;
  
  for (idx = 0; !file.eof () && idx < m_nEnbs; idx++)
    {
      getline (file, line);
      lineBuffer.clear ();
      lineBuffer.str (line);
      lineBuffer >> enb;
      lineBuffer >> ues;
      lineBuffer >> swtch;

      NS_ASSERT_MSG (idx == enb, "Invalid eNB idx order in topology file.");
      NS_ASSERT_MSG (swtch < m_nRing, "Invalid switch idx in topology file.");
      
      m_eNbUes.push_back (ues);
      m_eNbSwt.push_back (swtch);
    }
  NS_ASSERT_MSG (idx == m_nEnbs, "Missing topology information in file...");
  return true;  
}

const std::string 
SimulationScenario::GetVideoFilename (uint8_t idx)
{
  return m_videoDir + m_videoTrace [idx];
}

};  // namespace ns3
