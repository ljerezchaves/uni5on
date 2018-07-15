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

#include <ns3/csma-module.h>
#include "svelte-epc-helper.h"
#include "infrastructure/ring-network.h"
#include "infrastructure/radio-network.h"
#include "infrastructure/svelte-enb-application.h"
#include "logical/svelte-mme.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteEpcHelper");
NS_OBJECT_ENSURE_REGISTERED (SvelteEpcHelper);

// Initializing SvelteEpcHelper static members.
const Ipv4Address SvelteEpcHelper::m_ueAddr   = Ipv4Address ("7.0.0.0");
const Ipv4Address SvelteEpcHelper::m_htcAddr  = Ipv4Address ("7.64.0.0");
const Ipv4Address SvelteEpcHelper::m_mtcAddr  = Ipv4Address ("7.128.0.0");
const Ipv4Mask    SvelteEpcHelper::m_ueMask   = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SvelteEpcHelper::m_htcMask  = Ipv4Mask ("255.192.0.0");
const Ipv4Mask    SvelteEpcHelper::m_mtcMask  = Ipv4Mask ("255.192.0.0");

SvelteEpcHelper::SvelteEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

SvelteEpcHelper::~SvelteEpcHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteEpcHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteEpcHelper")
    .SetParent<EpcHelper> ()
  ;
  return tid;
}

void
SvelteEpcHelper::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on the OpenFlow backhaul network.
  m_backhaul->EnablePcap (prefix, promiscuous);
}

void
SvelteEpcHelper::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
SvelteEpcHelper::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the OpenFlow backhaul network and the LTE radio network for the
  // SVELTE infrastructure.
  m_mme = CreateObject<SvelteMme> ();
  m_backhaul = CreateObject<RingNetwork> ();
  m_lteRan = CreateObject<RadioNetwork> (Ptr<SvelteEpcHelper> (this));

  // Configure IP address helpers. // FIXME
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);

  // Configure the default PG-W address.
  // FIXME This may not make sense with multiple P-GW instances. Check this!
  m_pgwAddr = m_ueAddrHelper.NewAddress ();

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
SvelteEpcHelper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  return 0;
//  FIXME
//  // Retrieve the IPv4 address of the UE and save it into UeInfo, if necessary.
//  Ptr<Node> ueNode = ueDevice->GetNode ();
//  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
//  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed.");
//
//  int32_t interface = ueIpv4->GetInterfaceForDevice (ueDevice);
//  NS_ASSERT (interface >= 0);
//  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
//
//  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
//  NS_LOG_INFO ("Activate EPS bearer UE IP address: " << ueAddr);
//
//  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
//  if (ueInfo->GetUeAddr () != ueAddr)
//    {
//      ueInfo->SetUeAddr (ueAddr);
//
//      // Identify MTC UEs by the network address.
//      if (m_mtcAddr.IsEqual (ueAddr.CombineMask (m_mtcMask)))
//        {
//          ueInfo->SetMtc (true);
//        }
//    }
//
//  // Trick for default bearer.
//  if (tft->IsDefaultTft ())
//    {
//      // To avoid rules overlap on the P-GW, we are going to replace the
//      // default packet filter by two filters that includes the UE address and
//      // the protocol (TCP and UDP).
//      tft->RemoveFilter (0);
//
//      EpcTft::PacketFilter filterTcp;
//      filterTcp.protocol = TcpL4Protocol::PROT_NUMBER;
//      filterTcp.localAddress = ueAddr;
//      tft->Add (filterTcp);
//
//      EpcTft::PacketFilter filterUdp;
//      filterUdp.protocol = UdpL4Protocol::PROT_NUMBER;
//      filterUdp.localAddress = ueAddr;
//      tft->Add (filterUdp);
//    }
//
//  // Save the bearer context into UE info.
//  UeInfo::BearerInfo bearerInfo;
//  bearerInfo.tft = tft;
//  bearerInfo.bearer = bearer;
//  uint8_t bearerId = ueInfo->AddBearer (bearerInfo);
//
//  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
//  if (ueLteDevice)
//    {
//      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
//    }
//  return bearerId;
}

void
SvelteEpcHelper::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // Attach the eNB node to the OpenFlow backhaul network.
  Ipv4Address enbS1uAddr = m_backhaul->AttachEnb (enb, cellId);

  // Create the S1-U socket for the eNB node.
  TypeId udpTid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (enb, udpTid);
  enbS1uSocket->Bind (InetSocketAddress (enbS1uAddr, BackhaulNetwork::m_gtpuPort));

  // Create the LTE IPv4 and IPv6 sockets for the eNB node.
  TypeId pktTid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (enb, pktTid);
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Bind (enbLteSocketBindAddress);

  PacketSocketAddress enbLteSocketConnectAddress;
  enbLteSocketConnectAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Connect (enbLteSocketConnectAddress);

  Ptr<Socket> enbLteSocket6 = Socket::CreateSocket (enb, pktTid);
  PacketSocketAddress enbLteSocketBindAddress6;
  enbLteSocketBindAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Bind (enbLteSocketBindAddress6);

  PacketSocketAddress enbLteSocketConnectAddress6;
  enbLteSocketConnectAddress6.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Connect (enbLteSocketConnectAddress6);

  // Create the custom eNB application for the SVELTE architecture.
  Ptr<SvelteEnbApplication> enbApp = CreateObject<SvelteEnbApplication> (
      enbLteSocket, enbLteSocket6, enbS1uSocket, enbS1uAddr, cellId);
  enbApp->SetS1apSapMme (m_mme->GetS1apSapMme ());
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);

  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

//  // Create the eNB info. // FIXME
//  Ptr<EnbInfo> enbInfo = CreateObject<EnbInfo> (cellId);
//  enbInfo->SetEnbS1uAddr (enbS1uAddr);
//  enbInfo->SetSgwS1uAddr (sgwS1uAddr);
//  enbInfo->SetSgwS1uPortNo (sgwS1uPortNo);
//  enbInfo->SetS1apSapEnb (enbApp->GetS1apSapEnb ());
//
}

void
SvelteEpcHelper::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // TODO
}

void
SvelteEpcHelper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice);

  // Create the UE info. FIXME
//  CreateObject<UeInfo> (imsi);
}

Ptr<Node>
SvelteEpcHelper::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  // FIXME Should return the IP address for the correct slice.
  NS_FATAL_ERROR ("SVELTE has more than one P-GW node.");
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  NS_FATAL_ERROR ("Use the specific method for HTC or MTC UEs.");
}

Ipv4Address
SvelteEpcHelper::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  // FIXME Deveria retornar o IP correto de acordo com o slice.
  // Fazer variações para cada tipo de UE.
  return m_pgwAddr;
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignHtcUeAddress (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // FIXME O ideal é delegar isso pro slicenetwork. que vamos criar.
  m_ueAddrHelper.SetBase (m_htcAddr, m_htcMask);
  return m_ueAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignMtcUeAddress (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // FIXME O ideal é delegar isso pro slicenetwork. que vamos criar.
  m_ueAddrHelper.SetBase (m_mtcAddr, m_mtcMask);
  return m_ueAddrHelper.Assign (devices);
}

Ipv4Address
SvelteEpcHelper::GetMtcPgwAddress ()
{
  NS_LOG_FUNCTION (this);

  // FIXME deferia ser independente por slice.
  return m_pgwAddr;
}

Ipv4Address
SvelteEpcHelper::GetHtcPgwAddress ()
{
  NS_LOG_FUNCTION (this);

  // FIXME deferia ser independente por slice.
  return m_pgwAddr;
}

} // namespace ns3