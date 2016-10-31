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
#include "internet-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (EpcNetwork);

EpcNetwork::EpcNetwork ()
  : m_ofSwitchHelper (0),
    m_gatewayStats (0),
    m_epcCtrlNode (0),
    m_epcCtrlApp (0),
    m_webNetwork (0),
    m_networkStats (0)
{
  NS_LOG_FUNCTION (this);

  // Let's use point to point connections for OpenFlow channel
  m_ofSwitchHelper = CreateObject<OFSwitch13Helper> ();
  m_ofSwitchHelper->SetAttribute ("ChannelType",
                                  EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Configuring address helpers
  // Since we are using the OpenFlow network for S1-U links,
  // we use a /24 subnet which can hold up to 254 eNBs addresses on same subnet
  m_s1uAddrHelper.SetBase ("10.0.0.0", "255.255.255.0");

  // We are also using the OpenFlow network for all X2 links,
  // but we still we use a /30 subnet which can hold exactly two addresses
  m_x2AddrHelper.SetBase ("12.0.0.0", "255.255.255.252");

  // We use a /8 subnet for all UEs and the P-GW gateway address
  m_ueAddressHelper.SetBase ("7.0.0.0", "255.0.0.0", "0.0.0.2");

  // Creating some EPC nodes
  m_epcCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_epcCtrlNode);

  m_pgwNode = CreateObject<Node> ();
  Names::Add ("pgw", m_pgwNode);

  // Creating the Internet network object
  m_webNetwork = CreateObject<InternetNetwork> ();

  // Creating stats calculators
  // ObjectFactory statsFactory;
  // statsFactory.SetTypeId (LinkQueuesStatsCalculator::GetTypeId ());
  // statsFactory.Set ("LnkStatsFilename", StringValue ("pgw_stats.txt"));
  // m_gatewayStats = statsFactory.Create<LinkQueuesStatsCalculator> ();
  m_gatewayStats = CreateObjectWithAttributes<LinkQueuesStatsCalculator> (
    "LnkStatsFilename", StringValue ("pgw_stats.txt"));

  m_networkStats = CreateObject<NetworkStatsCalculator> ();
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

    // Attributes for connecting the EPC entities to the backhaul network
    .AddAttribute ("GatewayLinkDataRate",
                   "The data rate for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&EpcNetwork::m_gwLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("GatewayLinkDelay",
                   "The delay for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&EpcNetwork::m_gwLinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1500), // Ethernet II
                   MakeUintegerAccessor (&EpcNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())

    // Trace sources used by controller to be aware of backhaul network
    .AddTraceSource ("NewEpcAttach",
                     "New LTE EPC entity connected to the OpenFlow switch for "
                     "S1-U or X2 interface.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_newAttachTrace),
                     "ns3::EpcNetwork::AttachTracedCallback")
    .AddTraceSource ("NewSwitchConnection",
                     "New connection between two OpenFlow switches during "
                     "topology creation.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_newConnTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
    .AddTraceSource ("TopologyBuilt",
                     "OpenFlow networp topology built, with no more "
                     "connections between OpenFlow switches.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_topoBuiltTrace),
                     "ns3::EpcNetwork::TopologyTracedCallback")
  ;
  return tid;
}

void
EpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on Internet network
  m_webNetwork->EnablePcap (prefix + "internet");

  // Enable pcap on OpenFlow channel
  m_ofSwitchHelper->EnableOpenFlowPcap (prefix + "ofchannel");

  // Enable pcap on LTE EPC interfaces
  CsmaHelper helper;
  helper.EnablePcap (prefix + "lte-epc-s5", m_pgwS5Dev,   promiscuous);
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
EpcNetwork::GetServerNode ()
{
  // FIXME Temporiariamente salvando o nó servidor nesta classe
  return m_webNetwork->GetServerNode ();
  // return m_webNode;
}

Ptr<Node>
EpcNetwork::GetControllerNode ()
{
  return m_epcCtrlNode;
}

Ptr<EpcController>
EpcNetwork::GetControllerApp ()
{
  return m_epcCtrlApp;
}

void
EpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_epcCtrlNode = 0;
  m_epcCtrlApp = 0;
  m_ofSwitchHelper = 0;
  m_gatewayStats = 0;
  m_webNetwork = 0;
  m_networkStats = 0;
  m_nodeSwitchMap.clear ();

  // FIXME to sdmn-epc-helper
  m_tunDevice->SetSendCallback (MakeNullCallback<bool, Ptr<Packet>, const Address&, const Address&, uint16_t> ());
  m_tunDevice = 0;
  m_sgwPgwUserApp = 0;
  m_pgwNode->Dispose ();


  Object::DoDispose ();
}

void
EpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Connect the network stats calculator *before* topology creation.
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &NetworkStatsCalculator::NotifyTopologyBuilt, m_networkStats));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &NetworkStatsCalculator::NotifyNewSwitchConnection, m_networkStats));

  // Configuring csma links helper for connecting the P-GW to the ring
  m_gwHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_gwHelper.SetChannelAttribute ("DataRate", DataRateValue (m_gwLinkRate));
  m_gwHelper.SetChannelAttribute ("Delay", TimeValue (m_gwLinkDelay));

  
  // Create the ring network topology and Internet topology
  TopologyCreate ();


  // Configuring the P-GW (after controller)
  InstallPgw ();



  // FIXME A criação da topologia acontece dentro do Create Topology, na hora que chama o install Pgw.
   m_webNetwork->CreateTopology (GetPgwNode ());

  // Connect S1U and X2 connection callbacks *after* topology creation.
  // Connecting the P-GW to the OpenFlow network.
  m_pgwS5Dev = S5PgwAttach (m_pgwNode);
  NS_LOG_LOGIC ("Sgw S5 interface address: " << GetSgwS1uAddress ());

  // Connect the controller to the MME SessionCreated trace source *after*
  // topology creation.
  GetMmeElement ()->TraceConnectWithoutContext (
    "SessionCreated", MakeCallback (
      &EpcController::NotifySessionCreated, m_epcCtrlApp));

  // Connect the switches to the controller. From this point on it is not
  // possible anymor to add more ports to OpenFlow switches.
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
  m_pgwSwitchIdx = switchIdx;
  RegisterNodeAtSwitch (switchIdx, node);
}

