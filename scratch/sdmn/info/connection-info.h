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

namespace ns3 {

class ConnectionInfo;

/** A pair of switch datapath IDs. */
typedef std::pair<uint64_t, uint64_t> DpIdPair_t;

/** A list of connection information objects. */
typedef std::list<Ptr<ConnectionInfo> > ConnInfoList_t;

/**
 * \ingroup sdmnInfo
 * Metadata associated to a connection between
 * two any switches in the OpenFlow network.
 */
class ConnectionInfo : public Object
{
public:
  /** Metadata associated to a switch. */
  struct SwitchData
  {
    Ptr<OFSwitch13Device>     swDev;    //!< OpenFlow switch device.
    Ptr<CsmaNetDevice>        portDev;  //!< OpenFlow csma port device.
    uint32_t                  portNo;   //!< OpenFlow port number.
  };

  /** Link direction. */
  enum Direction
  {
    FWD = 0,  //!< Forward direction (from first to second switch).
    BWD = 1   //!< Backwad direction (from second to firts switch).
  };

  /**
   * Complete constructor.
   * \param sw1 First switch metadata.
   * \param sw2 Second switch metadata.
   * \param channel The CsmaChannel physical link connecting these switches.
   * \attention The switch order must be the same as created by the CsmaHelper.
   * Internal channel handling is based on this order to get corret full-duplex
   * links.
   */
  ConnectionInfo (SwitchData sw1, SwitchData sw2, Ptr<CsmaChannel> channel);
  virtual ~ConnectionInfo ();   //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the pair of switch datapath IDs for this connection, respecting the
   * internal order.
   * \return The pair of switch datapath IDs.
   */
  DpIdPair_t GetSwitchDpIdPair (void) const;

  /**
   * \name Private member accessors.
   * \param idx The internal switch index.
   * \return The requested field.
   */
  //\{
  uint32_t GetPortNo (uint8_t idx) const;
  uint64_t GetSwDpId (uint8_t idx) const;
  Ptr<const OFSwitch13Device> GetSwDev (uint8_t idx) const;
  Ptr<const CsmaNetDevice> GetPortDev (uint8_t idx) const;
  //\}

  /**
   * \name Link usage statistics.
   * Get link usage statistics, for both GBR and Non-GBR traffic.
   * \param dir The link direction.
   * \return The requested information.
   */
  //\{
  uint64_t GetGbrTxBytes    (Direction dir) const;
  uint64_t GetNonGbrTxBytes (Direction dir) const;
  DataRate GetGbrEwmaThp    (Direction dir) const;
  DataRate GetNonGbrEwmaThp (Direction dir) const;
  DataRate GetEwmaThp       (Direction dir) const;
  //\}

  /**
   * \name Reserved bit rate statistics.
   * Get reserved bit rate for both GBR and Non-GBR traffic.
   * \param dir The link direction.
   * \return The requested information.
   */
  //\{
  uint64_t GetResGbrBitRate      (Direction dir) const;
  uint64_t GetResNonGbrBitRate   (Direction dir) const;
  double   GetResGbrLinkRatio    (Direction dir) const;
  double   GetResNonGbrLinkRatio (Direction dir) const;
  //\}

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplexLink (void) const;

  /**
   * Inspect physical channel for the assigned bit rate.
   * \return The channel maximum nominal bit rate (bps).
   */
  uint64_t GetLinkBitRate (void) const;

  /**
   * For two switch, this methods asserts that boths datapath IDs are valid for
   * this connection, and identifies the link direction based on source and
   * destination datapath IDs.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \return The connection direction.
   */
  ConnectionInfo::Direction GetDirection (uint64_t src, uint64_t dst) const;

  /**
   * Check for available GBR bit rate between these two switches that can be
   * reserved by GRB bearers.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param bitRate The bit rate to check.
   * \return True if there's available GBR bit rate, false otherwise.
   */
  bool HasGbrBitRate (uint64_t src, uint64_t dst, uint64_t bitRate) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param bitRate The bit rate to reserve.
   * \return True if succeeded, false otherwise.
   */
  bool ReserveGbrBitRate (uint64_t src, uint64_t dst, uint64_t bitRate);

  /**
   * Release some bandwidth between these two switches.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param bitRate The bit rate to release.
   * \return True if succeeded, false otherwise.
   */
  bool ReleaseGbrBitRate (uint64_t src, uint64_t dst, uint64_t bitRate);

