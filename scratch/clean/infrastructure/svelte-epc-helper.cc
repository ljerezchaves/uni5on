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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteEpcHelper");
NS_OBJECT_ENSURE_REGISTERED (SvelteEpcHelper);

// Initializing SvelteEpcHelper static members.
const uint16_t    SvelteEpcHelper::m_gtpuPort = 2152;
const Ipv4Address SvelteEpcHelper::m_htcAddr  = Ipv4Address ("7.64.0.0");
const Ipv4Address SvelteEpcHelper::m_mtcAddr  = Ipv4Address ("7.128.0.0");
const Ipv4Address SvelteEpcHelper::m_s1uAddr  = Ipv4Address ("10.2.0.0");
const Ipv4Address SvelteEpcHelper::m_s5Addr   = Ipv4Address ("10.1.0.0");
const Ipv4Address SvelteEpcHelper::m_sgiAddr  = Ipv4Address ("8.0.0.0");
const Ipv4Address SvelteEpcHelper::m_ueAddr   = Ipv4Address ("7.0.0.0");
const Ipv4Address SvelteEpcHelper::m_x2Addr   = Ipv4Address ("10.3.0.0");
const Ipv4Mask    SvelteEpcHelper::m_htcMask  = Ipv4Mask ("255.192.0.0");
const Ipv4Mask    SvelteEpcHelper::m_mtcMask  = Ipv4Mask ("255.192.0.0");
const Ipv4Mask    SvelteEpcHelper::m_s1uMask  = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    SvelteEpcHelper::m_s5Mask   = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    SvelteEpcHelper::m_sgiMask  = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SvelteEpcHelper::m_ueMask   = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SvelteEpcHelper::m_x2Mask   = Ipv4Mask ("255.255.255.0");

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
SvelteEpcHelper::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
SvelteEpcHelper::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure IP address helpers.
  m_htcUeAddrHelper.SetBase (m_htcAddr, m_htcMask);
  m_mtcUeAddrHelper.SetBase (m_mtcAddr, m_mtcMask);
  m_s1uAddrHelper.SetBase   (m_s1uAddr, m_s1uMask);
  m_s5AddrHelper.SetBase    (m_s5Addr,  m_s5Mask);
  m_sgiAddrHelper.SetBase   (m_sgiAddr, m_sgiMask);
  m_x2AddrHelper.SetBase    (m_x2Addr,  m_x2Mask);

  // Configure the default PG-W address.
  // FIXME This may not make sense with multiple P-GW instances. Check this!
  Ipv4AddressHelper m_ueAddrHelper;
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
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

  // Add an IPv4 stack to the previously created eNB
  InternetStackHelper internet;
  internet.Install (enb);


  // FIXME Neste momento aqui é preciso fazer a conexão física com o backhaul.
  // Eu preciso saber qual o IP do eNB (enbS1uAddr) e do SGW (sgwS1uAddr) para que possa prosseguir
  Ipv4Address enbS1uAddr;
  Ipv4Address sgwS1uAddr;

  // Create the S1-U socket for the eNB
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (
      enb, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  enbS1uSocket->Bind (InetSocketAddress (enbS1uAddr, SvelteEpcHelper::m_gtpuPort));

  // Create the LTE IPv4 and IPv6 sockets for the eNB
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (
      enb, TypeId::LookupByName ("ns3::PacketSocketFactory"));
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Bind (enbLteSocketBindAddress);

  PacketSocketAddress enbLteSocketConnectAddress;
  enbLteSocketConnectAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Connect (enbLteSocketConnectAddress);

  Ptr<Socket> enbLteSocket6 = Socket::CreateSocket (
      enb, TypeId::LookupByName ("ns3::PacketSocketFactory"));
  PacketSocketAddress enbLteSocketBindAddress6;
  enbLteSocketBindAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Bind (enbLteSocketBindAddress6);

  PacketSocketAddress enbLteSocketConnectAddress6;
  enbLteSocketConnectAddress6.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Connect (enbLteSocketConnectAddress6);

  // Create the eNB application 
  // FIXME O eNB pode estar associado a mais de um S-GW. Como vamos gerenciar
  // isso? Talvez seja preciso criar uma versão modificada da aplicação do
  // enb.
  Ptr<EpcEnbApplication> enbApp = CreateObject<EpcEnbApplication> (
      enbLteSocket, enbLteSocket6, enbS1uSocket,
      enbS1uAddr, sgwS1uAddr, cellId);
//  enbApp->SetS1apSapMme (m_sdranCtrlApp->GetS1apSapMme ());
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);

  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

//  // Create the eNB info.
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
SvelteEpcHelper::AssignHtcUeIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_htcUeAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignMtcUeIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_mtcUeAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignS1Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignS5Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_s5AddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignSgiIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_sgiAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteEpcHelper::AssignX2Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_x2AddrHelper.Assign (devices);
}

Ipv4Address
SvelteEpcHelper::GetIpv4Addr (Ptr<const NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}

Ipv4Mask
SvelteEpcHelper::GetIpv4Mask (Ptr<const NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetMask ();
}

} // namespace ns3
