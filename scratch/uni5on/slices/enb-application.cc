/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#include <ns3/tunnel-id-tag.h>
#include "enb-application.h"
#include "gtpu-tag.h"
#include "../metadata/bearer-info.h"
#include "../metadata/ue-info.h"
#include "../uni5on-common.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EnbApplication");

EnbApplication::EnbApplication (
  Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6,
  Ptr<VirtualNetDevice> s1uPortDev, Ipv4Address enbS1uAddress, uint16_t cellId)
  : EpcEnbApplication (lteSocket, lteSocket6, 0, enbS1uAddress,
                       Ipv4Address::GetZero (), cellId)
{
  NS_LOG_FUNCTION (this << lteSocket << lteSocket6 << enbS1uAddress << cellId);

  // Save the pointers and set the send callback.
  m_s1uLogicalPort = s1uPortDev;
  m_s1uLogicalPort->SetSendCallback (
    MakeCallback (&EnbApplication::RecvFromS1uLogicalPort, this));
}

EnbApplication::~EnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
EnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EnbApplication")
    .SetParent<EpcEnbApplication> ()
    .AddTraceSource ("S1uRx",
                     "Trace source for a packet RX from the S1-U interface.",
                     MakeTraceSourceAccessor (&EnbApplication::m_rxS1uTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S1uTx",
                     "Trace source for a packet TX to the S1-U interface.",
                     MakeTraceSourceAccessor (&EnbApplication::m_txS1uTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

bool
EnbApplication::RecvFromS1uLogicalPort (
  Ptr<Packet> packet, const Address& source,
  const Address& dest, uint16_t protocolNo)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNo);

  // Fire trace sources.
  m_rxS1uTrace (packet);
  m_rxS1uSocketPktTrace (packet->Copy ());

  // Remove the EPC GTP-U packet tag from the packet.
  GtpuTag gtpuTag;
  packet->RemovePacketTag (gtpuTag);

  // We expect that the TEID will be available in the 32 LSB of tunnelId tag.
  TunnelIdTag tunnelIdTag;
  bool foud = packet->RemovePacketTag (tunnelIdTag);
  NS_ASSERT_MSG (foud, "Expected TunnelId tag not found.");
  uint64_t tagValue = tunnelIdTag.GetTunnelId ();
  uint32_t teid = tagValue;

  // Check for UE context information.
  auto it = m_teidRbidMap.find (teid);
  if (it == m_teidRbidMap.end ())
    {
      NS_LOG_ERROR ("TEID not found in map. Discarding packet.");
      return false;
    }

  // Send the packet to the UE over the LTE socket.
  SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
  return true;
}

void
EnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  NS_ABORT_MSG ("Unimplemented method.");
}

void
EnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_s1uLogicalPort = 0;
  EpcEnbApplication::DoDispose ();
}

void
EnbApplication::DoInitialContextSetupRequest (
  uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
  std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList)
{
  NS_LOG_FUNCTION (this << mmeUeS1Id << enbUeS1Id);

  // Save the mapping TEID --> S-GW S1-U IP address.
  for (auto const &erab : erabToBeSetupList)
    {
      // Side effect: create entry if it does not exist.
      m_teidSgwAddrMap [erab.sgwTeid] = erab.transportLayerAddress;
      NS_LOG_DEBUG ("eNB cell ID " << m_cellId <<
                    " mapping TEID " << GetUint32Hex (erab.sgwTeid) <<
                    " to S-GW S1-U IP " << m_teidSgwAddrMap [erab.sgwTeid]);
    }

  EpcEnbApplication::DoInitialContextSetupRequest (
    mmeUeS1Id, enbUeS1Id, erabToBeSetupList);
}

void
EnbApplication::DoPathSwitchRequestAcknowledge (
  uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t cgi,
  std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem>
  erabToBeSwitchedInUplinkList)
{
  NS_LOG_FUNCTION (this << enbUeS1Id << mmeUeS1Id << cgi);

  // Update the mapping TEID --> S-GW S1-U IP address.
  for (auto const &erab : erabToBeSwitchedInUplinkList)
    {
      // Side effect: create entry if it does not exist.
      m_teidSgwAddrMap [erab.enbTeid] = erab.transportLayerAddress;
      NS_LOG_DEBUG ("eNB cell ID " << m_cellId <<
                    " mapping TEID " << GetUint32Hex (erab.enbTeid) <<
                    " to S-GW S1-U IP " << m_teidSgwAddrMap [erab.enbTeid]);
    }

  EpcEnbApplication::DoPathSwitchRequestAcknowledge (
    enbUeS1Id, mmeUeS1Id, cgi, erabToBeSwitchedInUplinkList);
}

void
EnbApplication::DoUeContextRelease (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);

  // Remove the mapping TEID --> S-GW S1-U IP address.
  auto rntiIt = m_rbidTeidMap.find (rnti);
  if (rntiIt != m_rbidTeidMap.end ())
    {
      for (auto const &bidIt : rntiIt->second)
        {
          uint32_t teid = bidIt.second;
          m_teidSgwAddrMap.erase (teid);
          NS_LOG_DEBUG ("eNB cell ID " << m_cellId <<
                        " removed TEID " << GetUint32Hex (teid) <<
                        " from S-GW S1-U mapping.");
        }
    }

  EpcEnbApplication::DoUeContextRelease (rnti);
}

void
EnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());

  Ptr<BearerInfo> bInfo = BearerInfo::GetPointer (teid);

  // Add the EPC GTP-U packet tag to the packet.
  GtpuTag gtpuTag (
    teid, GtpuTag::ENB, bInfo->GetQosType (), bInfo->IsAggregated ());
  packet->AddPacketTag (gtpuTag);
  m_txS1uTrace (packet);

  // Check for UE context information.
  auto it = m_teidSgwAddrMap.find (teid);
  if (it == m_teidSgwAddrMap.end ())
    {
      NS_LOG_ERROR ("TEID not found in map. Discarding packet.");
      return;
    }

  // FIXME Temporary trick (must be replaces by OpenFlow rules in the switch).
  // Attach the TunnelId tag with TEID value.
  uint64_t tunnelId = static_cast<uint64_t> ((it->second).Get ());
  tunnelId <<= 32;
  tunnelId |= static_cast<uint64_t> (teid);
  TunnelIdTag tunnelIdTag (tunnelId);
  packet->ReplacePacketTag (tunnelIdTag);

  // Add the Ethernet header to the packet.
  AddHeader (packet);

  // Send the packet to the OpenFlow switch over the logical port.
  // Don't worry about source and destination addresses because they are note
  // used by the receive method.
  m_s1uLogicalPort->Receive (
    packet, Ipv4L3Protocol::PROT_NUMBER, Mac48Address (),
    Mac48Address (), NetDevice::PACKET_HOST);
}

void
EnbApplication::AddHeader (Ptr<Packet> packet, Mac48Address source,
                           Mac48Address dest, uint16_t protocolNo)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNo);

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
  header.SetLengthType (protocolNo);
  packet->AddHeader (header);

  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);
}

}  // namespace ns3
