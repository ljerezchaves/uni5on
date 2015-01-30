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

#ifndef VIDEO_HELPER_H
#define VIDEO_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
#include "ns3/video-client.h"

namespace ns3 {

class VideoClientHelper
{
public:
  /**
   * Create VideoClientHelper which will make life easier for people
   * trying to set up simulations with onoff-udp-client-server.
   */
  VideoClientHelper ();

  /**
   * Create VideoClientHelper which will make life easier for people
   * trying to set up simulations with onoff-udp-client-server.
   * \param ip The IP address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  VideoClientHelper (Address ip, uint16_t port, std::string filename);
  /**
   * Create VideoClientHelper which will make life easier for people
   * trying to set up simulations with onoff-udp-client-server.
   * \param ip The IPv4 address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  VideoClientHelper (Ipv4Address ip, uint16_t port, std::string filename);
  /**
   * Create VideoClientHelper which will make life easier for people
   * trying to set up simulations with onoff-udp-client-server.
   * \param ip The IPv6 address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \param filename the file from which packet traces will be loaded
   */
  VideoClientHelper (Ipv6Address ip, uint16_t port, std::string filename);

  /**
   * Record an attribute to be set in each Application after it is is created.
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one OnOff UDP trace client application on each of the input
   * nodes.
   * \param c the nodes
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * Create one OnOff UDP trace client application on node n.
   * \param n the node
   * \returns the applications created.
   */
  Ptr<Application>     Install (Ptr<Node> n);


private:
  ObjectFactory m_factory; //!< Object factory.
};


} // namespace ns3

#endif /* VIDEO_HELPER_H */
