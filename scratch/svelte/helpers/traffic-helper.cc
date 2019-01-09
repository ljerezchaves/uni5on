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
#include "../metadata/ue-info.h"

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

    // Slice.
    .AddAttribute ("SliceId", "The LTE logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceId::UNKN),
                   MakeEnumAccessor (&TrafficHelper::m_sliceId),
                   MakeEnumChecker (SliceId::HTC, "htc",
                                    SliceId::MTC, "mtc",
                                    SliceId::TMP, "tmp"))
    .AddAttribute ("SliceCtrl", "The LTE logical slice controller pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_controller),
                   MakePointerChecker<SliceController> ())
    .AddAttribute ("SliceNet", "The logical slice network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_slice),
                   MakePointerChecker<SliceNetwork> ())

    // Infrastructure.
    .AddAttribute ("RadioNet", "The LTE RAN network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_radio),
                   MakePointerChecker<RadioNetwork> ())

    // Traffic helper attributes.
    .AddAttribute ("UseOnlyDefaultBearer",
                   "Use only the default EPS bearer for all traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_useOnlyDefault),
                   MakeBooleanChecker ())

    // Traffic manager attributes.
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get "
                   "application inter-arrival start times.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=120.0]"),
                   MakePointerAccessor (&TrafficHelper::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Continuously restart applications after stop events.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_restartApps),
                   MakeBooleanChecker ())
    .AddAttribute ("StartAppsAt",
                   "The time to start the applications.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&TrafficHelper::m_startAppsAt),
                   MakeTimeChecker (Seconds (1)))
    .AddAttribute ("StopAppsAt",
                   "The time to stop the applications.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&TrafficHelper::m_stopAppsAt),
                   MakeTimeChecker (Time (0)))
  ;
  return tid;
}

