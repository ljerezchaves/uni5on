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
#include "epc-network.h"

namespace ns3 {

class BackhaulStatsCalculator;

/**
 * \ingroup sdmnEpc
 * This class creates and simple n-switch OpenFlow ring topology controlled by
 * a RingController. This OpenFlow network which will be used as backaul
 * infrastructure for the SDMN architecture.
 */
class RingNetwork : public EpcNetwork
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

  // Inherited from EpcNetwork.
  void TopologyCreate (void);
  uint64_t TopologyGetPgwSwitch (void);
  uint64_t TopologyGetSgwSwitch (Ptr<SdranCloud> sdran);
  uint64_t TopologyGetEnbSwitch (uint16_t cellId);

private:
  uint16_t                      m_numNodes;         //!< Number of switches.
  DataRate                      m_linkRate;         //!< Link data rate.
  Time                          m_linkDelay;        //!< Link delay.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.

}; // class RingNetwork

}; // namespace ns3
#endif // RING_NETWORK_H
