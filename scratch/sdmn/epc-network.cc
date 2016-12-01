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
#include "stats-calculator.h"
#include "pgw-user-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (EpcNetwork);

const uint16_t EpcNetwork::m_gtpuPort = 2152;

EpcNetwork::EpcNetwork ()
  : m_ofSwitchHelper (0),
    m_epcCtrlApp (0),
    m_epcCtrlNode (0),
    m_pgwNode (0),
    m_webNode (0),
    m_pgwStats (0),
    m_webStats (0),
    m_epcStats (0)
{
  NS_LOG_FUNCTION (this);

  // Let's use point to point connections for OpenFlow channel
  m_ofSwitchHelper = CreateObject<OFSwitch13Helper> ();
  m_ofSwitchHelper->SetAttribute ("ChannelType",
                                  EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Creating network nodes
  m_webNode = CreateObject<Node> ();
  Names::Add ("web", m_webNode);

  m_pgwNode = CreateObject<Node> ();
  Names::Add ("pgw", m_pgwNode);

  m_epcCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_epcCtrlNode);

  // Configure the P-GW node as an OpenFlow switch.
  // Doing this here will ensure the P-GW datapath ID equals to 1.
  m_pgwSwitchDev = (m_ofSwitchHelper->InstallSwitch (m_pgwNode)).Get (0);

  // Creating stats calculators
  m_pgwStats = CreateObjectWithAttributes<LinkQueuesStatsCalculator> (
      "LnkStatsFilename", StringValue ("pgw_stats.txt"));
  m_webStats = CreateObjectWithAttributes<LinkQueuesStatsCalculator> (
      "LnkStatsFilename", StringValue ("web_stats.txt"));
  m_epcStats = CreateObject<NetworkStatsCalculator> ();
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

    // IP network addresses.
    .AddAttribute ("UeNetworkAddr",
                   "The IPv4 network address used for UE devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("7.0.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_ueNetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("WebNetworkAddr",
                   "The IPv4 network address used for web devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("8.0.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_webNetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("S5NetworkAddr",
                   "The IPv4 network address used for S5 devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("10.1.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_s5NetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("X2NetworkAddr",
                   "The IPv4 network address used for X2 devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("10.2.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_x2NetworkAddr),
                   MakeIpv4AddressChecker ())

    // Trace sources used by stats calculators to be aware of backhaul network.
    .AddTraceSource ("NewSwitchConnection",
                     "New connection between two OpenFlow switches during "
                     "backhaul topology creation.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_newConnTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
    .AddTraceSource ("TopologyBuilt",
                     "OpenFlow backhaul network topology is built and no more "
                     "connections between OpenFlow switches will be created.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_topoBuiltTrace),
                     "ns3::EpcNetwork::TopologyTracedCallback")
  ;
  return tid;
}

void
EpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel
  m_ofSwitchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices
  CsmaHelper helper;
  helper.EnablePcap (prefix + "web-sgi",    m_sgiDevices, promiscuous);
  helper.EnablePcap (prefix + "lte-epc-s5", m_s5Devices,  promiscuous);
  helper.EnablePcap (prefix + "lte-epc-x2", m_x2Devices,  promiscuous);
  helper.EnablePcap (prefix + "ofnetwork",  m_ofSwitches, promiscuous);
}

void
EpcNetwork::SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_ofSwitchHelper->SetDeviceAttribute (n1, v1);
}

uint16_t
EpcNetwork::GetNSwitches (void) const
{
  return m_ofSwitches.GetN ();
}

Ptr<Node>
EpcNetwork::GetWebNode () const
{
  return m_webNode;
}

Ipv4Address
EpcNetwork::GetWebIpAddress () const
{
  return m_webSgiIpAddr;
}

Ptr<Node>
EpcNetwork::GetControllerNode () const
{
  return m_epcCtrlNode;
}

Ptr<EpcController>
EpcNetwork::GetControllerApp () const
{
  return m_epcCtrlApp;
}

void
EpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofSwitchHelper = 0;
  m_epcCtrlNode = 0;
  m_epcCtrlApp = 0;
  m_pgwNode = 0;
  m_pgwStats = 0;
  m_webStats = 0;
  m_epcStats = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
}

