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
  : m_ofCtrlApp (0),
    m_ofCtrlNode (0)
{
  NS_LOG_FUNCTION (this);
  m_ofHelper = CreateObjectWithAttributes<OFSwitch13Helper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));
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

    // Trace sources used to simplify OpenFlow queue and meter monitoring
    .AddTraceSource ("QueueDrop",
                     "Packet dropped by OpenFlow port queue.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_queueDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MeterDrop",
                     "Packet dropped by OpenFlow meter band.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_meterDropTrace),
                     "ns3::Packet::TracedCallback")
    
    // Trace sources used by controller to be aware of network topology creation
    .AddTraceSource ("NewAttach",
                     "New LTE EPC entity connected to OpenFlow switch.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_newAttachTrace),
                     "ns3::OpenFlowEpcNetwork::AttachTracedCallback")
    .AddTraceSource ("NewConnection",
                     "New connection between two OpenFlow switches.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_newConnTrace),
                     "ns3::OpenFlowEpcNetwork::ConnectionTracedCallback")
    .AddTraceSource ("ConnectionsOk",
                     "Connections between OpenFlow switches finished.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_connOkTrace),
                     "ns3::OpenFlowEpcNetwork::BoolTracedCallback")
  ;
  return tid; 
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofCtrlApp = 0;
  m_ofCtrlNode = 0;
  m_ofHelper = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
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

// CsmaHelper
// OpenFlowEpcNetwork::GetCsmaHelper ()
// {
//   return m_ofCsmaHelper;
// }
// 
// NodeContainer
// OpenFlowEpcNetwork::GetSwitchNodes ()
// {
//   return m_ofSwitches;
// }
// 
// NetDeviceContainer
// OpenFlowEpcNetwork::GetSwitchDevices ()
// {
//   return m_ofDevices;
// }
// 
// Ptr<OFSwitch13Controller>
// OpenFlowEpcNetwork::GetControllerApp ()
// {
//   return m_ofCtrlApp;
// }
// 
// Ptr<Node>
// OpenFlowEpcNetwork::GetControllerNode ()
// {
//   return m_ofCtrlNode;
// }

uint16_t 
OpenFlowEpcNetwork::GetNSwitches (void) const
{
  return m_ofSwitches.GetN ();
}

void
OpenFlowEpcNetwork::MeterDropPacket (std::string context, 
                                     Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);
  m_meterDropTrace (packet);
}

void
OpenFlowEpcNetwork::QueueDropPacket (std::string context,
                                     Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);
  m_queueDropTrace (packet);
}

// void
// OpenFlowEpcNetwork::DumpSwtStatistics ()
// {
//   std::vector<uint32_t> teid;
// 
//   Ptr<OFSwitch13NetDevice> swDev;
//   for (uint16_t i = 0; i < GetNSwitches (); i++)
//     {
//       swDev = GetSwitchDevice (i);
//       teid.push_back (swDev->GetNumberFlowEntries (1)); // TEID table is 1
//     }
//   m_swtTrace (teid);
// 
//   Simulator::Schedule (m_dumpTimeout, 
//     &OpenFlowEpcNetwork::DumpSwtStatistics, this);
// }
// 
// void
// OpenFlowEpcNetwork::DumpBwdStatistics ()
// { 
//   m_bwdTrace (m_ofCtrlApp->GetBandwidthStats ());
// 
//   Simulator::Schedule (m_dumpTimeout, 
//     &OpenFlowEpcNetwork::DumpBwdStatistics, this);
// }
// 
void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1, 
                                              const AttributeValue &v1)
{
  m_ofHelper->SetDeviceAttribute (n1, v1);
} 

Ptr<OFSwitch13NetDevice> 
OpenFlowEpcNetwork::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (index));
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
OpenFlowEpcNetwork::RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node)
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

// uint16_t
// OpenFlowEpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev)
// {
//   uint16_t i;
//   for (i = 0; i < GetNSwitches (); i++)
//     {
//       if (dev == GetSwitchDevice (i))
//         {
//           return i;
//         }
//     }
//   NS_FATAL_ERROR ("Device not registered.");
// }
// 
// uint16_t
// OpenFlowEpcNetwork::GetGatewaySwitchIdx ()
// {
//   return m_gatewaySwitch;
// }
// 
// Ptr<Node>
// OpenFlowEpcNetwork::GetGatewayNode ()
// {
//   return m_gatewayNode;
// }

void
OpenFlowEpcNetwork::InstallController (Ptr<OpenFlowEpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_ofCtrlApp, "Controller application already set.");
  
  // Installing the controller app into a new controller node
  m_ofCtrlApp = controller;
  m_ofCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_ofCtrlNode);

  m_ofHelper->InstallControllerApp (m_ofCtrlNode, m_ofCtrlApp);
}

};  // namespace ns3

