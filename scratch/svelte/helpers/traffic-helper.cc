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
#include "../applications/auto-pilot-client.h"
#include "../applications/auto-pilot-server.h"
#include "../applications/buffered-video-client.h"
#include "../applications/buffered-video-server.h"
#include "../applications/http-client.h"
#include "../applications/http-server.h"
#include "../applications/live-video-client.h"
#include "../applications/live-video-server.h"
#include "../applications/voip-client.h"
#include "../applications/voip-server.h"
#include "../infrastructure/radio-network.h"
#include "../logical/slice-controller.h"
#include "../logical/slice-network.h"
#include "../logical/traffic-manager.h"
#include "../statistics/qos-stats-calculator.h"

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

    // Traffic helper attributes.
    .AddAttribute ("RadioNet", "The LTE RAN network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_radio),
                   MakePointerChecker<RadioNetwork> ())
    .AddAttribute ("SliceNet", "The logical slice network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_slice),
                   MakePointerChecker<SliceNetwork> ())

    // Traffic manager attributes.
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get "
                   "application inter-arrival start times.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&TrafficHelper::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Continuously restart applications after stop events.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_restartApps),
                   MakeBooleanChecker ())
    .AddAttribute ("StartAppsAfter",
                   "The time before starting the applications.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&TrafficHelper::m_startAppsAfter),
                   MakeTimeChecker ())
    .AddAttribute ("StopRestartAppsAt",
                   "The time to disable the RestartApps attribute.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TrafficHelper::m_stopRestartAppsAt),
                   MakeTimeChecker ())

    // Applications to be installed.
    .AddAttribute ("EnableGbrAutoPilot",
                   "Enable GBR auto-pilot traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrAutoPilot),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrLiveVideo",
                   "Enable GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrLiveVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrVoip",
                   "Enable GBR VoIP traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrVoip),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonGbrAutoPilot",
                   "Enable Non-GBR auto-pilot traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrAutoPilot),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonGbrBufferedVideo",
                   "Enable Non-GBR buffered video traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrBuffVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonGbrHttp",
                   "Enable Non-GBR HTTP traffic over TCP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrHttp),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonGbrLiveVideo",
                   "Enable Non-GBR live video streaming traffic over UDP.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGbrLiveVideo),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
TrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_radio = 0;
  m_slice = 0;
  m_poissonRng = 0;
  m_lteHelper = 0;
  m_webNode = 0;
  m_ueManager = 0;
  m_ueDev = 0;
  m_ueNode = 0;
  m_videoRng = 0;
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (!m_radio, "No LTE RAN network.");
  NS_ABORT_MSG_IF (!m_slice, "No slice network.");

  // Saving pointers.
  m_lteHelper = m_radio->GetLteHelper ();
  m_webNode = m_slice->GetWebNode ();

  // Saving server metadata.
  NS_ASSERT_MSG (m_webNode->GetNDevices () == 2, "Single device expected.");
  Ptr<NetDevice> webDev = m_webNode->GetDevice (1);
  m_webAddr = Ipv4AddressHelper::GetAddress (webDev);
  m_webMask = Ipv4AddressHelper::GetMask (webDev);

  // Configuring the traffic manager object factory.
  m_managerFac.SetTypeId (TrafficManager::GetTypeId ());
  m_managerFac.Set ("PoissonInterArrival", PointerValue (m_poissonRng));
  m_managerFac.Set ("RestartApps", BooleanValue (m_restartApps));
  m_managerFac.Set ("StartAppsAfter", TimeValue (m_startAppsAfter));
  m_managerFac.Set ("StopRestartAppsAt", TimeValue (m_stopRestartAppsAt));

  // Random video selection.
  m_videoRng = CreateObject<UniformRandomVariable> ();
  m_videoRng->SetAttribute ("Min", DoubleValue (0));
  m_videoRng->SetAttribute ("Max", DoubleValue (14));

  // Configuring application helpers.
  m_autoPilotHelper = SvelteAppHelper (
      AutoPilotClient::GetTypeId (), AutoPilotServer::GetTypeId ());
  m_buffVideoHelper = SvelteAppHelper (
      BufferedVideoClient::GetTypeId (), BufferedVideoServer::GetTypeId ());
  m_httpHelper = SvelteAppHelper (
      HttpClient::GetTypeId (), HttpServer::GetTypeId ());
  m_liveVideoHelper = SvelteAppHelper (
      LiveVideoClient::GetTypeId (), LiveVideoServer::GetTypeId ());
  m_voipHelper = SvelteAppHelper (
      VoipClient::GetTypeId (), VoipServer::GetTypeId ());

  // Install the applications.
  InstallApplications ();

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
TrafficHelper::InstallApplications ()
{
  NS_LOG_FUNCTION (this);

  NodeContainer ueNodes = m_slice->GetUeNodes ();
  NetDeviceContainer ueDevices = m_slice->GetUeDevices ();

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

      // Each UE gets one traffic manager.
      m_ueManager = m_managerFac.Create<TrafficManager> ();
      m_ueManager->SetImsi (ueImsi);
      m_ueNode->AggregateObject (m_ueManager);

      // Connect the manager to the controller session created trace source.
      Config::ConnectWithoutContext (
        "/NodeList/*/ApplicationList/*/$ns3::SliceController/SessionCreated",
        MakeCallback (&TrafficManager::SessionCreatedCallback, m_ueManager));

      // Install applications into UEs.
      if (m_gbrAutoPilot)
        {
          // UDP uplink auto-pilot traffic over dedicated GBR EPS bearer.
          // This QCI 3 is typically associated with an operator controlled
          // service, i.e., a service where the data flow aggregate's
          // uplink/downlink packet filters are known at the point in time
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
          // identify this Non-GBR traffic on the network.
          EpsBearer bearer (EpsBearer::NGBR_IMS);
          InstallAutoPilot (bearer);
        }

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
  m_ueManager = 0;
  m_ueNode = 0;
  m_ueDev = 0;
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

void
TrafficHelper::InstallAutoPilot (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClientApp> cApp = m_autoPilotHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));

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
  m_ueManager->AddSvelteClientApp (cApp);

  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallBufferedVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_buffVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SvelteClientApp> cApp = m_buffVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));

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
  m_ueManager->AddSvelteClientApp (cApp);

  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallHttp (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClientApp> cApp = m_httpHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));

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
  m_ueManager->AddSvelteClientApp (cApp);

  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallLiveVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_liveVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SvelteClientApp> cApp = m_liveVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));

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
  m_ueManager->AddSvelteClientApp (cApp);

  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallVoip (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClientApp> cApp = m_voipHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));

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
  m_ueManager->AddSvelteClientApp (cApp);

  m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

} // namespace ns3