void
EpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Connect EPC stats calculator to trace sources *before* topology creation.
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &NetworkStatsCalculator::NotifyTopologyBuilt, m_epcStats));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &NetworkStatsCalculator::NotifyNewSwitchConnection, m_epcStats));

  // Configuring CSMA helper for connecting EPC nodes (P-GW and S-GWs) to the
  // backhaul topology. This same helper will be used to connect the P-GW to
  // the server node on the Internet.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Configuring IP addresses (don't change the masks!)
  // Using a /8 subnet for all UEs and the P-GW gateway address.
  m_ueAddrHelper.SetBase (m_ueNetworkAddr, "255.0.0.0");

  // Using a /30 subnet which can hold exactly two address for the Internet
  // connection (one used by the server and the other is logically associated
  // to the P-GW external interface.
  m_webAddrHelper.SetBase (m_webNetworkAddr, "255.255.255.252");

  // Using a /24 subnet which can hold up to 253 S-GWs and the P-GW addresses
  // on the same S5 backhaul network.
  m_s5AddrHelper.SetBase (m_s5NetworkAddr, "255.255.255.0");

  // Using a /30 subnet which can hold exactly two eNBs addresses for the point
  // to point X2 interface.
  m_x2AddrHelper.SetBase (m_x2NetworkAddr, "255.255.255.252");

  // Create the OpenFlow backhaul topology.
  TopologyCreate ();

  // Configuring the P-GW and the Internet topology.
  ConfigureGatewayAndInternet ();

  // The OpenFlow backhaul network topology is done. Connect the OpenFlow
  // switches to the EPC controller. From this point on it is not possible to
  // change the OpenFlow network configuration.
  m_ofSwitchHelper->CreateOpenFlowChannels ();

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
EpcNetwork::RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node)
{
  std::pair<NodeSwitchMap_t::iterator, bool> ret;
  std::pair<Ptr<Node>, uint16_t> entry (node, switchIdx);
  ret = m_nodeSwitchMap.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("Node " << node << " -- switch " << (int)switchIdx);
      return;
    }
  NS_FATAL_ERROR ("Can't register node at switch.");
}

void
EpcNetwork::RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node)
{
  m_pgwSwIdx = switchIdx;
  RegisterNodeAtSwitch (switchIdx, node);
}

Ptr<OFSwitch13Device>
EpcNetwork::GetSwitchDevice (uint16_t index) const
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return m_ofDevices.Get (index);
}

uint16_t
EpcNetwork::GetSwitchIdxForNode (Ptr<Node> node) const
{
  NodeSwitchMap_t::const_iterator ret;
  ret = m_nodeSwitchMap.find (node);
  if (ret != m_nodeSwitchMap.end ())
    {
      NS_LOG_DEBUG ("Found switch " << (int)ret->second << " for " << node);
      return ret->second;
    }
  NS_FATAL_ERROR ("Node not registered.");
}

uint16_t
EpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13Device> dev) const
{
  uint16_t i;
  for (i = 0; i < GetNSwitches (); i++)
    {
      if (dev == GetSwitchDevice (i))
        {
          return i;
        }
    }
  NS_FATAL_ERROR ("Device not registered.");
}

uint16_t
EpcNetwork::GetGatewaySwitchIdx () const
{
  return m_pgwSwIdx;
}

void
EpcNetwork::InstallController (Ptr<EpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_epcCtrlApp, "Controller application already set.");

  // Installing the controller application into controller node
  m_epcCtrlApp = controller;
  m_ofSwitchHelper->InstallController (m_epcCtrlNode, m_epcCtrlApp);
}

Ptr<NetDevice>
EpcNetwork::S1EnbAttach (Ptr<Node> node, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  uint16_t swIdx = TopologyGetEnbIndex (cellId);
  RegisterNodeAtSwitch (swIdx, node);
  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);

  // Creating a link between the switch and the node
  NetDeviceContainer devices;
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (node);
  devices = m_csmaHelper.Install (pair);  // FIXME Vai usar mesmo o gwHelper?

  Ptr<CsmaNetDevice> portDev, nodeDev;
  portDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  nodeDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  // Setting interface names for pacp filename
  Names::Add (Names::FindName (swNode) + "+" +
              Names::FindName (node), portDev);
  Names::Add (Names::FindName (node) + "+" +
              Names::FindName (swNode), nodeDev);

  // Set S1U IPv4 address for the new device at node
  Ipv4InterfaceContainer nodeIpIfaces =
    m_s5AddrHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);

  // Adding newly created csma device as openflow switch port.
  Ptr<OFSwitch13Device> swDev = GetSwitchDevice (swIdx);
  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
  uint32_t portNum = swPort->GetPortNo ();

  // Notify the controller of a new device attached to network
  m_epcCtrlApp->NewS5Attach (nodeDev, nodeAddr, swDev, swIdx, portNum);

  return nodeDev;
}

