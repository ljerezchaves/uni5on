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

#ifndef VOIP_HELPER_H_
#define VOIP_HELPER_H_

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

class VoipHelper
{
public:
  /**
   * Create a VoipHelper which will make life easier for people
   * trying to set up simulations with voip applications.
   */
  VoipHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of voip applications on input nodes.
   * \param firstNode The first node in pair.
   * \param secondNode The second node in pair.
   * \param firstAddr The IPv4 address of first node.
   * \param secondAddr The IPv4 address of second node.
   * \param firstPort The input port number in first node.
   * \param secondPort The input port number in second node.
   * \returns The pair of applications created.
   */
  ApplicationContainer Install (Ptr<Node> firstNode,   Ptr<Node> secondNode, 
                                Ipv4Address firstAddr, Ipv4Address secondAddr,
                                uint16_t firstPort,    uint16_t secondPort);

private:
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3
#endif /* VOIP_HELPER_H_ */
