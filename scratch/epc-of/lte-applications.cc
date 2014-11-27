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

/* Application port ranges */
uint16_t g_tcpHttpPort = 80;
uint16_t g_udpVoipPort = 16000;
uint16_t g_udpVideoPort = 20000;
uint16_t g_tcpUdpDualUePort = 50000;
uint16_t g_tcpUdpDualServerPort = 60000;

std::string g_videoTrace = "ns3/movies/jurassic-low.data";

  
void 
SetPingTraffic (Ptr<Node> dstNode, NodeContainer clients, 
    Ptr<UniformRandomVariable> rngStart)
{
  Ptr<Ipv4> dstIpv4 = dstNode->GetObject<Ipv4> ();
  Ipv4Address dstAddr = dstIpv4->GetAddress (1,0).GetLocal ();
  V4PingHelper ping = V4PingHelper (dstAddr);
  ApplicationContainer clientApps = ping.Install (clients);
  clientApps.Start (Seconds (rngStart->GetValue ()));
}


void
SetHttpTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper, 
    Ptr<UniformRandomVariable> rngStart)
{
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  for (uint32_t u = 0; u < clients.GetN (); u++, g_tcpHttpPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();

      // HTTP server
      HttpServerHelper httpServer (g_tcpHttpPort);
      ApplicationContainer serverApps = httpServer.Install (server);
      serverApps.Start (Seconds (0.1));

      // HTTP client
      HttpClientHelper httpClient (serverAddr, g_tcpHttpPort);
      ApplicationContainer clientApps = httpClient.Install (client);
      clientApps.Start (Seconds (rngStart->GetValue ()));

      // Traffic Flow Templat
      Ptr<EpcTft> tft = Create<EpcTft> ();
      EpcTft::PacketFilter filter;
      filter.remoteAddress = serverAddr;
      filter.remoteMask = serverMask;
      filter.localAddress = clientAddr;
      filter.localMask = clientMask;
      filter.remotePortStart = g_tcpHttpPort;
      filter.remotePortEnd = g_tcpHttpPort;
      tft->Add (filter);

      // Dedicated Non-GBR EPS bearer (QCI 8)
      GbrQosInformation qos;
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }
}


void
SetVoipTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper, 
    Ptr<UniformRandomVariable> rngStart)
{
  uint16_t voipPacketSize = 60;
  double   voipPacketInterval = 0.06;
 
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  ApplicationContainer sinkApps;
  for (uint32_t u = 0; u < clients.GetN (); u++, g_udpVoipPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();

      ApplicationContainer senderApps;

      // Downlink VoIP traffic
      UdpServerHelper voipSinkDown (g_udpVoipPort);
      sinkApps.Add (voipSinkDown.Install (client));
      VoipClientHelper voipSenderDown (clientAddr, g_udpVoipPort);
      voipSenderDown.SetAttribute ("Stream", IntegerValue (u));
      senderApps.Add (voipSenderDown.Install (server));

      EpcTft::PacketFilter filterDown;
      filterDown.direction = EpcTft::DOWNLINK;
      filterDown.remoteAddress = serverAddr;
      filterDown.remoteMask = serverMask;
      filterDown.localAddress = clientAddr;
      filterDown.localMask = clientMask;
      filterDown.localPortStart = g_udpVoipPort;
      filterDown.localPortEnd = g_udpVoipPort;

      // Uplink VoIP traffic
      UdpServerHelper voipSinkUp (g_udpVoipPort);
      sinkApps.Add (voipSinkUp.Install (server));
      VoipClientHelper voipSenderUp (serverAddr, g_udpVoipPort);
      voipSenderUp.SetAttribute ("Stream", IntegerValue (u));
      senderApps.Add (voipSenderUp.Install (client));

      EpcTft::PacketFilter filterUp;
      filterUp.direction = EpcTft::UPLINK;
      filterUp.remoteAddress = serverAddr;
      filterUp.remoteMask = serverMask;
      filterUp.localAddress = clientAddr;
      filterUp.localMask = clientMask;
      filterUp.remotePortStart = g_udpVoipPort;
      filterUp.remotePortEnd = g_udpVoipPort;

      // Traffic Flow Template
      Ptr<EpcTft> tft = Create<EpcTft> ();
      tft->Add (filterDown);
      tft->Add (filterUp);
 
      // Dedicated GBR EPS bearer (QCI 1)
      GbrQosInformation qos;
      qos.gbrDl = (voipPacketSize + 4) * 8 / voipPacketInterval;
      qos.mbrDl = qos.gbrDl;
      EpsBearer bearer (EpsBearer::GBR_CONV_VOICE, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
      
      senderApps.Start (Seconds (1));
    }
    sinkApps.Start (Seconds (0));
}

 
void
SetVideoTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper, 
    Ptr<UniformRandomVariable> rngStart)
{
  // Max and Average bitrate info extracted from
  // http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf
  uint64_t avgBitRate = 150000;
  uint64_t maxBitRate = 1600000;

  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  for (uint32_t u = 0; u < clients.GetN (); u++, g_udpVideoPort++)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();
 
      // Video server (send UDP datagrams to client)
      uint32_t MaxPacketSize = 1472;  // Back off 20 (IP) + 8 (UDP) bytes from MTU
      UdpTraceClientHelper traceClient (clientAddr, g_udpVideoPort, g_videoTrace);
      traceClient.SetAttribute ("MaxPacketSize", UintegerValue (MaxPacketSize));
      ApplicationContainer serverApps = traceClient.Install (server);
      serverApps.Start (Seconds (rngStart->GetValue ()));
      
      // Video client (receive UDP datagramas from server)
      UdpServerHelper udpHelper (g_udpVideoPort);
      ApplicationContainer clientApps = udpHelper.Install (client); 
      clientApps.Start (Seconds (rngStart->GetValue ()));
      clientApps.Start (Seconds (0.1)); // FIXME????
     
      // Traffic Flow Template
      Ptr<EpcTft> tft = Create<EpcTft> ();
      EpcTft::PacketFilter filter;
      filter.direction = EpcTft::DOWNLINK;
      filter.remoteAddress = serverAddr;
      filter.remoteMask = serverMask;
      filter.localAddress = clientAddr;
      filter.localMask = clientMask;
      filter.localPortStart = g_udpVideoPort;
      filter.localPortEnd = g_udpVideoPort;
      tft->Add (filter);
 
      // Dedicated GBR EPS bearer (QCI 4).
      GbrQosInformation qos;
      qos.gbrDl = avgBitRate;
      qos.mbrDl = maxBitRate;
      EpsBearer bearer (EpsBearer::GBR_NON_CONV_VIDEO, qos);
      lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
    }
}


