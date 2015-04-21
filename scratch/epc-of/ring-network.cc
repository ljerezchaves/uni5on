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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingNetwork");
NS_OBJECT_ENSURE_REGISTERED (RingNetwork);

RingNetwork::RingNetwork ()
{
  NS_LOG_FUNCTION (this);
  
  // since we are using the OpenFlow network for S1-U links,
  // we use a /24 subnet which can hold up to 254 eNBs addresses on same subnet
  m_s1uIpv4AddressHelper.SetBase ("10.0.0.0", "255.255.255.0");

  // we are also using the OpenFlow network for all X2 links, 
  // but we still we use a /30 subnet which can hold exactly two addresses
  m_x2Ipv4AddressHelper.SetBase ("12.0.0.0", "255.255.255.252");
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
                   UintegerValue (3),
                   MakeUintegerAccessor (&RingNetwork::m_nodes),
                   MakeUintegerChecker<uint16_t> (3))
    .AddAttribute ("LinkDataRate", 
                   "The data rate to be used for the CSMA OpenFlow links.",
                   DataRateValue (DataRate ("100Mb/s")),
                   MakeDataRateAccessor (&RingNetwork::m_linkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The delay to be used for the CSMA OpenFlow links.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&RingNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu", 
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   UintegerValue (1540), // Ethernet II + GTP/UDP/IP tunnel
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
  Object::DoDispose ();
  m_ringCtrlApp = 0;
}

void
RingNetwork::CreateTopology (Ptr<OpenFlowEpcController> controller, 
    std::vector<uint16_t> eNbSwitches)
{
  NS_LOG_FUNCTION (this);
  
  static bool created = false;
  NS_ASSERT_MSG (!created, "Topology already created.");
  NS_ASSERT_MSG (m_nodes >= 3, "Invalid number of nodes for the ring");
  
  SetController (controller);
  m_eNbSwitchIdx = eNbSwitches;
  
  m_ringCtrlApp = DynamicCast<RingController> (m_ofCtrlApp);
  NS_ASSERT_MSG (m_ringCtrlApp, "Expecting a RingController.");

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
  m_ofDevices = m_ofHelper->InstallSwitchesWithoutPorts (m_ofSwitches);

  // Configuring csma links to connect the switches
  m_ofCsmaHelper.SetChannelAttribute ("DataRate", 
                                      DataRateValue (m_linkDataRate));
  m_ofCsmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_ofCsmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));
  m_ofCsmaHelper.SetQueue ("ns3::CoDelQueue");

  // Connecting switches in ring topology (clockwise order)
  for (uint16_t i = 0; i < m_nodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_nodes;  // Next clockwise node

      // Creating a link between current and next node
      Ptr<Node> currNode = m_ofSwitches.Get (currIndex);
      Ptr<Node> nextNode = m_ofSwitches.Get (nextIndex);

      NodeContainer pair;
      pair.Add (currNode);
      pair.Add (nextNode);
      NetDeviceContainer devs = m_ofCsmaHelper.Install (pair);
    
      Names::Add (Names::FindName (currNode) + "+" + 
                  Names::FindName (nextNode), devs.Get (0));
      Names::Add (Names::FindName (nextNode) + "+" + 
                  Names::FindName (currNode), devs.Get (1));

      // Adding newly created csma devices as openflow switch ports.
      Ptr<OFSwitch13NetDevice> currDevice, nextDevice;
      Ptr<CsmaNetDevice> currPortDevice, nextPortDevice;
      uint32_t currPortNum, nextPortNum;

      currDevice = GetSwitchDevice (currIndex);
      currPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (0));
      currPortNum = currDevice->AddSwitchPort (currPortDevice)->GetPortNo ();

      nextDevice = GetSwitchDevice (nextIndex);
      nextPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (1));
      nextPortNum = nextDevice->AddSwitchPort (nextPortDevice)->GetPortNo ();

      // Notify the ring controller of this new connection.
      Ptr<ConnectionInfo> info = CreateObject<ConnectionInfo> ();
      info->switchIdx1 = currIndex;
      info->switchIdx2 = nextIndex;
      info->switchDev1 = currDevice;
      info->switchDev2 = nextDevice;
      info->portDev1 = currPortDevice;
      info->portDev2 = nextPortDevice;
      info->portNum1 = currPortNum;
      info->portNum2 = nextPortNum;
      info->maxDataRate = m_linkDataRate;
      m_ringCtrlApp->NotifyNewConnBtwnSwitches (info);

      // Registering trace sink for meter dropped packets
      currDevice->TraceConnect ("MeterDrop", Names::FindName (currNode),
        MakeCallback (&OpenFlowEpcController::MeterDropPacket, m_ofCtrlApp));

      // Registering trace sink for queue drop packets
      std::ostringstream currQueue;
      currQueue << Names::FindName (currNode) << "/" << currPortNum;
      currPortDevice->GetQueue ()->TraceConnect ("Drop", currQueue.str (),
        MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));
      
      std::ostringstream nextQueue;
      nextQueue << Names::FindName (nextNode) << "/" << nextPortNum;
      nextPortDevice->GetQueue ()->TraceConnect ("Drop", nextQueue.str (),
        MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));
    }

  m_ringCtrlApp->NotifyConnBtwnSwitchesOk ();
  created = true;
}

