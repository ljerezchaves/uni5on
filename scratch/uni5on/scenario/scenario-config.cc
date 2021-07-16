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

#include <ns3/csma-module.h>
#include "scenario-config.h"
#include "../infrastructure/radio-network.h"
#include "../infrastructure/ring-network.h"
#include "../infrastructure/transport-controller.h"
#include "../metadata/enb-info.h"
#include "../metadata/ue-info.h"
#include "../slices/enb-application.h"
#include "../slices/slice-controller.h"
#include "../slices/slice-network.h"
#include "../slices/stateless-mme.h"
#include "../traffic/traffic-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScenarioConfig");
NS_OBJECT_ENSURE_REGISTERED (ScenarioConfig);

ScenarioConfig::ScenarioConfig ()
  : m_transport (0),
  m_radio (0),
  m_mme (0),
  m_mbbController (0),
  m_mbbNetwork (0),
  m_mbbTraffic (0),
  m_mtcController (0),
  m_mtcNetwork (0),
  m_mtcTraffic (0),
  m_tmpController (0),
  m_tmpNetwork (0),
  m_tmpTraffic (0),
  m_admissionStats (0),
  m_transportStats (0),
  m_mobilityStats (0),
  m_scalingStats (0),
  m_trafficStats (0)
{
  NS_LOG_FUNCTION (this);
}

ScenarioConfig::~ScenarioConfig ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ScenarioConfig::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScenarioConfig")
    .SetParent<EpcHelper> ()

    .AddAttribute ("MbbController", "The MBB slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mbbControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MbbSlice", "The MBB slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mbbNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MbbTraffic", "The MBB slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mbbTrafficFac),
                   MakeObjectFactoryChecker ())

    .AddAttribute ("MtcController", "The MTC slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mtcControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcSlice", "The MTC slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mtcNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("MtcTraffic", "The MTC slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_mtcTrafficFac),
                   MakeObjectFactoryChecker ())

    .AddAttribute ("TmpController", "The TMP slice controller configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_tmpControllerFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("TmpSlice", "The TMP slice network configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_tmpNetworkFac),
                   MakeObjectFactoryChecker ())
    .AddAttribute ("TmpTraffic", "The TMP slice traffic configuration.",
                   ObjectFactoryValue (ObjectFactory ()),
                   MakeObjectFactoryAccessor (
                     &ScenarioConfig::m_tmpTrafficFac),
                   MakeObjectFactoryChecker ())
  ;
  return tid;
}

void
ScenarioConfig::ConfigurePcap (std::string prefix, uint8_t config)
{
  NS_LOG_FUNCTION (this << prefix);

  bool slcofp = HasPcapFlag (ScenarioConfig::PCSLCOFP, config);
  bool slcpgw = HasPcapFlag (ScenarioConfig::PCSLCPGW, config);
  bool slcsgi = HasPcapFlag (ScenarioConfig::PCSLCSGI, config);
  bool tpnofp = HasPcapFlag (ScenarioConfig::PCTPNOFP, config);
  bool tpnepc = HasPcapFlag (ScenarioConfig::PCTPNEPC, config);
  bool tpnswt = HasPcapFlag (ScenarioConfig::PCTPNSWT, config);
  bool promsc = HasPcapFlag (ScenarioConfig::PCPROMSC, config);

  // Enable PCAP on the transport network.
  m_transport->EnablePcap (prefix, promsc, tpnofp, tpnepc, tpnswt);

  // Enable PCAP on the logical network slices.
  if (m_mbbNetwork)
    {
      m_mbbNetwork->EnablePcap (prefix, promsc, slcofp, slcpgw, slcsgi);
    }
  if (m_mtcNetwork)
    {
      m_mtcNetwork->EnablePcap (prefix, promsc, slcofp, slcpgw, slcsgi);
    }
  if (m_tmpNetwork)
    {
      m_tmpNetwork->EnablePcap (prefix, promsc, slcofp, slcpgw, slcsgi);
    }
}

bool
ScenarioConfig::HasPcapFlag (PcapConfig flag, uint8_t config) const
{
  NS_LOG_FUNCTION (this);

  return (config & (static_cast<uint8_t> (flag)));
}

