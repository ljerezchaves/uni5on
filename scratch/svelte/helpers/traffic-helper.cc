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
#include "../applications/buffered-video-client.h"
#include "../applications/buffered-video-server.h"
#include "../applications/http-client.h"
#include "../applications/http-server.h"
#include "../applications/live-video-client.h"
#include "../applications/live-video-server.h"
#include "../applications/svelte-udp-client.h"
#include "../applications/svelte-udp-server.h"
#include "../applications/app-stats-calculator.h"
#include "../infrastructure/radio-network.h"
#include "../logical/slice-controller.h"
#include "../logical/slice-network.h"
#include "../logical/traffic-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficHelper");
NS_OBJECT_ENSURE_REGISTERED (TrafficHelper);

// Initial port number
uint16_t TrafficHelper::m_port = 10000;

// Trace files directory
const std::string TrafficHelper::m_videoDir = "./movies/";

// Trace files are sorted in increasing gbr bit rate
const std::string TrafficHelper::m_videoTrace [] = {
  "office-cam-low.txt", "office-cam-medium.txt", "office-cam-high.txt",
  "first-contact.txt", "star-wars-iv.txt", "ard-talk.txt", "mr-bean.txt",
  "n3-talk.txt", "the-firm.txt", "ard-news.txt", "jurassic-park.txt",
  "from-dusk-till-dawn.txt", "formula1.txt", "soccer.txt",
  "silence-of-the-lambs.txt"
};

// These values were obtained from observing the first 180 seconds of video
const uint64_t TrafficHelper::m_gbrBitRate [] = {
  120000, 128000, 450000, 400000, 500000, 500000, 600000, 650000, 700000,
  750000, 770000, 800000, 1100000, 1300000, 1500000
};