void
SetLenaDualStripeTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper, 
    Ptr<UniformRandomVariable> rngStart, uint32_t nBearers, bool useUdp, 
    bool uplink, bool downlink) 
{
  Ptr<Ipv4> serverIpv4 = server->GetObject<Ipv4> ();
  Ipv4Address serverAddr = serverIpv4->GetAddress (1,0).GetLocal ();
  Ipv4Mask serverMask = serverIpv4->GetAddress (1,0).GetMask ();

  for (uint32_t u = 0; u < clients.GetN (); ++u)
    {
      Ptr<Node> client = clients.Get (u);
      Ptr<NetDevice> clientDev = clientsDevs.Get (u);
      NS_ASSERT (clientDev->GetNode () == client);

      Ptr<Ipv4> clientIpv4 = client->GetObject<Ipv4> ();
      Ipv4Address clientAddr = clientIpv4->GetAddress (1, 0).GetLocal ();
      Ipv4Mask clientMask = clientIpv4->GetAddress (1, 0).GetMask ();

      for (uint32_t b = 0; b < nBearers; ++b)
        {
          ++g_tcpUdpDualUePort;
          ++g_tcpUdpDualServerPort;

          ApplicationContainer senderApps;
          ApplicationContainer sinkApps;

          // UDP traffic
          if (useUdp)
            {
              if (downlink)
                {
                  UdpClientHelper senderHelper (clientAddr, g_tcpUdpDualUePort);
                  senderApps.Add (senderHelper.Install (server));

                  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", 
                      InetSocketAddress (Ipv4Address::GetAny (), g_tcpUdpDualUePort));
                  sinkApps.Add (sinkHelper.Install (client));
                }
              if (uplink)
                {
                  UdpClientHelper senderHelper (serverAddr, g_tcpUdpDualServerPort);
                  senderApps.Add (senderHelper.Install (client));

                  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", 
                      InetSocketAddress (Ipv4Address::GetAny (), g_tcpUdpDualServerPort));
                  sinkApps.Add (sinkHelper.Install (server));
                }
            }

          // TCP traffic
          else
            {
              if (downlink)
                {
                  BulkSendHelper senderHelper ("ns3::TcpSocketFactory", 
                      InetSocketAddress (clientAddr, g_tcpUdpDualUePort));
                  senderApps.Add (senderHelper.Install (server));
                
                  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", 
                      InetSocketAddress (Ipv4Address::GetAny (), g_tcpUdpDualUePort));
                  sinkApps.Add (sinkHelper.Install (client));
                }
              if (uplink)
                {
                  BulkSendHelper senderHelper ("ns3::TcpSocketFactory", 
                      InetSocketAddress (serverAddr, g_tcpUdpDualServerPort));
                  senderApps.Add (senderHelper.Install (client));
                
                  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", 
                      InetSocketAddress (Ipv4Address::GetAny (), g_tcpUdpDualServerPort));
                  sinkApps.Add (sinkHelper.Install (server));
                }
            }
  
          // Traffic Flow Templates
          Ptr<EpcTft> tft = Create<EpcTft> ();
          uint8_t precedence = 255;
          if (downlink)
            {
              EpcTft::PacketFilter dlpf;
              dlpf.precedence = precedence--;
              dlpf.direction = EpcTft::BIDIRECTIONAL;
              dlpf.remoteAddress = serverAddr;
              dlpf.remoteMask = serverMask;
              dlpf.localAddress = clientAddr;
              dlpf.localMask = clientMask;
              dlpf.localPortStart = g_tcpUdpDualUePort;
              dlpf.localPortEnd = g_tcpUdpDualUePort;
              tft->Add (dlpf); 
            }
          if (uplink)
            {
              EpcTft::PacketFilter ulpf;
              ulpf.precedence = precedence--;
              ulpf.direction = EpcTft::BIDIRECTIONAL;
              ulpf.remoteAddress = serverAddr;
              ulpf.remoteMask = serverMask;
              ulpf.localAddress = clientAddr;
              ulpf.localMask = clientMask;
              ulpf.remotePortStart = g_tcpUdpDualServerPort;
              ulpf.remotePortEnd = g_tcpUdpDualServerPort;
              tft->Add (ulpf);
            }
 
          // Dedicated Non-GBR EPS beareres (QCI 8)
          if (downlink || uplink)
            {
              // These 1024 values only make sense for UDP flows, as TCP sends
              // data as fast as possible
              GbrQosInformation bearerInfo;
              bearerInfo.gbrDl = 1024;
              bearerInfo.gbrUl = 1024;
              EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_PREMIUM);
              lteHelper->ActivateDedicatedEpsBearer (clientDev, bearer, tft);
            }

          sinkApps.Start (Seconds (0));
          senderApps.Start (Seconds (rngStart->GetValue ()));
        } // end for bearer b
    } // end for user u
}

