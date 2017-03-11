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

#include "sgw-user-app.h"
#include "epc-network.h"
#include <ns3/tunnel-id-tag.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SgwUserApp");
NS_OBJECT_ENSURE_REGISTERED (SgwUserApp);

SgwUserApp::SgwUserApp ()
{
  NS_LOG_FUNCTION (this);
}

SgwUserApp::~SgwUserApp ()
{
  NS_LOG_FUNCTION (this);
}

SgwUserApp::SgwUserApp (Ptr<VirtualNetDevice> logicalPort)
{
  NS_LOG_FUNCTION (this << logicalPort);

  SetLogicalPort (logicalPort);
}

TypeId
SgwUserApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SgwUserApp")
    .SetParent<Application> ()
    .AddConstructor<SgwUserApp> ()
  ;
  return tid;
}

void
SgwUserApp::SetLogicalPort (Ptr<VirtualNetDevice> logicalPort)
{
  NS_LOG_FUNCTION (this << logicalPort);

  // Save the pointer and set the send callback.
  m_logicalPort = logicalPort;
  m_logicalPort->SetSendCallback (
    MakeCallback (&SgwUserApp::RecvFromLogicalPort, this));
}

bool
SgwUserApp::RecvFromLogicalPort (Ptr<Packet> packet, const Address& source,
                                 const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  // Retrieve the GTP TEID from TunnelId tag.
  TunnelIdTag tunnelIdTag;
  bool foud = packet->RemovePacketTag (tunnelIdTag);
  NS_ASSERT_MSG (foud, "Expected TunnelId tag not found.");

  // We expect that the destination address will be available in the 32 MSB of
  // tunnelId, while the TEID will be available in the 32 LSB of tunnelId.
  uint64_t tagValue = tunnelIdTag.GetTunnelId ();
  uint32_t teid = tagValue;
  Ipv4Address address (tagValue >> 32);

  // Add the GTP header.
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);

  // Send the packet to the tunnel socket.
  NS_LOG_DEBUG ("Send packet " << packet->GetUid () << " to tunnel " << teid <<
                " dst IP " << address << " port " << EpcNetwork::m_gtpuPort);
  return SentToTunnelSocket (
           packet, InetSocketAddress (address, EpcNetwork::m_gtpuPort));
}

bool
SgwUserApp::SendToLogicalPort (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Add the Ethernet header to the packet, using the logical port MAC address
  // as source. Note that the original Ethernet frame was removed by the
  // CsmaNetDevice when this packet arrived at this node, so here we don't now
  // the original MAC source and destination addresses. The destination address
  // must be set to the packet by the OpenFlow pipeline, and the source address
  // we set here using the logical port.
  AddHeader (packet, Mac48Address::ConvertFrom (m_logicalPort->GetAddress ()));

  // Send the packet to the OpenFlow switch over the logical port.
  return m_logicalPort->Receive (packet, Ipv4L3Protocol::PROT_NUMBER,
                                 Mac48Address (), Mac48Address (),
                                 NetDevice::PACKET_HOST);
}

void
SgwUserApp::RecvFromTunnelSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_ASSERT (socket == m_tunnelSocket);
  Ptr<Packet> packet = socket->Recv ();

  // Remove the GTP header.
  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);

  // Attach the TunnelId tag with TEID value.
  TunnelIdTag tunnelIdTag (gtpu.GetTeid ());
  packet->AddPacketTag (tunnelIdTag);

  // Send the packet to the logical port.
  NS_LOG_DEBUG ("Received packet " << packet->GetUid () <<
                " from tunnel " << gtpu.GetTeid ());
  SendToLogicalPort (packet);
}

bool
SgwUserApp::SentToTunnelSocket (Ptr<Packet> packet,
                                InetSocketAddress dstAddress)
{
  NS_LOG_FUNCTION (this << packet << dstAddress);

  // Send the packet to tunnel socket and check the return.
  int bytes = m_tunnelSocket->SendTo (packet, 0, dstAddress);
  if (bytes != (int)packet->GetSize ())
    {
      NS_LOG_ERROR ("Not all bytes were copied to the socket buffer.");
      return false;
    }
  return true;
}

void
SgwUserApp::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_tunnelSocket = 0;
  m_logicalPort = 0;
}

void
SgwUserApp::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  // Create and open the UDP socket for tunnel.
  m_tunnelSocket = Socket::CreateSocket (
      GetNode (), TypeId::LookupByName ("ns3::UdpSocketFactory"));
  m_tunnelSocket->Bind (
    InetSocketAddress (Ipv4Address::GetAny (), EpcNetwork::m_gtpuPort));
  m_tunnelSocket->SetRecvCallback (
    MakeCallback (&SgwUserApp::RecvFromTunnelSocket, this));
}

void
SgwUserApp::AddHeader (Ptr<Packet> packet, Mac48Address source,
                       Mac48Address dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  // All Ethernet frames must carry a minimum payload of 46 bytes. We need to
  // pad out if we don't have enough bytes. These must be real bytes since they
  // will be written to pcap files and compared in regression trace files.
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