// These values were obtained from observing the first 180 seconds of video
const uint64_t TrafficHelper::m_mbrBitRate [] = {
  128000, 600000, 500000, 650000, 600000, 700000, 800000, 750000, 800000,
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
    .AddAttribute ("EnableGbrAutPilot",
                   "Enable GBR auto-pilot traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrAutPilot),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrGameOpen",
                   "Enable GBR game Open Arena traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrGameOpen),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrGameTeam",
                   "Enable GBR game Team Fortress traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrGameTeam),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrLivVideo",
                   "Enable GBR live video streaming traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrLivVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableGbrVoipCall",
                   "Enable GBR VoIP call traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrVoipCall),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonAutPilot",
                   "Enable Non-GBR auto-pilot traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonAutPilot),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonBikeRace",
                   "Enable Non-GBR bicycle race traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonBikeRace),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonBufVideo",
                   "Enable Non-GBR buffered video traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonBufVideo),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonGpsTrack",
                   "Enable Non-GBR GPS team tracking traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonGpsTrack),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonHttpPage",
                   "Enable Non-GBR HTTP webpage traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonHttpPage),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableNonLivVideo",
                   "Enable Non-GBR live video streaming traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_nonLivVideo),
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
  Object::DoDispose ();
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (!m_radio, "No radio network.");
  NS_ABORT_MSG_IF (!m_slice, "No slice network.");

  // Saving pointers.
  m_lteHelper = m_radio->GetLteHelper ();
  m_webNode = m_slice->GetWebNode ();

  // Saving server metadata.
  NS_ASSERT_MSG (m_webNode->GetNDevices () == 2, "Single device expected.");
  Ptr<NetDevice> webDev = m_webNode->GetDevice (1);
  m_webAddr = Ipv4AddressHelper::GetAddress (webDev);
  m_webMask = Ipv4AddressHelper::GetMask (webDev);

  // Configure the traffic manager object factory.
  m_managerFac.SetTypeId (TrafficManager::GetTypeId ());
  m_managerFac.Set ("PoissonInterArrival", PointerValue (m_poissonRng));
  m_managerFac.Set ("RestartApps", BooleanValue (m_restartApps));
  m_managerFac.Set ("StartAppsAfter", TimeValue (m_startAppsAfter));
  m_managerFac.Set ("StopRestartAppsAt", TimeValue (m_stopRestartAppsAt));

  // Configure random video selection.
  m_videoRng = CreateObject<UniformRandomVariable> ();
  m_videoRng->SetAttribute ("Min", DoubleValue (0));
  m_videoRng->SetAttribute ("Max", DoubleValue (14));

  // Configure the helpers and install the applications.
  ConfigureHelpers ();
  InstallApplications ();

  Object::NotifyConstructionCompleted ();
}

void
TrafficHelper::ConfigureHelpers ()
{
  NS_LOG_FUNCTION (this);

  // -------------------------------------------------------------------------
  // Configuring HTC application helpers.
  //
  // BufferedVideo, HTTP, and LiveVideo applications have their own custom
  // implementation with internal attributes already configured. We just
  // instantiate their helpers here.
  m_bufVideoHelper = ApplicationHelper (
      BufferedVideoClient::GetTypeId (),
      BufferedVideoServer::GetTypeId ());
  m_httpPageHelper = ApplicationHelper (
      HttpClient::GetTypeId (),
      HttpServer::GetTypeId ());
  m_livVideoHelper = ApplicationHelper (
      LiveVideoClient::GetTypeId (),
      LiveVideoServer::GetTypeId ());


  // The VoIP application simulating the G.729 codec (~8.0 kbps for payload).
  m_voipCallHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_voipCallHelper.SetClientAttribute ("AppName", StringValue ("VoipCall"));

  // For traffic length, we are considering an estimative from Vodafone that
  // the average call length is 1 min and 40 sec. We are including a normal
  // standard deviation of 10 sec. See http://tinyurl.com/pzmyys2 and
  // http://www.theregister.co.uk/2013/01/30/mobile_phone_calls_shorter for
  // more information on this topic.
  m_voipCallHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=100.0|Variance=100.0]"));

  // Model chosen: 20B packets sent in both directions every 0.02 seconds.
  m_voipCallHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=20]"));
  m_voipCallHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::ConstantRandomVariable[Constant=0.02]"));
  m_voipCallHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=20]"));
  m_voipCallHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::ConstantRandomVariable[Constant=0.02]"));


  // The online game Open Arena.
  m_gameOpenHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_gameOpenHelper.SetClientAttribute ("AppName", StringValue ("GameOpen"));

  // For traffic length, we are using a synthetic average length of 90 seconds
  // with 10 sec stdev. This will force the application to periodically stop
  // and report statistics.
  m_gameOpenHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=100.0]"));

  // Traffic model.
  m_gameOpenHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::NormalRandomVariable[Mean=42.199|Variance=4.604]"));
  m_gameOpenHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.069|Max=0.103]"));
  m_gameOpenHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::NormalRandomVariable[Mean=172.400|Variance=85.821]"));
  m_gameOpenHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.041|Max=0.047]"));


  // The online game Team Fortress.
  m_gameTeamHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_gameTeamHelper.SetClientAttribute ("AppName", StringValue ("GameTeam"));

  // For traffic length, we are using a synthetic average length of 90 seconds
  // with 10 sec stdev. This will force the application to periodically stop
  // and report statistics.
  m_gameTeamHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=100.0]"));

  // Traffic model.
  m_gameTeamHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::NormalRandomVariable[Mean=76.523|Variance=13.399]"));
  m_gameTeamHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.031|Max=0.042]"));
  m_gameTeamHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::NormalRandomVariable[Mean=240.752|Variance=79.339]"));
  m_gameTeamHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.039|Max=0.046]"));


  // -------------------------------------------------------------------------
  // Configuring MTC application helpers.
  //
  // The auto-pilot includes both vehicle collision detection and avoidance on
  // highways. Clients sending data on position, in time intervals depending on
  // vehicle speed, while server performs calculations, collision detection
  // etc., and sends back control information.
  m_autPilotHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_autPilotHelper.SetClientAttribute ("AppName", StringValue ("AutPilot"));

  // For traffic length, we are using a synthetic average length of 90 seconds
  // with 10 sec stdev. This will force the application to periodically stop
  // and report statistics.
  m_autPilotHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=100.0]"));

  // Model chosen: 1kB packets sent towards the server with uniformly
  // distributed inter-arrival time ranging from 0.025 to 0.1s, server responds
  // every second with 1kB message.
  m_autPilotHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=1024]"));
  m_autPilotHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.025|Max=0.1]"));
  m_autPilotHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=1024]"));
  m_autPilotHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.999|Max=1.001]"));


  // The bicycle race is a virtual game where two or more players exchange real
  // data on bicycle position, speed etc. They are used by the application to
  // calculate the equivalent positions of the participants and to show them
  // the corresponding state of the race.
  m_bikeRaceHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_bikeRaceHelper.SetClientAttribute ("AppName", StringValue ("BikeRace"));

  // For traffic length, we are using a synthetic average length of 90 seconds
  // with 10 sec stdev. This will force the application to periodically stop
  // and report statistics.
  m_bikeRaceHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=100.0]"));

  // Model chosen: 1kB packets exchanged with uniformly distributed inter-
  // arrival time ranging from 0.1 to 0.5s.
  m_bikeRaceHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=1024]"));
  m_bikeRaceHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=0.5]"));
  m_bikeRaceHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=1024]"));
  m_bikeRaceHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=0.5]"));


  // The GPS Keep Alive messages in Team Tracking application model clients
  // with team members sending data on position, depending on activity.
  m_gpsTrackHelper = ApplicationHelper (
      SvelteUdpClient::GetTypeId (),
      SvelteUdpServer::GetTypeId ());
  m_gpsTrackHelper.SetClientAttribute ("AppName", StringValue ("GpsTrack"));

  // For traffic length, we are using a synthetic average length of 90 seconds
  // with 10 sec stdev. This will force the application to periodically stop
  // and report statistics.
  m_gpsTrackHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=100.0]"));

  // Model chosen: 0.5kB packets sent with uniform inter-arrival time
  // distribution ranging from 1s to 25s.
  m_gpsTrackHelper.SetClientAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=512]"));
  m_gpsTrackHelper.SetClientAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=25.0]"));
  m_gpsTrackHelper.SetServerAttribute (
    "PktSize",
    StringValue ("ns3::ConstantRandomVariable[Constant=512]"));
  m_gpsTrackHelper.SetServerAttribute (
    "PktInterval",
    StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=25.0]"));

}