NetDeviceContainer
EpcNetwork::X2Attach (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  // Create a P2P connection between the eNBs.
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2000));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0)));
  NetDeviceContainer enbDevices =  p2ph.Install (enb1, enb2);

  // // Creating a link between the firts eNB and its switch
  // uint16_t swIdx1 = GetSwitchIdxForNode (enb1);
  // Ptr<Node> swNode1 = m_ofSwitches.Get (swIdx1);
  // Ptr<OFSwitch13Device> swDev1 = GetSwitchDevice (swIdx1);

  // NodeContainer pair1;
  // pair1.Add (swNode1);
  // pair1.Add (enb1);
  // NetDeviceContainer devices1 = m_swHelper.Install (pair1);

  // Ptr<CsmaNetDevice> portDev1, enbDev1;
  // portDev1 = DynamicCast<CsmaNetDevice> (devices1.Get (0));
  // enbDev1 = DynamicCast<CsmaNetDevice> (devices1.Get (1));

  // // Creating a link between the second eNB and its switch
  // uint16_t swIdx2 = GetSwitchIdxForNode (enb2);
  // Ptr<Node> swNode2 = m_ofSwitches.Get (swIdx2);
  // Ptr<OFSwitch13Device> swDev2 = GetSwitchDevice (swIdx2);

  // NodeContainer pair2;
  // pair2.Add (swNode2);
  // pair2.Add (enb2);
  // NetDeviceContainer devices2 = m_swHelper.Install (pair2);

  // Ptr<CsmaNetDevice> portDev2, enbDev2;
  // portDev2 = DynamicCast<CsmaNetDevice> (devices2.Get (0));
  // enbDev2 = DynamicCast<CsmaNetDevice> (devices2.Get (1));

  // // Set X2 IPv4 address for the new devices
  // NetDeviceContainer enbDevices;
  // enbDevices.Add (enbDev1);
  // enbDevices.Add (enbDev2);

  Ipv4InterfaceContainer nodeIpIfaces = m_x2AddrHelper.Assign (enbDevices);
  m_x2AddrHelper.NewNetwork ();
  // Ipv4Address nodeAddr1 = nodeIpIfaces.GetAddress (0);
  // Ipv4Address nodeAddr2 = nodeIpIfaces.GetAddress (1);

  // // Adding newly created csma devices as openflow switch ports.
  // Ptr<OFSwitch13Port> swPort1 = swDev1->AddSwitchPort (portDev1);
  // uint32_t portNum1 = swPort1->GetPortNo ();

  // Ptr<OFSwitch13Port> swPort2 = swDev2->AddSwitchPort (portDev2);
  // uint32_t portNum2 = swPort2->GetPortNo ();

  // // Trace source notifying new devices attached to the network
  // m_newAttachTrace (enbDev1, nodeAddr1, swDev1, swIdx1, portNum1);
  // m_newAttachTrace (enbDev2, nodeAddr2, swDev2, swIdx2, portNum2);

  return enbDevices;
}

