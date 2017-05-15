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

#include "epc-controller.h"
#include "../info/ring-routing-info.h"

namespace ns3 {

/**
 * \ingroup sdmnEpc
 * OpenFlow EPC controller for ring network topology.
 */
class RingController : public EpcController
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

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from EpcController.
  void NotifyS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
                       Ptr<NetDevice> gwDev);
  void NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo);
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);

  bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo);
  bool TopologyRemoveRouting  (Ptr<RoutingInfo> rInfo);
  void TopologyBearerCreated  (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRequest  (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRelease  (Ptr<RoutingInfo> rInfo);
  // Inherited from EpcController.

private:
  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  void CreateSpanningTree (void);

  /**
   * Notify this controller when the Non-GBR allowed bit rate in any network
   * connection is adjusted. This is used to update Non-GBR meters bands based
   * on GBR resource reservation.
   * \param cInfo The connection information
   */
  void NonGbrAdjusted (Ptr<ConnectionInfo> cInfo);

  /**
   * Search for connection information between two switches by their indexes.
   * \param idx1 First switch index.
   * \param idx2 Second switch index.
   * \return Pointer to connection info saved.
   */
  Ptr<ConnectionInfo> GetConnectionInfo (uint16_t idx1, uint16_t idx2);

  /**
   * Retrieve the switch index for IP address.
   * \param ipAddr The IPv4 address.
   * \return The switch index in devices collection.
   */
  uint16_t GetSwitchIndex (Ipv4Address ipAddr) const;

  /**
   * Retrieve the switch index for switch device.
   * \param dev The OpenFlow switch device.
   * \return The switch index in devices collection.
   */
  uint16_t GetSwitchIndex (Ptr<OFSwitch13Device> dev) const;

  /**
   * Get the number of switches in the network.
   * \return The number of switches in the network.
   */
  uint16_t GetNSwitches (void) const;

  /**
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param idx The switch index in devices colection.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDpId (uint16_t idx) const;

  /**
   * Get the available GBR bit rate for this ring routing information,
   * considering both downlink and uplink paths.
   * \internal This method implements the GBR Distance-Based Reservation
   *           algorithm (DeBaR) proposed by prof. Deep Medhi. The general idea
   *           is a dynamic bit rate usage factor that can be adjusted based on
   *           the distance between the eNB switch and the gateway switch.
   * \param ringInfo The ring routing information.
   * \param useShortPath When true, get the available bit rate in the shortest
   *        path between source and destination nodes; otherwise, considers the
   *        inverted (long) path.
   * \return A pair of available GBR bit rates, for both downlink and uplink
   *         paths, in this strict order.
   */
  std::pair<uint64_t, uint64_t>
  GetAvailableGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          bool useShortPath = true);

  /**
   * Reserve the bit rate for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \return True if succeeded, false otherwise.
   */
  bool ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo);

  /**
   * Release the bit rate for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \return True if succeeded, false otherwise.
   */
  bool ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo);

  /**
   * Reserve the indicated bit rate at each link from source to destination
   * switch index following the indicated routing path.
   * \attention To avoid fatal errors, be sure that there is enough available
   *            bit rate in this link before reserving it.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param path The routing path.
   * \param bitRate The bit rate to reserve.
   * \return True if succeeded, false otherwise.
   */
  bool PerLinkReserve (uint16_t srcIdx, uint16_t dstIdx,
                       RingRoutingInfo::RoutingPath path,
                       uint64_t bitRate);

  /**
   * Release the indicated bit rate at each link from source to destination
   * switch index following the indicated routing path.
   * \attention To avoid fatal errors, be sure that there is enough reserved
   * bit rate in this link before releasing it.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param path The routing path.
   * \param bitRate The bit rate to release.
   * \return True if succeeded, false otherwise.
   */
  bool PerLinkRelease (uint16_t srcIdx, uint16_t dstIdx,
                       RingRoutingInfo::RoutingPath path,
                       uint64_t bitRate);

  /**
   * Get the next switch index following the given routing path.
   * \param idx Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */
  uint16_t NextSwitchIndex (uint16_t idx, RingRoutingInfo::RoutingPath path);

  /**
   * Look for the routing path from source to destination switch index with
   * lowest number of hops.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \return The routing path.
   */
  RingRoutingInfo::RoutingPath
  FindShortestPath (uint16_t srcIdx, uint16_t dstIdx);

  /**
   * Count the number of hops between source and destination switch index
   * following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param path The routing path.
   * \return The number of hops in routing path.
   */
  uint16_t HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                       RingRoutingInfo::RoutingPath path);

  /** Map saving IPv4 address / switch index. */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;

  IpSwitchMap_t             m_ipSwitchTable;  //!< IP / switch index table.
  OFSwitch13DeviceContainer m_ofDevices;      //!< OpenFlow devices.
  RoutingStrategy           m_strategy;       //!< Routing strategy in use.
  double                    m_debarStep;      //!< DeBaR increase step.
  bool                      m_debarShortPath; //!< DeBaR in shortest path.
  bool                      m_debarLongPath;  //!< DeBaR in longest path.
};

};  // namespace ns3
#endif // RING_CONTROLLER_H
