/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#include "sdran-cloud.h"
#include "epc-controller.h"
#include "epc-network.h"
#include "pgw-user-app.h"
#include "sdmn-mme.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdranCloud");
NS_OBJECT_ENSURE_REGISTERED (SdranCloud);

// Initializing SdranCloud static members.
uint32_t SdranCloud::m_enbCounter = 0;
uint32_t SdranCloud::m_sdranCounter = 0;
SdranCloud::NodeSdranMap_t SdranCloud::m_enbSdranMap;

SdranCloud::SdranCloud ()
{
  NS_LOG_FUNCTION (this);

  // Set the SDRAN cloud ID.
  m_sdranId = ++m_sdranCounter;

  // Create the S-GW node and set its name.
  m_sgwNode = CreateObject<Node> ();
  std::string sgwName = "sgw" + m_sdranId;
  Names::Add (sgwName, m_sgwNode);
}

SdranCloud::~SdranCloud ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdranCloud::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdranCloud")
    .SetParent<Object> ()
    .AddConstructor<SdranCloud> ()
    .AddAttribute ("NumSites", "The total number of cell sites managed by "
                   "this SDRAN cloud (each site has 3 eNBs).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SdranCloud::m_nSites),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("S1uNetworkAddr",
                   "The IPv4 network address used for S1-U devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("10.3.0.0")),
                   MakeIpv4AddressAccessor (&SdranCloud::m_s1uNetworkAddr),
                   MakeIpv4AddressChecker ())

    .AddAttribute ("LinkDataRate",
                   "The data rate for the link connecting the S-GW to the eNB.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&SdranCloud::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The delay for the link connecting the S-GW to the eNB.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&SdranCloud::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&SdranCloud::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

uint32_t
SdranCloud::GetId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sdranId;
}

uint32_t
SdranCloud::GetNSites (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nSites;
}

uint32_t
SdranCloud::GetNEnbs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nEnbs;
}

Ptr<Node>
SdranCloud::GetSgwNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwNode;
}

NodeContainer
SdranCloud::GetEnbNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbNodes;
}

Ptr<OFSwitch13Device>
SdranCloud::GetSgwSwitchDevice ()
{
  NS_LOG_FUNCTION (this);

  Ptr<OFSwitch13Device> devive = m_sgwNode->GetObject<OFSwitch13Device> ();
  NS_ASSERT_MSG (devive, "No OpenFlow device found for S-GW node");
  return devive;
}

//Ptr<NetDevice>
//SdranCloud::S1EnbAttach (Ptr<Node> node, uint16_t cellId)
//{
//  NS_LOG_FUNCTION (this << node << cellId);
//
//  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());
//
//  uint64_t dpId = TopologyGetEnbSwitch (cellId);
//  RegisterNodeAttachToSwitch (node, dpId);
//  Ptr<Node> swNode = GetSwitchNode (dpId);
//
//  // Creating a link between the switch and the node.
//  NetDeviceContainer devices;
//  NodeContainer pair;
//  pair.Add (swNode);
//  pair.Add (node);
//  devices = m_csmaHelper.Install (pair);
//  m_s1Devices.Add (devices); // FIXME isso ta certo?
//
//  Ptr<CsmaNetDevice> portDev, nodeDev;
//  portDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
//  nodeDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//  // Setting interface names for pacp filename.
//  Names::Add (Names::FindName (swNode) + "+" +
//              Names::FindName (node), portDev);
//  Names::Add (Names::FindName (node) + "+" +
//              Names::FindName (swNode), nodeDev);
//
//  // Set S1U IPv4 address for the new device at node.
//  Ipv4InterfaceContainer nodeIpIfaces =
//    m_s5AddrHelper.Assign (NetDeviceContainer (nodeDev));
//  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);
//
//  // Adding newly created csma device as openflow switch port.
//  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (dpId);
//  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
//  uint32_t portNum = swPort->GetPortNo ();
//
//  // Notify the controller of a new device attached to network.
//  m_epcCtrlApp->NewS5Attach (swDev, portNum, nodeDev, nodeAddr);
//
//  return nodeDev;
//}

