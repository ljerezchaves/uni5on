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
  /** Routing strategy to find the paths in the ring. */
  enum RoutingStrategy {
    HOPS = 0,
    BAND = 1
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
  void NotifyNewSwitchConnection (const Ptr<ConnectionInfo> connInfo);
 
//  void NotifyNewContextCreated (uint64_t imsi, uint16_t cellId, 
//      Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList);
  
//  bool NotifyAppStart (Ptr<Application> app);
  
//  bool NotifyAppStop (Ptr<Application> app);
  
  void CreateSpanningTree ();

protected:
  // Inherited from OpenFlowEpcController
  bool InstallTeidRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer = OFP_NO_BUFFER);
  bool GbrBearerRequest (Ptr<RoutingInfo> rInfo);
  bool GbrBearerRelease (Ptr<RoutingInfo> rInfo);

//  ofl_err HandleMultipartReply (ofl_msg_multipart_reply_header *msg, SwitchInfo
//      swtch, uint32_t xid);

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
   * \param ringInfo The ring routing information.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (const Ptr<RingRoutingInfo> ringInfo);

  /**
   * Release the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.
   * \param ringInfo The ring routing information.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (const Ptr<RingRoutingInfo> ringInfo);

  /**
   * Identify the next switch index based on routing path direction.
   * \param current Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */ 
  uint16_t NextSwitchIndex (uint16_t current, 
      RingRoutingInfo::RoutingPath path);

//  /**
//   * Using an OpenFlow Multipart OFPMP_FLOW message, query the switches for
//   * individual flow statistics and estimates an average traffic for a specific
//   * GTP tunnel. 
//   * \param teid The GTP tunnel identifier.
//   * \return Average traffic for specific tunnel.
//   */
//  DataRate GetTunnelAverageTraffic (uint32_t teid);
//
//  /**
//   * Query OpenFlow switch for Information about individual flow entries using
//   * the OFPMP_FLOW multipart request message
//   */
//  void QuerySwitchStats ();
//
//  /**
//   * Based on app traffic direction, check if this switch is the input switch
//   * for this traffic into the ring. 
//   * \param rInfo The routing information to check.
//   * \param switchIdx The switch index.
//   * \return True when this switch is the ring input switch for this traffic.
//   */
//  bool IsInputSwitch (const Ptr<RingRoutingInfo> rInfo, uint16_t switchIdx);
//
//  /**
//   * Update the internal traffic average bitrate based on information received
//   * from switches.
//   * \param rInfo The routing information to check.
//   * \param switchIdx The switch index.
//   * \param flowStats The multipart flow stats from switch.
//   */ 
//  void UpdateAverageTraffic (Ptr<RoutingInfo> rInfo, uint16_t switchIdx, 
//                             ofl_flow_stats* flowStats);

  RoutingStrategy   m_strategy;          //!< Routing strategy in use.
  double            m_bwFactor;          //!< Bandwidth saving factor
//  Time              m_timeout;           //!< Switch stats query timeout.
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

