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

#include "ring-openflow-network.h"

NS_LOG_COMPONENT_DEFINE ("RingOpenFlowNetwork");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RingOpenFlowNetwork);

// Initializing RingOpenFlowNetwork static members


RingOpenFlowNetwork::RingOpenFlowNetwork ()
{
  NS_LOG_FUNCTION (this);
  
  // since we are using the OpenFlow network for S1-U links,
  // we use a /24 subnet which can hold up to 254 eNBs addresses on same subnet
  m_s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.0");

  // we are also using the OpenFlow network for all X2 links, 
  // but we still we use a /30 subnet which can hold exactly two addresses
  m_x2Ipv4AddressHelper.SetBase ("12.0.0.0", "255.255.255.252");
}

RingOpenFlowNetwork::~RingOpenFlowNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
RingOpenFlowNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::RingOpenFlowNetwork") 
    .SetParent<OpenFlowEpcNetwork> ()
    .AddConstructor<RingOpenFlowNetwork> ()
    .AddAttribute ("NumSwitches", 
                   "The number of OpenFlow switches in the ring (at least 3).",
                   UintegerValue (3),
                   MakeUintegerAccessor (&RingOpenFlowNetwork::m_nodes),
                   MakeUintegerChecker<uint16_t> (3))
    .AddAttribute ("LinkDataRate", 
                   "The data rate to be used for the CSMA OpenFlow links to be created",
                   DataRateValue (DataRate ("10Mb/s")),
                   MakeDataRateAccessor (&RingOpenFlowNetwork::m_LinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The delay to be used for the CSMA OpenFlow links to be created",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&RingOpenFlowNetwork::m_LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu", 
                   "The MTU for CSMA OpenFlow links. Use at least 1500 bytes.",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&RingOpenFlowNetwork::m_LinkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid; 
}

void
RingOpenFlowNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  OpenFlowEpcNetwork::DoDispose ();
  Object::DoDispose ();
  m_ringCtrlApp = 0;
}

void
RingOpenFlowNetwork::CreateInternalTopology ()
{
  NS_LOG_FUNCTION (this);

  // Validating controller and number of switches in the ring
  m_ringCtrlApp = DynamicCast<RingController> (m_ofCtrlApp);
  NS_ASSERT_MSG (m_ringCtrlApp, "Expecting a RingController.");
  NS_ASSERT_MSG (m_nodes >= 3, "Invalid number of nodes for the ring");

  // Creating the switch nodes
  m_ofSwitches.Create (m_nodes);
  for (uint16_t i = 0; i < m_nodes; i++)
    {
      // Setting switch names
      std::ostringstream swName;
      swName << "sw" << i;
      Names::Add (swName.str(), m_ofSwitches.Get (i));
    }

  // Installing the Openflow switch devices for each switch node
  m_ofDevices = m_ofHelper.InstallSwitchesWithoutPorts (m_ofSwitches);

  // Configuring csma links to connect the switches
  m_ofCsmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_LinkDataRate));
  m_ofCsmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_LinkMtu));
  m_ofCsmaHelper.SetChannelAttribute ("Delay", TimeValue (m_LinkDelay));

  // For more than 3 nodes, connecting switches in ring topology (clockwise order)
  for (uint16_t i = 0; i < m_nodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_nodes;  // Next node in clockwise direction

      // Creating a link between current and next node
      NodeContainer pair;
      pair.Add (m_ofSwitches.Get (currIndex));
      pair.Add (m_ofSwitches.Get (nextIndex));
      NetDeviceContainer devs = m_ofCsmaHelper.Install (pair);
    
      // Adding newly created csma devices as openflow switch ports.
      Ptr<OFSwitch13NetDevice> currDevice = GetSwitchDevice (currIndex);
      Ptr<CsmaNetDevice> currPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (0));
      uint32_t currPortNum = currDevice->AddSwitchPort (currPortDevice);

      Ptr<OFSwitch13NetDevice> nextDevice = GetSwitchDevice (nextIndex);
      Ptr<CsmaNetDevice> nextPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (1));
      uint32_t nextPortNum = nextDevice->AddSwitchPort (nextPortDevice);

      // Notify the ring controller of this new connection.
      ConnectionInfo info = {
        .switchIdx1 = currIndex,
        .switchIdx2 = nextIndex,
        .switchDev1 = currDevice,
        .switchDev2 = nextDevice,
        .portDev1 = currPortDevice,
        .portDev2 = nextPortDevice,
        .portNum1 = currPortNum,
        .portNum2 = nextPortNum,
        .nominalDataRate = m_LinkDataRate,
        .availableDataRate = m_LinkDataRate
      };
      m_ringCtrlApp->NotifyNewSwitchConnection (info);
    }
  m_ringCtrlApp->CreateSpanningTree ();
}

