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

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/http-client.h"
#include "ns3/http-server.h"

namespace ns3 {

class HttpHelper
{
public:
    
  /**
   * Create a HttpHelper which will make life easier for people
   * trying to set up simulations with http client-server.
   */
  HttpHelper ();

 /**
   * Record an attribute to be set in each HttpClient Application after it is
   * is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetClientAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in each HttpServer Application after it is
   * is created.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetServerAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of HttpClient + HttpServer applications on input nodes
   * \param clientNode The node to install the HttpClient app.
   * \param serverNode The node to install the HttpServer app.
   * \param serverAddress The IPv4 address of the Http server.
   * \param serverPort The port number of the Http server
   * \return The pair of applications created.
   */
  ApplicationContainer Install (Ptr<Node> clientNode, Ptr<Node> serverNode, 
                               Ipv4Address serverAddress, uint16_t serverPort);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* HTTP_HELPER_H_ */
