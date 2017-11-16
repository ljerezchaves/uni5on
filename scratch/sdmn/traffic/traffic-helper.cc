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
#include "../apps/buffered-video-client.h"
#include "../apps/buffered-video-server.h"
#include "../apps/http-client.h"
#include "../apps/http-server.h"
#include "../apps/live-video-client.h"
#include "../apps/live-video-server.h"
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

    // HTC applications to be installed.
    .AddAttribute ("EnableHtcGbrLiveVideo",
                   "Enable HTC GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrLiveVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableHtcGbrVoip",
                   "Enable HTC GBR VoIP traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrVoip),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableHtcNonGbrBufferedVideo",
                   "Enable HTC Non-GBR buffered video traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrBuffVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableHtcNonGbrHttp",
                   "Enable HTC Non-GBR HTTP traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrHttp),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableHtcNonGbrLiveVideo",
                   "Enable HTC Non-GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrLiveVideo),
                   MakeBooleanChecker ())

    // MTC applications to be installed.
    .AddAttribute ("EnableMtcGbrAutoPilot",
                   "Enable MTC GBR auto-pilot traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrAutoPilot),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableMtcNonGbrAutoPilot",
                   "Enable MTC Non-GBR auto-pilot traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrAutoPilot),
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

  m_autoPilotHelper = SdmnAppHelper (AutoPilotClient::GetTypeId (),
                                     AutoPilotServer::GetTypeId ());
  m_buffVideoHelper = SdmnAppHelper (BufferedVideoClient::GetTypeId (),
                                     BufferedVideoServer::GetTypeId ());
  m_httpHelper = SdmnAppHelper (HttpClient::GetTypeId (),
                                HttpServer::GetTypeId ());
  m_liveVideoHelper = SdmnAppHelper (LiveVideoClient::GetTypeId (),
                                     LiveVideoServer::GetTypeId ());
  m_voipHelper = SdmnAppHelper (VoipClient::GetTypeId (),
                                VoipServer::GetTypeId ());
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
      //
      // WARNING: The QCIs used here for each application are strongly related
      // to the DSCP mapping implemented in the EPC controller. This will
      // further reflect on the priority queues used by both OpenFlow switches
      // and traffic control module. Be careful if you intend to change it.
      if (m_gbrLiveVideo)
        {
          // UDP downlink live video streaming over dedicated GBR EPS bearer.
          // This QCI 4 is typically associated with non-conversational video
          // and live streaming.
          GbrQosInformation qos;
          int videoIdx = m_videoRng->GetInteger ();
          qos.gbrDl = GetVideoGbr (videoIdx).GetBitRate ();
          qos.mbrDl = GetVideoMbr (videoIdx).GetBitRate ();
          EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);
          InstallLiveVideo (bearer, GetVideoFilename (videoIdx));
        }

      if (m_gbrVoip)
        {
          // UDP bidirectional VoIP traffic over dedicated GBR EPS bearer.
          // This QCI 1 is typically associated with conversational voice.
          GbrQosInformation qos;
          qos.gbrDl = 47200;  // ~46.09 Kbps
          qos.gbrUl = 47200;  // ~46.09 Kbps
          EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);
          InstallVoip (bearer);
        }

      if (m_nonGbrBuffVideo)
        {
          // TCP bidirectional buffered video streaming over dedicated Non-GBR
          // EPC bearer. This QCI 6 could be used for prioritization of non
          // real-time data of MPS subscribers.
          int videoIdx = m_videoRng->GetInteger ();
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);
          InstallBufferedVideo (bearer, GetVideoFilename (videoIdx));
        }

      if (m_nonGbrHttp)
        {
          // TCP bidirectional HTTP traffic over dedicated Non-GBR EPS bearer.
          // This QCI 8 could be used for a dedicated 'premium bearer' for any
          // subscriber, or could be used for the default bearer of a for
          // 'premium subscribers'.
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);
          InstallHttp (bearer);
        }

      if (m_nonGbrLiveVideo)
        {
          // UDP downlink live video streaming over dedicated Non-GBR EPS
          // bearer. This QCI 7 is typically associated with voice, live video
          // streaming and interactive games.
          int videoIdx = m_videoRng->GetInteger ();
          EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);
          InstallLiveVideo (bearer, GetVideoFilename (videoIdx));
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
      //
      // WARNING: The QCIs used here for each application are strongly related
      // to the DSCP mapping implemented in the EPC controller. This will
      // further reflect on the priority queues used by both OpenFlow switches
      // and traffic control module. Be careful if you intend to change it.
      if (m_gbrAutoPilot)
        {
          // UDP uplink auto-pilot traffic over dedicated GBR EPS bearer.
          // This QCI 3 is typically associated with an operator controlled
          // service, i.e., a service where the data flow aggregate's
          // uplink/downlink packet filters are known at the point in time
          // when the data flow aggregate is authorized.
          GbrQosInformation qos;
          qos.gbrUl = 150000;  // ~146 Kbps
          EpsBearer bearer (EpsBearer::GBR_GAMING, qos);
          InstallAutoPilot (bearer);
        }

      if (m_nonGbrAutoPilot)
        {
          // UDP uplink auto-pilot traffic over dedicated Non-GBR EPS bearer.
          // This QCI 5 is typically associated with IMS signalling, but we are
          // using it here as the last Non-GBR QCI available so we can uniquely
          // identify the MTC Non-GBR traffic on the network.
          EpsBearer bearer (EpsBearer::NGBR_IMS);
          InstallAutoPilot (bearer);
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
TrafficHelper::InstallAutoPilot (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SdmnClientApp> cApp = m_autoPilotHelper.Install (
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
TrafficHelper::InstallBufferedVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_buffVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SdmnClientApp> cApp = m_buffVideoHelper.Install (
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
TrafficHelper::InstallHttp (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
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

void
TrafficHelper::InstallLiveVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_liveVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SdmnClientApp> cApp = m_liveVideoHelper.Install (
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
TrafficHelper::InstallVoip (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
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

};  // namespace ns3
