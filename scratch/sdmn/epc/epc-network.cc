/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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
#include "epc-network.h"
#include "epc-controller.h"
#include "pgw-app.h"
#include "gtp-tunnel-app.h"
#include "../sdran/sdran-cloud.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (EpcNetwork);

// Initializing EpcNetwork static members.
const uint16_t    EpcNetwork::m_gtpuPort = 2152;
const Ipv4Address EpcNetwork::m_ueAddr   = Ipv4Address ("7.0.0.0");
const Ipv4Address EpcNetwork::m_sgiAddr  = Ipv4Address ("8.0.0.0");
const Ipv4Address EpcNetwork::m_s5Addr   = Ipv4Address ("10.1.0.0");
const Ipv4Address EpcNetwork::m_s1uAddr  = Ipv4Address ("10.2.0.0");
const Ipv4Address EpcNetwork::m_x2Addr   = Ipv4Address ("10.3.0.0");
const Ipv4Mask    EpcNetwork::m_ueMask   = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    EpcNetwork::m_sgiMask  = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    EpcNetwork::m_s5Mask   = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    EpcNetwork::m_s1uMask  = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    EpcNetwork::m_x2Mask   = Ipv4Mask ("255.255.255.0");

EpcNetwork::EpcNetwork ()
  : m_epcCtrlApp (0),
    m_epcCtrlNode (0),
    m_ofSwitchHelper (0),
    m_pgwNode (0),
    m_webNode (0)
{
  NS_LOG_FUNCTION (this);
}

EpcNetwork::~EpcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcNetwork")
    .SetParent<EpcHelper> ()

    // Attributes for connecting the EPC entities to the backhaul network.
    .AddAttribute ("EpcLinkDataRate",
                   "The data rate for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&EpcNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("EpcLinkDelay",
                   "The delay for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&EpcNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&EpcNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

Ptr<Node>
EpcNetwork::GetWebNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_webNode;
}

Ptr<Node>
EpcNetwork::GetSwitchNode (uint64_t dpId) const
{
  NS_LOG_FUNCTION (this << dpId);

  Ptr<Node> node = OFSwitch13Device::GetDevice (dpId)->GetObject<Node> ();
  NS_ASSERT_MSG (node, "No node found for this datapath ID");

  return node;
}

void
EpcNetwork::SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);

  m_ofSwitchHelper->SetDeviceAttribute (n1, v1);
}

void
EpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel.
  m_ofSwitchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  helper.EnablePcap (prefix + "web-sgi",    m_sgiDevices, promiscuous);
  helper.EnablePcap (prefix + "lte-epc-s5", m_s5Devices,  promiscuous);
  helper.EnablePcap (prefix + "lte-epc-x2", m_x2Devices,  promiscuous);
  helper.EnablePcap (prefix + "ofnetwork",  m_ofSwitches, promiscuous);
}

void
EpcNetwork::AttachSdranCloud (Ptr<SdranCloud> sdranCloud)
{
  NS_LOG_FUNCTION (this << sdranCloud);

  Ptr<Node> sgwNode = sdranCloud->GetSgwNode ();
  Ptr<OFSwitch13Device> sgwSwitchDev = sdranCloud->GetSgwSwitchDevice ();
  Ptr<SdranController> sdranCtrlApp = sdranCloud->GetSdranCtrlApp ();
  sdranCtrlApp->SetEpcCtlrApp (m_epcCtrlApp);

  // Get the switch datapath ID on the backhaul network to attach the S-GW.
  uint64_t swDpId = TopologyGetSgwSwitch (sdranCloud);
  Ptr<Node> swNode = GetSwitchNode (swDpId);

  // Connect the S-GW to the backhaul network over S5 interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, sgwNode);
  m_s5Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS5Dev, sgwS5Dev;
  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  sgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "_to_" +
              Names::FindName (sgwNode), swS5Dev);
  Names::Add (Names::FindName (sgwNode) + "_to_" +
              Names::FindName (swNode), sgwS5Dev);

  // Add the swS5Dev device as OpenFlow switch port on the backhaul switch.
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
  uint32_t swS5PortNo = swS5Port->GetPortNo ();

  // Add the sgwS5Dev as standard device on S-GW node.
  // It will be connected to a logical port through the GtpTunnelApp.
  m_s5AddrHelper.Assign (NetDeviceContainer (sgwS5Dev));
  NS_LOG_INFO ("S-GW S5 address: " << EpcNetwork::GetIpv4Addr (sgwS5Dev));

  // Create the virtual net device to work as the logical port on the S-GW S5
  // interface. This logical ports will connect to the S-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the sgwS5Dev.
  Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
  sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> sgwS5Port = sgwSwitchDev->AddSwitchPort (sgwS5PortDev);
  uint32_t sgwS5PortNo = sgwS5Port->GetPortNo ();

  // Create the S-GW S5 user-plane application.
  sgwNode->AddApplication (
    CreateObject<GtpTunnelApp> (sgwS5PortDev, sgwS5Dev));

  // Notify the EPC and SDRAN controllers of the new S-GW device attached
  // OpenFlow backhaul network.
  m_epcCtrlApp->NotifyS5Attach (swDev, swS5PortNo, sgwS5Dev);
  sdranCtrlApp->NotifySgwAttach (sgwS5PortNo, sgwS5Dev);
}

