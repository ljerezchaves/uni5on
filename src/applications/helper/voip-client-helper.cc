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

#include "voip-client-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/voip-client.h"

namespace ns3 {

VoipClientHelper::VoipClientHelper ()
{
}

VoipClientHelper::VoipClientHelper (Ipv4Address ip, uint16_t port)
{
  m_factory.SetTypeId (VoipClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (ip));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
VoipClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VoipClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<VoipClient> client = m_factory.Create<VoipClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

Ptr<Application>
VoipClientHelper::Install (Ptr<Node> n)
{
  Ptr<VoipClient> client = m_factory.Create<VoipClient> ();
  n->AddApplication (client);
  return client;
}

} // namespace ns3
