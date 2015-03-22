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
#include "ns3/voip-client.h"
#include "ns3/voip-server.h"

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
   * Record an attribute to be set in each client application after it is is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetClientAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in each server application after it is is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetServerAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of voip applications on input nodes.
   * \param clientNode The client node in pair.
   * \param serverNode The server node in pair.
   * \param clientAddr The IPv4 address of client node.
   * \param serverAddr The IPv4 address of server node.
   * \param clientPort The input port number in client node.
   * \param serverPort The input port number in server node.
   * \return The VoipClient application created.
   */
  Ptr<VoipClient> Install (Ptr<Node>   clientNode, Ptr<Node>   serverNode, 
                           Ipv4Address clientAddr, Ipv4Address serverAddr,
                           uint16_t    clientPort, uint16_t    serverPort);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* VOIP_HELPER_H_ */
