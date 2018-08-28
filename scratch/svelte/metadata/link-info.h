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

#ifndef LINK_INFO_H
#define LINK_INFO_H

#include <ns3/core-module.h>
#include <ns3/csma-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../svelte-common.h"

namespace ns3 {

class LinkInfo;

/** A pair of switch datapath IDs. */
typedef std::pair<uint64_t, uint64_t> DpIdPair_t;

/** A list of link information objects. */
typedef std::list<Ptr<LinkInfo> > LinkInfoList_t;

/**
 * \ingroup svelteInfra
 * Metadata associated to a link between two OpenFlow backhaul switches.
 *
 * FIXME Review this text.
 * This class is prepared to handle network slicing. In current implementation,
 * the total number of slices is set to three: default, GBR and M2M traffic.
 * When the slicing mechanism is disabled by the Slicing attribute at
 * BackhaulController, only the default slice will be used. In this case, the
 * maximum bit rate for this slice will be set to the link bit rate. When the
 * slicing mechanism is enabled, then the size of each slice is defined by the
 * GbrSliceQuota and M2mSliceQuota attributes, which indicate the link
 * bandwidth ratio that should be assigned to the GBR and M2M slices,
 * respectively. All remaining bandwidth is assigned to the default slice. Each
 * slice can have some reserved bit rate for GBR traffic. The amount of
 * reserved bit rate is updated by reserve and release procedures, and are
 * enforced by OpenFlow meters that are regularly updated every time the total
 * reserved bit rate changes over a threshold value indicated by the
 * AdjustmentStep attribute. All bandwidth that is not reserved on any slice is
 * shared among best-effort traffic of all slices that don't have strict QoS
 * requirements. With this approach, we can ensure that we don't waste
 * available bandwidth when not in use.
 */
class LinkInfo : public Object
{
public:
  /** Metadata associated to a network slice. */
  struct SliceData
  {
    uint64_t maxRate;               //!< Maximum bit rate.
    uint64_t resRate [2];           //!< Reserved bit rate.
    double   ewmaThp [2];           //!< EWMA throughput.
    uint64_t txBytes [2];           //!< Total TX bytes.
    uint64_t lastTxBytes [2];       //!< Last timeout TX bytes.
    int64_t  meterDiff [2];         //!< Current meter bit rate diff.
  };

  /** Metadata associated to a switch. */
  struct SwitchData
  {
    Ptr<OFSwitch13Device> swDev;    //!< OpenFlow switch device.
    Ptr<CsmaNetDevice>    portDev;  //!< OpenFlow CSMA port device.
    uint32_t              portNo;   //!< OpenFlow port number.
  };

  /** Link direction. */
  enum Direction
  {
    FWD = 0,  //!< Forward direction (from first to second switch).
    BWD = 1   //!< Backward direction (from second to first switch).
  };

  /**
   * Complete constructor.
   * \param sw1 First switch metadata.
   * \param sw2 Second switch metadata.
   * \param channel The CsmaChannel physical link connecting these switches.
   * \attention The switch order must be the same as created by the CsmaHelper.
   * Internal channel handling is based on this order to get correct
   * full-duplex links.
   */
  LinkInfo (
    SwitchData sw1, SwitchData sw2, Ptr<CsmaChannel> channel);
  virtual ~LinkInfo ();   //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private OpenFlow switch accessors.
   * \param idx The internal switch index.
   * \return The requested field.
   */
  //\{
  uint32_t GetPortNo (uint8_t idx) const;
  uint64_t GetSwDpId (uint8_t idx) const;
  Ptr<const OFSwitch13Device> GetSwDev (uint8_t idx) const;
  Ptr<const CsmaNetDevice> GetPortDev (uint8_t idx) const;
  Mac48Address GetPortMacAddr (uint8_t idx) const;
  //\}

  /**
   * For two switches, this methods asserts that both datapath IDs are valid
   * for this link, and identifies the link direction based on source and
   * destination datapath IDs.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \return The link direction.
   */
  LinkInfo::Direction GetDirection (
    uint64_t src, uint64_t dst) const;