Ptr<OFSwitch13Device>
EpcNetwork::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return m_ofDevices.Get (index);
}

uint16_t
EpcNetwork::GetSwitchIdxForNode (Ptr<Node> node)
{
  NodeSwitchMap_t::iterator ret;
  ret = m_nodeSwitchMap.find (node);
  if (ret != m_nodeSwitchMap.end ())
    {
      NS_LOG_DEBUG ("Found switch " << (int)ret->second << " for " << node);
      return ret->second;
    }
  NS_FATAL_ERROR ("Node not registered.");
}

uint16_t
EpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13Device> dev)
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
EpcNetwork::GetGatewaySwitchIdx ()
{
  return m_pgwSwitchIdx;
}

void
EpcNetwork::InstallController (Ptr<EpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_epcCtrlApp, "Controller application already set.");

  // Installing the controller application into controller node
  m_epcCtrlApp = controller;
  m_ofSwitchHelper->InstallController (m_epcCtrlNode, m_epcCtrlApp);

  // FIXME.... 
  m_epcCtrlNode->AddApplication (m_epcCtrlApp->m_pgwCtrlApp);

  // Connecting controller trace sinks to sources in this network
  TraceConnectWithoutContext (
    "NewEpcAttach", MakeCallback (
      &EpcController::NotifyNewEpcAttach, m_epcCtrlApp));
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &EpcController::NotifyTopologyBuilt, m_epcCtrlApp));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &EpcController::NotifyNewSwitchConnection, m_epcCtrlApp));
}



// TODO Acabei de trazer isso aqui do ring pra cá.
Ptr<NetDevice>
EpcNetwork::S5PgwAttach (Ptr<Node> pgwNode)
{
  NS_LOG_FUNCTION (this << pgwNode);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  uint16_t swIdx = TopologyGetSwIndexPgw ();
  RegisterGatewayAtSwitch (swIdx, pgwNode);
  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);

  // Creating a link between the switch and the node
  NetDeviceContainer devices;
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (pgwNode);
  devices = m_gwHelper.Install (pair);

  Ptr<CsmaNetDevice> portDev, nodeDev;
  portDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  nodeDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  // Setting interface names for pacp filename
  Names::Add (Names::FindName (swNode) + "+" +
              Names::FindName (pgwNode), portDev);
  Names::Add (Names::FindName (pgwNode) + "+" +
              Names::FindName (swNode), nodeDev);

  // Set S1U IPv4 address for the new device at node
  Ipv4InterfaceContainer nodeIpIfaces =
    m_s1uAddrHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);

  // Adding newly created csma device as openflow switch port.
  Ptr<OFSwitch13Device> swDev = GetSwitchDevice (swIdx);
  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
  uint32_t portNum = swPort->GetPortNo ();

  // Trace source notifying a new device attached to network
  m_newAttachTrace (nodeDev, nodeAddr, swDev, swIdx, portNum);

  // Let's set queues for link statistics.
  m_gatewayStats->SetQueues (nodeDev->GetQueue (), portDev->GetQueue ());

  return nodeDev;
}

