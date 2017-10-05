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

#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ofswitch13-module.h>
#include "../info/routing-info.h"

namespace ns3 {

class ConnectionInfo;

/** A pair of switch datapath IDs. */
typedef std::pair<uint64_t, uint64_t> DpIdPair_t;

/** A list of connection information objects. */
typedef std::list<Ptr<ConnectionInfo> > ConnInfoList_t;

/**
 * \ingroup sdmnInfo
 * Metadata associated to a connection between two OpenFlow switches.
 *
 * This class is prepared to handle network slicing. In current implementation,
 * the total number of slices is set to three: default, GBR and MTC traffic.
 * When the slicing mechanism is disabled by the Slicing attribute at
 * EpcController, only the default slice will be used. In this case, the
 * maximum bit rate for this slice will be set to the link bit rate. When the
 * slicing mechanism is enabled, then the size of each slice is defined by the
 * GbrSliceQuota and MtcSliceQuota attributes, which indicate the link
 * bandwidth ratio that should be assigned to the GBR and MTC slices,
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
class ConnectionInfo : public Object
{
public:
  /** Metadata associated to a network slice. */
  struct SliceData
  {
    uint64_t m_maxRate;               //!< Maximum bit rate.
    uint64_t m_resRate [2];           //!< Reserved bit rate.
    double   m_ewmaThp [2];           //!< EWMA throughput.
    uint64_t m_txBytes [2];           //!< Total TX bytes.
    uint64_t m_lastTxBytes [2];       //!< Last timeout TX bytes.
  };

  /** Metadata associated to a switch. */
  struct SwitchData
  {
    Ptr<OFSwitch13Device>     swDev;    //!< OpenFlow switch device.
    Ptr<CsmaNetDevice>        portDev;  //!< OpenFlow CSMA port device.
    uint32_t                  portNo;   //!< OpenFlow port number.
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
   * \param slicing True when slicing the network.
   * \attention The switch order must be the same as created by the CsmaHelper.
   * Internal channel handling is based on this order to get correct
   * full-duplex links.
   */
  ConnectionInfo (SwitchData sw1, SwitchData sw2,
                  Ptr<CsmaChannel> channel, bool slicing);
  virtual ~ConnectionInfo ();   //!< Dummy destructor, see DoDispose.

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
   * for this connection, and identifies the link direction based on source and
   * destination datapath IDs.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \return The connection direction.
   */
  ConnectionInfo::Direction GetDirection (uint64_t src, uint64_t dst) const;

  /**
   * Get the exponentially weighted moving average throughput metric for this
   * link on the given direction, optionally filtered by the network slice.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \return The EWMA throughput.
   */
  DataRate GetEwmaThp (uint64_t src, uint64_t dst,
                       Slice slice = Slice::ALL) const;

  /**
   * Inspect physical channel for the assigned bit rate.
   * \return The channel maximum nominal bit rate (bps).
   */
  uint64_t GetLinkBitRate (void) const;

  /**
   * Get the maximum bit rate for this link on the given direction, optionally
   * filtered by the network slice. If no slice is given, the this method will
   * return the GetLinkBitRate ();
   * \param slice The network slice.
   * \return The maximum bit rate.
   */
  uint64_t GetMaxBitRate (Slice slice = Slice::ALL) const;

  /**
   * Get the maximum bit rate for best-effort traffic over this link on the
   * given direction.
   * \param dir The link direction.
   * \return The maximum bit rate.
   */
  uint64_t GetMeterBitRate (Direction dir) const;

  /**
   * Get the reserved bit rate for traffic over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved bit rate.
   */
  uint64_t GetResBitRate (Direction dir, Slice slice = Slice::ALL) const;

  /**
   * Get the reserved link ratio for traffic over this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved link ratio.
   */
  double GetResLinkRatio (Direction dir, Slice slice = Slice::ALL) const;

  /**
   * Get the reserved slice ratio for traffic over this link on the given
   * direction for the given network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved link ratio.
   */
  double GetResSliceRatio (Direction dir, Slice slice) const;

  /**
   * Get the pair of switch datapath IDs for this connection, respecting the
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
  uint64_t GetTxBytes (Direction dir, Slice slice = Slice::ALL) const;

  /**
   * Check for available bit rate between these two switches that can be
   * further reserved by ReserveGbrBitRate () method.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \param bitRate The bit rate to check.
   * \return True if there is available bit rate, false otherwise.
   */
  bool HasBitRate (uint64_t src, uint64_t dst, Slice slice,
                   uint64_t bitRate) const;

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
  bool ReleaseBitRate (uint64_t src, uint64_t dst, Slice slice,
                       uint64_t bitRate);

  /**
   * Reserve the requested bit rate between these two switches on the given
   * network slice.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param slice The network slice.
   * \param bitRate The bit rate to reserve.
   * \return True if succeeded, false otherwise.
   */
  bool ReserveBitRate (uint64_t src, uint64_t dst, Slice slice,
                       uint64_t bitRate);

  /**
   * Get the entire list of connection information.
   * \return The list of connection information.
   */
  static ConnInfoList_t GetList (void);

  /**
   * Get the connection information from the global map for a pair of OpenFlow
   * datapath IDs.
   * \param dpId1 The first datapath ID.
   * \param dpId2 The second datapath ID.
   * \return The connection information for this pair of datapath IDs.
   */
  static Ptr<ConnectionInfo> GetPointer (uint64_t dpId1, uint64_t dpId2);

  /**
   * TracedCallback signature for Ptr<const ConnectionInfo>.
   * \param cInfo The connection information.
   */
  typedef void (*CInfoTracedCallback)(Ptr<const ConnectionInfo> cInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /**
   * Notify this connection of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Update link statistics.
   */
  void UpdateStatistics (void);

  /**
   * Register the connection information in global map for further usage.
   * \param cInfo The connection information to save.
   */
  static void RegisterConnectionInfo (Ptr<ConnectionInfo> cInfo);

  /** Default meter bit rate adjusted trace source. */
  TracedCallback<Ptr<const ConnectionInfo> > m_meterAdjustedTrace;

  SwitchData        m_switches [2];         //!< Switches metadata.
  Ptr<CsmaChannel>  m_channel;              //!< The CSMA link channel.
  Time              m_lastUpdate;           //!< Last update time.
  bool              m_slicing;              //!< Network slicing enabled.

  SliceData         m_slices [Slice::ALL];  //!< Slicing metadata.

  uint64_t          m_meterBitRate [2];     //!< Meter bit rate.
  int64_t           m_meterDiff [2];        //!< Current meter bit rate diff.
  int64_t           m_meterThresh;          //!< Meter bit rate threshold.

  DataRate          m_adjustmentStep;       //!< Meter adjustment step.
  double            m_alpha;                //!< EWMA alpha parameter.
  double            m_gbrSliceQuota;        //!< GBR slice quota.
  double            m_mtcSliceQuota;        //!< MTC slice quota.
  Time              m_timeout;              //!< Update timeout.

  /**
   * Map saving pair of switch datapath IDs / connection information.
   * The pair of switch datapath IDs are saved in increasing order.
   */
  typedef std::map<DpIdPair_t, Ptr<ConnectionInfo> > ConnInfoMap_t;

  static ConnInfoMap_t  m_connectionsMap;   //!< Global connection info map.
  static ConnInfoList_t m_connectionsList;  //!< Global connection info list.
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