Ptr<NetDevice>
RingNetwork::AttachToS1u (Ptr<Node> node, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());
  
  // Connect SgwPgw node to switch index 0 and other eNBs to switchs indexes
  // indicated by the user. As we know that the OpenFlowEpcHelper will callback
  // here first for SgwPgw node, we use the static counter to identify this
  // node.
  static uint32_t counter = 0;
  uint16_t swIdx;
  
  if (counter == 0)
    {
      // This is the SgwPgw node
      swIdx = 0;
      RegisterGatewayAtSwitch (swIdx, node);
    }
  else  
    {
      swIdx = m_eNbSwitchIdx.at (counter - 1);
    }
  counter++;
  RegisterNodeAtSwitch (swIdx, node);

  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);
  Ptr<OFSwitch13NetDevice> swDev = GetSwitchDevice (swIdx); 
 
  // Creating a link between switch and node
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (node);
  NetDeviceContainer devices = m_ofCsmaHelper.Install (pair);
 
  Ptr<CsmaNetDevice> portDev, nodeDev;
  portDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  nodeDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
 
  // Setting interface names for pacp filename
  Names::Add (Names::FindName (swNode) + "+" + Names::FindName (node), portDev);
  Names::Add (Names::FindName (node) + "+" + Names::FindName (swNode), nodeDev);
  
  // Set S1U IPv4 address for the new device at node
  Ipv4InterfaceContainer nodeIpIfaces = 
      m_s1uIpv4AddressHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);

  // Adding newly created csma device as openflow switch port.
  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
  uint32_t portNum = swPort->GetPortNo ();
  
  // Notify controller of a new device
  m_ringCtrlApp->NotifyNewAttachToSwitch (nodeDev, nodeAddr, swDev, swIdx, portNum);

  // Registering trace sink for queue drop packets
  std::ostringstream context;
  context << Names::FindName (swNode) << "/" << portNum;
  portDev->GetQueue ()->TraceConnect ("Drop", context.str (),
    MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));
 
  nodeDev->GetQueue ()->TraceConnect ("Drop", Names::FindName (node),
    MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));

  return nodeDev;
}

Ptr<NetDevice>
RingNetwork::AttachToX2 (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  NS_ASSERT (m_ofSwitches.GetN () == m_ofDevices.GetN ());

  // Retrieve the registered pair node/switch
  uint16_t swIdx = GetSwitchIdxForNode (node);
  NS_ASSERT (swIdx < m_ofDevices.GetN ());

  Ptr<Node> swNode = m_ofSwitches.Get (swIdx);
  Ptr<OFSwitch13NetDevice> swDev = GetSwitchDevice (swIdx); 

  // Creating a link between switch and node
  NodeContainer pair;
  pair.Add (swNode);
  pair.Add (node);
  NetDeviceContainer devices = m_ofCsmaHelper.Install (pair);

  Ptr<CsmaNetDevice> portDev, nodeDev;
  portDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  nodeDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  // Set X2 IPv4 address for the new device at node
  Ipv4InterfaceContainer nodeIpIfaces = 
      m_x2Ipv4AddressHelper.Assign (NetDeviceContainer (nodeDev));
  Ipv4Address nodeAddr = nodeIpIfaces.GetAddress (0);
  m_x2Ipv4AddressHelper.NewNetwork ();
  
  // Adding newly created csma device as openflow switch port.
  Ptr<OFSwitch13Port> swPort = swDev->AddSwitchPort (portDev);
  uint32_t portNum = swPort->GetPortNo ();

  // Notify controller of a new device
  m_ringCtrlApp->NotifyNewAttachToSwitch (nodeDev, nodeAddr, swDev, swIdx, portNum);

  // Registering trace sink for queue drop packets
  std::ostringstream context;
  context << Names::FindName (swNode) << "/" << portNum;
  portDev->GetQueue ()->TraceConnect ("Drop", context.str (),
    MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));
 
  nodeDev->GetQueue ()->TraceConnect ("Drop", Names::FindName (node),
    MakeCallback (&OpenFlowEpcController::QueueDropPacket, m_ofCtrlApp));

  return nodeDev;
}

};  // namespace ns3
