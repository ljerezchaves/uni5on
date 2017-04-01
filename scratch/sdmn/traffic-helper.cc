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
#include "lte-network.h"
#include "epc-network.h"
#include "epc-controller.h"
#include "apps/http-client.h"
#include "apps/http-server.h"
#include "apps/real-time-video-client.h"
#include "apps/real-time-video-server.h"
#include "apps/stored-video-client.h"
#include "apps/stored-video-server.h"
#include "apps/voip-client.h"
#include "apps/voip-server.h"
#include "stats/qos-stats-calculator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficHelper");
NS_OBJECT_ENSURE_REGISTERED (TrafficHelper);

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

  // Configuring the traffic manager object factory
  m_managerFactory.SetTypeId (TrafficManager::GetTypeId ());

  // Random video selection
  m_videoRng = CreateObject<UniformRandomVariable> ();
  m_videoRng->SetAttribute ("Min", DoubleValue (0));
  m_videoRng->SetAttribute ("Max", DoubleValue (14));

  //
  // For VoIP call, we are considering an estimative from Vodafone that the
  // average call length is 1 min and 40 sec. We are including a normal
  // standard deviation of 10 sec. See http://tinyurl.com/pzmyys2 and
  // http://www.theregister.co.uk/2013/01/30/mobile_phone_calls_shorter for
  // more information on this topic.
  //
  m_voipHelper = SdmnAppHelper (VoipClient::GetTypeId (),
                                VoipServer::GetTypeId ());
  m_voipHelper.SetClientAttribute ("AppName", StringValue ("Voip"));
  m_voipHelper.SetClientAttribute (
    "CallDuration",
    StringValue ("ns3::NormalRandomVariable[Mean=100.0|Variance=100.0]"));

  //
  // For stored video, we are considering a statistic that the majority of
  // YouTube brand videos are somewhere between 31 and 120 seconds long. So we
  // are using the average length of 1min 30sec, with 15sec stdev.
  // See http://tinyurl.com/q5xkwnn and http://tinyurl.com/klraxum for more
  // information on this topic. Note that this length means the size of the
  // video which will be sent to the client over a TCP connection.
  //
  m_stVideoHelper = SdmnAppHelper (StoredVideoClient::GetTypeId (),
                                   StoredVideoServer::GetTypeId ());
  m_stVideoHelper.SetClientAttribute ("AppName", StringValue ("BuffVid"));
  m_stVideoHelper.SetServerAttribute (
    "VideoDuration",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"));

  //
  // For real time video streaming, we are considering the same statistics for
  // the stored video (above). The difference here is that the traffic is sent
  // in real time, following the trace description.
  //
  m_rtVideoHelper = SdmnAppHelper (RealTimeVideoClient::GetTypeId (),
                                   RealTimeVideoServer::GetTypeId ());
  m_rtVideoHelper.SetClientAttribute ("AppName", StringValue ("LiveVid"));
  m_rtVideoHelper.SetClientAttribute (
    "VideoDuration",
    StringValue ("ns3::NormalRandomVariable[Mean=90.0|Variance=225.0]"));

  //
  // Setting average traffic duration for applications. For Non-GBR traffic,
  // the attributes are related to the amount of traffic which will be sent
  // over the network (mainly over TCP). For GBR traffic, the traffic duration
  // is the real active traffic time.
  //
  // For HTTP traffic, we are fixing the load of 3 web pages before stopping
  // the application and reporting statistics. Note that between page loads
  // there is the random reading time interval. If the reading time exceeds the
  // default switch rule idle timeout (witch is currently set to 15 seconds),
  // we also stop the application and report statistics. This avoids the
  // processes of reinstalling expired rules.
  //
  m_httpHelper = SdmnAppHelper (HttpClient::GetTypeId (),
                                HttpServer::GetTypeId ());
  m_httpHelper.SetClientAttribute ("AppName", StringValue ("Http"));
  m_httpHelper.SetClientAttribute ("MaxPages", UintegerValue (3));
  m_httpHelper.SetClientAttribute ("MaxReadingTime", TimeValue (Seconds (10)));
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
    .AddAttribute ("VoipTraffic",
                   "Enable GBR VoIP traffic over UDP.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrVoip),
                   MakeBooleanChecker ())
    .AddAttribute ("GbrLiveVideoTraffic",
                   "Enable GBR live video streaming traffic over UDP.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_gbrLVid),
                   MakeBooleanChecker ())
    .AddAttribute ("BufferedVideoTraffic",
                   "Enable Non-GBR biffered video streaming traffic over TCP.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_nonBVid),
                   MakeBooleanChecker ())
    .AddAttribute ("NonGbrLiveVideoTraffic",
                   "Enable Non-GBR live video streaming traffic over UDP.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_nonLVid),
                   MakeBooleanChecker ())
    .AddAttribute ("HttpTraffic",
                   "Enable Non-GBR HTTP traffic over TCP.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_nonHttp),
                   MakeBooleanChecker ())
    .AddAttribute ("FastTraffic",
                   "Enable short inter-arrival times.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::EnableFastTraffic),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
TrafficHelper::SetManagerAttribute (std::string name,
                                    const AttributeValue &value)
{
  m_managerFactory.Set (name, value);
}

void
TrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_lteNetwork = 0;
  m_webNode = 0;
  m_ueNode = 0;
  m_ueDev = 0;
  m_ueManager = 0;
  m_videoRng = 0;
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Install the applications.
  InstallApplications (m_lteNetwork->GetUeNodes (),
                       m_lteNetwork->GetUeDevices ());

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
TrafficHelper::InstallApplications (NodeContainer ueNodes,
                                    NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  // Install manager and applications into nodes.
  for (uint32_t u = 0; u < ueNodes.GetN (); u++)
    {
      m_ueNode = ueNodes.Get (u);
      m_ueDev = ueDevices.Get (u);
      NS_ASSERT (m_ueDev->GetNode () == m_ueNode);

      Ptr<Ipv4> clientIpv4 = m_ueNode->GetObject<Ipv4> ();
      m_ueAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      m_ueMask = clientIpv4->GetAddress (1, 0).GetMask ();

      // Each UE gets one traffic manager.
      m_ueManager = m_managerFactory.Create<TrafficManager> ();
      m_ueManager->SetImsi (DynamicCast<LteUeNetDevice> (m_ueDev)->GetImsi ());
      m_ueNode->AggregateObject (m_ueManager);

      // Connect the manager to new context created trace source.
      Config::ConnectWithoutContext (
        "/NodeList/*/ApplicationList/*/$ns3::EpcController/SessionCreated",
        MakeCallback (&TrafficManager::SessionCreatedCallback, m_ueManager));

      // Install applications into UEs
      if (m_gbrVoip)
        {
          InstallGbrVoip ();
        }
      if (m_gbrLVid)
        {
          InstallGbrLiveVideoStreaming ();
        }
      if (m_nonBVid)
        {
          InstallNonGbrBufferedVideoStreaming ();
        }
      if (m_nonLVid)
        {
          InstallNonGbrLiveVideoStreaming ();
        }
      if (m_nonHttp)
        {
          InstallNonGbrHttp ();
        }
    }
  m_ueNode = 0;
  m_ueDev = 0;
  m_ueManager = 0;
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
  static uint16_t portNo = 10000;
  portNo++;

  // Bidirectional VoIP traffic.
  Ptr<SdmnClientApp> cApp = m_voipHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, portNo);

  // TFT Packet filter.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  // Dedicated GBR EPS bearer (QCI 1).
  GbrQosInformation qos;
  qos.gbrDl = 47200;  // ~46.09 Kbps (considering tunnel overhead)
  qos.gbrUl = 47200;  // ~46.09 Kbps (considering tunnel overhead)
  EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);

  // Link EPC info to application.
  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_ueManager->AddSdmnClientApp (cApp);

  // Activate dedicated bearer.
  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallGbrLiveVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  static uint16_t portNo = 15000;
  portNo++;

  // Downlink real-time video traffic.
  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_rtVideoHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  Ptr<SdmnClientApp> cApp = m_rtVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, portNo);

  // TFT downlink packet filter.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::DOWNLINK;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  // Dedicated GBR EPS bearer (QCI 2).
  GbrQosInformation qos;
  qos.gbrDl = GetVideoGbr (videoIdx).GetBitRate ();
  qos.mbrDl = GetVideoMbr (videoIdx).GetBitRate ();
  EpsBearer bearer (EpsBearer::GBR_CONV_VIDEO, qos);

  // Link EPC info to application.
  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_ueManager->AddSdmnClientApp (cApp);

  // Activate dedicated bearer.
  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrBufferedVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  static uint16_t portNo = 20000;
  portNo++;

  // Downlink stored video traffic (with TCP bidirectional traffic filter).
  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_stVideoHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  Ptr<SdmnClientApp> cApp = m_stVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, portNo);

  // TFT Packet filter.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = TcpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  // Dedicated Non-GBR EPS bearer (QCI 6).
  GbrQosInformation qos;
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_OPERATOR, qos);

  // Link EPC info to application.
  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_ueManager->AddSdmnClientApp (cApp);

  // Activate dedicated bearer.
  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrLiveVideoStreaming ()
{
  NS_LOG_FUNCTION (this);
  static uint16_t portNo = 25000;
  portNo++;

  // Downlink real-time video traffic.
  int videoIdx = m_videoRng->GetInteger ();
  std::string filename = GetVideoFilename (videoIdx);
  m_rtVideoHelper.SetServerAttribute ("TraceFilename", StringValue (filename));

  Ptr<SdmnClientApp> cApp = m_rtVideoHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, portNo);

  // TFT downlink packet filter.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::DOWNLINK;
  filter.protocol = UdpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  // Dedicated Non-GBR EPS bearer (QCI 7).
  GbrQosInformation qos;
  EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING, qos);

  // Link EPC info to application.
  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_ueManager->AddSdmnClientApp (cApp);

  // Activate dedicated bearer.
  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::InstallNonGbrHttp ()
{
  NS_LOG_FUNCTION (this);
  static uint16_t portNo = 30000;
  portNo++;

  // Downlink HTTP web traffic (with TCP bidirectional traffic filter).
  Ptr<SdmnClientApp> cApp = m_httpHelper.Install (
      m_ueNode, m_webNode, m_ueAddr, m_webAddr, portNo);

  // TFT Packet filter.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  EpcTft::PacketFilter filter;
  filter.direction = EpcTft::BIDIRECTIONAL;
  filter.protocol = TcpL4Protocol::PROT_NUMBER;
  filter.remoteAddress = m_webAddr;
  filter.remoteMask = m_webMask;
  filter.remotePortStart = portNo;
  filter.remotePortEnd = portNo;
  filter.localAddress = m_ueAddr;
  filter.localMask = m_ueMask;
  filter.localPortStart = 0;
  filter.localPortEnd = 65535;
  tft->Add (filter);

  // Dedicated Non-GBR EPS bearer (QCI 8).
  GbrQosInformation qos;
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, qos);

  // Link EPC info to application.
  cApp->SetTft (tft);
  cApp->SetEpsBearer (bearer);
  m_ueManager->AddSdmnClientApp (cApp);

  // Activate dedicated bearer.
  GetLteHelper ()->ActivateDedicatedEpsBearer (m_ueDev, bearer, tft);
}

void
TrafficHelper::EnableFastTraffic (bool fastTraffic)
{
  NS_LOG_FUNCTION (this << fastTraffic);

  if (fastTraffic)
    {
      Ptr<RandomVariableStream> rng = CreateObject<UniformRandomVariable> ();
      rng->SetAttribute ("Min", DoubleValue (0.0));
      rng->SetAttribute ("Max", DoubleValue (12.0));
      SetManagerAttribute ("PoissonInterArrival", PointerValue (rng));
    }
}

};  // namespace ns3
