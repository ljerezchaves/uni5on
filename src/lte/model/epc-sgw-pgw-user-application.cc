/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *               2016 University of Campinas (Unicamp)
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
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/epc-gtpu-header.h"
#include "ns3/epc-gtpu-tag.h"
#include "ns3/abort.h"
#include "epc-sgw-pgw-user-application.h"
#include "epc-sgw-pgw-ctrl-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcSgwPgwUserApplication");

TypeId
EpcSgwPgwUserApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcSgwPgwUserApplication")
    .SetParent<Object> ()
    .SetGroupName ("Lte")
    .AddTraceSource ("S1uRx",
                     "Trace source indicating a packet received from S1-U interface.",
                     MakeTraceSourceAccessor (&EpcSgwPgwUserApplication::m_rxS1uTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S1uTx",
                     "Trace source indicating a packet transmitted over the S1-U interface.",
                     MakeTraceSourceAccessor (&EpcSgwPgwUserApplication::m_txS1uTrace),
                     "ns3::Packet::TracedCallback")
    ;
  return tid;
}

void
EpcSgwPgwUserApplication::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_s1uSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_s1uSocket = 0;
  m_controlPlane = 0;
}

EpcSgwPgwUserApplication::EpcSgwPgwUserApplication (
  const Ptr<VirtualNetDevice> tunDevice, const Ptr<Socket> s1uSocket,
  Ptr<EpcSgwPgwCtrlApplication> ctrlPlane)
  : m_s1uSocket (s1uSocket),
    m_tunDevice (tunDevice),
    m_gtpuUdpPort (2152), // fixed by the standard
    m_controlPlane (ctrlPlane)
{
  NS_LOG_FUNCTION (this << tunDevice << s1uSocket);
  m_s1uSocket->SetRecvCallback (
    MakeCallback (&EpcSgwPgwUserApplication::RecvFromS1uSocket, this));
}

EpcSgwPgwUserApplication::~EpcSgwPgwUserApplication ()
{
  NS_LOG_FUNCTION (this);
}

bool
EpcSgwPgwUserApplication::RecvFromTunDevice (Ptr<Packet> packet,
  const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << source << dest << packet << packet->GetSize ());

  // Get the IP address of the UE
  Ptr<Packet> pCopy = packet->Copy ();
  Ipv4Header ipv4Header;
  pCopy->RemoveHeader (ipv4Header);
  Ipv4Address ueAddr =  ipv4Header.GetDestination ();
  NS_LOG_LOGIC ("packet addressed to UE " << ueAddr);

  // TODO: Aqui, na hora que for mudar pro switch vai ser a hora que vai ter
  // que ter uma entrada no OpenFlow para cada fluxo dizendo o que fazer com o
  // pacote.  Não teremos mais o elememento classificador... 

  // find corresponding UeInfo address
  Ipv4Address enbAddr = m_controlPlane->GetEnbAddr (ueAddr);
  uint32_t teid = m_controlPlane->GetTeid (ueAddr, packet);
  SendToS1uSocket (packet, enbAddr, teid);

  // FIXME: Talvez seja melhor botar algum assert aqui pra facilitar depurar.
  // there is no reason why we should notify the TUN
  // VirtualNetDevice that he failed to send the packet: if we receive
  // any bogus packet, it will just be silently discarded.

  return true;
}

void
EpcSgwPgwUserApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT (socket == m_s1uSocket);
  Ptr<Packet> packet = socket->Recv ();

  // Packet leaving the EPC.
  m_rxS1uTrace (packet);
  EpcGtpuTag teidTag;
  packet->RemovePacketTag (teidTag);

  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();

  SendToTunDevice (packet, teid);
}

void
EpcSgwPgwUserApplication::SendToTunDevice (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid);
  NS_LOG_LOGIC (" packet size: " << packet->GetSize () << " bytes");
  m_tunDevice->Receive (packet, 0x0800, m_tunDevice->GetAddress (),
                        m_tunDevice->GetAddress (), NetDevice::PACKET_HOST);
}

void
EpcSgwPgwUserApplication::SendToS1uSocket (Ptr<Packet> packet,
  Ipv4Address enbAddr, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << enbAddr << teid);

  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);
  uint32_t flags = 0;

  // Packet entering the EPC.
  EpcGtpuTag teidTag (teid, EpcGtpuTag::PGW);
  packet->AddPacketTag (teidTag);
  m_txS1uTrace (packet);

  m_s1uSocket->SendTo (packet, flags, InetSocketAddress (enbAddr, m_gtpuUdpPort));
}

}  // namespace ns3
