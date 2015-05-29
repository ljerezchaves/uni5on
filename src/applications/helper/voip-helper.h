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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/voip-client.h"
#include "ns3/voip-server.h"

namespace ns3 {

/**
 * \ingroup applications 
 * Create a VoipHelper which will make life easier for people trying to set up
 * simulations with voip client/server.
 */
class VoipHelper
{
public:
  VoipHelper ();  //!< Default constructor

  /**
   * Record an attribute to be set in each client application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetClientAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in each server application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetServerAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of client + server applications on input nodes.
   * \param clientNode The node to install the client app.
   * \param serverNode The node to install the server app.
   * \param clientAddr The IPv4 address of the client.
   * \param serverAddr The IPv4 address of the server.
   * \param clientPort The port number on the client.
   * \param serverPort The port number on the server.
   * \return The client application created.
   */
  Ptr<VoipClient>
  Install (Ptr<Node> clientNode, Ptr<Node> serverNode, Ipv4Address clientAddr, 
           Ipv4Address serverAddr, uint16_t clientPort, uint16_t serverPort);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* VOIP_HELPER_H_ */