void
TrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_radio = 0;
  m_slice = 0;
  m_controller = 0;
  m_poissonRng = 0;
  m_lteHelper = 0;
  m_webNode = 0;
  t_ueManager = 0;
  t_ueDev = 0;
  t_ueNode = 0;
  m_gbrVidRng = 0;
  m_nonVidRng = 0;
  Object::DoDispose ();
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_sliceId == SliceId::UNKN, "Unknown slice ID.");
  NS_ABORT_MSG_IF (!m_radio, "No radio network.");
  NS_ABORT_MSG_IF (!m_slice, "No slice network.");
  NS_ABORT_MSG_IF (!m_controller, "No slice controller.");

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
  m_managerFac.Set ("StartAppsAt", TimeValue (m_startAppsAt));
  m_managerFac.Set ("StopAppsAt", TimeValue (m_stopAppsAt));

  // Configure random video selections.
  m_gbrVidRng = CreateObject<UniformRandomVariable> ();
  m_gbrVidRng->SetAttribute ("Min", DoubleValue (0));
  m_gbrVidRng->SetAttribute ("Max", DoubleValue (2));

  m_nonVidRng = CreateObject<UniformRandomVariable> ();
  m_nonVidRng->SetAttribute ("Min", DoubleValue (3));
  m_nonVidRng->SetAttribute ("Max", DoubleValue (14));

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
  // Buffered video application based on MPEG-4 video traces from
  // http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
  //
  m_bufVideoHelper = ApplicationHelper (BufferedVideoClient::GetTypeId (),
                                        BufferedVideoServer::GetTypeId ());
  m_bufVideoHelper.SetClientAttribute ("AppName", StringValue ("BufVideo"));

  // Traffic length: we are considering a statistic that the majority of
  // YouTube brand videos are somewhere between 31 and 120 seconds long.
  // So we are using the average length of 1 min 30 sec, with 15 sec stdev.
  // See http://tinyurl.com/q5xkwnn and http://tinyurl.com/klraxum for details.
  // Note that this length will only be used to get the size of the video which
  // will be sent to the client over a TCP connection.
  m_bufVideoHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"));


  //
  // The HTTP model is based on the distributions indicated in the paper 'An
  // HTTP Web Traffic Model Based on the Top One Million Visited Web Pages' by
  // Rastin Pries et. al. Each client will send a request to the server and
  // will receive the page content back, including inline content. These
  // requests repeats after a reading time period.
  //
  m_httpPageHelper = ApplicationHelper (HttpClient::GetTypeId (),
                                        HttpServer::GetTypeId ());
  m_httpPageHelper.SetClientAttribute ("AppName", StringValue ("HttpPage"));

  // Traffic length: we are using a arbitrary normally-distributed medium
  // traffic length of 60 sec with 15 sec stdev.
  m_httpPageHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=60.0|Variance=225.0]"));


  //
  // Live video application based on MPEG-4 video traces from
  // http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
  //
  m_livVideoHelper = ApplicationHelper (LiveVideoClient::GetTypeId (),
                                        LiveVideoServer::GetTypeId ());
  m_livVideoHelper.SetClientAttribute ("AppName", StringValue ("LivVideo"));

  // Traffic length: we are considering a statistic that the majority of
  // YouTube brand videos are somewhere between 31 and 120 seconds long.
  // So we are using the average length of 1 min 30 sec, with 15 sec stdev.
  // See http://tinyurl.com/q5xkwnn and http://tinyurl.com/klraxum for details.
  m_livVideoHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"));


  //
  // The VoIP application with the G.729 codec.
  //
  m_voipCallHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_voipCallHelper.SetClientAttribute ("AppName", StringValue ("VoipCall"));

  // Traffic length: we are considering an estimative from Vodafone that
  // the average call length is 1 min and 40 sec with a 10 sec stdev, See
  // http://tinyurl.com/pzmyys2 and https://tinyurl.com/yceqtej9 for details.
  m_voipCallHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=100.0|Variance=100.0]"));

  // Traffic model: 20B packets sent in both directions every 0.02 seconds.
  // Check http://goo.gl/iChPGQ for bandwidth calculation and discussion.
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


  //
  // The online game Open Arena.
  //
  m_gameOpenHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_gameOpenHelper.SetClientAttribute ("AppName", StringValue ("GameOpen"));

  // Traffic length: we are using a arbitrary normally-distributed short
  // traffic length of 45 sec with 10 sec stdev.
  m_gameOpenHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=45.0|Variance=100.0]"));

  // Traffic model:
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


  //
  // The online game Team Fortress.
  //
  m_gameTeamHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_gameTeamHelper.SetClientAttribute ("AppName", StringValue ("GameTeam"));

  // Traffic length: we are using a arbitrary normally-distributed short
  // traffic length of 45 sec with 10 sec stdev.
  m_gameTeamHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=45.0|Variance=100.0]"));

  // Traffic model:
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
  // The following applications were adapted from the MTC models presented on
  // the "Machine-to-Machine Communications: Architectures, Technology,
  // Standards, and Applications" book, chapter 3: "M2M traffic and models".

  //
  // The auto-pilot includes both vehicle collision detection and avoidance on
  // highways. Clients sending data on position, in time intervals depending on
  // vehicle speed, while server performs calculations, collision detection
  // etc., and sends back control information.
  //
  m_autPilotHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_autPilotHelper.SetClientAttribute ("AppName", StringValue ("AutPilot"));

  // Traffic length: we are using a arbitrary normally-distributed short
  // traffic length of 45 sec with 10 sec stdev.
  m_autPilotHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=45.0|Variance=100.0]"));

  // Traffic model: 1kB packets sent towards the server with uniformly
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


  //
  // The bicycle race is a virtual game where two or more players exchange real
  // data on bicycle position, speed, etc. They are used by the application to
  // calculate the equivalent positions of the participants and to show them
  // the corresponding state of the race.
  //
  m_bikeRaceHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_bikeRaceHelper.SetClientAttribute ("AppName", StringValue ("BikeRace"));

  // Traffic length: we are using a arbitrary normally-distributed short
  // traffic length of 45 sec with 10 sec stdev.
  m_bikeRaceHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=45.0|Variance=100.0]"));

  // Traffic model: 1kB packets exchanged with uniformly distributed inter-
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


  //
  // The GPS Keep Alive messages in Team Tracking application model clients
  // with team members sending data on position, depending on activity.
  //
  m_gpsTrackHelper = ApplicationHelper (SvelteUdpClient::GetTypeId (),
                                        SvelteUdpServer::GetTypeId ());
  m_gpsTrackHelper.SetClientAttribute ("AppName", StringValue ("GpsTrack"));

  // Traffic length: we are using a arbitrary normally-distributed long traffic
  // length of 120 sec with 20 sec stdev.
  m_gpsTrackHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=120.0|Variance=400.0]"));

  // Traffic model: 0.5kB packets sent with uniform inter-arrival time
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
TrafficHelper::InstallApplications ()
{
  NS_LOG_FUNCTION (this);

  NodeContainer ueNodes = m_slice->GetUeNodes ();
  NetDeviceContainer ueDevices = m_slice->GetUeDevices ();

  // Install traffic manager and applications into UE nodes.
  for (uint32_t u = 0; u < ueNodes.GetN (); u++)
    {
      t_ueNode = ueNodes.Get (u);
      t_ueDev = ueDevices.Get (u);
      NS_ASSERT (t_ueDev->GetNode () == t_ueNode);
      t_ueImsi = DynamicCast<LteUeNetDevice> (t_ueDev)->GetImsi ();

      Ptr<Ipv4> clientIpv4 = t_ueNode->GetObject<Ipv4> ();
      t_ueAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      t_ueMask = clientIpv4->GetAddress (1, 0).GetMask ();

      // Each UE gets one traffic manager.
      t_ueManager = m_managerFac.Create<TrafficManager> ();
      t_ueManager->SetController (m_controller);
      t_ueManager->SetImsi (t_ueImsi);
      t_ueNode->AggregateObject (t_ueManager);

      // Connect the manager to the controller session created trace source.
      Config::ConnectWithoutContext (
        "/NodeList/*/ApplicationList/*/$ns3::SliceController/SessionCreated",
        MakeCallback (&TrafficManager::NotifySessionCreated, t_ueManager));

      // Install applications into this UE according to network slice.
      if (m_sliceId == SliceId::HTC)
        {
          {
            // HTTP webpage traffic over default Non-GBR EPS bearer.
            InstallAppDefault (m_httpPageHelper);
          }
          {
            // Open Arena game over dedicated GBR EPS bearer.
            // QCI 3 is typically associated with real-time gaming.
            GbrQosInformation qos;
            qos.gbrDl = 45000;  // 45 Kbps
            qos.gbrUl = 12000;  // 12 Kbps
            EpsBearer bearer (EpsBearer::GBR_GAMING, qos);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_gameOpenHelper, bearer, filter);
          }
          {
            // Team Fortress game over dedicated GBR EPS bearer.
            // QCI 3 is typically associated with real-time gaming.
            GbrQosInformation qos;
            qos.gbrDl = 60000;  // 60 Kbps
            qos.gbrUl = 30000;  // 30 Kbps
            EpsBearer bearer (EpsBearer::GBR_GAMING, qos);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_gameTeamHelper, bearer, filter);
          }
          {
            // Live video streaming over dedicated GBR EPS bearer.
            // QCI 4 is typically associated with non-conversational video
            // streaming.
            int videoIdx = m_gbrVidRng->GetInteger ();
            m_livVideoHelper.SetServerAttribute (
              "TraceFilename", StringValue (GetVideoFilename (videoIdx)));
            GbrQosInformation qos;
            qos.gbrDl = GetVideoGbr (videoIdx).GetBitRate ();
            qos.mbrDl = GetVideoMbr (videoIdx).GetBitRate ();
            EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);

            // Downlink UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::DOWNLINK;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_livVideoHelper, bearer, filter);
          }
          {
            // VoIP call over dedicated GBR EPS bearer.
            // QCI 1 is typically associated with conversational voice.
            GbrQosInformation qos;
            qos.gbrDl = 45000;  // 45 Kbps
            qos.gbrUl = 45000;  // 45 Kbps
            EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_voipCallHelper, bearer, filter);
          }
          {
            // Buffered video streaming over dedicated Non-GBR EPS bearer.
            // QCI 6 is typically associated with voice, buffered video
            // streaming and TCP-based applications. It could be used for
            // prioritization of non real-time data of MPS subscribers.
            int videoIdx = m_nonVidRng->GetInteger ();
            m_bufVideoHelper.SetServerAttribute (
              "TraceFilename", StringValue (GetVideoFilename (videoIdx)));
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);

            // Bidirectional TCP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = TcpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_bufVideoHelper, bearer, filter);
          }
          {
            // HTTP webpage traffic over dedicated Non-GBR EPS bearer.
            // QCI 9 is typically associated with buffered video streaming and
            // TCP-based applications. It is typically used for the default
            // bearer of a UE for non privileged subscribers.
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);

            // Bidirectional TCP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = TcpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_httpPageHelper, bearer, filter);
          }
          {
            // Live video streaming over dedicated Non-GBR EPS bearer.
            // QCI 7 is typically associated with voice, live video streaming
            // and interactive games.
            int videoIdx = m_nonVidRng->GetInteger ();
            m_livVideoHelper.SetServerAttribute (
              "TraceFilename", StringValue (GetVideoFilename (videoIdx)));
            EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);

            // Downlink UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::DOWNLINK;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_livVideoHelper, bearer, filter);
          }
          continue;
        }

      if (m_sliceId == SliceId::MTC)
        {
          {
            // Auto-pilot traffic over dedicated GBR EPS bearer.
            // QCI 2 is typically associated with conversational live video
            // streaming. This is not the best QCI for this application, but it
            // will work and will priorize this traffic in the network.
            GbrQosInformation qos;
            qos.gbrDl = 12000;   //  12 Kbps
            qos.gbrUl = 150000;  // 150 Kbps
            EpsBearer bearer (EpsBearer::GBR_CONV_VIDEO, qos);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_autPilotHelper, bearer, filter);
            InstallAppDedicated (m_autPilotHelper, bearer, filter);
          }
          {
            // Auto-pilot traffic over dedicated Non-GBR EPS bearer.
            // QCI 8 is typically associated with buffered video streaming and
            // TCP-based applications. It could be used for a dedicated
            // 'premium bearer' for any subscriber, or could be used for the
            // default bearer of a UE for 'premium subscribers'.
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_autPilotHelper, bearer, filter);
            InstallAppDedicated (m_autPilotHelper, bearer, filter);
          }
          {
            // Virtual bicycle race traffic over dedicated Non-GBR EPS bearer.
            // QCI 8 is typically associated with buffered video streaming and
            // TCP-based applications. It could be used for a dedicated
            // 'premium bearer' for any subscriber, or could be used for the
            // default bearer of a UE for 'premium subscribers'.
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_bikeRaceHelper, bearer, filter);
            InstallAppDedicated (m_bikeRaceHelper, bearer, filter);
            InstallAppDedicated (m_bikeRaceHelper, bearer, filter);
          }
          {
            // GPS Team Tracking traffic over dedicated Non-GBR EPS bearer.
            // QCI 8 is typically associated with buffered video streaming and
            // TCP-based applications. It could be used for a dedicated
            // 'premium bearer' for any subscriber, or could be used for the
            // default bearer of a UE for 'premium subscribers'.
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_gpsTrackHelper, bearer, filter);
            InstallAppDedicated (m_gpsTrackHelper, bearer, filter);
            InstallAppDedicated (m_gpsTrackHelper, bearer, filter);
          }
          continue;
        }

      if (m_sliceId == SliceId::TMP)
        {
          {
            // HTTP webpage traffic over default Non-GBR EPS bearer.
            InstallAppDefault (m_httpPageHelper);
          }
          {
            // VoIP call over dedicated GBR EPS bearer.
            // QCI 1 is typically associated with conversational voice.
            GbrQosInformation qos;
            qos.gbrDl = 45000;  // 45 Kbps
            qos.gbrUl = 45000;  // 45 Kbps
            EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

            // Bidirectional UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_voipCallHelper, bearer, filter);
          }
          {
            // HTTP webpage traffic over dedicated Non-GBR EPS bearer.
            // QCI 9 is typically associated with buffered video streaming and
            // TCP-based applications. It is typically used for the default
            // bearer of a UE for non privileged subscribers.
            EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);

            // Bidirectional TCP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::BIDIRECTIONAL;
            filter.protocol = TcpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_httpPageHelper, bearer, filter);
          }
          {
            // Live video streaming over dedicated Non-GBR EPS bearer.
            // QCI 7 is typically associated with voice, live video streaming
            // and interactive games.
            int videoIdx = m_nonVidRng->GetInteger ();
            m_livVideoHelper.SetServerAttribute (
              "TraceFilename", StringValue (GetVideoFilename (videoIdx)));
            EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);

            // Downlink UDP traffic.
            EpcTft::PacketFilter filter;
            filter.direction = EpcTft::DOWNLINK;
            filter.protocol = UdpL4Protocol::PROT_NUMBER;
            InstallAppDedicated (m_livVideoHelper, bearer, filter);
          }
        }
    }

  t_ueManager = 0;
  t_ueNode = 0;
  t_ueDev = 0;
}

