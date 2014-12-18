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

#include "lte-applications.h"

void 
SetPingTraffic (Ptr<Node> dstNode, NodeContainer clients)
{
  Ptr<UniformRandomVariable> g_rngStart;
  g_rngStart = CreateObject<UniformRandomVariable> ();
  
  Ptr<Ipv4> dstIpv4 = dstNode->GetObject<Ipv4> ();
  Ipv4Address dstAddr = dstIpv4->GetAddress (1,0).GetLocal ();
  V4PingHelper ping = V4PingHelper (dstAddr);
  ApplicationContainer clientApps = ping.Install (clients);
  clientApps.Start (Seconds (g_rngStart->GetValue (0.1, 1.0)));
}


void
SetHttpTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper,
    Ptr<EpcSdnController> controller)
{
  static uint16_t httpPort = 80;

  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer serverApps, clientApps;
  for (uint32_t u = 0; u < clients.GetN (); u++, httpPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();

      // HTTP server
      HttpServerHelper httpServer (httpPort);
      Ptr<Application> httpServerApp = httpServer.Install (server);
      serverApps.Add (httpServerApp);
      httpServerApp->AggregateObject (tft);
      httpServerApp->SetAttribute ("Direction", 
                                   EnumValue (Application::BIDIRECTIONAL));

      // HTTP client
      HttpClientHelper httpClient (serverAddr, httpPort);
      Ptr<Application> httpClientApp = httpClient.Install (client);
      clientApps.Add (httpClientApp);
      httpClientApp->AggregateObject (tft);
      httpServerApp->SetAttribute ("Direction", 
                                   EnumValue (Application::BIDIRECTIONAL));

      // TFT Packet filter
      EpcTft::PacketFilter filter;
      filter.remoteAddress = serverAddr;
      filter.remoteMask = serverMask;
      filter.localAddress = clientAddr;
      filter.localMask = clientMask;
      filter.remotePortStart = httpPort;
      filter.remotePortEnd = httpPort;
      tft->Add (filter);

      // Dedicated Non-GBR EPS bearer (QCI 8)
      GbrQosInformation qos;
      //qos.mbrDl = qos.mbrUl = ; 
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }
  clientApps.Start (Seconds (1));
  serverApps.Start (Seconds (0));

  // Setting up app start callback to controller
  for (ApplicationContainer::Iterator i = clientApps.Begin (); 
       i != clientApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&EpcSdnController::NotifyAppStart, controller),
          MakeCallback (&EpcSdnController::NotifyAppStop, controller));
    }

}


ApplicationContainer
SetVoipTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper,
    Ptr<EpcSdnController> controller)
{
  static uint16_t voipPort = 16000;
  uint16_t voipPacketSize = 60;
  double   voipPacketInterval = 0.06;
 
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer sinkApps, senderApps;
  for (uint32_t u = 0; u < clients.GetN (); u++, voipPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();

      // Downlink VoIP traffic
      UdpServerHelper voipSinkDown (voipPort);
      sinkApps.Add (voipSinkDown.Install (client));
      VoipClientHelper voipSenderDown (clientAddr, voipPort);
      voipSenderDown.SetAttribute ("Stream", IntegerValue (u));
      Ptr<Application> voipSenderDownApp = voipSenderDown.Install (server);
      senderApps.Add (voipSenderDownApp);
      voipSenderDownApp->AggregateObject (tft);
      voipSenderDownApp->SetAttribute ("Direction", 
                                       EnumValue (Application::BIDIRECTIONAL));

      // TFT Packet filter
      EpcTft::PacketFilter filterDown;
      filterDown.direction = EpcTft::DOWNLINK;
      filterDown.remoteAddress = serverAddr;
      filterDown.remoteMask = serverMask;
      filterDown.localAddress = clientAddr;
      filterDown.localMask = clientMask;
      filterDown.localPortStart = voipPort;
      filterDown.localPortEnd = voipPort;
      tft->Add (filterDown);

      // Uplink VoIP traffic
      UdpServerHelper voipSinkUp (voipPort);
      sinkApps.Add (voipSinkUp.Install (server));
      VoipClientHelper voipSenderUp (serverAddr, voipPort);
      voipSenderUp.SetAttribute ("Stream", IntegerValue (u));
      Ptr<Application> voipSenderUpApp = voipSenderUp.Install (client);
      senderApps.Add (voipSenderUpApp);
      voipSenderUpApp->AggregateObject (tft);
      voipSenderUpApp->SetAttribute ("Direction", 
                                     EnumValue (Application::BIDIRECTIONAL));

      // TFT Packet filter
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
      qos.gbrDl = (voipPacketSize + 4) * 8 / voipPacketInterval;
      qos.mbrDl = qos.gbrUl = qos.mbrUl = qos.gbrDl;
      EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }
  senderApps.Start (Seconds (1));
  sinkApps.Start (Seconds (0));

  // Setting up app start callback to controller
  for (ApplicationContainer::Iterator i = senderApps.Begin (); 
       i != senderApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&EpcSdnController::NotifyAppStart, controller),
          MakeCallback (&EpcSdnController::NotifyAppStop, controller));
    }
  return sinkApps;
}


