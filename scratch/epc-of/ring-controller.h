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

namespace ns3 {

/**
 * OpenFlow EPC controller for ring network.
 */
class RingController : public OpenFlowEpcController
{
public:
  /** 
   * Routing strategy to find the paths in the ring. 
   */
  enum RoutingStrategy 
    {
      HOPS = 0,   //!< Select the path based on number of hops
      BAND = 1    //!< Select the path based on number of hops and bandwidth
    };
    
  RingController ();        //!< Default constructor
  ~RingController ();       //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  void DoDispose ();

  // Inherited from OpenFlowEpcController
  void NotifyNewConnBtwnSwitches (const Ptr<ConnectionInfo> connInfo);
  void NotifyConnBtwnSwitchesOk ();

protected:
  // Inherited from OpenFlowEpcController
  bool InstallTeidRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer);
  bool GbrBearerRequest (Ptr<RoutingInfo> rInfo);
  bool GbrBearerRelease (Ptr<RoutingInfo> rInfo);
  void CreateSpanningTree ();

private:
  /** 
   * Get the RingRoutingInfo associated to this rInfo metadata. When no ring
   * information is available, this function creates it.
   * \param rInfo The routing information to process.
   * \return The ring routing information for this bearer.
   */
  Ptr<RingRoutingInfo> GetRingRoutingInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Look for the routing path between srcSwitchIdx and dstSwitchIdx with
   * lowest number of hops.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \return The routing path.
   */
  RingRoutingInfo::RoutingPath FindShortestPath (uint16_t srcSwitchIdx,
      uint16_t dstSwitchIdx);

  /**
   * Look for available bandwidth in routingPath from source to destination
   * switch. It uses the information available at ConnectionInfo.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \return The bandwidth for this datapath.
   */
  DataRate GetAvailableBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
      RingRoutingInfo::RoutingPath routingPath);

  /**
   * Reserve the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param reserve The DataRate to reserve.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
    RingRoutingInfo::RoutingPath routingPath, DataRate reserve);

  /**
   * Release the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param release The DataRate to release.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
    RingRoutingInfo::RoutingPath routingPath, DataRate release);

  /**
   * Identify the next switch index based on routing path direction.
   * \param current Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */ 
  uint16_t NextSwitchIndex (uint16_t current, 
      RingRoutingInfo::RoutingPath routingPath);

  RoutingStrategy   m_strategy;          //!< Routing strategy in use.
  double            m_bwFactor;          //!< Bandwidth saving factor
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

