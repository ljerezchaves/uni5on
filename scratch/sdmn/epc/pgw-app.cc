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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <ns3/epc-gtpu-tag.h>
#include "pgw-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwApp");
NS_OBJECT_ENSURE_REGISTERED (PgwApp);

PgwApp::PgwApp (Ptr<VirtualNetDevice> logicalPort,
                Ptr<CsmaNetDevice> physicalDev)
  : GtpTunnelApp (logicalPort, physicalDev)
{
  NS_LOG_FUNCTION (this << logicalPort << physicalDev);

  // Set callbacks from parent class.
  m_txSocket = MakeCallback (&PgwApp::AttachEpcGtpuTag, this);
  m_rxSocket = MakeCallback (&PgwApp::RemoveEpcGtpuTag, this);
}

PgwApp::~PgwApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwApp")
    .SetParent<GtpTunnelApp> ()
    .AddTraceSource ("S5Rx",
                     "Trace source for packets received from S5 interface.",
                     MakeTraceSourceAccessor (&PgwApp::m_rxS5Trace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S5Tx",
                     "Trace source for packets sent to the S5 interface.",
                     MakeTraceSourceAccessor (&PgwApp::m_txS5Trace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
PgwApp::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  GtpTunnelApp::DoDispose ();
}

void
PgwApp::AttachEpcGtpuTag (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);

  // Packet entering the EPC: attach the tag and fire the TX trace source.
  EpcGtpuTag teidTag (teid, EpcGtpuTag::PGW);
  packet->AddPacketTag (teidTag);
  m_txS5Trace (packet);
}

void
PgwApp::RemoveEpcGtpuTag (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);

  // Packet leaving the EPC: fire the RX trace source and remove the tag.
  m_rxS5Trace (packet);
  EpcGtpuTag teidTag;
  packet->RemovePacketTag (teidTag);
}


} // namespace ns3