//
// Methods from EpcHelper are implemented at the end of this file.
//

void
EpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ofSwitchHelper = 0;
  m_epcCtrlNode = 0;
  m_epcCtrlApp = 0;
  m_pgwNode = 0;

  Object::DoDispose ();
}

void
EpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure CSMA helper for connecting EPC nodes (P-GW and S-GWs) to the
  // backhaul topology. This same helper will be used to connect the P-GW to
  // the server node on the Internet.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Configure IP address helpers.
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_sgiAddrHelper.SetBase (m_sgiAddr, m_sgiMask);
  m_s5AddrHelper.SetBase (m_s5Addr, m_s5Mask);
  m_x2AddrHelper.SetBase (m_x2Addr, m_x2Mask);

  // Set the default P-GW gateway logical address, which will be used to set
  // the static route at all UEs.
  m_pgwAddr = m_ueAddrHelper.NewAddress ();
  NS_LOG_INFO ("P-GW gateway address: " << GetUeDefaultGatewayAddress ());

  // Create the OFSwitch13 helper using p2p connections for OpenFlow channel.
  m_ofSwitchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Create the Internet web server node.
  m_webNode = CreateObject<Node> ();
  Names::Add ("web", m_webNode);

  InternetStackHelper internet;
  internet.Install (m_webNode);

  // Create the OpenFlow backhaul topology.
  TopologyCreate ();

  // Create and attach the P-GW element.
  Ptr<Node> pgwNode = CreateObject<Node> ();
  Names::Add ("pgw", pgwNode);
  AttachPgwNode (pgwNode);

  // The OpenFlow backhaul network topology is done and the P-GW gateways are
  // already connected to the S5 interface. Let's connect the OpenFlow switches
  // to the EPC controller. From this point on it is not possible to change the
  // OpenFlow network configuration.
  m_ofSwitchHelper->CreateOpenFlowChannels ();

  // Enable OpenFlow switch statistics.
  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  m_ofSwitchHelper->EnableDatapathStats (prefix + "ofswitch-stats", true);

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
EpcNetwork::InstallController (Ptr<EpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);

  NS_ASSERT_MSG (!m_epcCtrlApp, "Controller application already set.");

  // Create the controller node.
  m_epcCtrlNode = CreateObject<Node> ();
  Names::Add ("epc_ctrl", m_epcCtrlNode);

  // Installing the controller application into controller node.
  m_epcCtrlApp = controller;
  m_ofSwitchHelper->InstallController (m_epcCtrlNode, m_epcCtrlApp);
}