// MPEG4 trace files.
// http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf
static const std::string g_videoTrace [] = {"ns3/movies/jurassic.data",
  "ns3/movies/silence.data", "ns3/movies/star-wars.data",
  "ns3/movies/mr-bean.data", "ns3/movies/first-contact.data",
  "ns3/movies/from-dusk.data", "ns3/movies/the-firm.data",
  "ns3/movies/formula1.data", "ns3/movies/soccer.data",
  "ns3/movies/ard-news.data", "ns3/movies/ard-talk.data",
  "ns3/movies/ns3-talk.data", "ns3/movies/office-cam.data"};

static const uint64_t g_avgBitRate [] = {770000, 580000, 280000, 580000, 
  330000, 680000, 310000, 840000, 1100000, 720000, 540000, 550000, 400000};

static const uint64_t g_maxBitRate [] = {3300000, 4400000, 1900000, 3100000, 
  2500000, 3100000, 2100000, 2900000, 3600000, 3400000, 3100000, 3400000, 
  2000000};
 
ApplicationContainer
SetVideoTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper,
    Ptr<EpcSdnController> controller)
{
  static uint16_t videoPort = 20000;

  Ptr<UniformRandomVariable> rngVideo = CreateObject<UniformRandomVariable> ();
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer sinkApps, senderApps;
  for (uint32_t u = 0; u < clients.GetN (); u++, videoPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
      
      // Traffic Flow Template
      Ptr<EpcTft> tft = CreateObject<EpcTft> ();
 
      // Video server (send UDP datagrams to client)
      // Back off 20 (IP) + 8 (UDP) bytes from MTU
      int videoIdx = rngVideo->GetInteger (0, 12);
      OnOffUdpTraceClientHelper videoSender (clientAddr, videoPort, 
                                             g_videoTrace [videoIdx]);
      Ptr<Application> videoSenderApp = videoSender.Install (server);
      senderApps.Add (videoSenderApp);
      videoSenderApp->AggregateObject (tft);
      videoSenderApp->SetAttribute ("Direction", 
                                    EnumValue (Application::DOWNLINK));
      
      // Video sink (receive UDP datagramas from server)
      UdpServerHelper videoSink (videoPort);
      sinkApps.Add (videoSink.Install (client)); 
     
      // TFT Packet filter
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
      qos.gbrDl = g_avgBitRate [videoIdx];
      qos.mbrDl = g_maxBitRate [videoIdx];
      EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }
  senderApps.Start (Seconds (1));
  sinkApps.Start (Seconds (0));

  // Setting up app start callback to controller
  for (ApplicationContainer::Iterator i = senderApps.Begin (); 
       i != senderApps.End (); ++i)
    {
      Ptr<Application> app = *i;
      app->SetAppStartStopCallback (
          MakeCallback (&EpcSdnController::NotifyAppStart, controller),
          MakeCallback (&EpcSdnController::NotifyAppStop, controller));
    }
  return sinkApps;
}

