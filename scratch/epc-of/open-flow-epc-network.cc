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
  ;
  return tid; 
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ofCtrlApp = 0;
  m_ofCtrlNode = 0;
  m_nodeSwitchMap.clear ();
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

  // Installing the controller app into a new controller node
  m_ofCtrlNode = CreateObject<Node> ();
  m_ofHelper.InstallControllerApp (m_ofCtrlNode, controller);
  m_ofCtrlApp = controller;

  // Create the internal topology
  CreateInternalTopology ();
  created = true;
}

void
OpenFlowEpcNetwork::EnableDataPcap (std::string prefix, bool promiscuous)
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

void
OpenFlowEpcNetwork::RegisterNodeAtSwitch (uint8_t switchIdx, Ptr<Node> node)
{
  std::pair <NodeSwitchMap_t::iterator, bool> ret;
  ret = m_nodeSwitchMap.insert (std::pair<Ptr<Node>, uint8_t> (node, switchIdx));
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("Node " << node << " registered at switch index " << (int)switchIdx);
      return;
    }
  NS_FATAL_ERROR ("Can't register node at switch.");
}

uint8_t
OpenFlowEpcNetwork::GetSwitchIdxForNode (Ptr<Node> node)
{
  NodeSwitchMap_t::iterator ret;
  ret = m_nodeSwitchMap.find (node);
  if (ret != m_nodeSwitchMap.end ())
    {
      NS_LOG_DEBUG ("Found switch index " << (int)ret->second << " for node " << node);
      return ret->second;
    }
  NS_FATAL_ERROR ("Node not registered.");
}

};  // namespace ns3

