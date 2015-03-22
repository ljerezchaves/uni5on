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

namespace ns3 {

VoipHelper::VoipHelper ()
{
  m_clientFactory.SetTypeId (VoipClient::GetTypeId ());
  m_serverFactory.SetTypeId (VoipServer::GetTypeId ());
}

void
VoipHelper::SetClientAttribute (std::string name, const AttributeValue &value)
{
  m_clientFactory.Set (name, value);
}

void
VoipHelper::SetServerAttribute (std::string name, const AttributeValue &value)
{
  m_serverFactory.Set (name, value);
}

Ptr<VoipClient> 
VoipHelper::Install (Ptr<Node>   clientNode, Ptr<Node>   serverNode, 
                     Ipv4Address clientAddr, Ipv4Address serverAddr,
                     uint16_t    clientPort, uint16_t    serverPort)
{
  Ptr<VoipClient> clientApp = m_clientFactory.Create<VoipClient> ();
  Ptr<VoipServer> serverApp = m_serverFactory.Create<VoipServer> ();
  
  clientApp->SetAttribute ("ServerAddress", Ipv4AddressValue (serverAddr));
  clientApp->SetAttribute ("ServerPort", UintegerValue (serverPort));
  clientApp->SetAttribute ("LocalPort", UintegerValue (clientPort));
  clientApp->SetServerApp (serverApp);
  clientNode->AddApplication (clientApp);
  
  serverApp->SetAttribute ("ClientAddress", Ipv4AddressValue (clientAddr));
  serverApp->SetAttribute ("ClientPort", UintegerValue (clientPort));
  serverApp->SetAttribute ("LocalPort", UintegerValue (serverPort));
  serverApp->SetClientApp (clientApp);
  serverNode->AddApplication (serverApp);
  
  return clientApp;
}

} // namespace ns3
