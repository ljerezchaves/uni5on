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

// ------------------------------------------------------------------------ //
/**
 * Metadata associated to a ring routing path between 
 * two any switches in the OpenFlow ring network.
 */
class RingRoutingInfo : public Object
{
  friend class RingController;

public:
  /** Routing direction in the ring. */
  enum RoutingPath {
    CLOCK = 1,
    COUNTER = 2
  };

  RingRoutingInfo ();          //!< Default constructor
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

protected:
  /** 
   * Invert down/up routing direction.
   */
  void InvertRoutingPath ();

  /** 
   * Set both down and up paths, based on down path direction. 
   * Up path will get the inverse direction. 
   * \param down The down path direction. 
   */
  void SetDownAndUpPath (RoutingPath down);

  RoutingPath downPath;        //!< Downlink routing path
  RoutingPath upPath;          //!< Uplink routing path
};


// ------------------------------------------------------------------------ //
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
  void NotifyNewContextCreated (uint64_t imsi, uint16_t cellId, 
                                Ipv4Address enbAddr, Ipv4Address sgwAddr, 
                                BearerList_t bearerList);
  bool NotifyAppStart (Ptr<Application> app);
  bool NotifyAppStop (Ptr<Application> app);
  void CreateSpanningTree ();

protected:
  // Inherited from OpenFlowEpcController
  ofl_err HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                                  uint32_t xid, uint32_t teid);

  // Inherited from OFSwitch13Controller
  ofl_err HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, 
                             uint32_t xid);
//  ofl_err HandleMultipartReply (ofl_msg_multipart_reply_header *msg, 
//                                SwitchInfo swtch, uint32_t xid);

private:
  /**
   * Retrieve stored information for a specific GTP tunnel.
   * \param teid The GTP tunnel ID.
   * \return The ring routing information for this tunnel.
   */
  Ptr<RingRoutingInfo> GetTeidRingRoutingInfo (uint32_t teid);

  /**
   * Process the GBR resource allocation based on current ring strategy.
   * \param rrInfo The ring routing information to process.
   * \return True when the GBR request can be sastified.
   */
  bool ProcessGbrRequest (Ptr<RingRoutingInfo> rrInfo);

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
   * \param rrInfo The ring routing information to reserve.
   * \return True if success, false otherwise;
   */
  bool ReserveBandwidth (const Ptr<RingRoutingInfo> rrInfo);

  /**
   * Release the bandwidth for each link between source and destination
   * switches in routing path. It modifies the ConnectionInfo
   * structures saved by controller.
   * \param rrInfo The ring routing information to release.
   * \return True if success, false otherwise;
   */
  bool ReleaseBandwidth (const Ptr<RingRoutingInfo> rrInfo);

  /**
   * Identify the next switch index based on routing path direction.
   * \param current Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */ 
  uint16_t NextSwitchIndex (uint16_t current, 
                            RingRoutingInfo::RoutingPath path);
  /**
   * Configure the switches with OpenFlow commands for teid routing.
   * \param rrInfo The ring routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   * \return True if configuration succeeded, false otherwise.
   */
  bool InstallTeidRouting (Ptr<RingRoutingInfo> rrInfo, 
                           uint32_t buffer = OFP_NO_BUFFER);

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
//   * \param rrInfo The ring routing information to check.
//   * \param switchIdx The switch index.
//   * \return True when this switch is the ring input switch for this traffic.
//   */
//  bool IsInputSwitch (const Ptr<RingRoutingInfo> rrInfo, uint16_t switchIdx);
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
//  Time              m_timeout;           //!< Switch stats query timeout.
  double            m_bwFactor;          //!< Bandwidth saving factor
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

