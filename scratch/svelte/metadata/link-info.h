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

/** A list of link information objects. */
typedef std::vector<Ptr<LinkInfo> > LinkInfoList_t;

/**
 * \ingroup svelteInfra
 * Metadata associated to a link between two OpenFlow backhaul switches.
 *
 * The link is prepared to handle inter-slicing, and each slice has the
 * following metadata information associated to it:
 * - The slice quota, updated by the backhaul controller;
 * - The extra (over quota) bit rate, updated by the backhaul controller;
 * - The OpenFlow meter bit rate, updated by the backhaul controller;
 * - The reserved bit rate, updated by Reserve/ReleaseBitRate methods;
 * - The transmitted bytes, updated by NotifyTxPacket method;
 * - The average throughput, for both short-term and long-term periods of
 *   evaluation, periodically updated by UpdateEwmaThp method;
 */
class LinkInfo : public Object
{
  friend class BackhaulController;
  friend class RingController;

public:
  /** Link direction. */
  enum LinkDir
  {
    FWD = 0,  //!< Forward direction (from first to second switch).
    BWD = 1   //!< Backward direction (from second to first switch).
  };

// Total number of valid LinkDir items + 1.
#define N_LINK_DIRS (static_cast<int> (LinkDir::BWD) + 1)

  /** EWMA period of evaluation. */
  enum EwmaTerm
  {
    STERM = 0,  //!< Short-term EWMA evaluation.
    LTERM = 1  //!< Long-term EWMA evaluation.
  };

// Total number of valid EwmaTerm items + 1.
#define N_EWMA_TERMS (static_cast<int> (EwmaTerm::LTERM) + 1)

  /**
   * Complete constructor.
   * \param port1 First switch port.
   * \param port2 Second switch port.
   * \param channel The CsmaChannel physical link connecting these ports.
   * \attention The port order must be the same as created by the CsmaHelper.
   * Internal channel handling is based on this order to get correct
   * full-duplex links.
   */
  LinkInfo (Ptr<OFSwitch13Port> port1, Ptr<OFSwitch13Port> port2,
            Ptr<CsmaChannel> channel);
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
  Mac48Address          GetPortAddr   (uint8_t idx) const;
  Ptr<CsmaNetDevice>    GetPortDev    (uint8_t idx) const;
  uint32_t              GetPortNo     (uint8_t idx) const;
  Ptr<OFSwitch13Queue>  GetPortQueue  (uint8_t idx) const;
  Ptr<OFSwitch13Device> GetSwDev      (uint8_t idx) const;
  uint64_t              GetSwDpId     (uint8_t idx) const;
  Ptr<OFSwitch13Port>   GetSwPort     (uint8_t idx) const;
  //\}

  /**
   * For two switches, this methods asserts that both datapath IDs are valid
   * for this link, and identifies the link direction based on source and
   * destination datapath IDs.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \return The link direction.
   */
  LinkInfo::LinkDir GetLinkDir (
    uint64_t src, uint64_t dst) const;

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplexLink (void) const;

  /**
   * Inspect physical channel for the assigned bit rate, which is the same for
   * both directions in full-duplex links.
   * \return The channel maximum nominal bit rate (bps).
   */
  uint64_t GetLinkBitRate (void) const;

