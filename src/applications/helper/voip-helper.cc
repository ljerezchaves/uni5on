/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2014 University of Campinas (Unicamp)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "voip-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/voip-peer.h"

namespace ns3 {

VoipHelper::VoipHelper ()
{
  m_factory.SetTypeId (VoipPeer::GetTypeId ());
}

void
VoipHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

Ptr<VoipPeer>
VoipHelper::Install (Ptr<Node> firstNode,   Ptr<Node> secondNode, 
                     Ipv4Address firstAddr, Ipv4Address secondAddr, 
                     uint16_t firstPort,    uint16_t secondPort)
{
  Ptr<VoipPeer> firstApp  = m_factory.Create<VoipPeer> ();
  Ptr<VoipPeer> secondApp = m_factory.Create<VoipPeer> ();
  
  firstApp->SetAttribute ("PeerAddress", Ipv4AddressValue (secondAddr));
  firstApp->SetAttribute ("PeerPort", UintegerValue (secondPort));
  firstApp->SetAttribute ("LocalPort", UintegerValue (firstPort));
  firstApp->SetPeerApp (secondApp);
  firstNode->AddApplication (firstApp);
  
  secondApp->SetAttribute ("PeerAddress", Ipv4AddressValue (firstAddr));
  secondApp->SetAttribute ("PeerPort", UintegerValue (firstPort));
  secondApp->SetAttribute ("LocalPort", UintegerValue (secondPort));
  secondApp->SetPeerApp (firstApp);
  secondNode->AddApplication (secondApp);
  
  return firstApp;
}

} // namespace ns3
