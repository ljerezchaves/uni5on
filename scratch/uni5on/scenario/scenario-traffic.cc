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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "scenario-traffic.h"
#include "../applications/http-client.h"
#include "../applications/http-server.h"
#include "../applications/live-video-client.h"
#include "../applications/live-video-server.h"
#include "../applications/recorded-video-client.h"
#include "../applications/recorded-video-server.h"
#include "../applications/udp-generic-client.h"
#include "../applications/udp-generic-server.h"
#include "../metadata/ue-info.h"
#include "../traffic/movie-helper.h"
#include "../traffic/traffic-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScenarioTraffic");
NS_OBJECT_ENSURE_REGISTERED (ScenarioTraffic);

// ------------------------------------------------------------------------ //
ScenarioTraffic::ScenarioTraffic ()
{
  NS_LOG_FUNCTION (this);
}

ScenarioTraffic::~ScenarioTraffic ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ScenarioTraffic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScenarioTraffic")
    .SetParent<TrafficHelper> ()
    .AddConstructor<ScenarioTraffic> ()

    // Traffic manager attributes.
    .AddAttribute ("FullAppsAt",
                   "The time to set application start probability to 100%.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&ScenarioTraffic::m_fullProbAt),
                   MakeTimeChecker (Time (0)))
    .AddAttribute ("HalfAppsAt",
                   "The time to set application start probability to 50%.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&ScenarioTraffic::m_halfProbAt),
                   MakeTimeChecker (Time (0)))
    .AddAttribute ("NoneAppsAt",
                   "The time to set application start probability to 0%.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&ScenarioTraffic::m_zeroProbAt),
                   MakeTimeChecker (Time (0)))
  ;
  return tid;
}

void
ScenarioTraffic::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  TrafficHelper::DoDispose ();
}