void
ScenarioConfig::PrintLteRem (bool enable)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_radio, "No radio network available.");
  if (enable)
    {
      m_radio->PrintRadioEnvironmentMap ();
    }
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
ScenarioConfig::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi,
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
  uint8_t bearerId = ueInfo->AddEpsBearer (tft, bearer);

  // Activate the EPS bearer.
  NS_LOG_DEBUG ("Activating bearer id " << static_cast<uint16_t> (bearerId) <<
                " for UE IMSI " << imsi);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  NS_ASSERT_MSG (ueLteDevice, "LTE UE device not found.");
  ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);

  return bearerId;
}

void
ScenarioConfig::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice,
                        uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // Add an IPv4 stack to the previously created eNB node.
  InternetStackHelper internet;
  internet.Install (enb);

  // Attach the eNB node to the OpenFlow transport network over S1-U interface.
  uint16_t infraSwIdx = m_transport->GetEnbSwIdx (cellId);
  Ptr<CsmaNetDevice> enbS1uDev;
  Ptr<OFSwitch13Port> infraSwPort;
  std::tie (enbS1uDev, infraSwPort) = m_transport->AttachEpcNode (
      enb, infraSwIdx, EpsIface::S1);
  Ipv4Address enbS1uAddr = Ipv4AddressHelper::GetAddress (enbS1uDev);
  NS_LOG_DEBUG ("eNB cell ID " << cellId << " at switch index " << infraSwIdx);
  NS_LOG_INFO ("eNB " << enb << " attached to s1u with IP " << enbS1uAddr);

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

  // Create the custom eNB application for the UNI5ON architecture.
  Ptr<EnbApplication> enbApp = CreateObject<EnbApplication> (
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
ScenarioConfig::AddX2Interface (Ptr<Node> enb1Node, Ptr<Node> enb2Node)
{
  NS_LOG_FUNCTION (this << enb1Node << enb1Node);

  // Get the eNB device pointer from eNB node poiter.
  Ptr<LteEnbNetDevice> enb1Dev = 0, enb2Dev = 0;
  for (uint32_t i = 0; i < enb1Node->GetNDevices (); i++)
    {
      enb1Dev = enb1Node->GetDevice (i)->GetObject<LteEnbNetDevice> ();
      if (enb1Dev)
        {
          break;
        }
    }
  for (uint32_t i = 0; i < enb2Node->GetNDevices (); i++)
    {
      enb2Dev = enb2Node->GetDevice (i)->GetObject<LteEnbNetDevice> ();
      if (enb2Dev)
        {
          break;
        }
    }
  NS_ASSERT_MSG (enb1Dev, "eNB device not found for node " << enb1Node);
  NS_ASSERT_MSG (enb2Dev, "eNB device not found for node " << enb2Node);

  // Attach both eNB nodes to the OpenFlow transport network over X2 interface.
  uint16_t enb1CellId = enb1Dev->GetCellId ();
  uint16_t enb2CellId = enb2Dev->GetCellId ();
  uint16_t enb1InfraSwIdx = m_transport->GetEnbSwIdx (enb1CellId);
  uint16_t enb2InfraSwIdx = m_transport->GetEnbSwIdx (enb2CellId);
  Ptr<CsmaNetDevice> enb1X2Dev, enb2X2Dev;
  Ptr<OFSwitch13Port> enb1InfraSwPort, enb2InfraSwPort;
  std::tie (enb1X2Dev, enb1InfraSwPort) = m_transport->AttachEpcNode (
      enb1Node, enb1InfraSwIdx, EpsIface::X2, "x2_cell" +
      std::to_string (enb1CellId) + "to" + std::to_string (enb2CellId));
  std::tie (enb2X2Dev, enb2InfraSwPort) = m_transport->AttachEpcNode (
      enb2Node, enb2InfraSwIdx, EpsIface::X2, "x2_cell" +
      std::to_string (enb2CellId) + "to" + std::to_string (enb1CellId));
  Ipv4Address enb1X2Addr = Ipv4AddressHelper::GetAddress (enb1X2Dev);
  Ipv4Address enb2X2Addr = Ipv4AddressHelper::GetAddress (enb2X2Dev);
  NS_LOG_INFO ("eNB " << enb1Node << " attached to x2 with IP " << enb1X2Addr);
  NS_LOG_INFO ("eNB " << enb2Node << " attached to x2 with IP " << enb2X2Addr);

  // Add the X2 interface to both eNB X2 entities.
  Ptr<EpcX2> enb1X2 = enb1Node->GetObject<EpcX2> ();
  Ptr<EpcX2> enb2X2 = enb2Node->GetObject<EpcX2> ();
  enb1X2->AddX2Interface (enb1CellId, enb1X2Addr, enb2CellId, enb2X2Addr);
  enb2X2->AddX2Interface (enb2CellId, enb2X2Addr, enb1CellId, enb1X2Addr);
  enb1Dev->GetRrc ()->AddX2Neighbour (enb2CellId);
  enb2Dev->GetRrc ()->AddX2Neighbour (enb1CellId);
}

void
ScenarioConfig::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice);
}

