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
#include "traffic-manager.h"
#include "../sdran/lte-network.h"
#include "../apps/auto-pilot-client.h"
#include "../apps/auto-pilot-server.h"
#include "../apps/http-client.h"
#include "../apps/http-server.h"
#include "../apps/real-time-video-client.h"
#include "../apps/real-time-video-server.h"
#include "../apps/stored-video-client.h"
#include "../apps/stored-video-server.h"
#include "../apps/voip-client.h"
#include "../apps/voip-server.h"
#include "../epc/epc-network.h"
#include "../epc/epc-controller.h"
#include "../stats/qos-stats-calculator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficHelper");
NS_OBJECT_ENSURE_REGISTERED (TrafficHelper);

// Initial port number
uint16_t TrafficHelper::m_port = 10000;

// Trace files directory
const std::string TrafficHelper::m_videoDir = "./movies/";

// Trace files are sorted in increasing gbr bit rate
const std::string TrafficHelper::m_videoTrace [] = {
  "office-cam-low.txt", "office-cam-medium.txt", "first-contact.txt",
  "office-cam-high.txt", "star-wars-iv.txt", "ard-talk.txt", "mr-bean.txt",
  "n3-talk.txt", "the-firm.txt", "ard-news.txt", "jurassic-park.txt",
  "from-dusk-till-dawn.txt", "formula1.txt", "soccer.txt",
  "silence-of-the-lambs.txt"
};

// These values were obtained from observing the first 180 seconds of video
const uint64_t TrafficHelper::m_gbrBitRate [] = {
  120000, 128000, 400000, 450000, 500000, 500000, 600000, 650000, 700000,
  750000, 770000, 800000, 1100000, 1300000, 1500000
};

// These values were obtained from observing the first 180 seconds of video
const uint64_t TrafficHelper::m_mbrBitRate [] = {
  128000, 600000, 650000, 500000, 600000, 700000, 800000, 750000, 800000,
  1250000, 1000000, 1000000, 1200000, 1500000, 2000000
};


// ------------------------------------------------------------------------ //
TrafficHelper::TrafficHelper (Ptr<LteNetwork> lteNetwork,
                              Ptr<Node> webNode)
  : m_lteNetwork (lteNetwork),
    m_webNode (webNode)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_webNode->GetNDevices () == 2, "Single device expected.");
  Ptr<NetDevice> webDev = m_webNode->GetDevice (1);
  m_webAddr = EpcNetwork::GetIpv4Addr (webDev);
  m_webMask = EpcNetwork::GetIpv4Mask (webDev);
}

TrafficHelper::TrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

