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
  ;
  return tid;
}

void
SvelteEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  EpcEnbApplication::DoDispose ();
}

void
SvelteEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());

  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
  packet->AddHeader (gtpu);
  uint32_t flags = 0;

  EpcGtpuTag teidTag (teid, EpcGtpuTag::ENB);
  packet->AddPacketTag (teidTag);
  m_txS1uTrace (packet);

  // FIXME Implementar corretamente a lógica de identificar para qual S-GW o pacote deve ser enviado.
  m_s1uSocket->SendTo (packet, flags, InetSocketAddress (Ipv4Address::GetAny (), 2152));
}

}  // namespace ns3