  /**
   * Get the slice quota for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The slice quota.
   */
  int GetQuota (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the quota bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The maximum bit rate.
   */
  uint64_t GetQuotaBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the extra bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The extra bit rate.
   */
  uint64_t GetExtraBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the reserved bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved bit rate.
   */
  uint64_t GetResBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the free (not reserved) bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available bit rate.
   */
  uint64_t GetFreeBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * FIXME Delete this method.
   * Get the meter bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available bit rate.
   */
  uint64_t GetOldMeterBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the meter bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available bit rate.
   */
  uint64_t GetMeterBitRate (
    LinkDir dir, SliceId slice = SliceId::ALL) const;

  /**
   * Get the EWMA throughput bit rate for this link on the given direction,
   * optionally filtered by the network slice and QoS traffic type.
   * \param term The EWMA period of evaluation.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param type Traffic QoS type.
   * \return The EWMA throughput.
   */
  uint64_t GetThpBitRate (
    EwmaTerm term, LinkDir dir, SliceId slice = SliceId::ALL,
    QosType type = QosType::BOTH) const;

  /**
   * Get the EWMA throughput ratio for this link on the given direction,
   * considering all traffic types (GBR and Non-GBR) and optionally filtered
   * by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The bandwidth usage ratio.
   */
  // double GetThpSliceRatio (
  //   LinkDir dir, SliceId slice = SliceId::ALL) const;

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
   * Print the link metadata for a specific network slice.
   * \param os The output stream.
   * \param slice The network slice.
   * \return The output stream.
   * \internal Keep this method consistent with the PrintHeader () method.
   */
  std::ostream & PrintSliceValues (std::ostream &os, SliceId slice) const;

  /**
   * Get the string representing the given direction.
   * \param dir The link direction.
   * \return The link direction string.
   */
  static std::string LinkDirStr (LinkDir dir);

  /**
   * Get the list of link information.
   * \return The const reference to the list of link information.
   */
  static const LinkInfoList_t& GetList (void);

  /**
   * Get the link information from the global map for a pair of OpenFlow
   * datapath IDs.
   * \param dpId1 The first datapath ID.
   * \param dpId2 The second datapath ID.
   * \return The link information for this pair of datapath IDs.
   */
  static Ptr<LinkInfo> GetPointer (uint64_t dpId1, uint64_t dpId2);

  /**
   * Get the header for the PrintSliceValues () method.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the PrintSliceValues () method.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * TracedCallback signature for meter adjusted traced source.
   * \param lInfo The link information.
   * \param dir The link direction.
   * \param slice The network slice.
   */
  typedef void (*MeterAdjustedTracedCallback)(
    Ptr<const LinkInfo> lInfo, LinkInfo::LinkDir dir, SliceId slice);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /** Metadata associated to a network slice. */
  struct SliceMetadata
  {
    int quota;                          //!< Slice quota (0-100%).
    uint64_t extra;                     //!< Extra (over quota) bit rate.
    uint64_t meter;                     //!< OpenFlow meter bit rate.
    uint64_t reserved;                  //!< Reserved bit rate.

    // FIXME Remove in the future.
    int64_t  meterDiff;                 //!< Current meter bit rate diff.

    /** EWMA throughput for both short-term and long-term averages. */
    uint64_t ewmaThp [N_QOS_TYPES][N_EWMA_TERMS];

    /** TX byte counters for each LTE QoS type. */
    uint64_t txBytes [N_QOS_TYPES];
  };

  /** A pair of switch datapath IDs. */
  typedef std::pair<uint64_t, uint64_t> DpIdPair_t;

  /**
   * Notify this link of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (std::string context, Ptr<const Packet> packet);

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
   * Update the slice quota for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param quota The value to update.
   * \return True if succeeded, false otherwise.
   */
  bool UpdateQuota (
    LinkDir dir, SliceId slice, int quota);

  /**
   * Update the extra bit rate for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The value to update.
   */
  void UpdateExtraBitRate (
    LinkDir dir, SliceId slice, int64_t bitRate);

  /**
   * Update the meter bit rate for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The value to update.
   */
  void UpdateMeterBitRate (
    LinkDir dir, SliceId slice, int64_t bitRate);

  /**
   * Update EWMA link throughput statistics.
   */
  void UpdateEwmaThp (void);

  /**
   * Update the internal meter diff for firing the meter adjusted trace source
   * depending on current slicing operation mode.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The bit rate.
   */
  void UpdateMeterDiff (
    LinkDir dir, SliceId slice, int64_t bitRate);

  /**
   * Register the link information in global map for further usage.
   * \param lInfo The link information to save.
   */
  static void RegisterLinkInfo (Ptr<LinkInfo> lInfo);

  /** Default meter bit rate adjusted trace source. */
  TracedCallback<Ptr<const LinkInfo>, LinkDir, SliceId> m_meterAdjustedTrace;

  DataRate              m_adjustmentStep;       //!< Meter adjustment step.
  Ptr<CsmaChannel>      m_channel;              //!< The CSMA link channel.
  Ptr<OFSwitch13Port>   m_ports [2];            //!< OpenFlow ports.

  /** Metadata for each network slice in each link direction. */
  SliceMetadata         m_slices [N_SLICE_IDS][N_LINK_DIRS];

  // EWMA throughput calculation.
  double                m_ewmaLtAlpha;          //!< EWMA long-term alpha.
  double                m_ewmaStAlpha;          //!< EWMA short-term alpha.
  Time                  m_ewmaTimeout;          //!< EWMA update timeout.
  Time                  m_ewmaLastTime;         //!< Last EWMA update time.

  /**
   * Map saving pair of switch datapath IDs / link information.
   * The pair of switch datapath IDs are saved in increasing order.
   */
  typedef std::map<DpIdPair_t, Ptr<LinkInfo> > LinkInfoMap_t;
  static LinkInfoMap_t  m_linkInfoByDpIds;      //!< Global link info map.

  static LinkInfoList_t m_linkInfoList;         //!< Global link info list.
};

} // namespace ns3
#endif  // LINK_INFO_H
