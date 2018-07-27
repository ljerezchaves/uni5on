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

#ifndef RING_CONTROLLER_H
#define RING_CONTROLLER_H

#include "backhaul-controller.h"
#include "metadata/connection-info.h"
#include "metadata/ring-routing-info.h"

namespace ns3 {

/**
 * \ingroup svelteInfra
 * OpenFlow backhaul controller for ring topology.
 */
class RingController : public BackhaulController
{
public:
  /** Routing strategy to find the paths in the ring. */
  enum RoutingStrategy
  {
    SPO = 0,  //!< Shortest path only (path with lowest number of hops).
    SPF = 1   //!< Shortest path first (preferably the shortest path).
  };

  RingController ();            //!< Default constructor.
  virtual ~RingController ();   //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the string representing the given routing strategy.
   * \param strategy The routing strategy.
   * \return The routing strategy string.
   */
  static std::string RoutingStrategyStr (RoutingStrategy strategy);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from BackhaulController.
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);
  void NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo);

  void NotifyBearerCreated (Ptr<BearerInfo> bInfo);
  
  // FIXME Removendo temporariamente
  // bool TopologyBearerRequest  (Ptr<RoutingInfo> rInfo);
  // bool TopologyBitRateRelease (Ptr<RoutingInfo> rInfo);
  // bool TopologyBitRateReserve (Ptr<RoutingInfo> rInfo);
  // bool TopologyRoutingInstall (Ptr<RoutingInfo> rInfo);
  // bool TopologyRoutingRemove  (Ptr<RoutingInfo> rInfo);
  // Inherited from BackhaulController.

private:
  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  void CreateSpanningTree (void);

  /**
   * Look for the routing path from source to destination switch index with
   * lowest number of hops.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \return The routing path.
   */
  RingRoutingInfo::RoutingPath FindShortestPath (uint16_t srcIdx,
                                                 uint16_t dstIdx) const;

  /**
   * Search for connection information between two switches by their indexes.
   * \param idx1 First switch index.
   * \param idx2 Second switch index.
   * \return Pointer to connection info saved.
   */
  Ptr<ConnectionInfo> GetConnectionInfo (uint16_t idx1, uint16_t idx2) const;

  /**
   * Get the maximum slice usage on this ring network.
   * \param slice The network slice.
   * \return The maximum slice usage.
   */
  double GetSliceUsage (Slice slice) const;

//  /**
//   * Check for the available bit rate on the given slice.
//   * \param ringInfo The ring routing information.
//   * \param gbrInfo The GBR information.
//   * \param slice The network slice.
//   * \return True if there's available GBR bit rate, false otherwise.
//   */
//  bool HasBitRate (Ptr<const RingRoutingInfo> ringInfo,
//                   Ptr<const GbrInfo> gbrInfo, Slice slice) const;

  /**
   * Count the number of hops between source and destination switch index
   * following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param path The routing path.
   * \return The number of hops in routing path.
   */
  uint16_t HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                       RingRoutingInfo::RoutingPath path) const;

  /**
   * // FIXME Move to BackhaulController
   * Notify this controller when the maximum bit rate for best-effort
   * traffic in any network connection is adjusted. This is used to update
   * meters bands based on slicing resource reservation.
   * \param cInfo The connection information.
   * \param dir The link direction.
   * \param slice The network slice.
   */
  void MeterAdjusted (Ptr<const ConnectionInfo> cInfo,
                      ConnectionInfo::Direction dir, Slice slice);

  /**
   * Get the next switch index following the given routing path.
   * \param idx Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */
  uint16_t NextSwitchIndex (uint16_t idx,
                            RingRoutingInfo::RoutingPath path) const;

  RoutingStrategy           m_strategy;       //!< Routing strategy in use.
};

} // namespace ns3
#endif // RING_CONTROLLER_H