void
TrafficHelper::InstallApplications ()
{
  NS_LOG_FUNCTION (this);

  NodeContainer ueNodes = m_slice->GetUeNodes ();
  NetDeviceContainer ueDevices = m_slice->GetUeDevices ();

  // Install traffic manager and applications into UE nodes.
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
        MakeCallback (&TrafficManager::NotifySessionCreated, m_ueManager));

      // Install enabled applications into UEs.
      if (m_gbrAutPilot)
        {
          // UDP uplink auto-pilot traffic over dedicated GBR EPS bearer.
          // This QCI 3 is typically associated with an operator controlled
          // service, i.e., a service where the data flow aggregates
          // uplink/downlink packet filters are known at the point in time.
          GbrQosInformation qos;
          qos.gbrDl = 12000;   //  12 Kbps
          qos.gbrUl = 150000;  // 150 Kbps
          EpsBearer bearer (EpsBearer::GBR_GAMING, qos);
          InstallAutoPilot (bearer);
        }

      if (m_gbrLivVideo)
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

      if (m_gbrVoipCall)
        {
          // UDP bidirectional VoIP traffic over dedicated GBR EPS bearer.
          // This QCI 1 is typically associated with conversational voice.
          GbrQosInformation qos;
          qos.gbrDl = 45000;  // 45 Kbps
          qos.gbrUl = 45000;  // 45 Kbps
          EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);
          InstallVoip (bearer);
        }

      if (m_nonAutPilot)
        {
          // UDP uplink auto-pilot traffic over dedicated Non-GBR EPS bearer.
          // This QCI 5 is typically associated with IMS signaling, but we are
          // using it here as the last Non-GBR QCI available so we can uniquely
          // identify this Non-GBR traffic on the network.
          EpsBearer bearer (EpsBearer::NGBR_IMS);
          InstallAutoPilot (bearer);
        }

      if (m_nonBufVideo)
        {
          // TCP bidirectional buffered video streaming over dedicated Non-GBR
          // EPC bearer. This QCI 6 could be used for prioritization of non
          // real-time data of MPS subscribers.
          int videoIdx = m_videoRng->GetInteger ();
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);
          InstallBufferedVideo (bearer, GetVideoFilename (videoIdx));
        }

      if (m_nonHttpPage)
        {
          // TCP bidirectional HTTP traffic over dedicated Non-GBR EPS bearer.
          // This QCI 8 could be used for a dedicated 'premium bearer' for any
          // subscriber, or could be used for the default bearer of a for
          // 'premium subscribers'.
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);
          InstallHttp (bearer);
        }

      if (m_nonLivVideo)
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
  Ptr<SvelteClient> cApp = m_autPilotHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  m_ueManager->AddSvelteClient (cApp);

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

  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
  cApp->SetEpsBearer (bearer);
  cApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallBufferedVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_bufVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SvelteClient> cApp = m_bufVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  m_ueManager->AddSvelteClient (cApp);

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

  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
  cApp->SetEpsBearer (bearer);
  cApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallHttp (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClient> cApp = m_httpPageHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  m_ueManager->AddSvelteClient (cApp);

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

  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
  cApp->SetEpsBearer (bearer);
  cApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallLiveVideo (EpsBearer bearer, std::string name)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  m_livVideoHelper.SetServerAttribute ("TraceFilename", StringValue (name));
  Ptr<SvelteClient> cApp = m_livVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  m_ueManager->AddSvelteClient (cApp);

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

  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
  cApp->SetEpsBearer (bearer);
  cApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallVoip (EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);

  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClient> cApp = m_voipCallHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  m_ueManager->AddSvelteClient (cApp);

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

  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
  cApp->SetEpsBearer (bearer);
  cApp->SetEpsBearerId (bid);
}

} // namespace ns3
