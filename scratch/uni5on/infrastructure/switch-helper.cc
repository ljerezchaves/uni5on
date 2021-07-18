/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
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

#include "switch-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwitchHelper");
NS_OBJECT_ENSURE_REGISTERED (SwitchHelper);

SwitchHelper::SwitchHelper ()
{
  NS_LOG_FUNCTION (this);
}

SwitchHelper::~SwitchHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SwitchHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchHelper")
    .SetParent<OFSwitch13InternalHelper> ()
    .AddConstructor<SwitchHelper> ()
  ;
  return tid;
}

void
SwitchHelper::AddSwitch (Ptr<OFSwitch13Device> device)
{
  NS_LOG_FUNCTION (this << device);

  NS_ABORT_MSG_IF (m_blocked, "OpenFlow channels already configured.");

  // Get the node for this switch device.
  Ptr<Node> node = device->GetObject<Node> ();
  NS_ABORT_MSG_IF (!node, "OpenFlow device not aggregated to any node.");

  // Saving device and node.
  m_openFlowDevs.Add (device);
  m_switchNodes.Add (node);
}

void
SwitchHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  OFSwitch13InternalHelper::DoDispose ();
}

} // namespace ns3
