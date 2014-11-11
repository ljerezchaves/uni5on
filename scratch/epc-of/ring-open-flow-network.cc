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

#include "ring-open-flow-network.h"
#include "epc-sdn-controller.h"

NS_LOG_COMPONENT_DEFINE ("RingOpenFlowNetwork");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RingOpenFlowNetwork);

RingOpenFlowNetwork::RingOpenFlowNetwork ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RingOpenFlowNetwork::~RingOpenFlowNetwork ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId 
RingOpenFlowNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::RingOpenFlowNetwork") 
    .SetParent<OpenFlowEpcNetwork> ()
    .AddConstructor<RingOpenFlowNetwork> ()
    .AddAttribute ("NumSwitches", 
                   "The number of OpenFlow switches in the ring.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&RingOpenFlowNetwork::m_nodes),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid; 
}

void
RingOpenFlowNetwork::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  OpenFlowEpcNetwork::DoDispose ();
  Object::DoDispose ();
}

void
RingOpenFlowNetwork::CreateInternalTopology ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_nodes >= 1, "Invalid number of nodes for the ring");

  // Creating switch nodes
  m_ofSwitches.Create (m_nodes);
  for (uint16_t i = 0; i < m_nodes; i++)
    {
      std::ostringstream swName;
      swName << "sw" << i;
      Names::Add (swName.str(), m_ofSwitches.Get (i));
    }

  // Creating the switch devices for each switch node
  m_ofDevices = m_ofHelper.InstallSwitchesWithoutPorts (m_ofSwitches);

  // If the number of nodes in the ring is 1, return with no links
  if (m_nodes == 1) return;

  // Connecting switches in ring topology (clockwise order)
  for (uint16_t i = 0; i < m_nodes; i++)
    {
      int currentIndex = i;
      int nextIndex = (i + 1) % m_nodes;  // In clockwise direction

      NodeContainer pair;
      pair.Add (m_ofSwitches.Get (currentIndex));
      pair.Add (m_ofSwitches.Get (nextIndex));
      NetDeviceContainer devs = m_ofCsmaHelper.Install (pair);
    
      // Adding csma switch ports to openflow devices.
      Ptr<OFSwitch13NetDevice> currentDevice = DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (currentIndex));
      int currentPort = currentDevice->AddSwitchPort (devs.Get (0));

      Ptr<OFSwitch13NetDevice> nextDevice = DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (nextIndex));
      int nextPort = nextDevice->AddSwitchPort (devs.Get (1));

      // Installing default groups for EpcSdnController
      // Group #1 is used to send packets from current switch to the next one
      // in clockwise direction.
      std::ostringstream currentCmd;
      currentCmd << "group-mod cmd=add,type=ind,group=1 weight=0,port=any,group=any output=" << currentPort;
      DynamicCast<EpcSdnController> (m_ofCtrlApp)->ScheduleCommand (currentDevice, currentCmd.str ());
                                       
      // Group #2 is used to send packets from the next switch to the current
      // one in counterclockwise direction. 
      std::ostringstream nextCmd;
      nextCmd << "group-mod cmd=add,type=ind,group=2 weight=0,port=any,group=any output=" << nextPort;
      DynamicCast<EpcSdnController> (m_ofCtrlApp)->ScheduleCommand (nextDevice, nextCmd.str ());
    }
}

void
RingOpenFlowNetwork::RegisterNodeAtSwitch (uint8_t swtch, Ptr<Node> node)
{
  uint32_t nodeId = node->GetId ();
  m_nodeSwitchMap [nodeId];   // this creates a new entry if nodeId doesn't exist in the map
  std::map<uint32_t, uint8_t>::iterator i = m_nodeSwitchMap.find (nodeId);
  i->second = swtch;
}

Ptr<NetDevice>
RingOpenFlowNetwork::AttachToS1u (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  
  // Connect the SgwPgw to switch 0 and other eNBs to switchs 1 through m_nodes
  // - 1, in turns. In the case of a single node in the ring, connect all
  // gateways and eNBs to it. As we know that the OpenFlowEpcHelper will
  // callback here first for SgwPgw node, we use the static counter to identify
  // this node.
  static uint32_t counter = 0;
  
  uint16_t idx;   // switch index
  if (m_nodes == 1 || counter == 0 /* SgwPgw node */)
    idx = 0;
  else
    idx = 1 + ((counter - 1) % (m_nodes - 1));
  counter++;
  
  RegisterNodeAtSwitch (idx, node);
  return SwitchAttach (idx, node);
}

Ptr<NetDevice>
RingOpenFlowNetwork::AttachToX2 (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  
  // Get the switch index for this node
  uint8_t idx = m_nodeSwitchMap [node->GetId ()];
  return SwitchAttach (idx, node);
}

};  // namespace ns3
