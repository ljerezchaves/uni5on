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
#include <ns3/ofswitch13-module.h>
#include "pgw-user-app.h"
#include "epc-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwUserApp");
NS_OBJECT_ENSURE_REGISTERED (PgwUserApp);

PgwUserApp::PgwUserApp ()
{
}

PgwUserApp::~PgwUserApp ()
{
}

PgwUserApp::PgwUserApp (Ptr<VirtualNetDevice> pgwS5PortDevice,
                        Ipv4Address webSgiIpAddr, Mac48Address webSgiMacAddr,
                        Mac48Address pgwSgiMacAddr)
  : m_s5PortDevice (pgwS5PortDevice),
    m_webIpAddr (webSgiIpAddr),
    m_webMacAddr (webSgiMacAddr),
    m_pgwMacAddr (pgwSgiMacAddr)
{
  m_s5PortDevice->SetSendCallback (
    MakeCallback (&PgwUserApp::RecvFromLogicalPort, this));
}

TypeId
PgwUserApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwUserApp")
    .SetParent<Application> ()
    .AddTraceSource ("S5Rx",
                     "Trace source for packets received from S5 interface.",
                     MakeTraceSourceAccessor (&PgwUserApp::m_rxS5Trace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S5Tx",
                     "Trace source for packets sent to the S5 interface.",
                     MakeTraceSourceAccessor (&PgwUserApp::m_txS5Trace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
PgwUserApp::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_tunnelSocket = 0;
  m_s5PortDevice = 0;
}

void
PgwUserApp::StartApplication ()
{
  NS_LOG_FUNCTION (this << "Starting PgwUserApp");

  // Create and open the UDP socket for S5 tunnel
  m_tunnelSocket = Socket::CreateSocket (
      GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_tunnelSocket->Bind (
    InetSocketAddress (Ipv4Address::GetAny (), EpcNetwork::m_gtpuPort));
  m_tunnelSocket->SetRecvCallback (
    MakeCallback (&PgwUserApp::RecvFromTunnelSocket, this));
}

bool
PgwUserApp::RecvFromLogicalPort (Ptr<Packet> packet, const Address& source,
                                 const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  // Retrieve the GTP TEID from TunnelId tag, if available.
  TunnelIdTag tunnelIdTag;
  bool foud = packet->RemovePacketTag (tunnelIdTag);
  NS_ASSERT_MSG (foud, "Expected TunnelId tag but not found.");

  // We expect that the S-GW address will be available in the 32 msb of
  // tunnelId, while the TEID will be available in the 32 lsb of tunnelId.
  uint64_t tagValue = tunnelIdTag.GetTunnelId ();
  uint32_t teid = tagValue;
  Ipv4Address enbAddr (tagValue >> 32);

  // Add the GTP header
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);

  // Packet entering the EPC (fire the Tx trace source)
  EpcGtpuTag teidTag (teid, EpcGtpuTag::PGW);
  packet->AddPacketTag (teidTag);
  m_txS5Trace (packet);

  // Send the packet to the tunnel socket
  NS_LOG_DEBUG ("Send packet " << packet->GetUid () << " to tunnel " << teid);
  int bytes = m_tunnelSocket->SendTo (
      packet, 0, InetSocketAddress (enbAddr, EpcNetwork::m_gtpuPort));
  if (bytes != (int)packet->GetSize ())
    {
      NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
      return false;
    }
  return true;
}

void
PgwUserApp::RecvFromTunnelSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT (socket == m_tunnelSocket);
  Ptr<Packet> packet = socket->Recv ();

  // Packet leaving the EPC (fire the Rx trace source).
  m_rxS5Trace (packet);
  EpcGtpuTag teidTag;
  packet->RemovePacketTag (teidTag);

  // Remove the GTP header
  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);

  // Attach the TunnelId tag with TEID value.
  TunnelIdTag tunnelIdTag (gtpu.GetTeid ());
  packet->AddPacketTag (tunnelIdTag);

  // Add the Ethernet header to the packet and send it to the logical port
  AddHeader (packet, m_pgwMacAddr, m_webMacAddr, Ipv4L3Protocol::PROT_NUMBER);

  NS_LOG_DEBUG ("Receive packet " << packet->GetUid () << " from tunnel " <<
                teidTag.GetTeid ());
  m_s5PortDevice->Receive (packet, Ipv4L3Protocol::PROT_NUMBER, m_pgwMacAddr,
                           m_webMacAddr, NetDevice::PACKET_HOST);
}

void
PgwUserApp::AddHeader (Ptr<Packet> packet, Mac48Address source,
                       Mac48Address dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << source << dest << protocolNumber);

  //
  // All Ethernet frames must carry a minimum payload of 46 bytes. We need to
  // pad out if we don't have enough bytes. These must be real bytes since they
  // will be written to pcap files and compared in regression trace files.
  //
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (dest);
  header.SetLengthType (protocolNumber);
  packet->AddHeader (header);

  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);
}

} // namespace ns3
