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

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "openflow-epc-controller.h"
#include "ring-network.h"

namespace ns3 {

class RingNetwork;

/**
 * \ingroup epcof
 * OpenFlow EPC controller for ring network.
 */
class RingController : public OpenFlowEpcController
{
public:
  /** Routing strategy to find the paths in the ring. */
  enum RoutingStrategy
  {
    HOPS = 0,     //!< Select the path based on number of hops
    BAND = 1,     //!< Select the path based on available bandwidth
    SMART = 2     //!< Select the path based on hops and available bandwidth
  };

  RingController ();            //!< Default constructor
  virtual ~RingController ();   //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from OpenFlowEpcController
  void NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo);
  void NotifyTopologyBuilt (NetDeviceContainer devices);
  bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer);
  bool TopologyRemoveRouting (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRelease (Ptr<RoutingInfo> rInfo);
  void TopologyCreateSpanningTree ();

  /**
   * \return Number of switches in the network.
   */
  uint16_t GetNSwitches (void) const;

private:
  /**
   * Get the RingRoutingInfo associated to this rInfo metadata. When no ring
   * information is available, this function creates it.
   * \param rInfo The routing information to process.
   * \return The ring routing information for this bearer.
   */
  Ptr<RingRoutingInfo> GetRingRoutingInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Save connection information between two switches for further usage.
   * \param cInfo The connection information to save.
   */
  void SaveConnectionInfo (Ptr<ConnectionInfo> cInfo);

  /**
   * Search for connection information between two switches.
   * \param sw1 First switch index.
   * \param sw2 Second switch index.
   * \return Pointer to connection info saved.
   */
  Ptr<ConnectionInfo> GetConnectionInfo (uint16_t sw1, uint16_t sw2);

  /**
   * Look for the routing path from source to destination switch index with
   * lowest number of hops.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \return The routing path.
   */
  RingRoutingInfo::RoutingPath FindShortestPath (uint16_t srcSwitchIdx, 
                                                 uint16_t dstSwitchIdx);

  /**
   * Get the available bandwidth for this ring routing information, considering
   * both downlink and uplink paths.
   * \param ringInfo The ring routing information.
   * \param invertPath When true, considers the inverted downlink/uplink paths
   * while looking for the available bandwidth.
   * \return A pair of available bandwidth data rates, for both downlink and
   * uplink paths, in this order.
   */
  std::pair<DataRate, DataRate> 
  GetAvailableBandwidth (Ptr<const RingRoutingInfo> ringInfo, 
                         bool invertPath = false);

  /**
   * Reserve the bandwidth for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param reserveInfo The reserve information.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (Ptr<const RingRoutingInfo> ringInfo,
                         Ptr<ReserveInfo> reserveInfo);
  
  /**
   * Release the bandwidth for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param reserveInfo The reserve information.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (Ptr<const RingRoutingInfo> ringInfo,
                         Ptr<ReserveInfo> reserveInfo);

  /**
   * Reserve the indicated bandwidth at each link from source to
   * destination switch index following the indicated routing path. 
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param reserve The bandwidth to reserve.
   * \return True if success, false otherwise;
   */
  bool PerLinkReserve (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                       RingRoutingInfo::RoutingPath routingPath, 
                       DataRate reserve);

  /**
   * Release the indicated bandwidth at each link from source to
   * destination switch index following the indicated routing path. 
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param release The bandwidth to release.
   * \return True if success, false otherwise;
   */
  bool PerLinkRelease (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                       RingRoutingInfo::RoutingPath routingPath, 
                       DataRate release);

  /**
   * Get the next switch index following the indicated routing path.
   * \param current Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */
  uint16_t NextSwitchIndex (uint16_t current,
                            RingRoutingInfo::RoutingPath routingPath);

  /**
   * Remove meter rules from switches.
   * \param rInfo The routing information.
   * \return True if remove succeeded, false otherwise.
   */
  bool RemoveMeterRules (Ptr<RoutingInfo> rInfo);

  RoutingStrategy     m_strategy;         //!< Routing strategy in use.
  double              m_maxBwFactor;      //!< Bandwidth saving factor
  uint16_t            m_noSwitches;       //!< Number of switches in topology.

  /** 
   * Map saving <Pair of switch indexes / Connection information.
   * The pair of switch indexes are saved in increasing order.
   */
  typedef std::map<SwitchPair_t, Ptr<ConnectionInfo> > ConnInfoMap_t;
  ConnInfoMap_t       m_connections;      //!< Connections between switches.
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