void
TrafficHelper::InstallAppDedicated (
  ApplicationHelper& helper, EpsBearer& bearer, EpcTft::PacketFilter& filter)
{
  NS_LOG_FUNCTION (this);

  // When enable, install all applications over the default UE EPS bearer.
  if (m_useOnlyDefault)
    {
      InstallAppDefault (helper);
      return;
    }

  // Create the client and server applications.
  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClient> clientApp = helper.Install (
      t_ueNode, m_webNode, t_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  t_ueManager->AddSvelteClient (clientApp);

  // Setup common packet filter parameters.
  filter.remoteAddress   = m_webAddr;
  filter.remoteMask      = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd   = port;
  filter.localAddress    = t_ueAddr;
  filter.localMask       = t_ueMask;
  filter.localPortStart  = 0;
  filter.localPortEnd    = 65535;

  // Create the TFT for this bearer.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  tft->Add (filter);

  // Create the dedicated bearer for this traffic.
  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (t_ueDev, bearer, tft);
  clientApp->SetEpsBearer (bearer);
  clientApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallAppDefault (ApplicationHelper& helper)
{
  NS_LOG_FUNCTION (this);

  // Get default EPS bearer information for this UE.
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (t_ueImsi);
  uint8_t bid = ueInfo->GetDefaultBid ();
  EpsBearer bearer = ueInfo->GetEpsBearer (bid);

  // Create the client and server applications.
  uint16_t port = GetNextPortNo ();
  Ptr<SvelteClient> clientApp = helper.Install (
      t_ueNode, m_webNode, t_ueAddr, m_webAddr, port, Qci2Dscp (bearer.qci));
  t_ueManager->AddSvelteClient (clientApp);
  clientApp->SetEpsBearer (bearer);
  clientApp->SetEpsBearerId (bid);
}

} // namespace ns3
