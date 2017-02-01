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
#include "info/connection-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingNetwork");
NS_OBJECT_ENSURE_REGISTERED (RingNetwork);

RingNetwork::RingNetwork ()
{
  NS_LOG_FUNCTION (this);
}

RingNetwork::~RingNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RingNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingNetwork")
    .SetParent<EpcNetwork> ()
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
                   MakeDataRateAccessor (&RingNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("SwitchLinkDelay",
                   "The delay for the links between OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (100)), // 20km fiber cable latency.
                   MakeTimeAccessor (&RingNetwork::m_linkDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
RingNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  EpcNetwork::DoDispose ();
}

void
RingNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Configuring CSMA helper for connection between switches.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Chain up (the topology creation will be triggered by base class).
  EpcNetwork::NotifyConstructionCompleted ();
}

void
RingNetwork::TopologyCreate ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_numNodes >= 3, "Invalid number of nodes for the ring");

  // Install the EPC ring controller application for this topology.
  InstallController (CreateObject<RingController> ());

  // Creating the switch nodes.
  m_ofSwitches.Create (m_numNodes);
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      // Setting switch names.
      std::ostringstream swName;
      swName << "sw" << i;
      Names::Add (swName.str (), m_ofSwitches.Get (i));
    }

  // Installing the Openflow switch devices for each switch node.
  m_ofDevices = m_ofSwitchHelper->InstallSwitch (m_ofSwitches);

  // Connecting switches in ring topology (clockwise order).
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_numNodes;  // Next clockwise node.

      // Creating a link between current and next node.
      Ptr<Node> currNode = m_ofSwitches.Get (currIndex);
      Ptr<Node> nextNode = m_ofSwitches.Get (nextIndex);
      NetDeviceContainer devs = m_csmaHelper.Install (currNode, nextNode);

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
      // (RingController assumes this order when installing switch rules).
      ConnectionInfo::SwitchData currSwData = {
        currIndex, currDevice,
        currPortDevice, currPortNum
      };
      ConnectionInfo::SwitchData nextSwData = {
        nextIndex, nextDevice,
        nextPortDevice, nextPortNum
      };
      Ptr<ConnectionInfo> cInfo = CreateObject<ConnectionInfo> (
          currSwData, nextSwData, DynamicCast<CsmaChannel> (
            currPortDevice->GetChannel ()));

      // Fire trace source notifying new connection between switches.
      m_epcCtrlApp->NewSwitchConnection (cInfo);
      m_newConnTrace (cInfo);
    }

  // Fire trace source notifying that all connections between switches are ok.
  m_epcCtrlApp->TopologyBuilt (m_ofDevices);
  m_topoBuiltTrace (m_ofDevices);
}

uint16_t
RingNetwork::TopologyGetPgwIndex ()
{
  NS_LOG_FUNCTION (this);

  // Connect the P-GW node to switch index 0.
  return 0;
}

uint16_t
RingNetwork::TopologyGetEnbIndex (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);

  // Connect the eNBs nodes to switches indexes in clockwise direction,
  // starting at switch index 1. The three eNBs from same cell site are
  // connected to the same switch in the ring network.
  uint16_t siteId = 1 + ((cellId - 1) / 3);
  return siteId % m_numNodes;
}

};  // namespace ns3
