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
#include "epc-sdn-controller.h"

namespace ns3 {

/**
 * OpenFlow EPC controller for ring network.
 */
class RingController : public EpcSdnController
{
public:
  /** Indicates the direction that the traffic should be routed in the ring. */
  enum Direction {
    CLOCK = 1,
    COUNTERCLOCK = 2
  };

  RingController ();          //!< Default constructor
  ~RingController (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  void DoDispose ();

  // Inherited from EpcSdnController
  void NotifyNewSwitchConnection (ConnectionInfo connInfo);
  bool RequestNewDedicatedBearer (uint64_t imsi, uint16_t cellId, 
                                  Ptr<EpcTft> tft, EpsBearer bearer);
  void NotifyNewContextCreated (uint64_t imsi, uint16_t cellId, 
                                std::list<EpcS11SapMme::BearerContextCreated> bearerContextList);
  void NotifyAppStart (Ptr<Application> app, Time simTime);
  void CreateSpanningTree ();

protected:
  // Inherited from EpcSdnController
  ofl_err HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
                                  SwitchInfo swtch, 
                                  uint32_t xid, uint32_t teid);

private:
  /**
   * Look for the routing path between srcSwitchIdx and dstSwitchIdx with
   * lowest number of hops.
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \return The routing direction.
   */
  Direction FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx = 0);

  /**
   * Look for available bandwidth in routingPath from source to destination
   * switch. It uses the information available at ConnectionInfo.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing direction.
   * \return The bandwidth for this datapath.
   */
  DataRate GetAvailableBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                                  Direction routingPath);

  /**
   * Reserve the bandwidth for each link between source and destination
   * switches in routingPath direction. It modifies the ConnectionInfo
   * structures saved by controller.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing direction.
   * \param bandwidth The bandwidth to reserve.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                         Direction routingPath, DataRate bandwidth);

  /**
   * Release the bandwidth for each link between source and destination
   * switches in routingPath direction. It modifies the ConnectionInfo
   * structures saved by controller.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing direction.
   * \param bandwidth The bandwidth to release.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                         Direction routingPath, DataRate bandwidth);

  /**
   * Identify the next switch index base on path direction.
   * \param current Current switch index.
   * \param path The routing direction.
   * \return The next switch index.
   */ 
  inline uint16_t NextSwitchIndex (uint16_t current, Direction path);

  /**
   * Using an OpenFlow Multipart OFPMP_FLOW message, query the switches for
   * individual flow statistics and estimates an average traffic for a specific
   * GTP tunnel. 
   * \return Average traffic for specific tunnel.
   */
  DataRate GetTunnelAverageTraffic ();
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

