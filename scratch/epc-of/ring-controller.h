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
  /** Indicates the direction that the traffic should be routed in the ring in
   * respect to source node. */
  enum Routing {
    CLOCK = 1,
    COUNTERCLOCK = 2
  };

  /**
   * Metadata associated to a routing path between two any switches in the
   * OpenFlow network.
   */
  struct RoutingInfo
  {
    uint16_t gatewayIdx;    //!< Gateway switch index
    Ipv4Address gatewayAddr;//!< Gateway IPv4 address
    uint16_t enbIdx;        //!< eNB switch index
    Ipv4Address enbAddr;    //!< eNB IPv4 address
    Routing path;           //!< Downlink routing path (from gateway to eNB)
    uint32_t teid;          //!< GTP tunnel TEID
    DataRate reserved;      //!< GBR bandwitdh
    Ptr<Application> app;   //!< Traffic source application
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

  // Inherited from EpcSdnController
  void NotifyNewSwitchConnection (ConnectionInfo connInfo);
  void NotifyAppStart (Ptr<Application> app);
  void CreateSpanningTree ();

protected:
  // Inherited from EpcSdnController
  ofl_err HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                                  uint32_t xid, uint32_t teid);

private:
  /**
   * Look for the routing path between srcSwitchIdx and dstSwitchIdx with
   * lowest number of hops.
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \return The routing path.
   */
  Routing FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx);

  /**
   * Look for available bandwidth in routingPath from source to destination
   * switch. It uses the information available at ConnectionInfo.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \return The bandwidth for this datapath.
   */
  DataRate GetAvailableBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                                  Routing routingPath);

  /**
   * Reserve the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param bandwidth The bandwidth to reserve.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                         Routing routingPath, DataRate bandwidth);

  /**
   * Release the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param bandwidth The bandwidth to release.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                         Routing routingPath, DataRate bandwidth);

  /**
   * Identify the next switch index base on routing path.
   * \param current Current switch index.
   * \param path The routing path.
   * \return The next switch index.
   */ 
  inline uint16_t NextSwitchIndex (uint16_t current, Routing path);

  /**
   * Using an OpenFlow Multipart OFPMP_FLOW message, query the switches for
   * individual flow statistics and estimates an average traffic for a specific
   * GTP tunnel. 
   * \param teid The GTP tunnel identifier.
   * \return Average traffic for specific tunnel.
   */
  DataRate GetTunnelAverageTraffic (uint32_t teid);


  // FIXME doc
  void SaveTeidRouting (RoutingInfo info);

  // FIXME doc
  RoutingInfo GetTeidRoutingInfo (uint32_t teid);

  // FIXME doc
  bool HasTeidRoutingInfo (uint32_t teid);

    // FIXME doc
  bool ConfigureRoutingPath (uint32_t teid);

  /** Map saving TEID routing information */
  typedef std::map<uint32_t, RoutingInfo> TeidRouting_t;
  
  TeidRouting_t m_routes;       //!< Installed TEID routes.
};



};  // namespace ns3
#endif // RING_CONTROLLER_H