void
EpcNetwork::ConfigureGatewayAndInternet ()
{
  NS_LOG_FUNCTION (this);

  //
  // The first part is to connect the P-GW node to the Interent Web server.
  //
  // Creating the SGi interface
  m_sgiDevices = m_csmaHelper.Install (m_pgwNode, m_webNode);

  Ptr<CsmaNetDevice> pgwSgiDev, webSgiDev;
  pgwSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (0));
  webSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (1));

  Names::Add (Names::FindName (m_pgwNode) + "+" +
              Names::FindName (m_webNode), pgwSgiDev);
  Names::Add (Names::FindName (m_webNode) + "+" +
              Names::FindName (m_pgwNode), webSgiDev);

  // Add the pgwSgiDev as physical port on the P-GW OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = m_pgwSwitchDev->AddSwitchPort (pgwSgiDev);
  uint32_t pgwSgiPortNum = pgwSgiPort->GetPortNo ();

  // Note that the SGi device created on the P-GW node and configure as an
  // OpenFlow switch port will no have an IP address assigned to it. However,
  // we need this address on the PgwUserApp, so we set it here.
  m_pgwSgiAddr = m_webAddrHelper.NewAddress ();
  NS_LOG_LOGIC ("P-GW SGi interface address: " << m_pgwSgiAddr);

  // Set the IP address on the Internet Web server.
  InternetStackHelper internet;
  internet.Install (m_webNode);
  Ipv4InterfaceContainer webSgiIfContainer;
  webSgiIfContainer = m_webAddrHelper.Assign (NetDeviceContainer (webSgiDev));
  m_webSgiIpAddr = webSgiIfContainer.GetAddress (0);
  NS_LOG_LOGIC ("Web SGi interface address: " << m_webSgiIpAddr);

  // Defining static routes at the Internet Web server to the LTE network
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    m_ueNetworkAddr, Ipv4Mask ("255.0.0.0"), m_pgwSgiAddr, 1);

  //
  // The second part is to connect the P-GW node to the OpenFlow backhaul
  // infrasctructure.
  //
  // Get the switch index for P-GW attach.
  uint16_t swIdx = TopologyGetPgwIndex ();
  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);
  RegisterGatewayAtSwitch (swIdx, m_pgwNode);

  // Connecting the P-GW to the backhaul over S5 interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, m_pgwNode);
  m_s5Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS5Dev, pgwS5Dev;
  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  pgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "+" +
              Names::FindName (m_pgwNode), swS5Dev);
  Names::Add (Names::FindName (m_pgwNode) + "+" +
              Names::FindName (swNode), pgwS5Dev);

  // Configure the pgwS5Dev as standard device on P-GW node (with IP address).
  Ipv4InterfaceContainer pgwS5IfContainer;
  pgwS5IfContainer = m_s5AddrHelper.Assign (NetDeviceContainer (pgwS5Dev));
  m_pgwS5Addr = pgwS5IfContainer.GetAddress (0);
  NS_LOG_LOGIC ("P-GW S5 interface address: " << m_pgwS5Addr);

  // Create the virtual net device to work as the logical ports on the P-GW S5
  // interface. This logical ports will connect to the PG-W user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the pgwS5Dev.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAttribute ("Mtu", UintegerValue (3000));
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = m_pgwSwitchDev->AddSwitchPort (pgwS5PortDev);
  uint32_t pgwS5PortNum = pgwS5Port->GetPortNo ();

  // Create the P-GW S5 user-plane application
  Mac48Address webSgiMacAddr, pgwSgiMacAddr;
  webSgiMacAddr = Mac48Address::ConvertFrom (webSgiDev->GetAddress ());
  pgwSgiMacAddr = Mac48Address::ConvertFrom (pgwSgiDev->GetAddress ());

  Ptr<PgwUserApp> pgwUserApp = CreateObject <PgwUserApp> (
      pgwS5PortDev, m_webSgiIpAddr, webSgiMacAddr, pgwSgiMacAddr);
  m_pgwNode->AddApplication (pgwUserApp);

  // FIXME Remove
  pgwUserApp->m_controlPlane = m_epcCtrlApp;

  // Adding the swS5Dev device as OpenFlow switch port.
  Ptr<OFSwitch13Device> swDev = GetSwitchDevice (swIdx);
  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
  uint32_t swS5PortNum = swS5Port->GetPortNo ();

  // Notify the controller of the the P-GW device attached to the Internet and
  // to the OpenFlow backhaul network.
  m_epcCtrlApp->NewS5Attach (pgwS5Dev, m_pgwS5Addr, swDev, swIdx, swS5PortNum);
  m_epcCtrlApp->NewSgiAttach (m_pgwSwitchDev, pgwSgiDev, m_pgwSgiAddr,
                              pgwSgiPortNum, pgwS5PortNum, m_webSgiIpAddr);

  // Configure P-GW and Internet link statistics
  m_pgwStats->SetQueues (pgwS5Dev->GetQueue (), swS5Dev->GetQueue ());
  m_webStats->SetQueues (webSgiDev->GetQueue (), pgwSgiDev->GetQueue ());

  // Setting the default P-GW gateway address.
  // This address will be used to set the static route at UEs.
  m_pgwGwAddr = m_ueAddrHelper.NewAddress ();
  NS_LOG_LOGIC ("P-GW gateway address: " << GetUeDefaultGatewayAddress ());
}

