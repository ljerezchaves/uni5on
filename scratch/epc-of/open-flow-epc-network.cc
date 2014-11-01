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

#include "open-flow-epc-network.h"

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcNetwork");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcNetwork);

OpenFlowEpcNetwork::OpenFlowEpcNetwork ()
  : m_ofCtrlApp (0),
    m_ofCtrlNode (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

OpenFlowEpcNetwork::~OpenFlowEpcNetwork ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId 
OpenFlowEpcNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::OpenFlowEpcNetwork") 
    .SetParent<Object> ()
    .AddAttribute ("LinkDataRate", 
                   "The data rate to be used for the CSMA OpenFlow links to be created",
                   DataRateValue (DataRate ("10Mb/s")),
                   MakeDataRateAccessor (&OpenFlowEpcNetwork::m_LinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The delay to be used for the CSMA OpenFlow links to be created",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&OpenFlowEpcNetwork::m_LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu", 
                   "The MTU for CSMA OpenFlow links. Use at least 1500 bytes.",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&OpenFlowEpcNetwork::m_LinkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid; 
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ofCtrlApp = 0;
  m_ofCtrlNode = 0;
  Object::DoDispose ();
}

void
OpenFlowEpcNetwork::CreateTopology (Ptr<OFSwitch13Controller> controller)
{
  static bool created = false;
  if (created)
    {
      NS_LOG_WARN ("Topology already created.");
      return;
    }

  // Configuring csma links
  m_ofCsmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_LinkDataRate));
  m_ofCsmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_LinkMtu));
  m_ofCsmaHelper.SetChannelAttribute ("Delay", TimeValue (m_LinkDelay));

  m_ofCtrlNode = InstallControllerApp (controller);
  CreateInternalTopology ();
  created = true;
}

void
OpenFlowEpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  m_ofCsmaHelper.EnablePcap (prefix, m_ofSwitches, promiscuous);
}

void 
OpenFlowEpcNetwork::EnableOpenFlowPcap (std::string prefix)
{
  m_ofHelper.EnableOpenFlowPcap (prefix);
}

CsmaHelper
OpenFlowEpcNetwork::GetCsmaHelper ()
{
  return m_ofCsmaHelper;
}

NodeContainer
OpenFlowEpcNetwork::GetSwitchNodes ()
{
  return m_ofSwitches;
}

NetDeviceContainer
OpenFlowEpcNetwork::GetSwitchDevices ()
{
  return m_ofDevices;
}

Ptr<OFSwitch13Controller>
OpenFlowEpcNetwork::GetControllerApp ()
{
  return m_ofCtrlApp;
}

Ptr<Node>
OpenFlowEpcNetwork::GetControllerNode ()
{
  return m_ofCtrlNode;
}

void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1, 
                                              const AttributeValue &v1)
{
  m_ofHelper.SetDeviceAttribute (n1, v1);
} 

Ptr<NetDevice>
OpenFlowEpcNetwork::SwitchAttach (uint8_t switchIdx, Ptr<Node> node)
{
  NS_ASSERT (switchIdx < m_ofSwitches.GetN ());
  NS_ASSERT (switchIdx < m_ofDevices.GetN ());

  Ptr<Node> swtch = m_ofSwitches.Get (switchIdx);
  NodeContainer nodes;
  nodes.Add (swtch);
  nodes.Add (node);
  NetDeviceContainer devices = m_ofCsmaHelper.Install (nodes);

  // Add csma device as switch port
  DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (switchIdx))->AddSwitchPort (devices.Get (0));
  
  return devices.Get (1);
}

Ptr<Node>
OpenFlowEpcNetwork::InstallControllerApp (Ptr<OFSwitch13Controller> controller)
{
  Ptr<Node> node = CreateObject<Node> ();
  m_ofHelper.InstallControllerApp (node, controller);
  m_ofCtrlApp = controller;
  return node;
}

};  // namespace ns3

