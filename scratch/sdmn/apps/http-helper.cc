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

#include "http-helper.h"

namespace ns3 {

HttpHelper::HttpHelper ()
{
  m_clientFactory.SetTypeId (HttpClient::GetTypeId ());
  m_serverFactory.SetTypeId (HttpServer::GetTypeId ());
}

void
HttpHelper::SetClientAttribute (std::string name, const AttributeValue &value)
{
  m_clientFactory.Set (name, value);
}

void
HttpHelper::SetServerAttribute (std::string name, const AttributeValue &value)
{
  m_serverFactory.Set (name, value);
}

Ptr<HttpClient>
HttpHelper::Install (Ptr<Node> clientNode, Ptr<Node> serverNode,
                     Ipv4Address serverAddress, uint16_t serverPort)
{
  Ptr<HttpClient> clientApp = m_clientFactory.Create<HttpClient> ();
  Ptr<HttpServer> serverApp = m_serverFactory.Create<HttpServer> ();

  clientApp->SetServer (serverApp, serverAddress, serverPort);
  clientNode->AddApplication (clientApp);

  serverApp->SetAttribute ("LocalPort", UintegerValue (serverPort));
  serverApp->SetAttribute ("StartTime", TimeValue (Seconds (0)));
  serverApp->SetClient (clientApp);
  serverNode->AddApplication (serverApp);

  return clientApp;
}

} // namespace ns3