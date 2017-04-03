/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef PGW_APP_H
#define PGW_APP_H

#include "gtp-tunnel-app.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * This is the GTP tunneling application for the P-GW. It extends the GTP
 * tunnel application for attach and remove the EpcGtpuTag tag on packets
 * entering/leaving the OpenFlow EPC backhaul network over S5 interface.
 */
class PgwApp : public GtpTunnelApp
{
public:
  /**
   * Complete constructor.
   * \param logicalPort The OpenFlow logical port device.
   * \param physicalPort The physical network device on node.
   */
  PgwApp (Ptr<VirtualNetDevice> logicalPort,
          Ptr<CsmaNetDevice> physicalDev);
  virtual ~PgwApp ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Attach the EpcGtpuTag tag into packet and fire the S5Tx trace source.
   * \param packet The packet.
   * \param teid The tunnel TEID for this packet.
   */
  void AttachEpcGtpuTag (Ptr<Packet> packet, uint32_t teid);

  /**
   * Fire the S5Rx trace source and remove the EpcGtpuTag tag from packet.
   * \param packet The packet.
   * \param teid The tunnel TEID for this packet.
   */
  void RemoveEpcGtpuTag (Ptr<Packet> packet, uint32_t teid);

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

} // namespace ns3
#endif /* PGW_APP_H */
