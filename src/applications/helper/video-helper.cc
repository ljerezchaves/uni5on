/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include "video-helper.h"
#include "ns3/udp-server.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

VideoHelper::VideoHelper ()
{
  m_clientFactory.SetTypeId (VideoClient::GetTypeId ());
  m_serverFactory.SetTypeId (UdpServer::GetTypeId ());
}

void
VideoHelper::SetClientAttribute (std::string name, const AttributeValue &value)
{
  m_clientFactory.Set (name, value);
}

void
VideoHelper::SetServerAttribute (std::string name, const AttributeValue &value)
{
  m_serverFactory.Set (name, value);
}

Ptr<VideoClient>
VideoHelper::Install (Ptr<Node> clientNode, Ptr<Node> serverNode, 
                      Ipv4Address serverAddress, uint16_t serverPort)
{
  Ptr<VideoClient> clientApp = m_clientFactory.Create<VideoClient> ();
  Ptr<UdpServer> serverApp = m_serverFactory.Create<UdpServer> ();
  
  clientApp->SetAttribute ("RemoteAddress", Ipv4AddressValue (serverAddress));
  clientApp->SetAttribute ("RemotePort", UintegerValue (serverPort));
  clientApp->SetServerApp (serverApp);
  clientNode->AddApplication (clientApp);
  
  serverApp->SetAttribute ("Port", UintegerValue (serverPort));
  serverNode->AddApplication (serverApp);
  
  return clientApp;
}

} // namespace ns3