  /**
   * Get the connection information from the global map for a pair of OpenFlow
   * datapath IDs.
   * \param dpId1 The first datapath ID.
   * \param dpId2 The second datapath ID.
   * \return The connection information for this pair of datapath IDs.
   */
  static Ptr<ConnectionInfo> GetPointer (uint64_t dpId1, uint64_t dpId2);

  /**
   * Get the entire list of connection information.
   * \return The list of connection information.
   */
  static ConnInfoList_t GetList (void);

  /**
   * TracedCallback signature for Ptr<ConnectionInfo>.
   * \param cInfo The connection information and metadata.
   */
  typedef void (*ConnTracedCallback)(Ptr<ConnectionInfo> cInfo);

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
   * Get the guard bit rate, which is currently not been used neither by GBR
   * nor Non-GBR traffic.
   * \param dir The link direction.
   * \return The current guard bit rate.
   */
  uint64_t GetGuardBitRate (Direction dir) const;

  /**
   * \name Reserved bit rate adjustment.
   * Increase/decrease the reserved bit rate for both GBR and Non-GBR traffic.
   * \param dir The link direction.
   * \param bitRate The bitRate amount.
   * \return True if succeeded, false otherwise.
   */
  //\{
  bool IncResGbrBitRate    (Direction dir, uint64_t bitRate);
  bool DecResGbrBitRate    (Direction dir, uint64_t bitRate);
  bool IncResNonGbrBitRate (Direction dir, uint64_t bitRate);
  bool DecResNonGbrBitRate (Direction dir, uint64_t bitRate);
  //\}

  /**
   * Update the GBR reserve quota and GBR maximum bit rate.
   * \param value The value to set.
   */
  void SetGbrLinkQuota (double value);

  /**
   * Update the GBR safeguard bandwidth value.
   * \param value The value to set.
   */
  void SetGbrSafeguard (DataRate value);

  /**
   * Update the Non-GBR adjustment step value.
   * \param value The value to set.
   */
  void SetNonGbrAdjustStep (DataRate value);

  /**
   * Update link statistics.
   */
  void UpdateStatistics (void);

  /**
   * Register the connection information in global map for further usage.
   * \param cInfo The connection information to save.
   */
  static void RegisterConnectionInfo (Ptr<ConnectionInfo> cInfo);

  /** Non-GBR allowed bit rate adjusted trace source. */
  TracedCallback<Ptr<ConnectionInfo> > m_nonAdjustedTrace;

  SwitchData        m_switches [2];     //!< Switches metadata.
  Ptr<CsmaChannel>  m_channel;          //!< The CSMA link channel.
  Time              m_timeout;          //!< Update timeout.
  double            m_alpha;            //!< EWMA alpha parameter.
  Time              m_lastUpdate;       //!< Last update time.

  double            m_gbrLinkQuota;     //!< GBR link-capacity reserved quota.
  uint64_t          m_gbrSafeguard;     //!< GBR safeguard bit rate.
  uint64_t          m_nonAdjustStep;    //!< Non-GBR bit rate adjustment step.

  uint64_t          m_gbrMaxBitRate;    //!< GBR maximum allowed bit rate.
  uint64_t          m_gbrMinBitRate;    //!< GBR minimum allowed bit rate.
  uint64_t          m_gbrBitRate [2];   //!< GBR reserved bit rate.
  uint64_t          m_gbrTxBytes [2];   //!< GBR transmitted bytes.
  uint64_t          m_gbrAvgLast [2];   //!< GBR last transmitted bytes.
  double            m_gbrAvgThpt [2];   //!< GBR EWMA throughput.

  uint64_t          m_nonMaxBitRate;    //!< Non-GBR maximum allowed bit rate.
  uint64_t          m_nonMinBitRate;    //!< Non-GBR maximum allowed bit rate.
  uint64_t          m_nonBitRate [2];   //!< Non-GBR reserved bit rate.
  uint64_t          m_nonTxBytes [2];   //!< Non-GBR transmitted bytes.
  uint64_t          m_nonAvgLast [2];   //!< Non-GBR last transmitted bytes.
  double            m_nonAvgThpt [2];   //!< Non-GBR EWMA throughput.

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
