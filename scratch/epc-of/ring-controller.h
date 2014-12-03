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
  /** 
   * Indicates the direction that the traffic should be routed in the ring in
   * respect to source node. 
   */
  enum RoutingPath {
    CLOCK = 1,
    COUNTERCLOCK = 2
  };

  /**
   * Metadata associated to a routing path between two any switches in the
   * OpenFlow ring network.
   */
  struct RoutingInfo
  {
    uint16_t sgwIdx;        //!< Gateway switch index
    uint16_t enbIdx;        //!< eNB switch index
    Ipv4Address sgwAddr;    //!< Gateway IPv4 address
    Ipv4Address enbAddr;    //!< eNB IPv4 address
    RoutingPath downPath;   //!< Downlink routing path (from gateway to eNB)
    RoutingPath upPath;     //!< Downlink routing path (from gateway to eNB)
    uint32_t teid;          //!< GTP tunnel TEID
    DataRate reserved;      //!< GBR bandwitdh
    Ptr<Application> app;   //!< Traffic source application
    int timeout;            //!< Flow idle timeout
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
  void NotifyNewContextCreated (uint64_t imsi, uint16_t cellId, 
                                Ipv4Address enbAddr, Ipv4Address sgwAddr, 
                                ContextBearers_t bearerContextList);
  void NotifyAppStart (Ptr<Application> app);
  void CreateSpanningTree ();

protected:
  // Inherited from EpcSdnController
  ofl_err HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                                  uint32_t xid, uint32_t teid);

  // Inherited from OFSwitch13Controller
  ofl_err HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, 
                             uint32_t xid);

private:
  /**
   * Look for the routing path between srcSwitchIdx and dstSwitchIdx with
   * lowest number of hops.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \return The routing path.
   */
  RoutingPath FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx);

  /** 
   * Invert routing direction.
   * \param original The original downlink routing path
   * \return The reversed routing path.
   */
  RoutingPath InvertRoutingPath (RoutingPath original);

  /**
   * Look for available bandwidth in routingPath from source to destination
   * switch. It uses the information available at ConnectionInfo.  
   * \param srcSwitchIdx Sourche switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \return The bandwidth for this datapath.
   */
  DataRate GetAvailableBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx, 
                                  RoutingPath routingPath);

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
                         RoutingPath routingPath, DataRate bandwidth);

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
                         RoutingPath routingPath, DataRate bandwidth);

  /**
   * Identify the next switch index base on routing path.
   * \param current Current switch index.
   * \param path The routing path.
   * \return The next switch index.
   */ 
  inline uint16_t NextSwitchIndex (uint16_t current, RoutingPath path);

  /**
   * Using an OpenFlow Multipart OFPMP_FLOW message, query the switches for
   * individual flow statistics and estimates an average traffic for a specific
   * GTP tunnel. 
   * \param teid The GTP tunnel identifier.
   * \return Average traffic for specific tunnel.
   */
  DataRate GetTunnelAverageTraffic (uint32_t teid);

  /**
   * Save the RoutingInfo metadata for further usage and reserve the bandwith.
   * \param rInfo The routing information to save.
   */
  void SaveTeidRoutingInfo (RoutingInfo rInfo);

  /**
   * Retrieve stored information for a specific GTP tunnel
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  RoutingInfo GetTeidRoutingInfo (uint32_t teid);

  /**
   * Check for tunnel routing information.
   * \param teid The GTP tunnel ID.
   * \return True if routing info present, false otherwise.
   */ 
  bool HasTeidRoutingInfo (uint32_t teid);

  /**
   * Remove the RoutingInfo metadata and release the bandwitdh.
   * \param teid The GTP tunnel ID.
   */
  void DeleteTeidRoutingInfo (uint32_t teid);

  /**
   * Configure the switches with OpenFlow commands for teid routing.
   * \param teid The GTP tunnel ID.
   * \return True if configuration succeeded, false otherwise.
   */
  bool ConfigureTeidRouting (uint32_t teid);

  /** Map saving TEID routing information */
  typedef std::map<uint32_t, RoutingInfo> TeidRouting_t;
  
  TeidRouting_t m_routes;       //!< Installed TEID routes.
};



};  // namespace ns3
#endif // RING_CONTROLLER_H

