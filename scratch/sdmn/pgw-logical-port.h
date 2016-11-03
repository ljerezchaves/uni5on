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

#ifndef PGW_LOGICAL_PORT_H
#define PGW_LOGICAL_PORT_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

namespace ns3 {

/**
 * This handler is responsible for implementing the
 * GTP tunnel de/encapsulation on P-GW node.
 */
class PgwS5Handler: public Object
{
public:
  PgwS5Handler ();   //!< Default constructor
  virtual ~PgwS5Handler ();  //!< Dummy destructor

  /**
   * Complete constructor/
   * \param s5Address The IPv4 address for the P-GW S5 interface.
   * \param webMacAddr The MAC address of the Internet Web server.
   */
  PgwS5Handler (Ipv4Address s5Address, Mac48Address webMacAddr);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Receive logical port callback implementation, fired for packets received
   * from the S5 interface. Remove the GTP-U/UDP/IP headers from the packet and
   * return the GTP-U TEID id.
   * \param dpId The datapath id.
   * \param portNo The physical port number.
   * \param packet The received packet.
   * \return The GTP teid value from the header.
   * port.
   */
  uint64_t Receive (uint64_t dpId, uint32_t portNo, Ptr<Packet> packet);

  /**
   * Send logical port callback implementation, fired for packets coming from
   * the internet and about to be sent to the S5 interface. Add the
   * GTP-U/UDP/IP headers to the packet, using the tunnelId as GTP-U TEID
   * value.
   * \param dpId The datapath id.
   * \param portNo The physical port number.
   * \param packet The packet to send.
   * \param tunnelId The GTP teid value to use during encapsulation.
   */
  void Send (uint64_t dpId, uint32_t portNo, Ptr<Packet> packet,
             uint64_t tunnelId);

 Ptr<EpcSgwPgwCtrlApplication> m_controlPlane; // FIXME remover isso aqui

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  Ipv4Address     m_pgwS5Address;     //!< IP address of S5 interface device.
  Mac48Address    m_webMacAddress;    //!< MAC address of the Web server.

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
};

}; // namespace ns3
#endif /* PGW_LOGICAL_PORT_H */
