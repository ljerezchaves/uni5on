/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include "traffic-helper.h"
#include "simulation-scenario.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficHelper");

const std::string TrafficHelper::m_videoDir = "../movies/";

const std::string TrafficHelper::m_videoTrace [] = {"jurassic.data",
  "silence.data", "star-wars.data", "mr-bean.data", "first-contact.data",
  "from-dusk.data", "the-firm.data"};

const uint64_t TrafficHelper::m_avgBitRate [] = {770000, 580000, 280000,
  580000, 330000, 680000, 310000};

const uint64_t TrafficHelper::m_maxBitRate [] = {3300000, 4400000, 1900000,
  3100000, 2500000, 3100000, 2100000};


// ------------------------------------------------------------------------ //
TrafficHelper::TrafficHelper (Ptr<Node> server, Ptr<LteHelper> helper, 
                              Ptr<OpenFlowEpcController> controller,
                              Ptr<OpenFlowEpcNetwork> network)
  : m_lteHelper (helper),
    m_webNode (server)
{
  NS_LOG_FUNCTION (this);

  // Configuring server address and mask
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  m_webAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  m_webMask = serverIpv4->GetAddress (1,0).GetMask ();

  // Configuring the traffic manager object factory
  m_managerFactory.SetTypeId (TrafficManager::GetTypeId ());
  SetTfcManagerAttribute ("Controller", PointerValue (controller));
  SetTfcManagerAttribute ("Network", PointerValue (network));

  // Random stored video selection.
  m_stVideoRng = CreateObject<UniformRandomVariable> ();
  m_stVideoRng->SetAttribute ("Min", DoubleValue (0));
  m_stVideoRng->SetAttribute ("Max", DoubleValue (6));

  // Configuring application traffic attributes 
  m_httpHelper.SetClientAttribute ("MaxPages", UintegerValue (3)); 
  m_voipHelper.SetServerAttribute ("CallDuration", 
    StringValue ("ns3::NormalRandomVariable[Mean=100.0|Variance=900.0]"));
  m_stVideoHelper.SetServerAttribute ("VideoDuration", 
    StringValue ("ns3::NormalRandomVariable[Mean=75.0|Variance=2025.0]"));
  m_rtVideoHelper.SetServerAttribute ("VideoDuration", 
    StringValue ("ns3::NormalRandomVariable[Mean=600.0|Variance=90000.0]"));
  m_rtVideoHelper.SetServerAttribute ("TraceFilename", 
    StringValue (m_videoDir + "office-cam.data"));

  // FIXME Estou colocando esses tempos curtos aqui apenas pra testes.
  m_httpHelper.SetClientAttribute ("MaxPages", UintegerValue (1)); 
  m_voipHelper.SetServerAttribute ("CallDuration", 
    StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  m_stVideoHelper.SetServerAttribute ("VideoDuration", 
    StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  m_rtVideoHelper.SetServerAttribute ("VideoDuration", 
    StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  SetTfcManagerAttribute ("IdleRng", 
    StringValue ("ns3::ExponentialRandomVariable[Mean=10.0]"));
  SetTfcManagerAttribute ("StartRng", 
    StringValue ("ns3::ExponentialRandomVariable[Mean=10.0]"));
}

TrafficHelper::~TrafficHelper ()
{
  NS_LOG_FUNCTION (this);
  m_webNode = 0;
  m_lteHelper = 0;
  m_ueNode = 0;
  m_ueDev = 0;
  m_ueManager = 0;
  m_stVideoRng = 0;
}

void
TrafficHelper::SetTfcManagerAttribute (std::string name, 
                                       const AttributeValue &value)
{
  m_managerFactory.Set (name, value);
}

void 
TrafficHelper::Install (NodeContainer ueNodes, NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  // Installing manager and applications into nodes
  for (uint32_t u = 0; u < ueNodes.GetN (); u++)
    {
      m_ueNode = ueNodes.Get (u);
      m_ueDev = ueDevices.Get (u);
      NS_ASSERT (m_ueDev->GetNode () == m_ueNode);
 
      Ptr<Ipv4> clientIpv4 = m_ueNode->GetObject<Ipv4> ();
      m_ueAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      m_ueMask = clientIpv4->GetAddress (1, 0).GetMask ();

      m_ueManager = m_managerFactory.Create<TrafficManager> ();
      m_ueManager->m_imsi = (DynamicCast<LteUeNetDevice> (m_ueDev)->GetImsi ());
      m_ueNode->AggregateObject (m_ueManager);
      
      // Connecting the manager to new context created trace source.
      Config::ConnectWithoutContext (
        "/Names/SgwPgwApplication/ContextCreated",
        MakeCallback (&TrafficManager::ContextCreatedCallback, m_ueManager));

      InstallVoip ();
      InstallRealTimeVideo ();
      InstallStoredVideo ();
      InstallHttp ();
    }
  m_ueNode = 0;
  m_ueDev = 0;
  m_ueManager = 0;
}

/* NOTE about GbrQosInformation:
 * 1) The Maximum Bit Rate field is used by controller to install meter rules.
 *    When this value is left to 0, no meter rules will be installed.
 * 2) The Guaranteed Bit Rate field is used by the controller to reserve the
 *    requested bandwidth in OpenFlow network. This can be used even for
 *    Non-GBR bearers (as done in HTTP traffic), allowing resource reservation
 *    but without guarantee. When left to 0, no resources are reserved. 
 */

void
TrafficHelper::InstallHttp ()
{
  NS_LOG_FUNCTION (this);
  
  static uint16_t portNo = 10000;
  portNo++;

  // Bidirectional HTTP traffic.
  Ptr<HttpClient> cApp = 
    m_httpHelper.Install (m_ueNode, m_webNode, m_webAddr, portNo);
 
  // TFT Packet filter
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  tft->Add (filter);

  // Dedicated Non-GBR EPS bearer (QCI 8)
  GbrQosInformation qos;
  qos.gbrDl = 131072;     // Reserving 128 Kbps in downlink //FIXME?
  qos.gbrUl = 32768;      // Reserving 32 Kbps in uplink
  qos.mbrDl = 524288;     // Max of 512 Kbps in downlink
  qos.mbrUl = 131072;     // Max of 128 Kbps in uplink 
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, qos);

  // Link EPC info to application
  cApp->m_tft = tft;
  cApp->m_bearer = bearer;
  m_ueManager->AddEpcApplication (cApp);

  // Activate dedicated bearer
  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallVoip ()
{
  NS_LOG_FUNCTION (this);
 
  static uint16_t portNo = 20000;
  portNo++;
 
  // Bidirectional VoIP traffic.
  Ptr<VoipClient> cApp = m_voipHelper.Install (m_ueNode, m_webNode, m_ueAddr, 
                                               m_webAddr, portNo, portNo);
  
  // TFT downlink packet filter
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filterDown;
  filterDown.direction = EpcTft::DOWNLINK;
  filterDown.remoteAddress = m_webAddr;
  filterDown.remoteMask = m_webMask;
  filterDown.localAddress = m_ueAddr;
  filterDown.localMask = m_ueMask;
  filterDown.localPortStart = portNo;
  filterDown.localPortEnd = portNo;
  tft->Add (filterDown);

  // TFT uplink packet filter
  EpcTft::PacketFilter filterUp;
  filterUp.direction = EpcTft::UPLINK;
  filterUp.remoteAddress = m_webAddr;
  filterUp.remoteMask = m_webMask;
  filterUp.localAddress = m_ueAddr;
  filterUp.localMask = m_ueMask;
  filterUp.remotePortStart = portNo;
  filterUp.remotePortEnd = portNo;
  tft->Add (filterUp);
 
  // Dedicated GBR EPS bearer (QCI 1)
  GbrQosInformation qos;
  qos.gbrDl = 47200;  // ~46.09 Kbps
  qos.gbrUl = 47200;  // ~46.09 Kbps
  EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

  // Link EPC info to application
  cApp->m_tft = tft;
  cApp->m_bearer = bearer;
  m_ueManager->AddEpcApplication (cApp);

  // Activate dedicated bearer
  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallStoredVideo ()
{
  NS_LOG_FUNCTION (this);
 
  static uint16_t portNo = 30000;
  portNo++;

  // Bidirectional stored video traffic.
  // The StoredVideoClient is the one that requests the video to the server.
  int videoIdx = m_stVideoRng->GetInteger ();
  m_stVideoHelper.SetServerAttribute ("TraceFilename",
    StringValue (GetVideoFilename (videoIdx)));
  
  Ptr<StoredVideoClient> cApp = 
    m_stVideoHelper.Install (m_ueNode, m_webNode, m_webAddr, portNo);
  
  // TFT Packet filter
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  tft->Add (filter);

  // Dedicated Non-GBR EPS bearer (QCI 8)
  GbrQosInformation qos;
  qos.gbrDl = 1.5 * m_avgBitRate [videoIdx];
  qos.mbrDl = (qos.gbrDl + m_maxBitRate [videoIdx]) / 2;
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR, qos);
 
  // Link EPC info to application
  cApp->m_tft = tft;
  cApp->m_bearer = bearer;
  m_ueManager->AddEpcApplication (cApp);

  // Activate dedicated bearer
  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallRealTimeVideo ()
{
  NS_LOG_FUNCTION (this);
 
  static uint16_t portNo = 40000;
  portNo++;

  // Downlink real-time video traffic.
  Ptr<RealTimeVideoClient> cApp = 
    m_rtVideoHelper.Install (m_ueNode, m_webNode, m_ueAddr, portNo);

  // TFT downlink packet filter
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::DOWNLINK;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = portNo;
  filter.localPortEnd = portNo;
  tft->Add (filter);
 
  // Dedicated GBR EPS bearer (QCI 4).
  GbrQosInformation qos;
  qos.gbrDl = 524288;   //  512 Kbps (average + 50 %)
  qos.mbrDl = 1048576;  // 1024 Kbps (maximum / 2)
  EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);

  // Link EPC info to application
  cApp->m_tft = tft;
  cApp->m_bearer = bearer;
  m_ueManager->AddEpcApplication (cApp);

  // Activate dedicated bearer
  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

const std::string 
TrafficHelper::GetVideoFilename (uint8_t idx)
{
  return m_videoDir + m_videoTrace [idx];
}

};  // namespace ns3
