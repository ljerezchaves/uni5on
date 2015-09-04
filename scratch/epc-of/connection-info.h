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
    FORWARD = 0,  //!< Forward direction (from m_sw1 to m_sw2)
    BACKWARD = 1  //!< Backwad direction (from m_sw2 to m_sw1)
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
  ConnectionInfo (SwitchData sw1, SwitchData sw2,
                  Ptr<CsmaChannel> channel);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  uint16_t GetSwIdxFirst (void) const;
  uint16_t GetSwIdxSecond (void) const;
  uint32_t GetPortNoFirst (void) const;
  uint32_t GetPortNoSecond (void) const;
  Ptr<const OFSwitch13NetDevice> GetSwDevFirst (void) const;
  Ptr<const OFSwitch13NetDevice> GetSwDevSecond (void) const;
  Ptr<const CsmaNetDevice> GetPortDevFirst (void) const;
  Ptr<const CsmaNetDevice> GetPortDevSecond (void) const;
  //\}

  /**
   * Get the pair of switch indexes for this connection, respecting the
   * internal order.
   * \return The pair of switch indexes.
   */
  SwitchPair_t GetSwitchIndexPair (void) const;

  /**
   * Get the bandwidth reserved ratio for GBR traffic in indicated direction,
   * calculated with respect to the total link capacity.
   * \return The usage ratio.
   */
  //\{
  //!< The GBR reserved ratio in forward direction
  double GetForwardGbrReservedRatio (void) const;

  //!< The GBR reserved ratio in backward direction
  double GetBackwardGbrReservedRatio (void) const;
  //\}

  /**
   * Get the bandwidth allowed ratio for Non-GBR traffic in indicated
   * direction, calculated with respect to the total link capacity.
   * \return The allowed ratio.
   */
  //\{
  //!< The Non-GBR allowed ratio in forward direction
  double GetForwardNonGbrAllowedRatio (void) const;

  //!< The Non-GBR allowed ratio in backward direction
  double GetBackwardNonGbrAllowedRatio (void) const;
  //\}


  /**
   * Get the number of bytes successfully transmitted in indicated direction
   * since the simulation began, or since ResetStatistics was called, according
   * to whichever happened more recently.
   * \return The number of transmitted bytes.
   */
  //\{
  //!< Total bytes in forward direction
  uint32_t GetForwardBytes (void) const;

  //!< Total bytes in backward direction
  uint32_t GetBackwardBytes (void) const;

  //!< GBR bytes in forward direction
  uint32_t GetForwardGbrBytes (void) const;

  //!< GBR bytes in backward direction
  uint32_t GetBackwardGbrBytes (void) const;

  //!< Non-GBR bytes in forward direction
  uint32_t GetForwardNonGbrBytes (void) const;

  //!< Non-GBR bytes in backward direction
  uint32_t GetBackwardNonGbrBytes (void) const;
  //\}

  /**
   * Reset internal byte counters.
   */
  void ResetStatistics (void);

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplex (void) const;

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
   * Get the available bit rate between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \return The available bit rate from srcIdx to dstIdx.
   */
  uint64_t GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx) const;

  /**
   * Get the available bit rate between these two switches, considering the
   * DeBaR reservation factor.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param debarFactor DeBaR reservation factor.
   * \return The available bit rate from srcIdx to dstIdx.
   */
  uint64_t GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   double debarFactor) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \attention An attempt to reserve more resources than those available will
   * result in fatal error.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param bitRate The bit rate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveGbrBitRate (uint16_t srcIdx, uint16_t dstIdx, uint64_t bitRate);

  /**
   * Release some bandwidth between these two switches.
   * \attention An attempt to release more resources than those reserved will
   * result in fatal error.
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
  void SetGbrReserveQuota (double value);

  /**
   * Update the GBR safeguard bandwidth value.
   * \param value The value to set.
   */
  void SetGbrSafeguard (DataRate value);

  /**
   * Update the Non-GBR adjustment step value.
   * \param value The value to set.
   */
  void SetNonGbrAdjStep (DataRate value);


  SwitchData        m_sw1;          //!< First switch (lowest index)
  SwitchData        m_sw2;          //!< Second switch (highest index)
  Ptr<CsmaChannel>  m_channel;      //!< The link channel connecting switches

  double            m_gbrReserveQuota;  //!< GBR link-capacity reserved quota
  uint64_t          m_gbrMaxBitRate;    //!< GBR maximum allowed bit rate
  uint64_t          m_gbrSafeguard;     //!< GBR safeguard bit rate
  uint64_t          m_gbrReserved [2];  //!< GBR current reserved bit rate

  uint64_t          m_nonAllowed [2];   //!< Non-GBR allowed bit rate
  uint64_t          m_nonAdjustStep;    //!< Non-GBR bit rate adjustment step

  uint32_t          m_gbrTxBytes [2];   //!< GBR transmitted bytes
  uint32_t          m_nonTxBytes [2];   //!< Non-GBR transmitted bytes
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