void
EpcNetwork::AttachPgwNode (Ptr<Node> pgwNode)
{
  NS_LOG_FUNCTION (this);

  // Configure the P-GW node as an OpenFlow switch.
  m_pgwNode = pgwNode;
  Ptr<OFSwitch13Device> pgwSwitchDev =
    (m_ofSwitchHelper->InstallSwitch (pgwNode)).Get (0);

  // PART 1: Connect the P-GW to the Internet Web server.
  //
  // Connect the P-GW to the Web server over SGi interface.
  m_sgiDevices = m_csmaHelper.Install (pgwNode, m_webNode);

  Ptr<CsmaNetDevice> pgwSgiDev, webSgiDev;
  pgwSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (0));
  webSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (1));

  Names::Add (Names::FindName (pgwNode) + "_to_" +
              Names::FindName (m_webNode), pgwSgiDev);
  Names::Add (Names::FindName (m_webNode) + "_to_" +
              Names::FindName (pgwNode), webSgiDev);

  // Add the pgwSgiDev as physical port on the P-GW OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = pgwSwitchDev->AddSwitchPort (pgwSgiDev);
  uint32_t pgwSgiPortNo = pgwSgiPort->GetPortNo ();

  // Set the IP address on the Internet Web server and P-GW SGi interfaces.
  m_sgiAddrHelper.Assign (NetDeviceContainer (m_sgiDevices));
  NS_LOG_INFO ("Web SGi address: " << EpcNetwork::GetIpv4Addr (webSgiDev));
  NS_LOG_INFO ("P-GW SGi address: " << EpcNetwork::GetIpv4Addr (pgwSgiDev));

  // Define static routes at the Internet Web server to the LTE network.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    EpcNetwork::m_ueAddr, EpcNetwork::m_ueMask,
    EpcNetwork::GetIpv4Addr (pgwSgiDev), 1);

  // PART 2: Connect the P-GW to the OpenFlow backhaul infrastructure.
  //
  // Get the switch datapath ID on the backhaul network to attach the P-GW.
  uint64_t swDpId = TopologyGetPgwSwitch (pgwSwitchDev);
  Ptr<Node> swNode = GetSwitchNode (swDpId);

  // Connect the P-GW to the backhaul over S5 interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, pgwNode);
  m_s5Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS5Dev, pgwS5Dev;
  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  pgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "_to_" +
              Names::FindName (pgwNode), swS5Dev);
  Names::Add (Names::FindName (pgwNode) + "_to_" +
              Names::FindName (swNode), pgwS5Dev);

  // Add the swS5Dev device as OpenFlow switch port on the backhaul switch.
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
  uint32_t swS5PortNo = swS5Port->GetPortNo ();

  // Add the pgwS5Dev as standard device on P-GW node.
  // It will be connected to a logical port through the PgwApp.
  m_s5AddrHelper.Assign (NetDeviceContainer (pgwS5Dev));
  NS_LOG_INFO ("P-GW S5 address: " << EpcNetwork::GetIpv4Addr (pgwS5Dev));

  // Create the virtual net device to work as the logical ports on the P-GW S5
  // interface. This logical ports will connect to the P-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the pgwS5Dev.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = pgwSwitchDev->AddSwitchPort (pgwS5PortDev);
  uint32_t pgwS5PortNo = pgwS5Port->GetPortNo ();

  // Create the P-GW S5 user-plane application.
  pgwNode->AddApplication (CreateObject <PgwApp> (pgwS5PortDev, pgwS5Dev));

  // Notify the EPC controller of the new P-GW device attached to the Internet
  // and to the OpenFlow backhaul network.
  m_epcCtrlApp->NotifyS5Attach (swDev, swS5PortNo, pgwS5Dev);
  m_epcCtrlApp->NotifyPgwAttach (pgwSwitchDev, pgwS5PortNo, pgwSgiPortNo,
                                 pgwS5Dev, pgwSgiDev, webSgiDev);
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
EpcNetwork::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi,
                               Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // Retrieve the IPv4 address of the UE and notify it to the S-GW.
  Ptr<Node> ueNode = ueDevice->GetNode ();
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed.");

  int32_t interface = ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);

  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  UeInfo::GetPointer (imsi)->SetUeAddr (ueAddr);

  NS_LOG_INFO ("Activate EPS bearer UE IP address: " << ueAddr);

  // Trick for default bearer.
  if (tft->IsDefaultTft ())
    {
      // To avoid rules overlap on the P-GW, we are going to replace the
      // default packet filter by two filters that includes the UE address and
      // the protocol (TCP and UDP).
      tft->RemoveFilter (0);

      EpcTft::PacketFilter filterTcp;
      filterTcp.protocol = TcpL4Protocol::PROT_NUMBER;
      filterTcp.localAddress = ueAddr;
      tft->Add (filterTcp);

      EpcTft::PacketFilter filterUdp;
      filterUdp.protocol = UdpL4Protocol::PROT_NUMBER;
      filterUdp.localAddress = ueAddr;
      tft->Add (filterUdp);
    }

  // Save the bearer context into UE info.
  UeInfo::BearerInfo bearerInfo;
  bearerInfo.tft = tft;
  bearerInfo.bearer = bearer;
  uint8_t bearerId = UeInfo::GetPointer (imsi)->AddBearer (bearerInfo);

  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
    }
  return bearerId;
}

void
EpcNetwork::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice,
                    uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  SdranCloud::GetPointer (enb)->AddEnb (enb, lteEnbNetDevice, cellId);
}

void
EpcNetwork::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);
  // TODO
}

void
EpcNetwork::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice);

  // Create the UE info.
  CreateObject<UeInfo> (imsi);
}

Ptr<Node>
EpcNetwork::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  return m_pgwNode;
}

Ipv4InterfaceContainer
EpcNetwork::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  return m_ueAddrHelper.Assign (ueDevices);
}

Ipv4Address
EpcNetwork::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  return m_pgwAddr;
}

Ipv4Address
EpcNetwork::GetIpv4Addr (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}

Ipv4Mask
EpcNetwork::GetIpv4Mask (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetMask ();
}

};  // namespace ns3