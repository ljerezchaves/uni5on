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
#include "../metadata/link-info.h"
#include "ring-controller.h"

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
    .SetParent<BackhaulNetwork> ()
    .AddConstructor<RingNetwork> ()
    .AddAttribute ("NumRingSwitches",
                   "The number of OpenFlow switches in the ring (at least 3).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (3),
                   MakeUintegerAccessor (&RingNetwork::m_numNodes),
                   MakeUintegerChecker<uint16_t> (3))
    .AddAttribute ("RingLinkDataRate",
                   "The data rate for the links between OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Mb/s")),
                   MakeDataRateAccessor (&RingNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("RingLinkDelay",
                   "The delay for the links between OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   // The default value is for 40km fiber cable latency.
                   TimeValue (MicroSeconds (200)),
                   MakeTimeAccessor (&RingNetwork::m_linkDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
RingNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  BackhaulNetwork::DoDispose ();
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
  BackhaulNetwork::NotifyConstructionCompleted ();
}

void
RingNetwork::CreateTopology (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Creating ring backhaul network with " << m_numNodes <<
               " switches.");

  NS_ASSERT_MSG (m_numNodes >= 3, "Invalid number of nodes for the ring.");

  // Install the ring controller application for this topology.
  Ptr<RingController> ringController = CreateObject<RingController> ();
  m_controllerNode = CreateObject<Node> ();
  Names::Add ("ring_ctrl", m_controllerNode);
  m_switchHelper->InstallController (m_controllerNode, ringController);
  m_controllerApp = ringController;

  // Create the switch nodes and install the OpenFlow switch devices.
  m_switchNodes.Create (m_numNodes);
  m_switchDevices = m_switchHelper->InstallSwitch (m_switchNodes);

  // Set the name for each switch node.
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream swName;
      swName << "sw" << m_switchDevices.Get (i)->GetDatapathId ();
      Names::Add (swName.str (), m_switchNodes.Get (i));
    }

  // Connecting switches in ring topology (clockwise order).
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_numNodes;  // Next clockwise node.

      // Creating a link between current and next node.
      Ptr<Node> currNode = m_switchNodes.Get (currIndex);
      Ptr<Node> nextNode = m_switchNodes.Get (nextIndex);
      NetDeviceContainer devs = m_csmaHelper.Install (currNode, nextNode);

      // Set device names for pcap files.
      BackhaulNetwork::SetDeviceNames (devs.Get (0), devs.Get (1), "~");

      // Adding newly created csma devices as OpenFlow switch ports.
      Ptr<OFSwitch13Device> currDev, nextDev;
      Ptr<CsmaNetDevice> currPortDev, nextPortDev;
      uint32_t currPortNo, nextPortNo;

      currDev = m_switchDevices.Get (currIndex);
      currPortDev = DynamicCast<CsmaNetDevice> (devs.Get (0));
      currPortNo = currDev->AddSwitchPort (currPortDev)->GetPortNo ();

      nextDev = m_switchDevices.Get (nextIndex);
      nextPortDev = DynamicCast<CsmaNetDevice> (devs.Get (1));
      nextPortNo = nextDev->AddSwitchPort (nextPortDev)->GetPortNo ();

      // Switch order inside LinkInfo object must respect clockwise order
      // (RingController assumes this order when installing switch rules).
      LinkInfo::SwitchData currSwData = {currDev, currPortDev, currPortNo};
      LinkInfo::SwitchData nextSwData = {nextDev, nextPortDev, nextPortNo};
      Ptr<LinkInfo> lInfo = CreateObject<LinkInfo> (
          currSwData, nextSwData, DynamicCast<CsmaChannel> (
            currPortDev->GetChannel ()), ringController->GetSlicingMode ());

      // Fire trace source notifying new connection between switches.
      m_controllerApp->NotifyTopologyConnection (lInfo);
    }

  // Fire trace source notifying that all connections between switches are ok.
  m_controllerApp->NotifyTopologyBuilt (m_switchDevices);
}

} // namespace ns3