Ptr<NetDevice>
EpcNetwork::S1EnbAttach (Ptr<Node> node, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  uint16_t swIdx = TopologyGetSwIndexEnb (cellId);
  RegisterNodeAtSwitch (swIdx, node);
  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);

  // Creating a link between the switch and the node
  NetDeviceContainer devices;
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (node);
  devices = m_gwHelper.Install (pair);  // FIXME Vai usar mesmo o gwHelper?

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
    m_s1uAddrHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);

  // Adding newly created csma device as openflow switch port.
  Ptr<OFSwitch13Device> swDev = GetSwitchDevice (swIdx);
  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
  uint32_t portNum = swPort->GetPortNo ();

  // Trace source notifying a new device attached to network
  m_newAttachTrace (nodeDev, nodeAddr, swDev, swIdx, portNum);

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



//----------------------------------------------------------------------------------------------------------


void
EpcNetwork::InstallPgw ()
{
  NS_LOG_FUNCTION (this);



//////  // TODO: Esta aqui é a conexão do PGW com a internet.
//////  CsmaHelper csmaHelper; 
//////  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492));
//////  csmaHelper.SetChannelAttribute ("DataRate",
//////                                    DataRateValue (DataRate ("10Gb/s")));
//////  csmaHelper.SetChannelAttribute ("Delay", TimeValue (Seconds (0)));
//////
//////  // Creating a single web node and connecting it to the EPC PGW node
//////  Ptr<Node> m_webNode = CreateObject<Node> ();
//////  Names::Add ("srv", m_webNode);
//////
//////  InternetStackHelper internet;
//////  internet.Install (m_webNode);
//////
//////  NodeContainer webNodes;
//////  webNodes.Add (m_pgwNode);
//////  webNodes.Add (m_webNode);
//////
//////  NetDeviceContainer webDevices;
//////  webDevices = csmaHelper.Install (webNodes);
//////  
//////  Ptr<CsmaNetDevice> webDev, pgwDev;
//////  pgwDev = DynamicCast<CsmaNetDevice> (webDevices.Get (0));
//////  webDev = DynamicCast<CsmaNetDevice> (webDevices.Get (1));
//////
//////  Names::Add (Names::FindName (m_pgwNode) + "+" +
//////              Names::FindName (m_webNode), pgwDev);
//////  Names::Add (Names::FindName (m_webNode) + "+" +
//////              Names::FindName (m_pgwNode), webDev);
//////
//////  // FIXME
//////  // m_internetStats->SetQueues (webDev->GetQueue (), pgwDev->GetQueue ());
//////
//////  Ipv4AddressHelper ipv4h;
//////  ipv4h.SetBase ("192.168.0.0", "255.255.255.0");
//////  ipv4h.Assign (NetDeviceContainer (pgwDev));  // No IP address to the P-GW device
//////
//////  // Defining static routes at the web node to the LTE network
//////  Ipv4StaticRoutingHelper ipv4RoutingHelper;
//////  Ptr<Ipv4StaticRouting> webHostStaticRouting =
//////    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
//////  webHostStaticRouting->AddNetworkRouteTo (
//////    Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"),
//////    Ipv4Address ("192.168.0.1"), 1);
//////
//////  // Configure the P-GW node as an OpenFlow switch
//////  OFSwitch13DeviceContainer pgwSwitchDevice;
//////  pgwSwitchDevice = m_ofSwitchHelper->InstallSwitch (m_pgwNode);
//////
//////  // Adding to the switch the port with Internet connection
//////  pgwSwitchDevice.Get (0)->AddSwitchPort (pgwDev);
//////
//////  // Ok. A porta tá la... configurar as regras pro tráfego.
//////
//////  // Agora precisa conectar o switch ao backhaul
//////  
//////
//////  return;

  // FIXME Isso aqui vai sumir no futuro, porque o switch já vai ter o stack
  // instalado pelo helper do ofswitch13
   InternetStackHelper internet;
  internet.Install (m_pgwNode);

  // create S1-U socket
  Ptr<Socket> sgwPgwS1uSocket = Socket::CreateSocket (m_pgwNode, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = sgwPgwS1uSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_gtpuUdpPort));
  NS_ASSERT (retval == 0);

  // create TUN device implementing tunneling of user data over GTP-U/UDP/IP
  m_tunDevice = CreateObject<VirtualNetDevice> ();
  // allow jumbo packets
  m_tunDevice->SetAttribute ("Mtu", UintegerValue (30000));

  // yes we need this
  m_tunDevice->SetAddress (Mac48Address::Allocate ());

  m_pgwNode->AddDevice (m_tunDevice);
  NetDeviceContainer tunDeviceContainer;
  tunDeviceContainer.Add (m_tunDevice);

  // the TUN device is on the same subnet as the UEs, so when a packet
  // addressed to an UE arrives at the intenet to the WAN interface of
  // the PGW it will be forwarded to the TUN device.
  Ipv4InterfaceContainer tunDeviceIpv4IfContainer = m_ueAddressHelper.Assign (tunDeviceContainer);

  // create EpcSgwPgwApplication
  NS_ASSERT (m_epcCtrlApp);