Ptr<NetDevice>
RingOpenFlowNetwork::AttachToS1u (Ptr<Node> node, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());
  
  // Connect SgwPgw node to switch index 0 and other eNBs to switchs indexes 1
  // through m_nodes - 1, in turns. In the case of a single node in the ring,
  // connect all gateways and eNBs to it. As we know that the OpenFlowEpcHelper
  // will callback here first for SgwPgw node, we use the static counter to
  // identify this node.
  static uint32_t counter = 0;
  uint16_t switchIdx;
  
  if (counter == 0)
    {
      switchIdx = 0;
      
      // This is the SgwPgw node
      RegisterNodeAtSwitch (switchIdx, node);
      RegisterGatewayAtSwitch (switchIdx);
    }
  else  
    {
      switchIdx = 1 + ((counter - 1) % (m_nodes - 1));
      
      // This is an eNB node
      RegisterNodeAtSwitch (switchIdx, node);
      RegisterCellIdAtSwitch (switchIdx, cellId);
    }
  counter++;

  Ptr<Node> swtchNode = m_ofSwitches.Get (switchIdx);
  Ptr<OFSwitch13NetDevice> swtchDev = GetSwitchDevice (switchIdx); 
 
  // Creating a link between switch and node
  NodeContainer pair;
  pair.Add (swtchNode);
  pair.Add (node);
  NetDeviceContainer devices = m_ofCsmaHelper.Install (pair);
  
  // Set S1U IPv4 address for the new device at node
  Ptr<NetDevice> nodeDev = devices.Get (1);
  Ipv4InterfaceContainer nodeIpIfaces = 
      m_s1uIpv4AddressHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeIpAddress = nodeIpIfaces.GetAddress (0);

  // Adding newly created csma device as openflow switch port.
  uint32_t portNum = swtchDev->AddSwitchPort (devices.Get (0));

  // Notify controller of a new IP device and configure local traffic delivery.
  m_ringCtrlApp->NotifyNewIpDevice (nodeDev, nodeIpAddress);
  m_ringCtrlApp->ConfigurePortDelivery (swtchDev, nodeDev, nodeIpAddress, portNum);

  return nodeDev;
}

Ptr<NetDevice>
RingOpenFlowNetwork::AttachToX2 (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  // Retrieve the registered pair node/switch
  uint16_t switchIdx = GetSwitchIdxForNode (node);
  NS_ASSERT (switchIdx < m_ofDevices.GetN ());

  Ptr<Node> swtchNode = m_ofSwitches.Get (switchIdx);
  Ptr<OFSwitch13NetDevice> swtchDev = GetSwitchDevice (switchIdx); 

  // Creating a link between switch and node
  NodeContainer pair;
  pair.Add (swtchNode);
  pair.Add (node);
  NetDeviceContainer devices = m_ofCsmaHelper.Install (pair);

  // Set X2 IPv4 address for the new device at node
  Ptr<NetDevice> nodeDev = devices.Get (1);
  Ipv4InterfaceContainer nodeIpIfaces = 
      m_x2Ipv4AddressHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeIpAddress = nodeIpIfaces.GetAddress (0);
  m_x2Ipv4AddressHelper.NewNetwork ();
  
  // Adding newly created csma device as openflow switch port.
  uint32_t portNum = swtchDev->AddSwitchPort (devices.Get (0));

  // Notify controller of a new IP device and configure flow table entry for
  // local traffic delivery.
  m_ringCtrlApp->NotifyNewIpDevice (nodeDev, nodeIpAddress);
  m_ringCtrlApp->ConfigurePortDelivery (swtchDev, nodeDev, nodeIpAddress, portNum);

  return nodeDev;
}

};  // namespace ns3
