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
#include "traffic-helper.h"
#include "../infrastructure/backhaul-controller.h"
#include "../infrastructure/radio-network.h"
#include "../infrastructure/ring-network.h"
#include "../infrastructure/svelte-enb-application.h"
#include "../logical/slice-controller.h"
#include "../logical/slice-network.h"
#include "../logical/svelte-mme.h"
#include "../metadata/enb-info.h"
#include "../metadata/ue-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteHelper");
NS_OBJECT_ENSURE_REGISTERED (SvelteHelper);

SvelteHelper::SvelteHelper ()
  : m_backhaul (0),
  m_radio (0),
  m_mme (0),
  m_mtcController (0),
  m_mtcNetwork (0),
  m_mtcTraffic (0),
  m_htcController (0),
  m_htcNetwork (0),
  m_htcTraffic (0),
  m_tmpController (0),
  m_tmpNetwork (0),
  m_tmpTraffic (0),
  m_admissionStats (0),
  m_backhaulStats (0),
  m_handoverStats (0),
  m_pgwTftStats (0),
  m_trafficStats (0)
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
    .AddAttribute ("MtcController", "The MTC slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_mtcControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcSlice", "The MTC slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_mtcNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcTraffic", "The MTC slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_mtcTrafficFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("HtcController", "The HTC slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_htcControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("HtcSlice", "The HTC slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_htcNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("HtcTraffic", "The HTC slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_htcTrafficFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("TmpController", "The TMP slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_tmpControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("TmpSlice", "The TMP slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_tmpNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("TmpTraffic", "The TMP slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &SvelteHelper::m_tmpTrafficFac),
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
  if (m_mtcNetwork)
    {
      m_mtcNetwork->EnablePcap (prefix, promiscuous);
    }
  if (m_htcNetwork)
    {
      m_htcNetwork->EnablePcap (prefix, promiscuous);
    }
  if (m_tmpNetwork)
    {
      m_tmpNetwork->EnablePcap (prefix, promiscuous);
    }
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
      filterTcp.localAddress = ueInfo->GetAddr ();
      tft->Add (filterTcp);

      EpcTft::PacketFilter filterUdp;
      filterUdp.protocol = UdpL4Protocol::PROT_NUMBER;
      filterUdp.localAddress = ueInfo->GetAddr ();
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
  enbS1uSocket->Bind (InetSocketAddress (enbS1uAddr, GTPU_PORT));

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
  Ptr<EnbInfo> enbInfo = CreateObject<EnbInfo> (
      cellId, enbS1uAddr, infraSwIdx, infraSwPort->GetPortNo (), enbApp);
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

Ipv6InterfaceContainer
SvelteHelper::AssignUeIpv6Address (NetDeviceContainer ueDevices)
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

Ipv6Address
SvelteHelper::GetUeDefaultGatewayAddress6 ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

void
SvelteHelper::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // This will force output files to get closed.
  m_admissionStats->Dispose ();
  m_backhaulStats->Dispose ();
  m_handoverStats->Dispose ();
  m_pgwTftStats->Dispose ();
  m_trafficStats->Dispose ();

  m_mme = 0;
  m_backhaul = 0;
  m_radio = 0;

  m_mtcController = 0;
  m_mtcNetwork = 0;
  m_mtcTraffic = 0;
  m_htcController = 0;
  m_htcNetwork = 0;
  m_htcTraffic = 0;
  m_tmpController = 0;
  m_tmpNetwork = 0;
  m_tmpTraffic = 0;

  m_admissionStats = 0;
  m_backhaulStats = 0;
  m_handoverStats = 0;
  m_pgwTftStats = 0;
  m_trafficStats = 0;

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
  ApplicationContainer sliceControllers;
  UintegerValue quotaValue;
  uint16_t sumQuota = 0;

  // Create the MTC logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_mtcControllerFac, m_mtcNetworkFac, m_mtcTrafficFac))
    {
      m_mtcControllerFac.Set ("SliceId", EnumValue (SliceId::MTC));
      m_mtcControllerFac.Set ("Mme", PointerValue (m_mme));
      m_mtcControllerFac.Set ("BackhaulCtrl", PointerValue (backahulCtrl));
      m_mtcController = m_mtcControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_mtcController);
      m_mtcController->GetAttribute ("Quota", quotaValue);
      sumQuota += quotaValue.Get ();

      m_mtcNetworkFac.Set ("SliceId", EnumValue (SliceId::MTC));
      m_mtcNetworkFac.Set ("SliceCtrl", PointerValue (m_mtcController));
      m_mtcNetworkFac.Set ("BackhaulNet", PointerValue (m_backhaul));
      m_mtcNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_mtcNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.1.0.0"));
      m_mtcNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_mtcNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.1.0.0"));
      m_mtcNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_mtcNetwork = m_mtcNetworkFac.Create<SliceNetwork> ();

      m_mtcTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_mtcTrafficFac.Set ("SliceNet", PointerValue (m_mtcNetwork));
      m_mtcTraffic = m_mtcTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("MTC slice being ignored by now.");
    }

  // Create the HTC logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_htcControllerFac, m_htcNetworkFac, m_htcTrafficFac))
    {
      m_htcControllerFac.Set ("SliceId", EnumValue (SliceId::HTC));
      m_htcControllerFac.Set ("Mme", PointerValue (m_mme));
      m_htcControllerFac.Set ("BackhaulCtrl", PointerValue (backahulCtrl));
      m_htcController = m_htcControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_htcController);
      m_htcController->GetAttribute ("Quota", quotaValue);
      sumQuota += quotaValue.Get ();

      m_htcNetworkFac.Set ("SliceId", EnumValue (SliceId::HTC));
      m_htcNetworkFac.Set ("SliceCtrl", PointerValue (m_htcController));
      m_htcNetworkFac.Set ("BackhaulNet", PointerValue (m_backhaul));
      m_htcNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_htcNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.2.0.0"));
      m_htcNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_htcNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.2.0.0"));
      m_htcNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_htcNetwork = m_htcNetworkFac.Create<SliceNetwork> ();

      m_htcTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_htcTrafficFac.Set ("SliceNet", PointerValue (m_htcNetwork));
      m_htcTraffic = m_htcTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("HTC slice being ignored by now.");
    }

  // Create the TMP logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_tmpControllerFac, m_tmpNetworkFac, m_tmpTrafficFac))
    {
      m_tmpControllerFac.Set ("SliceId", EnumValue (SliceId::TMP));
      m_tmpControllerFac.Set ("Mme", PointerValue (m_mme));
      m_tmpControllerFac.Set ("BackhaulCtrl", PointerValue (backahulCtrl));
      m_tmpController = m_tmpControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_tmpController);
      m_tmpController->GetAttribute ("Quota", quotaValue);
      sumQuota += quotaValue.Get ();

      m_tmpNetworkFac.Set ("SliceId", EnumValue (SliceId::TMP));
      m_tmpNetworkFac.Set ("SliceCtrl", PointerValue (m_tmpController));
      m_tmpNetworkFac.Set ("BackhaulNet", PointerValue (m_backhaul));
      m_tmpNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_tmpNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.3.0.0"));
      m_tmpNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_tmpNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.3.0.0"));
      m_tmpNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_tmpNetwork = m_tmpNetworkFac.Create<SliceNetwork> ();

      m_tmpTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_tmpTrafficFac.Set ("SliceNet", PointerValue (m_tmpNetwork));
      m_tmpTraffic = m_tmpTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("TMP slice being ignored by now.");
    }

  // Validate slice quotas.
  NS_ABORT_MSG_IF (sumQuota != 100, "Inconsistent initial quotas.");

  // Notify the backhaul controller of the slice controllers.
  backahulCtrl->NotifySlicesBuilt (sliceControllers);

  // Creating the statistic calculators.
  m_admissionStats  = CreateObject<AdmissionStatsCalculator> ();
  m_backhaulStats   = CreateObject<BackhaulStatsCalculator> ();
  m_handoverStats   = CreateObject<HandoverStatsCalculator> ();
  m_pgwTftStats     = CreateObject<PgwTftStatsCalculator> ();
  m_trafficStats    = CreateObject<TrafficStatsCalculator> ();

  Object::NotifyConstructionCompleted ();
}

bool
SvelteHelper::AreFactoriesOk (ObjectFactory &controller,
                              ObjectFactory &network,
                              ObjectFactory &traffic) const
{
  NS_LOG_FUNCTION (this);

  return (controller.GetTypeId () == SliceController::GetTypeId ()
          && network.GetTypeId () == SliceNetwork::GetTypeId ()
          && traffic.GetTypeId () == TrafficHelper::GetTypeId ());
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