//  m_epcCtrlApp->m_pgwCtrlApp = CreateObject<EpcSgwPgwCtrlApplication> ();
  m_sgwPgwUserApp = CreateObject<EpcSgwPgwUserApplication> (m_tunDevice, sgwPgwS1uSocket, m_epcCtrlApp->m_pgwCtrlApp);
//  Names::Add ("SgwPgwApplication", m_epcCtrlApp->m_pgwCtrlApp);
//  m_pgwNode->AddApplication (m_epcCtrlApp->m_pgwCtrlApp);
  m_pgwNode->AddApplication (m_sgwPgwUserApp);

  // connect SgwPgwApplication and virtual net device for tunneling
  m_tunDevice->SetSendCallback (MakeCallback (&EpcSgwPgwUserApplication::RecvFromTunDevice, m_sgwPgwUserApp));

  // Create MME and connect with SGW via S11 interface
//  m_epcCtrlApp->m_mme = CreateObject<EpcMme> ();
//  GetMmeElement ()->SetS11SapSgw (m_epcCtrlApp->m_pgwCtrlApp->GetS11SapSgw ());
//  m_epcCtrlApp->m_pgwCtrlApp->SetS11SapMme (GetMmeElement ()->GetS11SapMme ());


}

void
EpcNetwork::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId)
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
  int retval = enbS1uSocket->Bind (InetSocketAddress (enbAddress, m_gtpuUdpPort));
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
  m_epcCtrlApp->m_pgwCtrlApp->AddEnb (cellId, enbAddress, sgwAddress);
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
  m_epcCtrlApp->m_pgwCtrlApp->AddUe (imsi);
}

uint8_t
EpcNetwork::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // we now retrieve the IPv4 address of the UE and notify it to the SGW;
  // we couldn't do it before since address assignment is triggered by
  // the user simulation program, rather than done by the EPC
  Ptr<Node> ueNode = ueDevice->GetNode ();
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed before EPS bearers can be activated");
  int32_t interface =  ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  NS_LOG_LOGIC (" UE IP address: " << ueAddr);
  m_epcCtrlApp->m_pgwCtrlApp->SetUeAddress (imsi, ueAddr);

  uint8_t bearerId = GetMmeElement ()->AddBearer (imsi, tft, bearer);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
    }
  return bearerId;
}

Ptr<Node>
EpcNetwork::GetPgwNode ()
{
  return m_pgwNode;
}

Ipv4InterfaceContainer
EpcNetwork::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  return m_ueAddressHelper.Assign (ueDevices);
}

Ipv4Address
EpcNetwork::GetUeDefaultGatewayAddress ()
{ // FIXME Tirar a dependencia em ser o primeiro device... talvez usar o GetAddressForDevice
  // return the address of the tun device
  return m_pgwNode->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
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
  return GetAddressForDevice (m_pgwS5Dev);
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
  int32_t idx = ipv4->GetInterfaceForDevice(device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}










};  // namespace ns3