//
// Implementing methods inherited from EpcHelper
//
uint8_t
EpcNetwork::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi,
                               Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // Retrieve the IPv4 address of the UE and notify it to the S-GW;
  Ptr<Node> ueNode = ueDevice->GetNode ();
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed.");

  int32_t interface = ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  NS_LOG_LOGIC (" UE IP address: " << ueAddr);
  m_epcCtrlApp->SetUeAddress (imsi, ueAddr);

  uint8_t bearerId = GetMmeElement ()->AddBearer (imsi, tft, bearer);
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

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // add an IPv4 stack to the previously created eNB
  InternetStackHelper internet;
  internet.Install (enb);
  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB after node creation: " <<
                enb->GetObject<Ipv4> ()->GetNInterfaces ());

  // Callback the OpenFlow network to connect this eNB to the network.
  Ptr<NetDevice> enbDevice = S1EnbAttach (enb, cellId);
  m_s5Devices.Add (enbDevice);

  NS_LOG_LOGIC ("number of Ipv4 ifaces of the eNB after OpenFlow dev + Ipv4 addr: " <<
                enb->GetObject<Ipv4> ()->GetNInterfaces ());

  Ipv4Address enbAddress = GetAddressForDevice (enbDevice);
  Ipv4Address sgwAddress = GetSgwS1uAddress ();

  // create S1-U socket for the ENB
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = enbS1uSocket->Bind (InetSocketAddress (enbAddress, m_gtpuPort));
  NS_ASSERT (retval == 0);

  // create LTE socket for the ENB
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::PacketSocketFactory"));
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Bind (enbLteSocketBindAddress);
  NS_ASSERT (retval == 0);
  PacketSocketAddress enbLteSocketConnectAddress;
  enbLteSocketConnectAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress.SetProtocol (Ipv4L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Connect (enbLteSocketConnectAddress);
  NS_ASSERT (retval == 0);

  NS_LOG_INFO ("create EpcEnbApplication");
  Ptr<EpcEnbApplication> enbApp = CreateObject<EpcEnbApplication> (enbLteSocket, enbS1uSocket, enbAddress, sgwAddress, cellId);
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);
  NS_ASSERT_MSG (enb->GetApplication (0)->GetObject<EpcEnbApplication> () != 0, "cannot retrieve EpcEnbApplication");
  NS_LOG_LOGIC ("enb: " << enb << ", enb->GetApplication (0): " << enb->GetApplication (0));

  NS_LOG_INFO ("Create EpcX2 entity");
  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

  NS_LOG_INFO ("connect S1-AP interface");
  GetMmeElement ()->AddEnb (cellId, enbAddress, enbApp->GetS1apSapEnb ());
  m_epcCtrlApp->AddEnb (cellId, enbAddress, sgwAddress);
  enbApp->SetS1apSapMme (GetMmeElement ()->GetS1apSapMme ());
}

void
EpcNetwork::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // Callback the OpenFlow network to connect each eNB to the network.
  NetDeviceContainer enbDevices;
  enbDevices = X2Attach (enb1, enb2);
  m_x2Devices.Add (enbDevices);

  Ipv4Address enb1X2Address = GetAddressForDevice (enbDevices.Get (0));
  Ipv4Address enb2X2Address = GetAddressForDevice (enbDevices.Get (1));

  // Add X2 interface to both eNBs' X2 entities
  Ptr<EpcX2> enb1X2 = enb1->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb1LteDev = enb1->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb1CellId = enb1LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #1 = " << enb1LteDev << " - CellId = " << enb1CellId);

  Ptr<EpcX2> enb2X2 = enb2->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb2LteDev = enb2->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb2CellId = enb2LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #2 = " << enb2LteDev << " - CellId = " << enb2CellId);

  enb1X2->AddX2Interface (enb1CellId, enb1X2Address, enb2CellId, enb2X2Address);
  enb2X2->AddX2Interface (enb2CellId, enb2X2Address, enb1CellId, enb1X2Address);

  enb1LteDev->GetRrc ()->AddX2Neighbour (enb2LteDev->GetCellId ());
  enb2LteDev->GetRrc ()->AddX2Neighbour (enb1LteDev->GetCellId ());
}

void
EpcNetwork::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice );

  GetMmeElement ()->AddUe (imsi);
  m_epcCtrlApp->AddUe (imsi);
}

Ptr<Node>
EpcNetwork::GetPgwNode ()
{
  return m_pgwNode;
}

Ipv4InterfaceContainer
EpcNetwork::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  return m_ueAddrHelper.Assign (ueDevices);
}

Ipv4Address
EpcNetwork::GetUeDefaultGatewayAddress ()
{
  return m_pgwGwAddr;
}

Ptr<EpcMme>
EpcNetwork::GetMmeElement ()
{ // FIXME Vai pro controlador
  return m_epcCtrlApp->m_mme;
}

Ipv4Address
EpcNetwork::GetSgwS1uAddress ()
{
  // FIXME: quando mudar as interface e separar s5 e s1 vai ter que mudar aqui
  return m_pgwS5Addr;
}

Ipv4Address
EpcNetwork::GetSgwS5Address ()
{
  // TODO Ainda nao temos o sgw e as interfaces diferentes
  return Ipv4Address ("1.1.1.1");
}

Ipv4Address
EpcNetwork::GetAddressForDevice (Ptr<NetDevice> device)
{
  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}










};  // namespace ns3

