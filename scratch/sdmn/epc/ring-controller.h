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

  /**
   * Get the string representing the given routing strategy.
   * \param strategy The routing strategy.
   * \return The routing strategy string.
   */
  static std::string RoutingStrategyStr (RoutingStrategy strategy);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from EpcController.
  void NotifyS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
                       Ptr<NetDevice> gwDev);
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);
  void NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo);

  void TopologyBearerCreated   (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRelease   (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRequest   (Ptr<RoutingInfo> rInfo);
  double TopologyLinkUsage     (Ptr<RoutingInfo> rInfo);
  bool TopologyRoutingInstall  (Ptr<RoutingInfo> rInfo);
  bool TopologyRoutingRemove   (Ptr<RoutingInfo> rInfo);
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
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param idx The switch index in devices colection.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDpId (uint16_t idx) const;

  /**
   * Get the number of switches in the network.
   * \return The number of switches in the network.
   */
  uint16_t GetNSwitches (void) const;

  /**
   * Get the average link use ratio between source and destination switch
   * indexes following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param slice The network slice.
   * \param path The routing path.
   * \return The link use ratio on this routing path.
   */
  double GetPathUseRatio (uint16_t srcIdx, uint16_t dstIdx, Slice slice,
                          RingRoutingInfo::RoutingPath path) const;

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
   * Check for the available GBR bit rate.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \param slice The network slice.
   * \return True if there's available GBR bit rate, false otherwise.
   */
  bool HasGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                      Ptr<const GbrInfo> gbrInfo, Slice slice) const;

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
   * Notify this controller when the the maximum bit rate for best-effort
   * traffic in any network connection is adjusted. This is used to update
   * meters bands based on slicing resource reservation.
   * \param cInfo The connection information
   */
  void MeterAdjusted (Ptr<const ConnectionInfo> cInfo);

  /**
   * Get the next switch index following the given routing path.
   * \param idx Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */
  uint16_t NextSwitchIndex (uint16_t idx,
                            RingRoutingInfo::RoutingPath path) const;

  /**
   * Release the bit rate for this GBR bearer in the ring network.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \param slice The network slice.
   * \return True if succeeded, false otherwise.
   */
  bool ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo, Slice slice);

  /**
   * Reserve the bit rate for this GBR bearer in the ring network.
   * \attention To avoid fatal errors, be sure that there is available GBR
   *            bit rate over the routing path before reserving it.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \param slice The network slice.
   * \return True if succeeded, false otherwise.
   */
  bool ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo, Slice slice);

  /** Map saving IPv4 address / switch index. */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;

  IpSwitchMap_t             m_ipSwitchTable;  //!< IP / switch index table.
  OFSwitch13DeviceContainer m_ofDevices;      //!< OpenFlow devices.
  RoutingStrategy           m_strategy;       //!< Routing strategy in use.
};

};  // namespace ns3
#endif // RING_CONTROLLER_H
