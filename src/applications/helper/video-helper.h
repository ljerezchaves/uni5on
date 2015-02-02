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

class VideoHelper
{
public:
  /**
   * Create a VideoHelper which will make life easier for people
   * trying to set up simulations with video-client.
   */
  VideoHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one video client application on each of the input nodes.
   * \param c The nodes.
   * \returns The applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * Create one video client application on node n.
   * \param node The node.
   * \param address The IPv4 address of the remote UDP server
   * \param port The port number of the remote UDP server
   * \returns The applications created.
   */
  Ptr<Application> Install (Ptr<Node> node, Ipv4Address address, uint16_t port);

private:
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3
#endif /* VIDEO_HELPER_H */