Ptr<Node>
ScenarioConfig::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv4InterfaceContainer
ScenarioConfig::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv6InterfaceContainer
ScenarioConfig::AssignUeIpv6Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv4Address
ScenarioConfig::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

Ipv6Address
ScenarioConfig::GetUeDefaultGatewayAddress6 ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG ("Unimplemented method.");
}

void
ScenarioConfig::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // This will force output files to get closed.
  m_admissionStats->Dispose ();
  m_transportStats->Dispose ();
  m_scalingStats->Dispose ();
  m_trafficStats->Dispose ();
  m_mobilityStats->Dispose ();

  m_mme = 0;
  m_radio = 0;
  m_transport = 0;

  m_mbbController = 0;
  m_mbbNetwork = 0;
  m_mbbTraffic = 0;
  m_mtcController = 0;
  m_mtcNetwork = 0;
  m_mtcTraffic = 0;
  m_tmpController = 0;
  m_tmpNetwork = 0;
  m_tmpTraffic = 0;

  m_admissionStats = 0;
  m_transportStats = 0;
  m_scalingStats = 0;
  m_trafficStats = 0;
  m_mobilityStats = 0;

  Object::DoDispose ();
}

void
ScenarioConfig::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the UNI5ON infrastructure (don't change the order).
  m_mme = CreateObject<StatelessMme> ();
  m_transport = CreateObject<RingNetwork> ();
  m_radio = CreateObject<RadioNetwork> (Ptr<ScenarioConfig> (this));

  Ptr<TransportController> transportCtrl = m_transport->GetControllerApp ();
  ApplicationContainer sliceControllers;
  int sumQuota = 0;

  // Create the MBB logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_mbbControllerFac, m_mbbNetworkFac, m_mbbTrafficFac))
    {
      m_mbbControllerFac.Set ("SliceId", EnumValue (SliceId::MBB));
      m_mbbControllerFac.Set ("Mme", PointerValue (m_mme));
      m_mbbControllerFac.Set ("TransportCtrl", PointerValue (transportCtrl));
      m_mbbController = m_mbbControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_mbbController);
      sumQuota += m_mbbController->GetQuota ();

      m_mbbNetworkFac.Set ("SliceId", EnumValue (SliceId::MBB));
      m_mbbNetworkFac.Set ("SliceCtrl", PointerValue (m_mbbController));
      m_mbbNetworkFac.Set ("TransportNet", PointerValue (m_transport));
      m_mbbNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_mbbNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.1.0.0"));
      m_mbbNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_mbbNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.1.0.0"));
      m_mbbNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_mbbNetwork = m_mbbNetworkFac.Create<SliceNetwork> ();

      m_mbbTrafficFac.Set ("SliceId", EnumValue (SliceId::MBB));
      m_mbbTrafficFac.Set ("SliceCtrl", PointerValue (m_mbbController));
      m_mbbTrafficFac.Set ("SliceNet", PointerValue (m_mbbNetwork));
      m_mbbTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_mbbTraffic = m_mbbTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("MBB slice being ignored by now.");
    }

  // Create the MTC logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_mtcControllerFac, m_mtcNetworkFac, m_mtcTrafficFac))
    {
      m_mtcControllerFac.Set ("SliceId", EnumValue (SliceId::MTC));
      m_mtcControllerFac.Set ("Mme", PointerValue (m_mme));
      m_mtcControllerFac.Set ("TransportCtrl", PointerValue (transportCtrl));
      m_mtcController = m_mtcControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_mtcController);
      sumQuota += m_mtcController->GetQuota ();

      m_mtcNetworkFac.Set ("SliceId", EnumValue (SliceId::MTC));
      m_mtcNetworkFac.Set ("SliceCtrl", PointerValue (m_mtcController));
      m_mtcNetworkFac.Set ("TransportNet", PointerValue (m_transport));
      m_mtcNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_mtcNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.2.0.0"));
      m_mtcNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_mtcNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.2.0.0"));
      m_mtcNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_mtcNetwork = m_mtcNetworkFac.Create<SliceNetwork> ();

      m_mtcTrafficFac.Set ("SliceId", EnumValue (SliceId::MTC));
      m_mtcTrafficFac.Set ("SliceCtrl", PointerValue (m_mtcController));
      m_mtcTrafficFac.Set ("SliceNet", PointerValue (m_mtcNetwork));
      m_mtcTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_mtcTraffic = m_mtcTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("MTC slice being ignored by now.");
    }

  // Create the TMP logical slice controller, network, and traffic helper.
  if (AreFactoriesOk (m_tmpControllerFac, m_tmpNetworkFac, m_tmpTrafficFac))
    {
      m_tmpControllerFac.Set ("SliceId", EnumValue (SliceId::TMP));
      m_tmpControllerFac.Set ("Mme", PointerValue (m_mme));
      m_tmpControllerFac.Set ("TransportCtrl", PointerValue (transportCtrl));
      m_tmpController = m_tmpControllerFac.Create<SliceController> ();

      sliceControllers.Add (m_tmpController);
      sumQuota += m_tmpController->GetQuota ();

      m_tmpNetworkFac.Set ("SliceId", EnumValue (SliceId::TMP));
      m_tmpNetworkFac.Set ("SliceCtrl", PointerValue (m_tmpController));
      m_tmpNetworkFac.Set ("TransportNet", PointerValue (m_transport));
      m_tmpNetworkFac.Set ("RadioNet", PointerValue (m_radio));
      m_tmpNetworkFac.Set ("UeAddress", Ipv4AddressValue ("7.3.0.0"));
      m_tmpNetworkFac.Set ("UeMask", Ipv4MaskValue ("255.255.0.0"));
      m_tmpNetworkFac.Set ("WebAddress", Ipv4AddressValue ("8.3.0.0"));
      m_tmpNetworkFac.Set ("WebMask", Ipv4MaskValue ("255.255.0.0"));
      m_tmpNetwork = m_tmpNetworkFac.Create<SliceNetwork> ();

      m_tmpTrafficFac.Set ("SliceId", EnumValue (SliceId::TMP));
      m_tmpTrafficFac.Set ("SliceCtrl", PointerValue (m_tmpController));
      m_tmpTrafficFac.Set ("SliceNet", PointerValue (m_tmpNetwork));
      m_tmpTrafficFac.Set ("RadioNet", PointerValue (m_radio));
      m_tmpTraffic = m_tmpTrafficFac.Create<TrafficHelper> ();
    }
  else
    {
      NS_LOG_WARN ("TMP slice being ignored by now.");
    }

  // Validate slice quotas.
  NS_ABORT_MSG_IF (sumQuota > 100, "Inconsistent initial quotas.");

  // Notify the transport controller of the slice controllers.
  transportCtrl->NotifySlicesBuilt (sliceControllers);

  // Creating the statistic calculators.
  m_admissionStats  = CreateObject<AdmissionStatsCalculator> ();
  m_transportStats  = CreateObject<TransportStatsCalculator> ();
  m_mobilityStats   = CreateObject<MobilityStatsCalculator> ();
  m_scalingStats    = CreateObject<PgwuScalingStatsCalculator> ();
  m_trafficStats    = CreateObject<TrafficStatsCalculator> ();

  Object::NotifyConstructionCompleted ();
}

bool
ScenarioConfig::AreFactoriesOk (ObjectFactory &controller,
                                ObjectFactory &network,
                                ObjectFactory &traffic) const
{
  NS_LOG_FUNCTION (this);

  if (controller.GetTypeId () == TypeId ()
      || network.GetTypeId () == TypeId ()
      || traffic.GetTypeId () == TypeId ())
    {
      return false;
    }

  bool ok = true;
  ok &= (controller.GetTypeId () == SliceController::GetTypeId ()
         || controller.GetTypeId ().IsChildOf (SliceController::GetTypeId ()));
  ok &= (network.GetTypeId () == SliceNetwork::GetTypeId ()
         || network.GetTypeId ().IsChildOf (SliceNetwork::GetTypeId ()));
  ok &= (traffic.GetTypeId () == TrafficHelper::GetTypeId ()
         || traffic.GetTypeId ().IsChildOf (TrafficHelper::GetTypeId ()));
  return ok;
}

} // namespace ns3
