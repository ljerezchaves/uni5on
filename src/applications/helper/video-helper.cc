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
#include "ns3/udp-client.h"
#include "ns3/udp-trace-client.h"
#include "ns3/video-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

VideoHelper::VideoHelper ()
{
  m_factory.SetTypeId (VideoClient::GetTypeId ());
}

void
VideoHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<VideoClient> client = m_factory.Create<VideoClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

Ptr<VideoClient>
VideoHelper::Install (Ptr<Node> node, Ipv4Address address, uint16_t port)
{
  Ptr<VideoClient> client = m_factory.Create<VideoClient> ();
  client->SetAttribute ("RemoteAddress", Ipv4AddressValue (address));
  client->SetAttribute ("RemotePort", UintegerValue (port));
  node->AddApplication (client);
  return client;
}

} // namespace ns3
