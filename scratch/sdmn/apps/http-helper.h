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

#ifndef HTTP_HELPER_H_
#define HTTP_HELPER_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "http-client.h"
#include "http-server.h"

namespace ns3 {

/**
 * \ingroup applications
 * Create a HttpHelper which will make life easier for people trying to set up
 * simulations with http client/server.
 */
class HttpHelper
{
public:
  HttpHelper ();  //!< Default constructor

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
   * \param serverAddress The IPv4 address of the server.
   * \param serverPort The port number on the server
   * \return The client application created.
   */
  Ptr<HttpClient>
  Install (Ptr<Node> clientNode, Ptr<Node> serverNode,
           Ipv4Address serverAddress, uint16_t serverPort);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* HTTP_HELPER_H_ */
