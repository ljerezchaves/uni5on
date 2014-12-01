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

#ifndef VOIP_CLIENT_HELPER_H_
#define VOIP_CLIENT_HELPER_H_

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

class VoipClientHelper
{
public:
  VoipClientHelper ();
  VoipClientHelper (Ipv4Address ip, uint16_t port);

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c);
  Ptr<Application>     Install (Ptr<Node> n);
private:
  ObjectFactory m_factory;
};

} // namespace ns3

#endif /* VOIP_CLIENT_HELPER_H_ */
