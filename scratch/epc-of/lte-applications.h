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

#ifndef LTE_APPLICATIONS_H
#define LTE_APPLICATIONS_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include <ns3/applications-module.h>
#include "epc-sdn-controller.h"

using namespace ns3;

/** IPv4 ICMP ping application over default EPS bearer (QCI 9).
 * This QCI is typically used for the default bearer of a UE/PDN for non
 * privileged subscribers. 
 */
void
SetPingTraffic (Ptr<Node> dstNode, NodeContainer clients);


/* HTTP/TCP download traffic over dedicated Non-GBR EPS bearer (QCI 8). 
 * This QCI 8 could be used for a dedicated 'premium bearer' for any
 * subscriber, or could be used for the default bearer of a for 'premium
 * subscribers'.
 *
 * This HTTP model is based on the distributions indicated in the paper 'An
 * HTTP Web Traffic Model Based on the Top One Million Visited Web Pages' by
 * Rastin Pries et. al. Each client will send a get request to the server at
 * port 80 and will get the page content back including inline content. These
 * requests repeats over time following appropriate distribution.
 */
void 
SetHttpTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper, 
    Ptr<EpcSdnController> controller);


/* VoIP/UDP bidiretional traffic over dedicated GBR EPS bearer (QCI 1). 
 * This QCI is typically associated with an operator controlled service.
 *
 * This VoIP traffic simulates the G.729 codec (~8.5 kbps for payload). Check
 * http://goo.gl/iChPGQ for bandwidth calculation and discussion. This code
 * will install a bidirectional voip traffic between UE and WebServer (in fact,
 * in install a VoipClient and an UdpServer application at each node). The
 * calls start and stop link in a Poisson procees.
 */
ApplicationContainer
SetVoipTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper,
    Ptr<EpcSdnController> controller);


/* UDP downlink video streaming over dedicated GBR EPS bearer (QCI 4).
 * This QCI is typically associated with an operator controlled service.
 *
 * This video traffic is based on a MPEG-4 video traces from the Jurassic Park
 * movie, with low quality (~150 kbps). See
 * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf for detailed
 * information. This code install an UdpTraceClient at the web server and an
 * UdpServer application at each client).
 */
ApplicationContainer
SetVideoTraffic (Ptr<Node> server, NodeContainer clients, 
    NetDeviceContainer clientsDevs, Ptr<LteHelper> lteHelper,
    Ptr<EpcSdnController> controller);

#endif // LTE_APPLICATIONS_H