void
SdranCloud::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice,
                    uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // Add an IPv4 stack to the previously created eNB
  InternetStackHelper internet;
  internet.Install (enb);

  // Creating a link between the eNB node and the S-GW node.
  NetDeviceContainer devices = m_csmaHelper.Install (m_sgwNode, enb);
  m_s1Devices.Add (devices);

  Ptr<CsmaNetDevice> sgwCsmaDevice, enbCsmaDevice;
  sgwCsmaDevice = DynamicCast<CsmaNetDevice> (devices.Get (0));
  enbCsmaDevice = DynamicCast<CsmaNetDevice> (devices.Get (1));

  // Setting interface names for pacp filename.
  Names::Add (Names::FindName (m_sgwNode) + "+" +
              Names::FindName (enb), sgwCsmaDevice);
  Names::Add (Names::FindName (enb) + "+" +
              Names::FindName (m_sgwNode), enbCsmaDevice);

  // Set S1-U IPv4 address for the devices.
  Ipv4InterfaceContainer s1uIpIfaces = m_s1uAddrHelper.Assign (devices);
  Ipv4Address sgwAddress = s1uIpIfaces.GetAddress (0);
  Ipv4Address enbAddress = s1uIpIfaces.GetAddress (1);
  m_s1uAddrHelper.NewNetwork ();

  // // Adding newly created csma device as openflow switch port.
  // Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (dpId);
  // Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (sgwCsmaDevice);
  // uint32_t portNum = swPort->GetPortNo ();

  // // Notify the controller of a new device attached to network.
  // m_epcCtrlApp->NewS1uAttach (swDev, portNum, enbCsmaDevice, nodeAddr);


  // Create the virtual net device to work as the logical ports on the S-GW S1-U
  // interface. This logical ports will connect to the S-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S1-U UDP socket binded to the sgwS5Dev.
  Ptr<VirtualNetDevice> sgwS1uPortDev = CreateObject<VirtualNetDevice> ();
  sgwS1uPortDev->SetAttribute ("Mtu", UintegerValue (3000));
  sgwS1uPortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> sgwS1uPort = GetSgwSwitchDevice ()->AddSwitchPort (sgwS1uPortDev);
  // uint32_t sgwS1uPortNum = sgwS1uPort->GetPortNo (); // FIXME

  // Create the P-GW S5 user-plane application.
  Ptr<PgwUserApp> sgwUserApp = CreateObject <PgwUserApp> (sgwS1uPortDev);
  m_sgwNode->AddApplication (sgwUserApp);

  // Notify the controller...





  // Create the S1-U socket for the eNB
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (
      enb, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  enbS1uSocket->Bind (InetSocketAddress (enbAddress, EpcNetwork::m_gtpuPort));

  // Create the LTE socket for the eNB
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

  // Create the eNB application
  Ptr<EpcEnbApplication> enbApp = CreateObject<EpcEnbApplication> (
      enbLteSocket, enbS1uSocket, enbAddress, sgwAddress, cellId);
  enb->AddApplication (enbApp);

  NS_ASSERT_MSG (enb->GetNApplications () == 1,
                 "Don't install other applications on eNB.");
  NS_ASSERT_MSG (enb->GetApplication (0)->GetObject<EpcEnbApplication> () != 0,
                 "Cannot retrieve EpcEnbApplication from eNB.");

  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

  // Create the eNB info.
  Ptr<EnbInfo> enbInfo = CreateObject<EnbInfo> (cellId);
  enbInfo->SetEnbAddress (enbAddress);
  enbInfo->SetSgwAddress (sgwAddress);
  enbInfo->SetS1apSapEnb (enbApp->GetS1apSapEnb ());

  enbApp->SetS1apSapMme (SdmnMme::Get ()->GetS1apSapMme ());
}


// NetDeviceContainer
// SdranCloud::X2Attach (Ptr<Node> enb1, Ptr<Node> enb2)
// {
//   NS_LOG_FUNCTION (this << enb1 << enb2);
//
//   NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());
//
//   // Create a P2P connection between the eNBs.
//   PointToPointHelper p2ph;
//   p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
//   p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2000));
//   p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0)));
//   NetDeviceContainer enbDevices =  p2ph.Install (enb1, enb2);
//
//   // // TODO Creating a link between the firts eNB and its switch
//   // uint16_t swIdx1 = GetSwitchIdxForNode (enb1);
//   // Ptr<Node> swNode1 = m_ofSwitches.Get (swIdx1);
//   // Ptr<OFSwitch13Device> swDev1 = GetSwitchDevice (swIdx1);
//
//   // NodeContainer pair1;
//   // pair1.Add (swNode1);
//   // pair1.Add (enb1);
//   // NetDeviceContainer devices1 = m_swHelper.Install (pair1);
//
//   // Ptr<CsmaNetDevice> portDev1, enbDev1;
//   // portDev1 = DynamicCast<CsmaNetDevice> (devices1.Get (0));
//   // enbDev1 = DynamicCast<CsmaNetDevice> (devices1.Get (1));
//
//   // // Creating a link between the second eNB and its switch
//   // uint16_t swIdx2 = GetSwitchIdxForNode (enb2);
//   // Ptr<Node> swNode2 = m_ofSwitches.Get (swIdx2);
//   // Ptr<OFSwitch13Device> swDev2 = GetSwitchDevice (swIdx2);
//
//   // NodeContainer pair2;
//   // pair2.Add (swNode2);
//   // pair2.Add (enb2);
//   // NetDeviceContainer devices2 = m_swHelper.Install (pair2);
//
//   // Ptr<CsmaNetDevice> portDev2, enbDev2;
//   // portDev2 = DynamicCast<CsmaNetDevice> (devices2.Get (0));
//   // enbDev2 = DynamicCast<CsmaNetDevice> (devices2.Get (1));
//
//   // // Set X2 IPv4 address for the new devices
//   // NetDeviceContainer enbDevices;
//   // enbDevices.Add (enbDev1);
//   // enbDevices.Add (enbDev2);
//
//   Ipv4InterfaceContainer nodeIpIfaces = m_x2AddrHelper.Assign (enbDevices);
//   m_x2AddrHelper.NewNetwork ();
//   // Ipv4Address nodeAddr1 = nodeIpIfaces.GetAddress (0);
//   // Ipv4Address nodeAddr2 = nodeIpIfaces.GetAddress (1);
//
//   // // Adding newly created csma devices as openflow switch ports.
//   // Ptr<OFSwitch13Port> swPort1 = swDev1->AddSwitchPort (portDev1);
//   // uint32_t portNum1 = swPort1->GetPortNo ();
//
//   // Ptr<OFSwitch13Port> swPort2 = swDev2->AddSwitchPort (portDev2);
//   // uint32_t portNum2 = swPort2->GetPortNo ();
//
//   // // Trace source notifying new devices attached to the network
//   // m_newAttachTrace (enbDev1, nodeAddr1, swDev1, swIdx1, portNum1);
//   // m_newAttachTrace (enbDev2, nodeAddr2, swDev2, swIdx2, portNum2);
//
//   return enbDevices;
// }

