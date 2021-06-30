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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef RING_NETWORK_H
#define RING_NETWORK_H

#include <ns3/csma-module.h>
#include "transport-network.h"

namespace ns3 {

/**
 * \ingroup uni5onInfra
 * OpenFlow transport network with ring topology.
 */
class RingNetwork : public TransportNetwork
{
public:
  RingNetwork ();           //!< Default constructor.
  virtual ~RingNetwork ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  // Inherited from TransportNetwork.
  uint16_t GetEnbSwIdx (uint16_t cellId) const;

protected:
  /** Destructor implementation. */
  void DoDispose ();

  // Inherited from TransportNetwork.
  void CreateTopology (void);

private:
  uint16_t                      m_numNodes;       //!< Number of switches.
  bool                          m_skipFirst;      //!< Skip the first switch.

}; // class RingNetwork

} // namespace ns3
#endif // RING_NETWORK_H
