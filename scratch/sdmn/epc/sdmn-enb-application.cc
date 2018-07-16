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

#include "sdmn-enb-application.h"
#include "epc-gtpu-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdmnEnbApplication");

TypeId
SdmnEnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdmnEnbApplication")
    .SetParent<EpcEnbApplication> ()
    .AddTraceSource ("S1uRx",
                     "Trace source indicating a packet received from S1-U interface.",
                     MakeTraceSourceAccessor (&SdmnEnbApplication::m_rxS1uTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S1uTx",
                     "Trace source indicating a packet transmitted over the S1-U interface.",
                     MakeTraceSourceAccessor (&SdmnEnbApplication::m_txS1uTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

SdmnEnbApplication::SdmnEnbApplication (
  Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6, Ptr<Socket> s1uSocket,
  Ipv4Address enbS1uAddress, Ipv4Address sgwS1uAddress, uint16_t cellId)
  : EpcEnbApplication (lteSocket, lteSocket6, s1uSocket, enbS1uAddress,
                       sgwS1uAddress, cellId)
{
  NS_LOG_FUNCTION (this << lteSocket << lteSocket6 << s1uSocket <<
                   enbS1uAddress << sgwS1uAddress << cellId);
}

SdmnEnbApplication::~SdmnEnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}

void
SdmnEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT (socket == m_s1uSocket);
  Ptr<Packet> packet = socket->Recv ();

  m_rxS1uTrace (packet);
  EpcGtpuTag teidTag;
  packet->RemovePacketTag (teidTag);

  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();
  std::map<uint32_t, EpsFlowId_t>::iterator it = m_teidRbidMap.find (teid);
  NS_ASSERT (it != m_teidRbidMap.end ());

  m_rxS1uSocketPktTrace (packet->Copy ());
  SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
}

void
SdmnEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  EpcEnbApplication::DoDispose ();
}

void
SdmnEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);
  uint32_t flags = 0;

  EpcGtpuTag teidTag (teid, EpcGtpuTag::ENB);
  packet->AddPacketTag (teidTag);
  m_txS1uTrace (packet);

  m_s1uSocket->SendTo (packet, flags, InetSocketAddress (m_sgwS1uAddress, m_gtpuUdpPort));
}

}  // namespace ns3
