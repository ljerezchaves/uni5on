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

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcNetwork");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcNetwork);

DataRate
ConnectionInfo::GetAvailableDataRate ()
{
  return maxDataRate - reservedDataRate;
}

DataRate
ConnectionInfo::GetAvailableDataRate (double bwFactor)
{
  return (maxDataRate * (1. - bwFactor)) - reservedDataRate;
}

bool
ConnectionInfo::ReserveDataRate (DataRate dr)
{
  reservedDataRate = reservedDataRate + dr;
  return (reservedDataRate <= maxDataRate);
}

bool
ConnectionInfo::ReleaseDataRate (DataRate dr)
{
  reservedDataRate = reservedDataRate - dr;
  return (reservedDataRate >= 0);
}

OpenFlowEpcNetwork::OpenFlowEpcNetwork ()
  : m_ofCtrlApp (0),
    m_ofCtrlNode (0)
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
    .AddAttribute ("ControllerApp", 
                   "The OpenFlow controller for this EPC OpenFlow network.",
                   PointerValue (),
                   MakePointerAccessor (&OpenFlowEpcNetwork::SetController),
                   MakePointerChecker<OpenFlowEpcController> ())
  ;
  return tid; 
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofCtrlApp = 0;
  m_ofCtrlNode = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
}

void
OpenFlowEpcNetwork::SetController (Ptr<OpenFlowEpcController> controller)
{
  // Installing the controller app into a new controller node
  m_ofCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_ofCtrlNode);

  m_ofHelper.InstallControllerApp (m_ofCtrlNode, controller);
  m_ofCtrlApp = controller;
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

void 
OpenFlowEpcNetwork::EnableOpenFlowAscii (std::string prefix)
{
  m_ofHelper.EnableOpenFlowAscii (prefix);
}

void 
OpenFlowEpcNetwork::EnableDatapathLogs (std::string level)
{
  m_ofHelper.EnableDatapathLogs (level);
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

uint16_t 
OpenFlowEpcNetwork::GetNSwitches ()
{
  return m_ofSwitches.GetN ();
}

void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1, 
                                              const AttributeValue &v1)
{
  m_ofHelper.SetDeviceAttribute (n1, v1);
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
OpenFlowEpcNetwork::RegisterGatewayAtSwitch (uint16_t switchIdx)
{
  m_gatewaySwitch = switchIdx;
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
OpenFlowEpcNetwork::GetSwitchIdxForGateway ()
{
  return m_gatewaySwitch;
}

};  // namespace ns3

