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

#ifndef SDMN_ENB_APPLICATION_H
#define SDMN_ENB_APPLICATION_H

#include <ns3/lte-module.h>

namespace ns3 {

/**
 * \ingroup sdmnEpc
 * SDMN specialized eNB application.
 */
class SdmnEnbApplication : public EpcEnbApplication
{
public:
  /**
   * Complete constructor
   * \param lteSocket The socket to be used to send/receive IPv4 packets
   *                  to/from the LTE radio interface.
   * \param lteSocket6 The socket to be used to send/receive IPv6 packets
   *                   to/from the LTE radio interface.
   * \param s1uSocket The socket to be used to send/receive packets to/from the
   *                  S1-U interface connected with the SGW.
   * \param enbS1uAddress The IPv4 address of the S1-U interface of this eNB.
   * \param sgwS1uAddress the IPv4 address at which this eNB will be able to
                          reach its S-GW for S1-U communication.
   * \param cellId The identifier of the eNB.
   */
  SdmnEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6,
                      Ptr<Socket> s1uSocket, Ipv4Address enbS1uAddress,
                      Ipv4Address sgwS1uAddress, uint16_t cellId);

  virtual ~SdmnEnbApplication (void); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Receive a packet from the S-GW via the S1-U interface.
   * \param socket The S1-U socket.
   */
  void RecvFromS1uSocket (Ptr<Socket> socket);

protected:
  /** Destructor implementation. */
  void DoDispose (void);

  /**
   * Send a packet to the S-GW via the S1-U interface
   * \param packet packet to be sent
   * \param teid the Tunnel Enpoint IDentifier
   */
  void SendToS1uSocket (Ptr<Packet> packet, uint32_t teid);

private:
  /**
   * Trace source fired when a packet arrives at this eNB from S1-U interface.
   */
  TracedCallback<Ptr<const Packet> > m_rxS1uTrace;

  /**
   * Trace source fired when a packet leaves this eNB over the S1-U interface.
   */
  TracedCallback<Ptr<const Packet> > m_txS1uTrace;
};

} //namespace ns3
#endif /* SDMN_ENB_APPLICATION_H */