void
SdranCloud::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

//  // Callback the OpenFlow network to connect each eNB to the network.
//  NetDeviceContainer enbDevices;
//  enbDevices = X2Attach (enb1, enb2);
//  m_x2Devices.Add (enbDevices);
//
//  Ipv4Address enb1X2Address = GetAddressForDevice (enbDevices.Get (0));
//  Ipv4Address enb2X2Address = GetAddressForDevice (enbDevices.Get (1));
//
//  // Add X2 interface to both eNBs' X2 entities
//  Ptr<EpcX2> enb1X2 = enb1->GetObject<EpcX2> ();
//  Ptr<LteEnbNetDevice> enb1LteDev = enb1->GetDevice (0)->GetObject<LteEnbNetDevice> ();
//  uint16_t enb1CellId = enb1LteDev->GetCellId ();
//  NS_LOG_DEBUG ("LteEnbNetDevice #1 = " << enb1LteDev << " - CellId = " << enb1CellId);
//
//  Ptr<EpcX2> enb2X2 = enb2->GetObject<EpcX2> ();
//  Ptr<LteEnbNetDevice> enb2LteDev = enb2->GetDevice (0)->GetObject<LteEnbNetDevice> ();
//  uint16_t enb2CellId = enb2LteDev->GetCellId ();
//  NS_LOG_DEBUG ("LteEnbNetDevice #2 = " << enb2LteDev << " - CellId = " << enb2CellId);
//
//  enb1X2->AddX2Interface (enb1CellId, enb1X2Address, enb2CellId, enb2X2Address);
//  enb2X2->AddX2Interface (enb2CellId, enb2X2Address, enb1CellId, enb1X2Address);
//
//  enb1LteDev->GetRrc ()->AddX2Neighbour (enb2LteDev->GetCellId ());
//  enb2LteDev->GetRrc ()->AddX2Neighbour (enb1LteDev->GetCellId ());
}

void
SdranCloud::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sgwNode = 0;
}

void
SdranCloud::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("SDRAN cloud with " << m_nSites << " cell sites.");

  // Set the number of eNBs based on the number of cell sites.
  m_nEnbs = 3 * m_nSites;

  // Create the eNBs nodes and set their names.
  m_enbNodes.Create (m_nEnbs);
  NodeContainer::Iterator it;
  for (it = m_enbNodes.Begin (); it != m_enbNodes.End (); ++it)
    {
      std::ostringstream enbName;
      enbName << "enb" << ++m_enbCounter;
      Names::Add (enbName.str (), *it);
    }

  // Set the constant mobility model for eNB positioning
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (m_enbNodes);

  // Configuring CSMA helper for connecting EPC nodes (P-GW and S-GWs) to the
  // backhaul topology. This same helper will be used to connect the P-GW to
  // the server node on the Internet.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Configure the S1-U address helper.
  m_s1uAddrHelper.SetBase (m_s1uNetworkAddr, "255.255.255.0");

  // Register this object and chain up
  RegisterSdranCloud (Ptr<SdranCloud> (this));
  Object::NotifyConstructionCompleted ();
}

Ipv4Address
SdranCloud::GetAddressForDevice (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}

Ptr<SdranCloud>
SdranCloud::GetPointer (Ptr<Node> enb)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<SdranCloud> sdran = 0;
  NodeSdranMap_t::iterator ret;
  ret = m_enbSdranMap.find (enb);
  if (ret != m_enbSdranMap.end ())
    {
      sdran = ret->second;
    }
  return sdran;
}

void
SdranCloud::RegisterSdranCloud (Ptr<SdranCloud> sdran)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Saving map by eNB node.
  NodeContainer enbs = sdran->GetEnbNodes ();
  for (NodeContainer::Iterator it = enbs.Begin (); it != enbs.End (); ++it)
    {
      std::pair<NodeSdranMap_t::iterator, bool> ret;
      std::pair<Ptr<Node>, Ptr<SdranCloud> > entry (*it, sdran);
      ret = m_enbSdranMap.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Can't register eNB at SDRAN cloud.");
        }
    }
}

} // namespace ns3
