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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcNetwork);

OpenFlowEpcNetwork::OpenFlowEpcNetwork ()
  : m_ofCtrlNode (0),
    m_ofHelper (0),
    m_created (false),
    m_gatewayStats (0),
    m_gatewayNode (0),
    m_bandwidthStats (0),
    m_switchStats (0)
{
  NS_LOG_FUNCTION (this);
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
                     "New LTE EPC entity connected to OpenFlow switch for S1U "
                     "or X1 interface.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_newAttachTrace),
                     "ns3::OpenFlowEpcNetwork::AttachTracedCallback")
    .AddTraceSource ("NewSwitchConnection",
                     "New connection between two OpenFlow switches when "
                     "building topology.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_newConnTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
    .AddTraceSource ("TopologyBuilt",
                     "Topology built and no more connections between "
                     "OpenFlow switches.",
                     MakeTraceSourceAccessor (
                       &OpenFlowEpcNetwork::m_topoBuiltTrace),
                     "ns3::OpenFlowEpcNetwork::TopologyTracedCallback")
  ;
  return tid;
}

void
OpenFlowEpcNetwork::EnableDataPcap (std::string prefix, bool promiscuous)
{
  m_ofCsmaHelper.EnablePcap (prefix, m_ofSwitches, promiscuous);
}

void
OpenFlowEpcNetwork::EnableOpenFlowPcap (std::string prefix)
{
  m_ofHelper->EnableOpenFlowPcap (prefix);
}

void
OpenFlowEpcNetwork::EnableOpenFlowAscii (std::string prefix)
{
  m_ofHelper->EnableOpenFlowAscii (prefix);
}

void
OpenFlowEpcNetwork::EnableDatapathLogs (std::string level)
{
  m_ofHelper->EnableDatapathLogs (level);
}

void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1,
                                              const AttributeValue &v1)
{
  m_ofHelper->SetDeviceAttribute (n1, v1);
}

bool
OpenFlowEpcNetwork::IsTopologyCreated (void) const
{
  return m_created;
}

uint16_t
OpenFlowEpcNetwork::GetNSwitches (void) const
{
  return m_ofSwitches.GetN ();
}

Ptr<OFSwitch13NetDevice>
OpenFlowEpcNetwork::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (index));
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofCtrlNode = 0;
  m_ofHelper = 0;
  m_gatewayStats = 0;
  m_gatewayNode = 0;
  m_bandwidthStats = 0;
  m_switchStats = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
}

void 
OpenFlowEpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // For the OpenFlow control channel, let's use point to point connections
  // between controller and switches.
  m_ofHelper = CreateObject<OFSwitch13Helper> ();
  m_ofHelper->SetAttribute ("ChannelType", 
                            EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Creating the gateway stats calculator for this OpenFlow network
  ObjectFactory statsFactory;
  statsFactory.SetTypeId (LinkQueuesStatsCalculator::GetTypeId ());
  statsFactory.Set ("LnkStatsFilename", StringValue ("pgw_stats.txt"));
  m_gatewayStats = statsFactory.Create<LinkQueuesStatsCalculator> ();
  
  // Creating the bandwidth stats calculator for this OpenFlow network
  m_bandwidthStats = CreateObject<BandwidthStatsCalculator> ();
  TraceConnectWithoutContext ("TopologyBuilt", MakeCallback (
    &BandwidthStatsCalculator::NotifyTopologyBuilt, m_bandwidthStats));
  TraceConnectWithoutContext ("NewSwitchConnection", MakeCallback (
    &BandwidthStatsCalculator::NotifyNewSwitchConnection, m_bandwidthStats));

  // Creating the switch rules stats calculator for this OpenFlow network
  m_switchStats = CreateObject<SwitchRulesStatsCalculator> ();
  TraceConnectWithoutContext ("TopologyBuilt", MakeCallback (
    &SwitchRulesStatsCalculator::NotifyTopologyBuilt, m_switchStats));

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
  m_gatewayNode = node;
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
OpenFlowEpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev)
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

Ptr<Node>
OpenFlowEpcNetwork::GetGatewayNode ()
{
  return m_gatewayNode;
}

Ptr<Node>
OpenFlowEpcNetwork::GetControllerNode ()
{
  return m_ofCtrlNode;
}

void
OpenFlowEpcNetwork::InstallController (Ptr<OpenFlowEpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_ofCtrlNode, "Controller application already set.");

  // Installing the controller app into a new controller node
  m_ofCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_ofCtrlNode);

  m_ofHelper->InstallControllerApp (m_ofCtrlNode, controller);
}

};  // namespace ns3

