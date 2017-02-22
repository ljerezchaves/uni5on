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

#ifndef PGW_USER_APP_H
#define PGW_USER_APP_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/virtual-net-device-module.h>

namespace ns3 {

/**
 * This is the tunneling application for P-GW S5 interface.
 * This GTP tunnel application is responsible for implementing the logical port
 * operations to encapsulate and de-encapsulated packets withing GTP tunnel. It
 * provides the callback implementations that are used by the logical switch
 * port and UDP socket. This application is stateless: it only adds/removes
 * protocols headers over packets leaving/entering the OpenFlow switch based on
 * information that is carried by packet tags.
 */
class PgwUserApp : public Application
{
public:
  PgwUserApp ();           //!< Default constructor.
  virtual ~PgwUserApp ();  //!< Dummy destructor, see DoDispose.

  /**
   * Complete constructor.
   * \param logicalPort The P-GW S5 OpenFlow logical port device.
   */
  PgwUserApp (Ptr<VirtualNetDevice> logicalPort);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Save the logical port and set the send callback.
   * \param logicalPort The P-GW S5 OpenFlow logical port device.
   */
  void SetLogicalPort (Ptr<VirtualNetDevice> logicalPort);

  /**
   * Method to be assigned to the send callback of the VirtualNetDevice
   * implementing the OpenFlow logical port. It is called when the OpenFlow
   * switch sends a packet out over the logical port. The logical port
   * callbacks here, and we must encapsulate the packet withing GTP and forward
   * it to the UDP tunnel socket.
   * \param packet The packet received from the logical port.
   * \param source Ethernet source address.
   * \param dst Ethernet destination address.
   * \param protocolNumber The type of payload contained in this packet.
   */
  bool RecvFromLogicalPort (Ptr<Packet> packet, const Address& source,
                            const Address& dest, uint16_t protocolNumber);

  /**
   * Send a packet to the logical port.
   * \param packet The packet to send.
   */
  bool SendToLogicalPort (Ptr<Packet>);

  /**
   * Method to be assigned to the receive callback of the UDP tunnel socket. It
   * is called when the tunnel socket receives a packet, and must forward the
   * packet to the logical port.
   * \param socket Pointer to the tunnel socket.
   */
  void RecvFromTunnelSocket (Ptr<Socket> socket);

  /**
   * Send a packet to the UDP tunnel socket.
   * \param packet The packet to send.
   * \param dstAddress The address of remote host.
   */
  bool SentToTunnelSocket (Ptr<Packet> packet, InetSocketAddress dstAddress);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from Application.
  virtual void StartApplication (void);

private:
  /**
   * Adds the necessary Ethernet headers and trailers to a packet of data.
   * \param packet Packet to which header should be added.
   * \param source MAC source address from which packet should be sent.
   * \param dest MAC destination address to which packet should be sent.
   * \param protocolNumber The type of payload contained in this packet.
   */
  void AddHeader (Ptr<Packet> packet, Mac48Address source = Mac48Address (),
                  Mac48Address dest = Mac48Address (),
                  uint16_t protocolNumber = Ipv4L3Protocol::PROT_NUMBER);

  /**
   * Trace source fired when a packet arrives this P-GW from
   * the S5 interface (leaving the EPC).
   */
  TracedCallback<Ptr<const Packet> > m_rxS5Trace;

  /**
   * Trace source fired when a packet leaves this P-GW over
   * the S5 interface (entering the EPC).
   */
  TracedCallback<Ptr<const Packet> > m_txS5Trace;

  Ptr<Socket>           m_tunnelSocket;   //!< UDP tunnel socket.
  Ptr<VirtualNetDevice> m_logicalPort;    //!< OpenFlow logical port device.
};

} // namespace ns3
#endif /* PGW_USER_APP_H */
