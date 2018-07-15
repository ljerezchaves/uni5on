/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#ifndef SVELTE_ENB_APPLICATION_H
#define SVELTE_ENB_APPLICATION_H

#include <ns3/lte-module.h>

namespace ns3 {

/**
 * \ingroup svelteInfra
 * This eNB specialized application can handle connection with multiple S-GWs.
 */
class SvelteEnbApplication : public EpcEnbApplication
{
public:
  /**
   * Complete constructor.
   * \param lteSocket The socket to be used to send/receive IPv4 packets
   *                  to/from the LTE radio interface.
   * \param lteSocket6 The socket to be used to send/receive IPv6 packets
   *                   to/from the LTE radio interface.
   * \param s1uSocket The socket to be used to send/receive packets to/from the
   *                  S1-U interface connected with the SGW.
   * \param enbS1uAddress The IPv4 address of the S1-U interface of this eNB.
   * \param cellId The identifier of the eNB.
   */
  SvelteEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6,
                        Ptr<Socket> s1uSocket, Ipv4Address enbS1uAddress,
                        uint16_t cellId);

  virtual ~SvelteEnbApplication (void);  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  void DoDispose (void);

  /**
   * Send a packet to the SGW via the S1-U interface
   * \param packet packet to be sent
   * \param teid the Tunnel Enpoint IDentifier
   */
  void SendToS1uSocket (Ptr<Packet> packet, uint32_t teid);
};

} //namespace ns3
#endif /* SVELTE_ENB_APPLICATION_H */

