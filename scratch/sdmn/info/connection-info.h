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
typedef std::vector<Ptr<ConnectionInfo> > ConnInfoList_t;

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
    uint32_t                  portNum;  //!< OpenFlow port number.
  };

  /** Link direction. */
  enum Direction
  {
    FWD = 0,  //!< Forward direction (from first to second switch).
    BWD = 1   //!< Backwad direction (from second to firts switch).
  };

  ConnectionInfo ();            //!< Default constructor.
  virtual ~ConnectionInfo ();   //!< Dummy destructor, see DoDispose.

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
  uint32_t                    GetPortNo   (uint8_t idx) const;
  uint64_t                    GetSwDpId   (uint8_t idx) const;
  Ptr<const OFSwitch13Device> GetSwDev    (uint8_t idx) const;
  Ptr<const CsmaNetDevice>    GetPortDev  (uint8_t idx) const;
  //\}

  /**
   * \name Link usage statistics.
   * Get link usage statistics, for both GBR and Non-GBR traffic.
   * \param dir The link direction.
   * \return The requested information.
   */
  //\{
  uint32_t GetGbrBytes        (Direction dir) const;
  uint64_t GetGbrBitRate      (Direction dir) const;
  double   GetGbrLinkRatio    (Direction dir) const;
  uint32_t GetNonGbrBytes     (Direction dir) const;
  uint64_t GetNonGbrBitRate   (Direction dir) const;
  double   GetNonGbrLinkRatio (Direction dir) const;
  //\}

  /**
   * Reset internal transmitted byte counters.
   */
  void ResetTxBytes (void);

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
   * Get the available bit rate between these two switches. Optionally, this
   * function can considers the DeBaR reservation factor.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param debarFactor DeBaR reservation factor.
   * \return The available bit rate from src to dst.
   */
  uint64_t GetAvailableGbrBitRate (uint64_t src, uint64_t dst,
                                   double debarFactor = 1.0) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param bitRate The bit rate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveGbrBitRate (uint64_t src, uint64_t dst, uint64_t bitRate);

  /**
   * Release some bandwidth between these two switches.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \param bitRate The bit rate to release.
   * \return True if everything is ok, false otherwise.
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
   * \return The connection information for this pair of datapath IDs.
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

  /**
   * Notify this connection of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (std::string context, Ptr<const Packet> packet);

private:
  /**
   * Get the guard bit rate, which is currently not been used neither by GBR
   * nor Non-GBR traffic.
   * \param dir The link direction.
   * \return The current guard bit rate.
   */
  uint64_t GetGuardBitRate (Direction dir) const;

  /**
   * \name Bit rate adjustment.
   * Increase/decrease the GBR reserved bit rate and Non-GBR allowed bit rate.
   * \param dir The link direction.
   * \param bitRate The bitRate amount.
   * \return True if everything is ok, false otherwise.
   */
  //\{
  bool IncreaseGbrBitRate (Direction dir, uint64_t bitRate);
  bool DecreaseGbrBitRate (Direction dir, uint64_t bitRate);
  bool IncreaseNonGbrBitRate (Direction dir, uint64_t bitRate);
  bool DecreaseNonGbrBitRate (Direction dir, uint64_t bitRate);
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
   * Register the connection information in global map for further usage.
   * \param cInfo The connection information to save.
   */
  static void RegisterConnectionInfo (Ptr<ConnectionInfo> cInfo);

  /** Non-GBR allowed bit rate adjusted trace source. */
  TracedCallback<Ptr<ConnectionInfo> > m_nonAdjustedTrace;

  SwitchData        m_switches [2];     //!< Switches metadata.
  Ptr<CsmaChannel>  m_channel;          //!< The CSMA link channel.

  double            m_gbrLinkQuota;     //!< GBR link-capacity reserved quota.
  uint64_t          m_gbrSafeguard;     //!< GBR safeguard bit rate.
  uint64_t          m_nonAdjustStep;    //!< Non-GBR bit rate adjustment step.

  uint64_t          m_gbrMaxBitRate;    //!< GBR maximum allowed bit rate.
  uint64_t          m_gbrMinBitRate;    //!< GBR maximum allowed bit rate.
  uint64_t          m_gbrBitRate [2];   //!< GBR current reserved bit rate.
  uint32_t          m_gbrTxBytes [2];   //!< GBR transmitted bytes.

  uint64_t          m_nonMaxBitRate;    //!< Non-GBR maximum allowed bit rate.
  uint64_t          m_nonMinBitRate;    //!< Non-GBR maximum allowed bit rate.
  uint64_t          m_nonBitRate [2];   //!< Non-GBR allowed bit rate.
  uint32_t          m_nonTxBytes [2];   //!< Non-GBR transmitted bytes.

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
