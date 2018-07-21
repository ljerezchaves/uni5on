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
#include "svelte-helper.h"
#include "infrastructure/radio-network.h"
#include "infrastructure/ring-network.h"
#include "infrastructure/svelte-enb-application.h"
#include "logical/enb-info.h"
#include "logical/slice-controller.h"
#include "logical/slice-network.h"
#include "logical/svelte-mme.h"
#include "logical/ue-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteHelper");
NS_OBJECT_ENSURE_REGISTERED (SvelteHelper);

SvelteHelper::SvelteHelper ()
{
  NS_LOG_FUNCTION (this);
}

SvelteHelper::~SvelteHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteHelper")
    .SetParent<EpcHelper> ()
    .AddAttribute ("HtcSlice", "The HTC slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ("ns3::SliceNetwork")),
                   MakeObjectFactoryAccessor (&SvelteHelper::m_htcNetFactory),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("HtcController", "The HTC slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ("ns3::SliceController")),
                   MakeObjectFactoryAccessor (&SvelteHelper::m_htcCtrlFactory),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcSlice", "The MTC slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ("ns3::SliceNetwork")),
                   MakeObjectFactoryAccessor (&SvelteHelper::m_mtcNetFactory),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcController", "The MTC slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ("ns3::SliceController")),
                   MakeObjectFactoryAccessor (&SvelteHelper::m_mtcCtrlFactory),
                   MakeObjectFactoryChecker ())
  ;
  return tid;
}

void
SvelteHelper::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on the infrastructure and logical networks.
  m_backhaul->EnablePcap (prefix, promiscuous);
  m_htcNetwork->EnablePcap (prefix, promiscuous);
}

void
SvelteHelper::PrintLteRem (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_radio, "No LTE radio network available.");
  m_radio->PrintRadioEnvironmentMap ();
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
SvelteHelper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi,
                                 Ptr<EpcTft> tft, EpsBearer bearer)
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
//      // Identify MTC UEs by the network address. // FIXME O ideal é marcar o slice do UE.
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
SvelteHelper::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice,
                      uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // Add an IPv4 stack to the previously created eNB node.
  InternetStackHelper internet;
  internet.Install (enb);

  // Get the number of switches on the backhaul.
  UintegerValue value;
  m_backhaul->GetAttribute ("NumRingSwitches", value);
  uint16_t numSwitches = value.Get ();

  // Connect the eNBs to switches in clockwise direction, skipping the first
  // switch (index 0), which is exclusive for the P-GW connection. The three
  // eNBs from the same cell site are always connected to the same switch in
  // the ring network.
  uint16_t siteId = (cellId - 1) / 3;
  uint16_t switchIdx = 1 + (siteId % (numSwitches - 1));

  // Attach the eNB node to the OpenFlow backhaul network.
  Ptr<CsmaNetDevice> enbS1uDev = m_backhaul->AttachEpcNode (enb, switchIdx, LteInterface::S1U);
  Ipv4Address enbS1uAddr = Ipv4AddressHelper::GetFirstAddress (enbS1uDev);
  NS_LOG_INFO ("eNB attached to the S1-U interface with IP " << enbS1uAddr);

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

  // Create the eNB info.
  Ptr<EnbInfo> enbInfo = CreateObject<EnbInfo> (cellId);
  enbInfo->SetEnbS1uAddr (enbS1uAddr);
  enbInfo->SetS1apSapEnb (enbApp->GetS1apSapEnb ());
}

void
SvelteHelper::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // TODO
}

void
SvelteHelper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice);

  // Create the UE info.
  CreateObject<UeInfo> (imsi);
}

Ptr<Node>
SvelteHelper::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv4InterfaceContainer
SvelteHelper::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv4Address
SvelteHelper::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

void
SvelteHelper::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
SvelteHelper::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the SVELTE infrastructure.
  m_mme = CreateObject<SvelteMme> ();
  m_backhaul = CreateObject<RingNetwork> ();
  m_radio = CreateObject<RadioNetwork> (Ptr<SvelteHelper> (this));

  // Create the LTE HTC slice network and controller.
  m_htcCtrlFactory.Set ("Mme", PointerValue (m_mme));
  m_htcController = m_htcCtrlFactory.Create<SliceController> ();

  m_htcNetFactory.Set ("SliceId", EnumValue (LogicalSlice::HTC));
  m_htcNetFactory.Set ("Controller", PointerValue (m_htcController));
  m_htcNetFactory.Set ("Backhaul", PointerValue (m_backhaul));
  m_htcNetFactory.Set ("Radio", PointerValue (m_radio));
  m_htcNetFactory.Set ("UeAddress", Ipv4AddressValue ("7.1.0.0"));
  m_htcNetFactory.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
  m_htcNetFactory.Set ("WebAddress", Ipv4AddressValue ("8.1.0.0"));
  m_htcNetFactory.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
  m_htcNetwork = m_htcNetFactory.Create<SliceNetwork> ();

  // Create the LTE MTC network slice.
  // TODO

  // Configure and install applications and traffic managers.
  // TODO

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

} // namespace ns3
