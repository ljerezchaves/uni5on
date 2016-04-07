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

#include "ring-network.h"
#include "ring-controller.h"
#include "connection-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingNetwork");
NS_OBJECT_ENSURE_REGISTERED (RingNetwork);

RingNetwork::RingNetwork ()
{
  NS_LOG_FUNCTION (this);

  // Configuring address helpers
  // Since we are using the OpenFlow network for S1-U links,
  // we use a /24 subnet which can hold up to 254 eNBs addresses on same subnet
  m_s1uAddrHelper.SetBase ("10.0.0.0", "255.255.255.0");

  // We are also using the OpenFlow network for all X2 links,
  // but we still we use a /30 subnet which can hold exactly two addresses
  m_x2AddrHelper.SetBase ("12.0.0.0", "255.255.255.252");
}

RingNetwork::~RingNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RingNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingNetwork")
    .SetParent<OpenFlowEpcNetwork> ()
    .AddConstructor<RingNetwork> ()
    .AddAttribute ("NumSwitches",
                   "The number of OpenFlow switches in the ring (at least 3).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (3),
                   MakeUintegerAccessor (&RingNetwork::m_numNodes),
                   MakeUintegerChecker<uint16_t> (3))
    .AddAttribute ("SwitchLinkDataRate",
                   "The data rate for the links between OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Mb/s")),
                   MakeDataRateAccessor (&RingNetwork::m_swLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("SwitchLinkDelay",
                   "The delay for the links between OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (100)), // 20km fiber cable latency
                   MakeTimeAccessor (&RingNetwork::m_swLinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("GatewayLinkDataRate",
                   "The data rate for the link connecting the gateway to the "
                   "OpenFlow network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&RingNetwork::m_gwLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("GatewayLinkDelay",
                   "The delay for the link connecting the gateway to the "
                   "OpenFlow network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&RingNetwork::m_gwLinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1500), // Ethernet II
                   MakeUintegerAccessor (&RingNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
RingNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  OpenFlowEpcNetwork::DoDispose ();
}

void
RingNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Configuring csma link helper for connection between switches
  m_swHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_swHelper.SetChannelAttribute ("DataRate", DataRateValue (m_swLinkRate));
  m_swHelper.SetChannelAttribute ("Delay", TimeValue (m_swLinkDelay));

  // Configuring csma links helper for connecting the gateway to the ring
  m_gwHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_gwHelper.SetChannelAttribute ("DataRate", DataRateValue (m_gwLinkRate));
  m_gwHelper.SetChannelAttribute ("Delay", TimeValue (m_gwLinkDelay));

  // Chain up (the topology creation will be triggered by base class)
  OpenFlowEpcNetwork::NotifyConstructionCompleted ();
}

void
RingNetwork::CreateTopology ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_numNodes >= 3, "Invalid number of nodes for the ring");

  // Creating the ring controller application
  InstallController (CreateObject<RingController> ());

  // Creating the switch nodes
  m_ofSwitches.Create (m_numNodes);
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      // Setting switch names
      std::ostringstream swName;
      swName << "sw" << i;
      Names::Add (swName.str (), m_ofSwitches.Get (i));
    }

  // Installing the Openflow switch devices for each switch node
  m_ofDevices = m_ofSwitchHelper->InstallSwitchesWithoutPorts (m_ofSwitches);

  // Connecting switches in ring topology (clockwise order)
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_numNodes;  // Next clockwise node

      // Creating a link between current and next node
      Ptr<Node> currNode = m_ofSwitches.Get (currIndex);
      Ptr<Node> nextNode = m_ofSwitches.Get (nextIndex);

      NodeContainer pair;
      pair.Add (currNode);
      pair.Add (nextNode);
      NetDeviceContainer devs = m_swHelper.Install (pair);

      // Setting interface names for pacp filename
      Names::Add (Names::FindName (currNode) + "+" +
                  Names::FindName (nextNode), devs.Get (0));
      Names::Add (Names::FindName (nextNode) + "+" +
                  Names::FindName (currNode), devs.Get (1));

      // Adding newly created csma devices as Openflow switch ports.
      Ptr<OFSwitch13Device> currDevice, nextDevice;
      Ptr<CsmaNetDevice> currPortDevice, nextPortDevice;
      uint32_t currPortNum, nextPortNum;

      currDevice = GetSwitchDevice (currIndex);
      currPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (0));
      currPortNum = currDevice->AddSwitchPort (currPortDevice)->GetPortNo ();

      nextDevice = GetSwitchDevice (nextIndex);
      nextPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (1));
      nextPortNum = nextDevice->AddSwitchPort (nextPortDevice)->GetPortNo ();

      // Switch order inside ConnectionInfo object must respect clockwise order
      // (RingController assume this order when installing switch rules).
      ConnectionInfo::SwitchData currSw = {
        currIndex, currDevice,
        currPortDevice, currPortNum
      };
      ConnectionInfo::SwitchData nextSw = {
        nextIndex, nextDevice,
        nextPortDevice, nextPortNum
      };
      Ptr<ConnectionInfo> cInfo = CreateObject<ConnectionInfo> (
          currSw, nextSw, DynamicCast<CsmaChannel> (
            currPortDevice->GetChannel ()));

      // Fire trace source notifying new connection between switches.
      m_newConnTrace (cInfo);
    }

  // Fire trace source notifying that all connections between switches are ok.
  m_topoBuiltTrace (m_ofDevices);
}

Ptr<NetDevice>
RingNetwork::S1Attach (Ptr<Node> node, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  // Connect the SgwPgw node to switch index 0 and other eNBs nodes to switches
  // indexes 1..N. The three eNBs from same cell site are connected to the same
  // switch in the ring network. The cellId == 0 is used by the SgwPgw node.

  uint16_t swIdx = 0;
  if (cellId == 0)
    {
      // This is the SgwPgw node
      RegisterGatewayAtSwitch (swIdx, node);
    }
  else
    {
      // This is an eNB
      swIdx = 1 + ((cellId - 1) / 3);
    }
  RegisterNodeAtSwitch (swIdx, node);
  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);

  // Creating a link between the switch and the node
  NetDeviceContainer devices;
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (node);
  if (cellId == 0)
    {
      // For the SgwPgw node, use the gateway helper with higher data rate
      devices = m_gwHelper.Install (pair);
    }
  else
    {
      // For an eNB node, use the default switch helper with default data rate
      devices = m_swHelper.Install (pair);
    }

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

  // Only for the gateway link, let's set queues for link statistics.
  if (cellId == 0)
    {
      m_gatewayStats->SetQueues (nodeDev->GetQueue (), portDev->GetQueue ());
    }

  return nodeDev;
}

NetDeviceContainer
RingNetwork::X2Attach (Ptr<Node> enb1, Ptr<Node> enb2)
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
RingNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on switch datapath
  m_swHelper.EnablePcap (prefix + "ofnetwork", m_ofSwitches, true);

  // Chain up
  OpenFlowEpcNetwork::EnablePcap (prefix, promiscuous);
}

};  // namespace ns3
