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
#include "infrastructure/backhaul-controller.h"
#include "infrastructure/metadata/enb-info.h"
#include "infrastructure/radio-network.h"
#include "infrastructure/ring-network.h"
#include "infrastructure/svelte-enb-application.h"
#include "logical/slice-controller.h"
#include "logical/slice-network.h"
#include "logical/svelte-mme.h"
#include "logical/metadata/ue-info.h"

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

  // To avoid rules overlap on the P-GW, we are going to replace the default
  // packet filter by two filters that includes the UE address and protocol.
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  if (tft->IsDefaultTft ())
    {
      tft->RemoveFilter (0);

      EpcTft::PacketFilter filterTcp;
      filterTcp.protocol = TcpL4Protocol::PROT_NUMBER;
      filterTcp.localAddress = ueInfo->GetUeAddr ();
      tft->Add (filterTcp);

      EpcTft::PacketFilter filterUdp;
      filterUdp.protocol = UdpL4Protocol::PROT_NUMBER;
      filterUdp.localAddress = ueInfo->GetUeAddr ();
      tft->Add (filterUdp);
    }

  // Save the bearer context into UE info.
  UeInfo::BearerInfo bearerInfo;
  bearerInfo.tft = tft;
  bearerInfo.bearer = bearer;
  uint8_t bearerId = ueInfo->AddBearer (bearerInfo);

  // Activate the EPS bearer.
  NS_LOG_DEBUG ("Activating bearer id " << static_cast<uint16_t> (bearerId) <<
                " for UE IMSI " << imsi);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  NS_ASSERT_MSG (ueLteDevice, "LTE UE device not found.");
  ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);

  return bearerId;
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

  // Attach the eNB node to the OpenFlow backhaul network.
  uint16_t infraSwIdx = GetEnbInfraSwIdx (cellId);
  Ptr<CsmaNetDevice> enbS1uDev;
  Ptr<OFSwitch13Port> infraSwPort;
  std::tie (enbS1uDev, infraSwPort) = m_backhaul->AttachEpcNode (
      enb, infraSwIdx, LteIface::S1U);
  Ipv4Address enbS1uAddr = Ipv4AddressHelper::GetAddress (enbS1uDev);
  NS_LOG_INFO ("eNB " << enb << " attached to the s1u " <<
               "interface with IP " << enbS1uAddr);

  // Create the S1-U socket for the eNB node.
  TypeId udpSocketTid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (enb, udpSocketTid);
  enbS1uSocket->Bind (
    InetSocketAddress (enbS1uAddr, BackhaulNetwork::m_gtpuPort));

  // Create the LTE IPv4 and IPv6 sockets for the eNB node.
  TypeId pktSocketTid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (enb, pktSocketTid);
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Bind (enbLteSocketBindAddress);

  PacketSocketAddress enbLteSocketAddress;
  enbLteSocketAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  enbLteSocket->Connect (enbLteSocketAddress);

  Ptr<Socket> enbLteSocket6 = Socket::CreateSocket (enb, pktSocketTid);
  PacketSocketAddress enbLteSocketBindAddress6;
  enbLteSocketBindAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Bind (enbLteSocketBindAddress6);

  PacketSocketAddress enbLteSocketAddress6;
  enbLteSocketAddress6.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketAddress6.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketAddress6.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  enbLteSocket6->Connect (enbLteSocketAddress6);

  // Create the custom eNB application for the SVELTE architecture.
  Ptr<SvelteEnbApplication> enbApp = CreateObject<SvelteEnbApplication> (
      enbLteSocket, enbLteSocket6, enbS1uSocket, enbS1uAddr, cellId);
  enbApp->SetS1apSapMme (m_mme->GetS1apSapMme ());
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);

  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

  // Saving eNB metadata.
  Ptr<EnbInfo> enbInfo = CreateObject<EnbInfo> (cellId);
  enbInfo->SetS1uAddr (enbS1uAddr);
  enbInfo->SetInfraSwIdx (infraSwIdx);
  enbInfo->SetInfraSwPortNo (infraSwPort->GetPortNo ());
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

  Ptr<BackhaulController> backahulCtrl = m_backhaul->GetControllerApp ();

  // Create the LTE HTC slice network and controller.
  m_htcCtrlFactory.Set ("SliceId", EnumValue (SliceId::HTC));
  m_htcCtrlFactory.Set ("Mme", PointerValue (m_mme));
  m_htcCtrlFactory.Set ("BackhaulCtrl", PointerValue (backahulCtrl));
  m_htcController = m_htcCtrlFactory.Create<SliceController> ();

  m_htcNetFactory.Set ("SliceId", EnumValue (SliceId::HTC));
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

uint16_t
SvelteHelper::GetEnbInfraSwIdx (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);

  // Connect the eNBs to switches in increasing index order, skipping the first
  // switch (index 0), which is exclusive for the P-GW connection. The three
  // eNBs from the same cell site are always connected to the same switch.
  uint16_t siteId = (cellId - 1) / 3;
  return 1 + (siteId % (m_backhaul->GetNSwitches () - 1));
}

} // namespace ns3