TrafficHelper::~TrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TrafficHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficHelper")
    .SetParent<Object> ()
    .AddConstructor<TrafficHelper> ()

    // HTC traffic manager attributes.
    .AddAttribute ("HtcPoissonInterArrival",
                   "An exponential random variable used to get HTC "
                   "application inter-arrival start times.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&TrafficHelper::m_htcPoissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("HtcRestartApps",
                   "Continuously restart HTC applications after stop events.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_htcRestartApps),
                   MakeBooleanChecker ())

    // MTC traffic manager attributes.
    .AddAttribute ("MtcPoissonInterArrival",
                   "An exponential random variable used to get MTC "
                   "application inter-arrival start times.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=60.0]"),
                   MakePointerAccessor (&TrafficHelper::m_mtcPoissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("MtcRestartApps",
                   "Continuously restart MTC applications after stop events.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_mtcRestartApps),
                   MakeBooleanChecker ())

    // Applications to be installed.
    .AddAttribute ("AutoPilotTraffic",
                   "Enable GBR auto-pilot MTC traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_plotEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("BufferedVideoTraffic",
                   "Enable Non-GBR buffered video streaming traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_stvdEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("GbrLiveVideoTraffic",
                   "Enable GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_rtvgEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("HttpTraffic",
                   "Enable Non-GBR HTTP traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_httpEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("NonGbrLiveVideoTraffic",
                   "Enable Non-GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_rtvnEnalbe),
                   MakeBooleanChecker ())
    .AddAttribute ("VoipTraffic",
                   "Enable GBR VoIP traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_voipEnable),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
TrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_lteNetwork = 0;
  m_webNode = 0;
  m_ueNode = 0;
  m_ueDev = 0;
  m_htcManager = 0;
  m_mtcManager = 0;
  m_videoRng = 0;
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Configuring the traffic manager object factory for HTC and MTC UEs.
  m_htcFactory.SetTypeId (TrafficManager::GetTypeId ());
  m_htcFactory.Set ("PoissonInterArrival", PointerValue (m_htcPoissonRng));
  m_htcFactory.Set ("RestartApps", BooleanValue (m_htcRestartApps));

  m_mtcFactory.SetTypeId (TrafficManager::GetTypeId ());
  m_mtcFactory.Set ("PoissonInterArrival", PointerValue (m_mtcPoissonRng));
  m_mtcFactory.Set ("RestartApps", BooleanValue (m_mtcRestartApps));

  // Random video selection.
  m_videoRng = CreateObject<UniformRandomVariable> ();
  m_videoRng->SetAttribute ("Min", DoubleValue (0));
  m_videoRng->SetAttribute ("Max", DoubleValue (14));

  // Configuring SDMN application helpers.
  m_voipHelper = SdmnAppHelper (VoipClient::GetTypeId (),
                                VoipServer::GetTypeId ());
  m_plotHelper = SdmnAppHelper (AutoPilotClient::GetTypeId (),
                                AutoPilotServer::GetTypeId ());
  m_stvdHelper = SdmnAppHelper (StoredVideoClient::GetTypeId (),
                                StoredVideoServer::GetTypeId ());
  m_rtvdHelper = SdmnAppHelper (RealTimeVideoClient::GetTypeId (),
                                RealTimeVideoServer::GetTypeId ());
  m_httpHelper = SdmnAppHelper (HttpClient::GetTypeId (),
                                HttpServer::GetTypeId ());

  // Install the HTC applications.
  InstallHtcApplications (m_lteNetwork->GetHtcUeNodes (),
                          m_lteNetwork->GetHtcUeDevices ());

  // Install the MTC applications.
  InstallMtcApplications (m_lteNetwork->GetMtcUeNodes (),
                          m_lteNetwork->GetMtcUeDevices ());

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
TrafficHelper::InstallHtcApplications (NodeContainer ueNodes,
                                       NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  // Install manager and applications into nodes.
  for (uint32_t u = 0; u < ueNodes.GetN (); u++)
    {
      m_ueNode = ueNodes.Get (u);
      m_ueDev = ueDevices.Get (u);
      NS_ASSERT (m_ueDev->GetNode () == m_ueNode);
      uint64_t ueImsi = DynamicCast<LteUeNetDevice> (m_ueDev)->GetImsi ();

      Ptr<Ipv4> clientIpv4 = m_ueNode->GetObject<Ipv4> ();
      m_ueAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      m_ueMask = clientIpv4->GetAddress (1, 0).GetMask ();

      // Each HTC UE gets one HTC traffic manager.
      m_htcManager = m_htcFactory.Create<TrafficManager> ();
      m_htcManager->SetImsi (ueImsi);
      m_ueNode->AggregateObject (m_htcManager);

      // Connect the manager to the controller session created trace source.
      Config::ConnectWithoutContext (
        "/NodeList/*/ApplicationList/*/$ns3::EpcController/SessionCreated",
        MakeCallback (&TrafficManager::SessionCreatedCallback, m_htcManager));

      // Install HTC applications into UEs.
      if (m_voipEnable)
        {
          InstallGbrVoip ();
        }
      if (m_rtvgEnable)
        {
          InstallGbrLiveVideoStreaming ();
        }
      if (m_stvdEnable)
        {
          InstallNonGbrBufferedVideoStreaming ();
        }
      if (m_rtvnEnalbe)
        {
          InstallNonGbrLiveVideoStreaming ();
        }
      if (m_httpEnable)
        {
          InstallNonGbrHttp ();
        }
    }
  m_ueNode = 0;
  m_ueDev = 0;
  m_htcManager = 0;
}

void
TrafficHelper::InstallMtcApplications (NodeContainer ueNodes,
                                       NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  // Install manager and applications into nodes.
  for (uint32_t u = 0; u < ueNodes.GetN (); u++)
    {
      m_ueNode = ueNodes.Get (u);
      m_ueDev = ueDevices.Get (u);
      NS_ASSERT (m_ueDev->GetNode () == m_ueNode);
      uint64_t ueImsi = DynamicCast<LteUeNetDevice> (m_ueDev)->GetImsi ();

      Ptr<Ipv4> clientIpv4 = m_ueNode->GetObject<Ipv4> ();
      m_ueAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      m_ueMask = clientIpv4->GetAddress (1, 0).GetMask ();

      // Each MTC UE gets one MTC traffic manager.
      m_mtcManager = m_mtcFactory.Create<TrafficManager> ();
      m_mtcManager->SetImsi (ueImsi);
      m_ueNode->AggregateObject (m_mtcManager);

      // Connect the manager to the controller session created trace source.
      Config::ConnectWithoutContext (
        "/NodeList/*/ApplicationList/*/$ns3::EpcController/SessionCreated",
        MakeCallback (&TrafficManager::SessionCreatedCallback, m_mtcManager));

      // Install MTC applications into UEs
      if (m_plotEnable)
        {
          InstallGbrAutoPilot ();
        }
    }
  m_ueNode = 0;
  m_ueDev = 0;
  m_mtcManager = 0;
}

uint16_t
TrafficHelper::GetNextPortNo ()
{
  NS_ABORT_MSG_IF (m_port == 0xFFFF, "No more ports available for use.");
  return m_port++;
}

const std::string
TrafficHelper::GetVideoFilename (uint8_t idx)
{
  return m_videoDir + m_videoTrace [idx];
}

const DataRate
TrafficHelper::GetVideoGbr (uint8_t idx)
{
  return DataRate (m_gbrBitRate [idx]);
}

const DataRate
TrafficHelper::GetVideoMbr (uint8_t idx)
{
  return DataRate (m_mbrBitRate [idx]);
}

Ptr<LteHelper>
TrafficHelper::GetLteHelper ()
{
  return m_lteNetwork->GetLteHelper ();
}

void
TrafficHelper::InstallGbrVoip ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  // Dedicated GBR EPS bearer (QCI 1).
  GbrQosInformation qos;
  qos.gbrDl = 47200;  // ~46.09 Kbps
  qos.gbrUl = 47200;  // ~46.09 Kbps
  EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

  Ptr<SdmnClientApp> cApp = m_voipHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_htcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallGbrAutoPilot ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  // Dedicated GBR EPS bearer (QCI 3).
  GbrQosInformation qos;
  qos.gbrUl = 150000;  // ~146 Kbps
  EpsBearer bearer (EpsBearer::GBR_GAMING, qos);

  Ptr<SdmnClientApp> cApp = m_plotHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::UPLINK;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);


  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_mtcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallGbrLiveVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_rtvdHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  // Dedicated GBR EPS bearer (QCI 4).
  GbrQosInformation qos;
  qos.gbrDl = GetVideoGbr (videoIdx).GetBitRate ();
  qos.mbrDl = GetVideoMbr (videoIdx).GetBitRate ();
  EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);

  Ptr<SdmnClientApp> cApp = m_rtvdHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::DOWNLINK;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_htcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrBufferedVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  // Dedicated Non-GBR EPS bearer (QCI 6).
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);

  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_stvdHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  Ptr<SdmnClientApp> cApp = m_stvdHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = TcpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_htcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrLiveVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  // Dedicated Non-GBR EPS bearer (QCI 7).
  EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);

  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_rtvdHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  Ptr<SdmnClientApp> cApp = m_rtvdHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::DOWNLINK;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_htcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrHttp ()
{
  NS_LOG_FUNCTION (this);
  uint16_t port = GetNextPortNo ();

  // Dedicated Non-GBR EPS bearer (QCI 8).
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);

  Ptr<SdmnClientApp> cApp = m_httpHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port,
      EpcController::Qci2Dscp (bearer.qci));

  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = TcpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd = port;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_htcManager->AddSdmnClientApp (cApp);

  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

};  // namespace ns3
