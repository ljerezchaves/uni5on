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

#include "real-time-video-helper.h"

namespace ns3 {

RealTimeVideoHelper::RealTimeVideoHelper ()
{
  m_clientFactory.SetTypeId (RealTimeVideoClient::GetTypeId ());
  m_serverFactory.SetTypeId (RealTimeVideoServer::GetTypeId ());
}

void
RealTimeVideoHelper::SetClientAttribute (std::string name, const AttributeValue &value)
{
  m_clientFactory.Set (name, value);
}

void
RealTimeVideoHelper::SetServerAttribute (std::string name, const AttributeValue &value)
{
  m_serverFactory.Set (name, value);
}

Ptr<RealTimeVideoClient>
RealTimeVideoHelper::Install (Ptr<Node> clientNode, Ptr<Node> serverNode,
                              Ipv4Address clientAddress, uint16_t clientPort)
{
  Ptr<RealTimeVideoClient> clientApp = m_clientFactory.Create<RealTimeVideoClient> ();
  Ptr<RealTimeVideoServer> serverApp = m_serverFactory.Create<RealTimeVideoServer> ();

  clientApp->SetAttribute ("LocalPort", UintegerValue (clientPort));
  clientApp->SetServer (serverApp);
  clientNode->AddApplication (clientApp);

  serverApp->SetClient (clientApp, clientAddress, clientPort);
  serverApp->SetAttribute ("StartTime", TimeValue (Seconds (0)));
  serverNode->AddApplication (serverApp);

  return clientApp;
}

} // namespace ns3
