/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#include <ns3/epc-gtpu-header.h>
#include "gtpu-tag.h"
#include "pgwu-tunnel-app.h"
#include "../metadata/ue-info.h"
#include "../metadata/bearer-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwuTunnelApp");
NS_OBJECT_ENSURE_REGISTERED (PgwuTunnelApp);

PgwuTunnelApp::PgwuTunnelApp (Ptr<VirtualNetDevice> logicalPort,
                              Ptr<CsmaNetDevice> physicalDev)
  : GtpuTunnelApp (logicalPort, physicalDev)
{
  NS_LOG_FUNCTION (this << logicalPort << physicalDev);

  // Set callbacks from parent class.
  m_txSocket = MakeCallback (&PgwuTunnelApp::AttachEpcGtpuTag, this);
  m_rxSocket = MakeCallback (&PgwuTunnelApp::RemoveEpcGtpuTag, this);
}

PgwuTunnelApp::~PgwuTunnelApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwuTunnelApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwuTunnelApp")
    .SetParent<GtpuTunnelApp> ()
    .AddTraceSource ("S5Rx",
                     "Trace source for packets received from S5 interface.",
                     MakeTraceSourceAccessor (&PgwuTunnelApp::m_rxS5Trace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S5Tx",
                     "Trace source for packets sent to the S5 interface.",
                     MakeTraceSourceAccessor (&PgwuTunnelApp::m_txS5Trace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
PgwuTunnelApp::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  GtpuTunnelApp::DoDispose ();
}

void
PgwuTunnelApp::AttachEpcGtpuTag (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);

  // Ignoring TEID parameter and classify the packet again. This is useful when
  // aggregating different bearers withing the same tunnel. Using this
  // independent classifier ensures that the EPC packet tags can continue to
  // differentiate the bearers withing the EPC.
  Ptr<Packet> packetCopy = packet->Copy ();

  GtpuHeader gtpuHeader;
  Ipv4Header ipv4Header;
  packetCopy->RemoveHeader (gtpuHeader);
  packetCopy->PeekHeader (ipv4Header);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (ipv4Header.GetDestination ());
  NS_ASSERT_MSG (ueInfo, "No UE info for this IP address.");
  teid = ueInfo->Classify (packetCopy);

  // Packet entering the EPC. Attach the tag and fire the S5 TX trace source.
  Ptr<BearerInfo> bInfo = BearerInfo::GetPointer (teid);
  GtpuTag gtpuTag (
    teid, GtpuTag::PGW, bInfo->GetQosType (), bInfo->IsAggregated ());
  packet->AddPacketTag (gtpuTag);
  m_txS5Trace (packet);
}

void
PgwuTunnelApp::RemoveEpcGtpuTag (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);
  // Packet leaving the EPC.

  // Fire the RX trace source and remove the tag.
  m_rxS5Trace (packet);
  GtpuTag gtpuTag;
  packet->RemovePacketTag (gtpuTag);
}

} // namespace ns3
