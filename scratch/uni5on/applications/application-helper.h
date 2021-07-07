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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef APPLICATION_HELPER_H
#define APPLICATION_HELPER_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "base-client.h"
#include "base-server.h"

namespace ns3 {

/**
 * \ingroup uni5onApps
 * The helper to set up client and server applications.
 */
class ApplicationHelper
{
public:
  ApplicationHelper ();     //!< Default constructor.

  /**
   * Complete constructor.
   * \param clientType The TypeId of client application class.
   * \param serverType The TypeId of server application class.
   */
  ApplicationHelper (TypeId clientType, TypeId serverType);

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
   * \param dscp The DSCP value used to set the socket type of service field.
   * \return The client application created.
   */
  Ptr<BaseClient> Install (
    Ptr<Node> clientNode, Ptr<Node> serverNode,
    Ipv4Address clientAddr, Ipv4Address serverAddr,
    uint16_t port, Ipv4Header::DscpType dscp = Ipv4Header::DscpDefault);

  /**
   * Get the mapped IP ToS value for a specific DSCP.
   * \param dscp The IP DSCP value.
   * \return The IP ToS mapped for this DSCP.
   *
   * \internal
   * We are mapping the DSCP value (RFC 2474) to the IP Type of Service (ToS)
   * (RFC 1349) field because the pfifo_fast queue discipline from the traffic
   * control module still uses the old IP ToS definition. Thus, we are
   * 'translating' the DSCP values so we can keep the queuing consistency
   * both on traffic control module and OpenFlow port queues.
   * \verbatim
   * DSCP_EF   --> ToS 0x10 --> prio 6 --> pfifo band 0
   * DSCP_AF41 --> ToS 0x18 --> prio 4 --> pfifo band 1
   * DSCP_AF31 --> ToS 0x00 --> prio 0 --> pfifo band 1
   * DSCP_AF32 --> ToS 0x00 --> prio 0 --> pfifo band 1
   * DSCP_AF21 --> ToS 0x00 --> prio 0 --> pfifo band 1
   * DSCP_AF11 --> ToS 0x00 --> prio 0 --> pfifo band 1
   * DSCP_BE   --> ToS 0x08 --> prio 2 --> pfifo band 2
   * \endverbatim
   * \see See the ns3::Socket::IpTos2Priority for details.
   */
  uint8_t Dscp2Tos (Ipv4Header::DscpType dscp) const;

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* APPLICATION_HELPER_H */
