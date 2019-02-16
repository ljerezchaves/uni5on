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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "svelte-enb-application.h"
#include "../svelte-common.h"
#include "../logical/epc-gtpu-tag.h"
#include "../metadata/ue-info.h"
#include "../metadata/routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteEnbApplication");

SvelteEnbApplication::SvelteEnbApplication (
  Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6, Ptr<Socket> s1uSocket,
  Ipv4Address enbS1uAddress, uint16_t cellId)
  : EpcEnbApplication (lteSocket, lteSocket6, s1uSocket, enbS1uAddress,
                       Ipv4Address::GetZero (), cellId)
{
  NS_LOG_FUNCTION (this << lteSocket << lteSocket6 << s1uSocket <<
                   enbS1uAddress << cellId);
}

SvelteEnbApplication::~SvelteEnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteEnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteEnbApplication")
    .SetParent<EpcEnbApplication> ()
    .AddTraceSource ("S1uRx",
                     "Trace source for a packet RX from the S1-U interface.",
                     MakeTraceSourceAccessor (
                       &SvelteEnbApplication::m_rxS1uTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("S1uTx",
                     "Trace source for a packet TX to the S1-U interface.",
                     MakeTraceSourceAccessor (
                       &SvelteEnbApplication::m_txS1uTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

void
SvelteEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_ASSERT (socket == m_s1uSocket);
  Ptr<Packet> packet = socket->Recv ();

  // Remove the EPC GTP-U packet tag from the packet.
  m_rxS1uTrace (packet);
  EpcGtpuTag teidTag;
  packet->RemovePacketTag (teidTag);

  // Remove the GTP-U header.
  GtpuHeader gtpu;
  packet->RemoveHeader (gtpu);
  uint32_t teid = gtpu.GetTeid ();
  m_rxS1uSocketPktTrace (packet->Copy ());

  // Check for UE context information.
  auto it = m_teidRbidMap.find (teid);
  if (it == m_teidRbidMap.end ())
    {
      NS_LOG_ERROR ("TEID not found in map. Discarding packet.");
      return;
    }

  // Send the packet to the UE over the LTE socket.
  SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
}

void
SvelteEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  EpcEnbApplication::DoDispose ();
}

void
SvelteEnbApplication::DoInitialContextSetupRequest (
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
SvelteEnbApplication::DoPathSwitchRequestAcknowledge (
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
SvelteEnbApplication::DoUeContextRelease (uint16_t rnti)
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
SvelteEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);

  // Attach the GTP-U header.
  GtpuHeader gtpu;
  if (rInfo->IsAggregated ())
    {
      // Trick for traffic aggregation: use the teid of the default bearer.
      gtpu.SetTeid (rInfo->GetUeInfo ()->GetDefaultTeid ());
    }
  else
    {
      gtpu.SetTeid (teid);
    }
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);

  // Add the EPC GTP-U packet tag to the packet.
  EpcGtpuTag teidTag (teid, EpcGtpuTag::ENB, rInfo->GetQosType ());
  packet->AddPacketTag (teidTag);
  m_txS1uTrace (packet);

  // Check for UE context information.
  auto it = m_teidSgwAddrMap.find (teid);
  if (it == m_teidSgwAddrMap.end ())
    {
      NS_LOG_ERROR ("TEID not found in map. Discarding packet.");
      return;
    }

  // Send the packet to the S-GW over the S1-U socket.
  m_s1uSocket->SendTo (packet, 0, InetSocketAddress (it->second, GTPU_PORT));
}

}  // namespace ns3
