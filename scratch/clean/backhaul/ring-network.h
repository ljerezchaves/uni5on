/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#ifndef RING_NETWORK_H
#define RING_NETWORK_H

#include <ns3/csma-module.h>
#include "backhaul-network.h"

namespace ns3 {

// class BackhaulStatsCalculator; FIXME

/**
 * \ingroup svelteBackhaul
 * OpenFlow backhaul network for ring topology.
 */
class RingNetwork : public BackhaulNetwork
{
public:
  RingNetwork ();           //!< Default constructor.
  virtual ~RingNetwork ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

  // Inherited from BackhaulNetwork.
  void TopologyCreate (void);

private:
  CsmaHelper                    m_csmaHelper;     //!< Connection helper.
  uint16_t                      m_numNodes;       //!< Number of switches.
  DataRate                      m_linkRate;       //!< Backhaul link data rate.
  Time                          m_linkDelay;      //!< Backhaul link delay.

}; // class RingNetwork

} // namespace ns3
#endif // RING_NETWORK_H
