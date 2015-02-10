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

NS_LOG_COMPONENT_DEFINE ("SimulationScenario");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SimulationScenario);

const std::string SimulationScenario::m_videoDir = "../ns3/movies/";

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
    m_rngStart (0)
{
  NS_LOG_FUNCTION (this);
  
  // Create the experiment with minimal configuration
  std::vector<uint32_t> eNbUes (1);
  std::vector<uint16_t> eNbSwt (1);
  eNbUes.at (0) = 1;
  eNbSwt.at (0) = 1;
  SimulationScenario (1, 3, eNbUes, eNbSwt);
}

SimulationScenario::~SimulationScenario ()
{
  NS_LOG_FUNCTION (this);
}

SimulationScenario::SimulationScenario (uint32_t nEnbs, uint32_t nRing, 
    std::vector<uint32_t> eNbUes, std::vector<uint16_t> eNbSwt)
{
  NS_LOG_FUNCTION (this);

  // OpenFlow ring network (for EPC)
  m_opfNetwork = CreateObject<RingNetwork> ();
  m_controller = CreateObject<RingController> ();
  
  m_controller->SetAttribute ("OFNetwork", PointerValue (m_opfNetwork));
  m_controller->SetAttribute ("Strategy", EnumValue (RingController::BAND));
  m_controller->SetAttribute ("BwReserve", DoubleValue (0.9));
  
  m_opfNetwork->SetAttribute ("Controller", PointerValue (m_controller));
  m_opfNetwork->SetAttribute ("NumSwitches", UintegerValue (nRing));
  m_opfNetwork->SetAttribute ("LinkDataRate", DataRateValue (DataRate ("10Mb/s")));
  m_opfNetwork->CreateTopology (eNbSwt);
  
  // LTE EPC core (with callbacks setup)
  m_epcHelper = CreateObject<OpenFlowEpcHelper> ();
  m_epcHelper->SetS1uConnectCallback (
      MakeCallback (&OpenFlowEpcNetwork::AttachToS1u, m_opfNetwork));
//  m_epcHelper->SetX2ConnectCallback (
//      MakeCallback (&OpenFlowEpcNetwork::AttachToX2, m_opfNetwork));
//  m_epcHelper->SetAddBearerCallback (
//      MakeCallback (&OpenFlowEpcController::RequestNewDedicatedBearer, m_controller));
  m_epcHelper->SetCreateSessionRequestCallback (
      MakeCallback (&OpenFlowEpcController::NotifyNewContextCreated, m_controller));
  
  // LTE radio access network
  m_lteNetwork = CreateObject<LteSquaredGridNetwork> ();
  m_lteNetwork->SetAttribute ("RoomLength", DoubleValue (100.0));
  m_lteNetwork->SetAttribute ("Enbs", UintegerValue (nEnbs));
  m_lteNetwork->CreateTopology (m_epcHelper, eNbUes);
  m_lteHelper = m_lteNetwork->GetLteHelper ();

  // Internet network
  m_webNetwork = CreateObject<InternetNetwork> ();
  Ptr<Node> pgw = m_epcHelper->GetPgwNode ();
  m_webHost = m_webNetwork->CreateTopology (pgw);

  // UE Nodes and UE devices
  m_ueNodes = m_lteNetwork->GetUeNodes ();
  m_ueDevices = m_lteNetwork->GetUeDevices ();

  m_rngStart = CreateObject<UniformRandomVariable> ();
  m_rngStart->SetAttribute ("Min", DoubleValue (0.));
  m_rngStart->SetAttribute ("Max", DoubleValue (5.));
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
  ;
  return tid;
}

void 
SimulationScenario::EnablePingTraffic ()
{
  Ptr<Ipv4> dstIpv4 = m_webHost->GetObject<Ipv4> ();
  Ipv4Address dstAddr = dstIpv4->GetAddress (1,0).GetLocal ();
  V4PingHelper ping = V4PingHelper (dstAddr);
  ApplicationContainer clientApps = ping.Install (m_ueNodes);
  clientApps.Start (Seconds (m_rngStart->GetValue ()));
}

void
SimulationScenario::EnableHttpTraffic ()
{
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
  httpHelper.SetClientAttribute ("TcpTimeout", TimeValue (Seconds (5))); 
  // The HTTP client/server TCP timeout was selected based on HTTP traffic
  // model and dedicated bearer idle timeout.  Every time the TCP socket is
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
      qos.mbrDl = qos.mbrUl = 256000;
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
  static uint16_t voipPort = 16000;
  uint16_t voipPacketSize = 60;
  double   voipPacketInterval = 0.06;
 
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
      qos.gbrDl = qos.gbrUl = (voipPacketSize + 4) * 8 / voipPacketInterval;
      qos.mbrDl = qos.mbrUl = (voipPacketSize + 4) * 8 / voipPacketInterval;
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
      qos.gbrDl = m_avgBitRate [videoIdx];
      qos.mbrDl = m_maxBitRate [videoIdx];
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
SimulationScenario::PrintStats ()
{
  m_controller->GetBlockRatioStatistics ();
}

void
SimulationScenario::EnableDatapathLogs (std::string level)
{
  m_opfNetwork->EnableDatapathLogs (level);
}

void
SimulationScenario::EnableTraces ()
{
  m_webNetwork->EnablePcap ("web");
  m_opfNetwork->EnableOpenFlowPcap ("openflow-channel");
  m_opfNetwork->EnableDataPcap ("ofn", true);
  m_epcHelper->EnablePcapS1u ("epc");
//  m_epcHelper->EnablePcapX2 ("epc");
//  m_lteNetwork->EnableTraces ();

//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTxDrop", 
//                    MakeCallback (&SimulationScenario::MacDropTrace, this));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::CsmaNetDevice/MacTxBackoff", 
//                    MakeCallback (&SimulationScenario::MacDropTrace, this));
}

void
SimulationScenario::MacDropTrace (std::string context, Ptr<const Packet> p)
{
  // Parsing context string to get node ID
  std::vector <std::string> elements;
  size_t pos1 = 0, pos2;
  while (pos1 != context.npos)
  {
    pos1 = context.find ("/", pos1);
    pos2 = context.find ("/", pos1 + 1);
    elements.push_back (context.substr (pos1 + 1, pos2 - (pos1 + 1)));
    pos1 = pos2;
    pos2 = context.npos;
  }
  Ptr<Node> node = NodeList::GetNode (atoi (elements.at (1).c_str ()));
  NS_LOG_DEBUG (context << " " << p << " " << Names::FindName (node));
}

const std::string 
SimulationScenario::GetVideoFilename (uint8_t idx)
{
  return m_videoDir + m_videoTrace [idx];
}

};  // namespace ns3