void
ScenarioTraffic::ConfigureHelpers (void)
{
  NS_LOG_FUNCTION (this);

  // -------------------------------------------------------------------------
  // Configuring application helpers for MBB and TMP slices.

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
  // Pre-recorded video application based on MPEG-4 video traces from
  // http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
  //
  m_recVideoHelper = ApplicationHelper (RecordedVideoClient::GetTypeId (),
                                        RecordedVideoServer::GetTypeId ());
  m_recVideoHelper.SetClientAttribute ("AppName", StringValue ("RecVideo"));

  // Traffic length: we are considering a statistic that the majority of
  // YouTube brand videos are somewhere between 31 and 120 seconds long.
  // So we are using the average length of 1 min 30 sec, with 15 sec stdev.
  // See http://tinyurl.com/q5xkwnn and http://tinyurl.com/klraxum for details.
  // Note that this length will only be used to get the size of the video which
  // will be sent to the client over a TCP connection.
  m_recVideoHelper.SetClientAttribute (
    "TrafficLength",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"));


  //
  // The VoIP application with the G.729 codec.
  //
  m_voipCallHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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
  m_gameOpenHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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
  m_gameTeamHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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
  // Configuring application helpers for MTC slice.
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
  m_autPilotHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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
  m_bikeRaceHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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
  m_gpsTrackHelper = ApplicationHelper (UdpGenericClient::GetTypeId (),
                                        UdpGenericServer::GetTypeId ());
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

void
ScenarioTraffic::ConfigureUeTraffic (Ptr<UeInfo> ueInfo)
{
  NS_LOG_FUNCTION (this << ueInfo->GetImsi ());

  // Schedule traffic manager start probability updates.
  Ptr<TrafficManager> ueManager = ueInfo->GetTrafficManager ();
  if (!m_fullProbAt.IsZero ())
    {
      Simulator::Schedule (m_fullProbAt, &TrafficManager::SetAttribute,
                           ueManager, "StartProb", DoubleValue (1.0));
    }
  if (!m_halfProbAt.IsZero ())
    {
      Simulator::Schedule (m_halfProbAt, &TrafficManager::SetAttribute,
                           ueManager, "StartProb", DoubleValue (0.5));
    }
  if (!m_zeroProbAt.IsZero ())
    {
      Simulator::Schedule (m_zeroProbAt, &TrafficManager::SetAttribute,
                           ueManager, "StartProb", DoubleValue (0.0));
    }

  // Install applications into this UE according to network slice.
  //
  // The QCIs used here for each application are strongly related to the DSCP
  // mapping, which will reflect on the queues used by both OpenFlow switches
  // and traffic control module. Be careful if you intend to change it.
  //
  // Some notes about internal GbrQosInformation usage:
  // - The Maximum Bit Rate field is used by controller to install meter rules
  //   for this traffic. When this value is left to 0, no meter rules will be
  //   installed.
  // - The Guaranteed Bit Rate field is used by the controller to reserve the
  //   requested bandwidth in OpenFlow EPC network (only for GBR beares).
  switch (GetSliceId ())
    {
    case SliceId::MBB:
      {
        // VoIP call over dedicated GBR EPS bearer.
        // QCI 1 is typically associated with conversational voice.
        GbrQosInformation qos;
        qos.gbrDl = 45000;    // 45 Kbps
        qos.gbrUl = 45000;    // 45 Kbps
        EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_voipCallHelper, bearer, filter);
      }
      {
        // Video call over dedicated GBR EPS bearer.
        // QCI 2 is typically associated with conversational live video
        // streaming.
        MovieHelper::VideoTrace video;
        video = MovieHelper::GetRandomVideo (QosType::GBR);
        m_livVideoHelper.SetServerAttribute (
          "TraceFilename", StringValue (video.name));
        m_livVideoHelper.SetClientAttribute (
          "TraceFilename", StringValue (video.name));
        GbrQosInformation qos;
        qos.gbrDl = video.gbr.GetBitRate ();
        qos.gbrUl = video.gbr.GetBitRate ();
        qos.mbrDl = video.mbr.GetBitRate ();
        qos.mbrUl = video.mbr.GetBitRate ();
        EpsBearer bearer (EpsBearer::GBR_CONV_VIDEO, qos);

        // Downlink UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_livVideoHelper, bearer, filter);
      }
      {
        // Open Arena game over dedicated Non-GBR EPS bearer.
        // QCI 6 is typically associated with voice, buffered video
        // streaming and TCP-based applications. It could be used for
        // prioritization of non real-time data of MPS subscribers.
        EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_gameOpenHelper, bearer, filter);
      }
      {
        // Team Fortress game over dedicated Non-GBR EPS bearer.
        // QCI 6 is typically associated with voice, buffered video
        // streaming and TCP-based applications. It could be used for
        // prioritization of non real-time data of MPS subscribers.
        EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_gameTeamHelper, bearer, filter);
      }
      {
        // Live video streaming over dedicated Non-GBR EPS bearer.
        // QCI 7 is typically associated with voice, live video streaming
        // and interactive games.
        MovieHelper::VideoTrace video;
        video = MovieHelper::GetRandomVideo (QosType::NON);
        m_livVideoHelper.SetServerAttribute (
          "TraceFilename", StringValue (video.name));
        m_livVideoHelper.SetClientAttribute (
          "TraceFilename", StringValue (video.name));
        EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);

        // Downlink UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_livVideoHelper, bearer, filter);
      }
      {
        // Pre-recorded video streaming over dedicated Non-GBR EPS bearer.
        // QCI 8 is typically associated with buffered video streaming and
        // TCP-based applications. It could be used for a dedicated
        // 'premium bearer' for any subscriber, or could be used for the
        // default bearer of a UE for 'premium subscribers'.
        MovieHelper::VideoTrace video;
        video = MovieHelper::GetRandomVideo (QosType::NON);
        m_recVideoHelper.SetServerAttribute (
          "TraceFilename", StringValue (video.name));
        EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);

        // Bidirectional TCP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = TcpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_recVideoHelper, bearer, filter);
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
        InstallAppDedicated (ueInfo, m_httpPageHelper, bearer, filter);
      }
      {
        // HTTP webpage traffic over default Non-GBR EPS bearer.
        InstallAppDefault (ueInfo, m_httpPageHelper);
      }
      break;

    case SliceId::MTC:
      {
        // Auto-pilot traffic over dedicated GBR EPS bearer.
        // QCI 3 is typically associated with real-time gaming.
        GbrQosInformation qos;
        qos.gbrDl = 15000;     //  15 Kbps
        qos.gbrUl = 180000;    // 180 Kbps
        EpsBearer bearer (EpsBearer::GBR_GAMING, qos);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_autPilotHelper, bearer, filter);
      }
      {
        // Auto-pilot traffic over dedicated Non-GBR EPS bearer.
        // QCI 6 is typically associated with voice, buffered video
        // streaming and TCP-based applications. It could be used for
        // prioritization of non real-time data of MPS subscribers.
        EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_autPilotHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_autPilotHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_autPilotHelper, bearer, filter);
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
        InstallAppDedicated (ueInfo, m_bikeRaceHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_bikeRaceHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_bikeRaceHelper, bearer, filter);
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
        InstallAppDedicated (ueInfo, m_gpsTrackHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_gpsTrackHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_gpsTrackHelper, bearer, filter);
      }
      break;

    case SliceId::TMP:
      {
        // VoIP call over dedicated GBR EPS bearer.
        // QCI 1 is typically associated with conversational voice.
        GbrQosInformation qos;
        qos.gbrDl = 45000;    // 45 Kbps
        qos.gbrUl = 45000;    // 45 Kbps
        EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

        // Bidirectional UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_voipCallHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_voipCallHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_voipCallHelper, bearer, filter);
        InstallAppDedicated (ueInfo, m_voipCallHelper, bearer, filter);
      }
      {
        // Live video streaming over dedicated Non-GBR EPS bearer.
        // QCI 7 is typically associated with voice, live video streaming
        // and interactive games.
        MovieHelper::VideoTrace videoDl, videoUl;
        videoDl = MovieHelper::GetRandomVideo (QosType::NON);
        videoUl = MovieHelper::GetRandomVideo (QosType::NON);
        m_livVideoHelper.SetServerAttribute (
          "TraceFilename", StringValue (videoDl.name));
        m_livVideoHelper.SetClientAttribute (
          "TraceFilename", StringValue (videoUl.name));
        EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);

        // Downlink UDP traffic.
        EpcTft::PacketFilter filter;
        filter.direction = EpcTft::BIDIRECTIONAL;
        filter.protocol = UdpL4Protocol::PROT_NUMBER;
        InstallAppDedicated (ueInfo, m_livVideoHelper, bearer, filter);
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
        InstallAppDedicated (ueInfo, m_httpPageHelper, bearer, filter);
      }
      {
        // HTTP webpage traffic over default Non-GBR EPS bearer.
        InstallAppDefault (ueInfo, m_httpPageHelper);
      }
      break;

    default:
      NS_LOG_ERROR ("Invalid slice ID.");

    }
}

} // namespace ns3
