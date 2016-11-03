/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#include <ns3/lte-module.h>
#include "pgw-logical-port.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PgwS5Handler);
NS_LOG_COMPONENT_DEFINE ("PgwS5Handler");

PgwS5Handler::PgwS5Handler ()
{
  NS_LOG_FUNCTION (this);
}

PgwS5Handler::~PgwS5Handler ()
{
  NS_LOG_FUNCTION (this);
}

PgwS5Handler::PgwS5Handler (Ipv4Address s5Address, Mac48Address webMacAddr)
  : m_pgwS5Address (s5Address),
    m_webMacAddress (webMacAddr)
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwS5Handler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwS5Handler")
    .SetParent<Object> ()
    .AddConstructor<PgwS5Handler> ()
    .AddTraceSource ("S5Rx",
                     "Trace source for packets received from S5 interface.",
                     MakeTraceSourceAccessor (&PgwS5Handler::m_rxS5Trace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S5Tx",
                     "Trace source for packets sent to the S5 interface.",
                     MakeTraceSourceAccessor (&PgwS5Handler::m_txS5Trace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
PgwS5Handler::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Object::DoDispose ();
}

uint64_t
PgwS5Handler::Receive (uint64_t dpId, uint32_t portNo, Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << dpId << portNo << packet);

  uint32_t teid = 0;

  // Remove the existing Ethernet header and trailer
  EthernetTrailer ethTrailer;
  packet->RemoveTrailer (ethTrailer);
  EthernetHeader ethHeader;
  packet->RemoveHeader (ethHeader);

  // We expect to receive only IP packets
  if (ethHeader.GetLengthType () == Ipv4L3Protocol::PROT_NUMBER)
    {
      // Remove IP header
      Ipv4Header ipHeader;
      ipHeader.EnableChecksum ();
      packet->RemoveHeader (ipHeader);

      // Trim any residual frame padding from underlying devices
      if (ipHeader.GetPayloadSize () < packet->GetSize ())
        {
          packet->RemoveAtEnd (packet->GetSize () - ipHeader.GetPayloadSize ());
        }

      NS_ASSERT_MSG (ipHeader.IsChecksumOk (), "Invalid IP checksum.");
      NS_ASSERT_MSG (ipHeader.GetDestination () == m_pgwS5Address,
                     "This packet is not addressed to this gateway.");

      // Remove UDP header
      UdpHeader udpHeader;
      udpHeader.EnableChecksums ();
      udpHeader.InitializeChecksum (ipHeader.GetSource (),
        ipHeader.GetDestination (), UdpL4Protocol::PROT_NUMBER);
      packet->RemoveHeader (udpHeader);

      NS_ASSERT_MSG (udpHeader.IsChecksumOk (), "Invalid UDP checksum.");
      NS_ASSERT_MSG (udpHeader.GetDestinationPort () == 2152,
                     "Invalid UDP port for GTP tunnel.");

      // Fire the S5Rx trace source (packet leaving the EPC)
      m_rxS5Trace (packet);
      EpcGtpuTag teidTag;
      packet->RemovePacketTag (teidTag);

      // Remove GTP-U header and set the return value
      GtpuHeader gtpuHeader;
      packet->RemoveHeader (gtpuHeader);
      teid = gtpuHeader.GetTeid ();

      NS_ASSERT_MSG (teid == teidTag.GetTeid (), "Invalid GTP TEID value.");
    }

  // Packets received from the S5 interface will be forward to the SGi
  // interface. In this case we need to update the dst MAC address to match the
  // Internet Web server.
  ethHeader.SetDestination (m_webMacAddress);
  
  // Add Ethernet header and trailer back
  packet->AddHeader (ethHeader);
  packet->AddTrailer (ethTrailer);

  return (uint64_t)teid;
}

void
PgwS5Handler::Send (uint64_t dpId, uint32_t portNo, Ptr<Packet> packet,
                        uint64_t tunnelId)
{
  NS_LOG_FUNCTION (this << dpId << portNo << packet << tunnelId);

  // Remove the existing Ethernet header and trailer
  EthernetTrailer ethTrailer;
  packet->RemoveTrailer (ethTrailer);
  EthernetHeader ethHeader;
  packet->RemoveHeader (ethHeader);

  // Get the UE address from packet header
  // FIXME Remover isso aqui no futuro
  Ipv4Header innerIpHeader;
  packet->PeekHeader (innerIpHeader);
  Ipv4Address ueAddr = innerIpHeader.GetDestination ();
  Ipv4Address dstAddr = m_controlPlane->GetEnbAddr (ueAddr);
  tunnelId = m_controlPlane->GetTeid (ueAddr, packet);

  // We expect to send only IP packets
  if (ethHeader.GetLengthType () == Ipv4L3Protocol::PROT_NUMBER)
    {
      // Add GTP-U header using parameter tunnelId
      GtpuHeader gtpuHeader;
      gtpuHeader.SetTeid (tunnelId);
      gtpuHeader.SetLength (packet->GetSize () +
                            gtpuHeader.GetSerializedSize () - 8);
      packet->AddHeader (gtpuHeader);

      // Fire the S5Tx trace source (packet entering the EPC)
      EpcGtpuTag teidTag ((uint32_t)tunnelId, EpcGtpuTag::PGW);
      packet->AddPacketTag (teidTag);
      m_txS5Trace (packet);

      // Add UDP header
      UdpHeader udpHeader;
      udpHeader.EnableChecksums ();
      udpHeader.InitializeChecksum (m_pgwS5Address, dstAddr,
                                    UdpL4Protocol::PROT_NUMBER);
      udpHeader.SetDestinationPort (2152);
      udpHeader.SetSourcePort (2152);
      packet->AddHeader (udpHeader);

      // Add IP header
      Ipv4Header ipHeader;
      ipHeader.SetSource (m_pgwS5Address);
      ipHeader.SetDestination (dstAddr);
      ipHeader.SetProtocol (UdpL4Protocol::PROT_NUMBER);
      ipHeader.SetPayloadSize (packet->GetSize ());
      ipHeader.SetTtl (64);
      ipHeader.SetTos (0);
      ipHeader.SetDontFragment ();
      ipHeader.SetIdentification (0);
      ipHeader.EnableChecksum ();
      packet->AddHeader (ipHeader);
    }

  // FIXME. Supostamente isso nao deveria estar aqui. Vamos tentar mover essas modificações pras regras a serem instaladas no switch.
  ethHeader.SetDestination (Mac48Address::GetBroadcast ());

  // Add Ethernet header and trailer back
  packet->AddHeader (ethHeader);
  packet->AddTrailer (ethTrailer);
}

}; // namespace ns3
