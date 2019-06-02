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

#include "backhaul-controller.h"
#include "../metadata/ring-info.h"
#include "../svelte-common.h"

namespace ns3 {

class EnbInfo;

/**
 * \ingroup svelteInfra
 * OpenFlow backhaul controller for ring topology.
 */
class RingController : public BackhaulController
{
  friend class RingNetwork;

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
   * Get the routing strategy attribute.
   * \return The routing strategy.
   */
  RoutingStrategy GetRoutingStrategy (void) const;

  /**
   * Get the string representing the given routing strategy.
   * \param strategy The routing strategy.
   * \return The routing strategy string.
   */
  static std::string RoutingStrategyStr (RoutingStrategy strategy);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  // Inherited from BackhaulController.
  bool BearerRequest (Ptr<RoutingInfo> rInfo);
  bool BearerReserve (Ptr<RoutingInfo> rInfo);
  bool BearerRelease (Ptr<RoutingInfo> rInfo);
  bool BearerInstall (Ptr<RoutingInfo> rInfo);
  bool BearerRemove  (Ptr<RoutingInfo> rInfo);
  bool BearerUpdate  (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo);
  void NotifyBearerCreated (Ptr<RoutingInfo> rInfo);
  void NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices);
  // Inherited from BackhaulController.

  // Inherited from OFSwitch13Controller.
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:
  /**
   * Check the available bit rate for this bearer for given LTE interface. This
   * method checks for "doubled" resources on overlapping links.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \param overlap The optional overlapping links.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateRequest (Ptr<RingInfo> ringInfo, LteIface iface,
                       LinkInfoSet_t *overlap = 0) const;

  /**
   * Check the forward and backward available bit rate on links from the source
   * to the destination switch following the given routing path. This method
   * checks for "doubled" resources on overlapping links, respecting the block
   * threshold.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param fwdBitRate The forwarding bit rate.
   * \param bwdBitRate The backward bit rate.
   * \param path The routing path.
   * \param slice The network slice.
   * \param blockThs The reserved block threshold.
   * \param overlap The optional overlapping links.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateRequest (uint16_t srcIdx, uint16_t dstIdx,
                       int64_t fwdBitRate, int64_t bwdBitRate,
                       RingInfo::RingPath path, SliceId slice,
                       double blockThs, LinkInfoSet_t *overlap = 0) const;

  /**
   * Reserve the bit rate for this bearer for the given LTE logical interface.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateReserve (Ptr<RingInfo> ringInfo, LteIface iface);

  /**
   * Reserve the forward and backward bit rate on links from the source to the
   * destination switch following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param fwdBitRate The forwarding bit rate.
   * \param bwdBitRate The backward bit rate.
   * \param path The routing path.
   * \param slice The network slice.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateReserve (uint16_t srcIdx, uint16_t dstIdx,
                       int64_t fwdBitRate, int64_t bwdBitRate,
                       RingInfo::RingPath path, SliceId slice);

  /**
   * Release the bit rate for this bearer for the given LTE logical interface.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateRelease (Ptr<RingInfo> ringInfo, LteIface iface);

  /**
   * Release the forward and backward bit rate on links from the source to the
   * destination switch following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param fwdBitRate The forwarding bit rate.
   * \param bwdBitRate The backward bit rate.
   * \param path The routing path.
   * \param slice The network slice.
   * \return True if succeeded, false otherwise.
   */
  bool BitRateRelease (uint16_t srcIdx, uint16_t dstIdx,
                       int64_t fwdBitRate, int64_t bwdBitRate,
                       RingInfo::RingPath path, SliceId slice);

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
  RingInfo::RingPath FindShortestPath (uint16_t srcIdx, uint16_t dstIdx) const;

  /**
   * Save the backhaul lInfo pointer for the given LTE interface, optionally
   * following the given routing path.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \param links The set of links to populate.
   */
  void GetLinks (Ptr<RingInfo> ringInfo, LteIface iface,
                 LinkInfoSet_t *links) const;

  /**
   * Check for the available resources on the backhaul infrastructure for the
   * given LTE interface. When any of the requested resources is not available,
   * this method must set the routing information with the block reason.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \param overlap The optional overlapping links.
   * \return True if succeeded, false otherwise.
   */
  bool HasAvailableResources (Ptr<RingInfo> ringInfo, LteIface iface,
                              LinkInfoSet_t *overlap = 0) const;

  /**
   * Check for the CPU usage on switches for the given LTE interface,
   * optionally following the given routing path.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool HasSwitchCpu (Ptr<RingInfo> ringInfo, LteIface iface) const;

  /**
   * Check for the flow table usage on switches for the given LTE interface,
   * optionally following the given routing path.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool HasSwitchTable (Ptr<RingInfo> ringInfo, LteIface iface) const;

  /**
   * Count the number of hops between source and destination switch indexes
   * following the given routing path.
   * \param srcIdx Source switch index.
   * \param dstIdx Destination switch index.
   * \param path The routing path.
   * \return The number of hops in routing path.
   */
  uint16_t HopCounter (uint16_t srcIdx, uint16_t dstIdx,
                       RingInfo::RingPath path) const;

  /**
   * Get the next switch index following the given routing path.
   * \param idx Current switch index.
   * \param path The routing path direction.
   * \return The next switch index.
   */
  uint16_t NextSwitchIndex (uint16_t idx, RingInfo::RingPath path) const;

  /**
   * Install forwarding rules on switches for the given LTE interface.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool RulesInstall (Ptr<RingInfo> ringInfo, LteIface iface);

  /**
   * Remove forwarding rules from switches for the given LTE interface.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \return True if succeeded, false otherwise.
   */
  bool RulesRemove (Ptr<RingInfo> ringInfo, LteIface iface);

  /**
   * Update forwarding rules on switches for the given LTE interface after a
   * successful handover procedure.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  bool RulesUpdate (Ptr<RingInfo> ringInfo, LteIface iface,
                    Ptr<EnbInfo> dstEnbInfo);

  /**
   * Set the default ring routing paths to the shortest ones.
   * \param ringInfo The ring routing information.
   * \param iface The LTE logical interface.
   */
  void SetShortestPath (Ptr<RingInfo> ringInfo, LteIface iface) const;

  /**
   * Apply the infrastructure inter-slicing OpenFlow meters.
   * \param swtch The switch information.
   * \param slice The network slice.
   */
  void SlicingMeterApply (Ptr<const RemoteSwitch> swtch, SliceId slice);

  RoutingStrategy           m_strategy;       //!< Routing strategy in use.
};

} // namespace ns3
#endif // RING_CONTROLLER_H
