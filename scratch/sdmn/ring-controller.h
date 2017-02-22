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
#include "info/ring-routing-info.h"

namespace ns3 {

/** A pair of switches indexes. */
typedef std::pair<uint16_t, uint16_t> IndexPair_t;

/**
 * \ingroup sdmn
 * OpenFlow EPC controller for ring network.
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
  void NewS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
                    Ptr<NetDevice> gwDev, Ipv4Address gwIp);
  void NewSwitchConnection (Ptr<ConnectionInfo> cInfo);
  void TopologyBuilt (OFSwitch13DeviceContainer devices);
  bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer);
  bool TopologyRemoveRouting (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo);
  bool TopologyBearerRelease (Ptr<RoutingInfo> rInfo);
  void TopologyCreateSpanningTree ();
  // Inherited from EpcController.

  /**
   * Notify this controller when the Non-GBR allowed bit rate in any network
   * connection is adjusted. This is used to update Non-GBR meters bands based
   * on GBR resource reservation.
   * \param cInfo The connection information
   */
  void NonGbrAdjusted (Ptr<ConnectionInfo> cInfo);

  /** \return The number of switches in the network. */
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
   * Search for connection information between two switches by their indexes.
   * \param sw1 First switch index.
   * \param sw2 Second switch index.
   * \return Pointer to connection info saved.
   */
  Ptr<ConnectionInfo> GetConnectionInfoByIdx (uint16_t sw1, uint16_t sw2);

  /**
   * Save the pair IP address / switch index in switch table.
   * \param ipAddr The IPv4 address.
   * \param swtchDev The OpenFlow switch device.
   */
  void SaveIpSwitchPair (Ipv4Address ipAddr, Ptr<OFSwitch13Device> swtchDev);

  /**
   * Retrieve the switch index for IP address.
   * \param ipAddr The IPv4 address.
   * \return The switch index in devices collection.
   */
  uint16_t GetSwitchIndex (Ipv4Address ipAddr) const;

  /**
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param index The switch index in devices colection.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDatapathId (uint16_t index) const;

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
   * Calculate the number of hops between source and destination for the
   * indicated routing path.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \return The number of hops in routing path.
   */
  uint16_t HopCounter (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                       RingRoutingInfo::RoutingPath routingPath);

  /**
   * Get the available GBR bit rate for this ring routing information,
   * considering both downlink and uplink paths.
   * \internal
   * This method implements the GBR Distance-Based Reservation algorithm
   * (DeBaR) proposed by prof. Deep Medhi. The general idea is a dynamic bit
   * rate usage factor that can be adjusted based on the distance between the
   * eNB switch and the gateway switch.
   * \param ringInfo The ring routing information.
   * \param useShortPath When true, get the available bit rate in the shortest
   * path between source and destination nodes; otherwise, considers the
   * inverted (long) path.
   * \return A pair of available GBR bit rates, for both downlink and uplink
   * paths, in this strict order.
   */
  std::pair<uint64_t, uint64_t>
  GetAvailableGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          bool useShortPath = true);

  /**
   * Reserve the bit rate for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \return True if success, false otherwise;
   */
  bool ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo);

  /**
   * Release the bit rate for this bearer in network.
   * \param ringInfo The ring routing information.
   * \param gbrInfo The GBR information.
   * \return True if success, false otherwise;
   */
  bool ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                          Ptr<GbrInfo> gbrInfo);

  /**
   * Reserve the indicated bit rate at each link from source to
   * destination switch index following the indicated routing path.
   * \attention To avoid fatal errors, be sure that there is enough available
   * bit rate in this link before reserving it.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param bitRate The bit rate to reserve.
   * \return True if success, false otherwise;
   */
  bool PerLinkReserve (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                       RingRoutingInfo::RoutingPath routingPath,
                       uint64_t bitRate);

  /**
   * Release the indicated bit rate at each link from source to
   * destination switch index following the indicated routing path.
   * \attention To avoid fatal errors, be sure that there is enough reserved
   * bit rate in this link before releasing it.
   * \param srcSwitchIdx Source switch index.
   * \param dstSwitchIdx Destination switch index.
   * \param routingPath The routing path.
   * \param bitRate The bit rate to release.
   * \return True if success, false otherwise;
   */
  bool PerLinkRelease (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                       RingRoutingInfo::RoutingPath routingPath,
                       uint64_t bitRate);

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

  OFSwitch13DeviceContainer m_ofDevices;      //!< OpenFlow devices.
  RoutingStrategy           m_strategy;       //!< Routing strategy in use.
  double                    m_debarStep;      //!< DeBaR increase step.
  bool                      m_debarShortPath; //!< DeBaR in shortest path.
  bool                      m_debarLongPath;  //!< DeBaR in longest path.

  /**
   * Map saving pair of switch indexes / connection information.
   * The pair of switch datapath IDs are saved in increasing order.
   */
  typedef std::map<DpIdPair_t, Ptr<ConnectionInfo> > ConnInfoMap_t;
  ConnInfoMap_t       m_connections;      //!< Connections between switches.

  /** Map saving IPv4 address / Switch index */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;
  IpSwitchMap_t       m_ipSwitchTable;    //!< eNB IP / Switch Index table.
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

