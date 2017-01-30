/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#ifndef SDMN_APP_HELPER_H_
#define SDMN_APP_HELPER_H_

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "sdmn-client-app.h"
#include "sdmn-server-app.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * \defgroup sdmnApps Apps
 *
 * SDMN applications.
 */
/**
 * \ingroup sdmnApps
 * This helper will make life easier for people trying to set up client/server
 * applications on the SDMN architecture.
 */
class SdmnAppHelper
{
public:
  SdmnAppHelper ();     //!< Default constructor.

  /**
   * Complete constructor.
   * \param clientType The TypeId of client application class.
   * \param serverType The TypeId of server application class.
   */
  SdmnAppHelper (TypeId clientType, TypeId serverType);

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
   * \param port The port number on both client and server.
   * \return The client application created.
   */
  Ptr<SdmnClientApp> Install (Ptr<Node> clientNode, Ptr<Node> serverNode,
                              Ipv4Address clientAddr, Ipv4Address serverAddr,
                              uint16_t port);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* SDMN_APP_HELPER_H_ */
