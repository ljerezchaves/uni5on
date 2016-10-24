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

#include "openflow-epc-network.h"
#include "openflow-epc-controller.h"
#include "stats-calculator.h"
#include "internet-network.h"
#include "sdmn-epc-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcNetwork);

OpenFlowEpcNetwork::OpenFlowEpcNetwork ()
  : m_ofSwitchHelper (0),
    m_gatewayStats (0),
    m_ofCtrlNode (0),
    m_ofCtrlApp (0),
    m_ofEpcHelper (0),
    m_webNetwork (0),
    m_networkStats (0)
{
  NS_LOG_FUNCTION (this);

  // Let's use point to point connections for OpenFlow channel
  m_ofSwitchHelper = CreateObject<OFSwitch13Helper> ();
  m_ofSwitchHelper->SetAttribute ("ChannelType",
                                  EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Creating the OpenFlow EPC helper (will create the SgwPgw node and app).
  m_ofEpcHelper = CreateObject<SdmnEpcHelper> ();

  // Creating the Internet network object
  m_webNetwork = CreateObject<InternetNetwork> ();

  // Creating stats calculators
  ObjectFactory statsFactory;
  statsFactory.SetTypeId (LinkQueuesStatsCalculator::GetTypeId ());
  statsFactory.Set ("LnkStatsFilename", StringValue ("pgw_stats.txt"));
  m_gatewayStats = statsFactory.Create<LinkQueuesStatsCalculator> ();

  m_networkStats = CreateObject<NetworkStatsCalculator> ();
}

OpenFlowEpcNetwork::~OpenFlowEpcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
OpenFlowEpcNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenFlowEpcNetwork")
    .SetParent<Object> ()

    // Trace sources used by controller to be aware of network topology
    .AddTraceSource ("NewEpcAttach",
                     "New LTE EPC entity connected to the OpenFlow switch for "
                     "S1-U or X2 interface.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_newAttachTrace),
                     "ns3::OpenFlowEpcNetwork::AttachTracedCallback")
    .AddTraceSource ("NewSwitchConnection",
                     "New connection between two OpenFlow switches during "
                     "topology creation.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_newConnTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
    .AddTraceSource ("TopologyBuilt",
                     "OpenFlow networp topology built, with no more "
                     "connections between OpenFlow switches.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_topoBuiltTrace),
                     "ns3::OpenFlowEpcNetwork::TopologyTracedCallback")
  ;
  return tid;
}

void
OpenFlowEpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel
  m_ofSwitchHelper->EnableOpenFlowPcap (prefix + "ofchannel");

  // Enable pcap on LTE EPC interfaces
  m_ofEpcHelper->EnablePcapS1u (prefix + "lte-epc");
  m_ofEpcHelper->EnablePcapX2 (prefix + "lte-epc");

  // Enable pcap on Internet network
  m_webNetwork->EnablePcap (prefix + "internet");
}

void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1,
                                              const AttributeValue &v1)
{
  m_ofSwitchHelper->SetDeviceAttribute (n1, v1);
}

uint16_t
OpenFlowEpcNetwork::GetNSwitches (void) const
{
  return m_ofSwitches.GetN ();
}

Ptr<Node>
OpenFlowEpcNetwork::GetGatewayNode ()
{
  NS_ASSERT_MSG (m_ofEpcHelper, "Invalid EPC helper.");
  return m_ofEpcHelper->GetPgwNode ();
}

Ptr<Node>
OpenFlowEpcNetwork::GetServerNode ()
{
  return m_webNetwork->GetServerNode ();
}

Ptr<Node>
OpenFlowEpcNetwork::GetControllerNode ()
{
  return m_ofCtrlNode;
}

Ptr<OpenFlowEpcController>
OpenFlowEpcNetwork::GetControllerApp ()
{
  return m_ofCtrlApp;
}

Ptr<SdmnEpcHelper>
OpenFlowEpcNetwork::GetEpcHelper ()
{
  return m_ofEpcHelper;
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofCtrlNode = 0;
  m_ofCtrlApp = 0;
  m_ofSwitchHelper = 0;
  m_gatewayStats = 0;
  m_ofEpcHelper = 0;
  m_webNetwork = 0;
  m_networkStats = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
}

void
OpenFlowEpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Connect the network stats calculator *before* topology creation.
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &NetworkStatsCalculator::NotifyTopologyBuilt, m_networkStats));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &NetworkStatsCalculator::NotifyNewSwitchConnection, m_networkStats));

  // Create the ring network topology and Internet topology
  CreateTopology ();
  m_webNetwork->CreateTopology (GetGatewayNode ());

  // Connect S1U and X2 connection callbacks *after* topology creation.
  m_ofEpcHelper->SetS1uConnectCallback (
    MakeCallback (&OpenFlowEpcNetwork::S1Attach, this));
  m_ofEpcHelper->SetX2ConnectCallback (
    MakeCallback (&OpenFlowEpcNetwork::X2Attach, this));

  // Connect the controller to the MME SessionCreated trace source *after*
  // topology creation.
  m_ofEpcHelper->GetMmeElement ()->TraceConnectWithoutContext (
    "SessionCreated", MakeCallback (
      &OpenFlowEpcController::NotifySessionCreated, m_ofCtrlApp));

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
OpenFlowEpcNetwork::RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node)
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
OpenFlowEpcNetwork::RegisterGatewayAtSwitch (uint16_t switchIdx,
                                             Ptr<Node> node)
{
  m_gatewaySwitch = switchIdx;
}

Ptr<OFSwitch13Device>
OpenFlowEpcNetwork::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return m_ofDevices.Get (index);
}

uint16_t
OpenFlowEpcNetwork::GetSwitchIdxForNode (Ptr<Node> node)
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
OpenFlowEpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13Device> dev)
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
OpenFlowEpcNetwork::GetGatewaySwitchIdx ()
{
  return m_gatewaySwitch;
}

void
OpenFlowEpcNetwork::InstallController (Ptr<OpenFlowEpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_ofCtrlNode, "Controller application already set.");

  // Installing the controller app into a new controller node
  m_ofCtrlApp = controller;
  m_ofCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_ofCtrlNode);

  m_ofSwitchHelper->InstallControllerApp (m_ofCtrlNode, m_ofCtrlApp);

  // Connecting controller trace sinks to sources in this network
  TraceConnectWithoutContext (
    "NewEpcAttach", MakeCallback (
      &OpenFlowEpcController::NotifyNewEpcAttach, m_ofCtrlApp));
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &OpenFlowEpcController::NotifyTopologyBuilt, m_ofCtrlApp));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &OpenFlowEpcController::NotifyNewSwitchConnection, m_ofCtrlApp));
}

};  // namespace ns3