  /**
   * Get the EWMA throughput bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The EWMA throughput.
   */
  uint64_t GetThpBitRate (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the EWMA throughput ratio for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The bandwidth usage ratio.
   */
  double GetThpSliceRatio (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the available (not reserved) bit rate for traffic over this link on
   * the given direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available bit rate.
   */
  uint64_t GetFreeBitRate (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the available bit rate ratio for traffic over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available slice ratio.
   */
  double GetFreeSliceRatio (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the reserved bit rate for traffic over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved bit rate.
   */
  uint64_t GetResBitRate (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the reserved bit rate ratio for traffic over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved slice ratio.
   */
  double GetResSliceRatio (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the maximum bit rate for this link (same for both directions),
   * optionally filtered by the network slice. If no slice is given, the this
   * method will return the GetLinkBitRate ();
   * \param slice The network slice.
   * \return The maximum bit rate.
   */
  uint64_t GetMaxBitRate (
    SliceId slice = SliceId::ALL) const;

  /**
   * Get the pair of switch datapath IDs for this link, respecting the
   * internal order.
   * \return The pair of switch datapath IDs.
   */
  DpIdPair_t GetSwitchDpIdPair (void) const;

  /**
   * Get the total number of transmitted bytes over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The TX bytes.
   */
  uint64_t GetTxBytes (
    Direction dir, SliceId slice = SliceId::ALL) const;

  /**
   * Check for available bit rate between these two switches that can be
   * further reserved by ReserveGbrBitRate () method.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \param bitRate The bit rate to check.
   * \return True if there is available bit rate, false otherwise.
   */
  bool HasBitRate (
    uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate) const;

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplexLink (void) const;

  /**
   * Release the requested bit rate between these two switches on the given
   * network slice.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \param bitRate The bit rate to release.
   * \return True if succeeded, false otherwise.
   */
  bool ReleaseBitRate (
    uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate);

  /**
   * Reserve the requested bit rate between these two switches on the given
   * network slice.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \param bitRate The bit rate to reserve.
   * \return True if succeeded, false otherwise.
   */
  bool ReserveBitRate (
    uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate);

  /**
   * Get the string representing the given direction.
   * \param dir The link direction.
   * \return The link direction string.
   */
  static std::string DirectionStr (Direction dir);

  /**
   * Get the entire list of link information.
   * \return The list of link information.
   */
  static LinkInfoList_t GetList (void);

  /**
   * Get the link information from the global map for a pair of OpenFlow
   * datapath IDs.
   * \param dpId1 The first datapath ID.
   * \param dpId2 The second datapath ID.
   * \return The link information for this pair of datapath IDs.
   */
  static Ptr<LinkInfo> GetPointer (uint64_t dpId1, uint64_t dpId2);

  /**
   * TracedCallback signature for meter adjusted traced source.
   * \param lInfo The link information.
   * \param dir The link direction.
   * \param slice The network slice.
   */
  typedef void (*MeterAdjustedTracedCallback)(
    Ptr<const LinkInfo> lInfo, LinkInfo::Direction dir, SliceId slice);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /**
   * Inspect physical channel for the assigned bit rate, which is the same for
   * both directions in full-duplex links.
   * \return The channel maximum nominal bit rate (bps).
   */
  uint64_t GetLinkBitRate (void) const;

  /**
   * Notify this link of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Update the internal meter diff for firing the meter adjusted trace source
   * depending on current slicing operation mode.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The bit rate.
   * \param reserve True when reserving the bit rate, false when releasing.
   */
  void UpdateMeterDiff (
    Direction dir, SliceId slice, uint64_t bitRate, bool reserve);

  /**
   * Update link statistics.
   */
  void UpdateStatistics (void);

  /**
   * Register the link information in global map for further usage.
   * \param lInfo The link information to save.
   */
  static void RegisterLinkInfo (Ptr<LinkInfo> lInfo);

  /** Default meter bit rate adjusted trace source. */
  TracedCallback<Ptr<const LinkInfo>, Direction, SliceId> m_meterAdjustedTrace;

  SwitchData        m_switches [2];             //!< Metadata for switches.
  SliceData         m_slices [SliceId::ALL];    //!< Metadata for slices.
  Ptr<CsmaChannel>  m_channel;                  //!< The CSMA link channel.
  Time              m_lastUpdate;               //!< Last update time.

  DataRate          m_adjustmentStep;           //!< Meter adjustment step.
  double            m_alpha;                    //!< EWMA alpha parameter.
  Time              m_timeout;                  //!< Update timeout.

  /**
   * Map saving pair of switch datapath IDs / link information.
   * The pair of switch datapath IDs are saved in increasing order.
   */
  typedef std::map<DpIdPair_t, Ptr<LinkInfo> > LinkInfoMap_t;

  static LinkInfoMap_t  m_linksMap;             //!< Global link info map.
  static LinkInfoList_t m_linksList;            //!< Global link info list.
};

} // namespace ns3
#endif  // LINK_INFO_H
