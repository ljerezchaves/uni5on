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
#include "../info/connection-info.h"
#include "../sdran/sdran-cloud.h"
#include "../stats/backhaul-stats-calculator.h"

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
                   TimeValue (MicroSeconds (200)), // 40km fiber cable latency.
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
RingNetwork::TopologyCreate (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_numNodes >= 3, "Invalid number of nodes for the ring");

  // Install the EPC ring controller application for this topology.
  Ptr<RingController> ringController =
    CreateObjectWithAttributes<RingController> (
      "PgwTftSwitches", UintegerValue (GetNTftNodes ()));
  InstallController (ringController);

  // Create the switch nodes.
  m_backNodes.Create (m_numNodes);

  // Install the Openflow switch devices for each switch node.
  m_backOfDevices = m_ofSwitchHelper->InstallSwitch (m_backNodes);

  // Set the name for each switch node.
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream swName;
      swName << "sw" << m_backOfDevices.Get (i)->GetDatapathId ();
      Names::Add (swName.str (), m_backNodes.Get (i));
    }

  // Connecting switches in ring topology (clockwise order).
  for (uint16_t i = 0; i < m_numNodes; i++)
    {
      uint16_t currIndex = i;
      uint16_t nextIndex = (i + 1) % m_numNodes;  // Next clockwise node.

      // Creating a link between current and next node.
      Ptr<Node> currNode = m_backNodes.Get (currIndex);
      Ptr<Node> nextNode = m_backNodes.Get (nextIndex);
      NetDeviceContainer devs = m_csmaHelper.Install (currNode, nextNode);

      // Setting interface names for pacp filename
      Names::Add (Names::FindName (currNode) + "_to_" +
                  Names::FindName (nextNode), devs.Get (0));
      Names::Add (Names::FindName (nextNode) + "_to_" +
                  Names::FindName (currNode), devs.Get (1));

      // Adding newly created csma devices as Openflow switch ports.
      Ptr<OFSwitch13Device> currDevice, nextDevice;
      Ptr<CsmaNetDevice> currPortDevice, nextPortDevice;
      uint32_t currPortNo, nextPortNo;

      currDevice = m_backOfDevices.Get (currIndex);
      currPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (0));
      currPortNo = currDevice->AddSwitchPort (currPortDevice)->GetPortNo ();

      nextDevice = m_backOfDevices.Get (nextIndex);
      nextPortDevice = DynamicCast<CsmaNetDevice> (devs.Get (1));
      nextPortNo = nextDevice->AddSwitchPort (nextPortDevice)->GetPortNo ();

      // Switch order inside ConnectionInfo object must respect clockwise order
      // (RingController assumes this order when installing switch rules).
      ConnectionInfo::SwitchData currSwData = {
        currDevice, currPortDevice, currPortNo
      };
      ConnectionInfo::SwitchData nextSwData = {
        nextDevice, nextPortDevice, nextPortNo
      };
      Ptr<ConnectionInfo> cInfo = CreateObject<ConnectionInfo> (
          currSwData, nextSwData, DynamicCast<CsmaChannel> (
            currPortDevice->GetChannel ()), ringController->GetSlicingMode ());

      // Fire trace source notifying new connection between switches.
      m_epcCtrlApp->NotifyTopologyConnection (cInfo);
    }

  // Fire trace source notifying that all connections between switches are ok.
  m_epcCtrlApp->NotifyTopologyBuilt (m_backOfDevices);
}

uint64_t
RingNetwork::TopologyGetPgwSwitch ()
{
  NS_LOG_FUNCTION (this);

  // Always connect the P-GW node to the first switch.
  return m_backOfDevices.Get (0)->GetDatapathId ();
}

uint64_t
RingNetwork::TopologyGetSgwSwitch (Ptr<SdranCloud> sdran)
{
  NS_LOG_FUNCTION (this << sdran);

  // Connect the S-GW nodes to switches indexes in clockwise direction,
  // starting at switch index 1.
  uint16_t swIdx = sdran->GetId () % m_numNodes;
  return m_backOfDevices.Get (swIdx)->GetDatapathId ();
}

uint64_t
RingNetwork::TopologyGetEnbSwitch (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);

  // Connect the eNBs nodes to switches indexes in clockwise direction,
  // starting at switch index 1. The three eNBs from same cell site are
  // connected to the same switch in the ring network.
  uint16_t siteId = 1 + ((cellId - 1) / 3);
  uint16_t swIdx = siteId % m_numNodes;
  return m_backOfDevices.Get (swIdx)->GetDatapathId ();
}

};  // namespace ns3
