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

/** A pair of switches index */
typedef std::pair<uint16_t, uint16_t> SwitchPair_t;

/**
 * \ingroup epcof
 * Metadata associated to a connection between
 * two any switches in the OpenFlow network.
 */
class ConnectionInfo : public Object
{
  friend class RingController;

public:
  /**
   * Metadata associated to a switch.
   */
  struct SwitchData
  {
    uint16_t                  swIdx;    //!< Switch index
    Ptr<OFSwitch13NetDevice>  swDev;    //!< OpenFlow switch device
    Ptr<CsmaNetDevice>        portDev;  //!< OpenFlow csma port device
    uint32_t                  portNum;  //!< OpenFlow port number
  };

  /**
   * Link direction index.
   */
  enum Direction
  {
    FWD = 0,  //!< Forward direction (from first to second switch)
    BWD = 1   //!< Backwad direction (from second to firts switch)
  };

  ConnectionInfo ();            //!< Default constructor
  virtual ~ConnectionInfo ();   //!< Dummy destructor, see DoDipose

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
   * Get the pair of switch indexes for this connection, respecting the
   * internal order.
   * \return The pair of switch indexes.
   */
  SwitchPair_t GetSwitchIndexPair (void) const;

  /**
   * \name Private member accessors.
   * \param idx The internal switch index.
   * \return The requested field.
   */
  //\{
  uint16_t                        GetSwIdx    (uint8_t idx) const;
  uint32_t                        GetPortNo   (uint8_t idx) const;
  Ptr<const OFSwitch13NetDevice>  GetSwDev    (uint8_t idx) const;
  Ptr<const CsmaNetDevice>        GetPortDev  (uint8_t idx) const;
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
   * For two switch indexes, this methods asserts that boths indexes are valid
   * for this connection, and identifies the link direction based on source and
   * destination indexes.
   * \param src The source switch index.
   * \param dst The destination switch index.
   * \return The connection direction.
   */
  ConnectionInfo::Direction GetDirection (uint16_t src, uint16_t dst) const;

  /**
   * TracedCallback signature for Ptr<ConnectionInfo>.
   * \param cInfo The connection information and metadata.
   */
  typedef void (* ConnTracedCallback)(Ptr<ConnectionInfo> cInfo);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Notify this connection of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Get the available bit rate between these two switches. Optionally, this
   * function can considers the DeBaR reservation factor.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param debarFactor DeBaR reservation factor.
   * \return The available bit rate from srcIdx to dstIdx.
   */
  uint64_t GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   double debarFactor = 1.0) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param bitRate The bit rate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveGbrBitRate (uint16_t srcIdx, uint16_t dstIdx, uint64_t bitRate);

  /**
   * Release some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param bitRate The bit rate to release.
   * \return True if everything is ok, false otherwise.
   */
  bool ReleaseGbrBitRate (uint16_t srcIdx, uint16_t dstIdx, uint64_t bitRate);

private:
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

  /** Non-GBR allowed bit rate adjusted trace source. */
  TracedCallback<Ptr<ConnectionInfo> > m_nonAdjustedTrace;

  SwitchData        m_switches [2];     //!< Switches metadata
  Ptr<CsmaChannel>  m_channel;          //!< The CSMA link channel

  double            m_gbrLinkQuota;     //!< GBR link-capacity reserved quota
  uint64_t          m_gbrMaxBitRate;    //!< GBR maximum allowed bit rate
  uint64_t          m_gbrSafeguard;     //!< GBR safeguard bit rate
  uint64_t          m_gbrBitRate [2];   //!< GBR current reserved bit rate
  uint32_t          m_gbrTxBytes [2];   //!< GBR transmitted bytes

  uint64_t          m_nonAdjustStep;    //!< Non-GBR bit rate adjustment step
  uint64_t          m_nonBitRate [2];   //!< Non-GBR allowed bit rate
  uint32_t          m_nonTxBytes [2];   //!< Non-GBR transmitted bytes
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
