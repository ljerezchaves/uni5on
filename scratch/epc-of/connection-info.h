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
    FORWARD = 0,  //!< Forward direction
    BACKWARD = 1  //!< Backwad direction
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
   * Get the pair of switch indexes for this connection, respecting the
   * internal order.
   * \param The pair of switch indexes.
   */
  SwitchPair_t GetSwitchIndexPair (void) const;

  /**
   * \name Switch metadata member accessors.
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
   * Return the bandwidth reserved ratio, considering both forward and backward
   * paths for full-duplex channel. This method ignores the saving reserve
   * factor.
   * \return The usage ratio.
   */
  double GetReservedRatio (void) const;

  /**
   * Return the bandwidth reserved ratio in forward direction, This method ignores
   * the saving reserve factor.
   * \return The usage ratio.
   */
  double GetFowardReservedRatio (void) const;

  /**
   * Return the bandwidth reserved ratio in backward direction, This method
   * ignores the saving reserve factor.
   * \return The usage ratio.
   */
  double GetBackwardReservedRatio (void) const;

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplex (void) const;

  /**
   * Inspect physical channel for the assigned data rate.
   * \return The channel maximum nominal data rate.
   */
  DataRate LinkDataRate (void) const;

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
   * Get the available bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \return The available data rate from srcIdx to dstIdx.
   */
  DataRate GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx) const;

  /**
   * Get the available bandwidth between these two switches, considering the
   * maximum bandwidth reservation factor.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param maxBwFactor The maximum bandwidth reservation factor.
   * \return The available data rate from srcIdx to dstIdx.
   */
  DataRate GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx, 
                                 double maxBwFactor) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param rate The DataRate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate rate);

  /**
   * Release some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param rate The DataRate to release.
   * \return True if everything is ok, false otherwise.
   */
  bool ReleaseDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate rate);

private:
  SwitchData        m_sw1;          //!< First switch (lowest index)
  SwitchData        m_sw2;          //!< Second switch (highest index)
  Ptr<CsmaChannel>  m_channel;      //!< The link channel connecting switches

  DataRate          m_reserved [2]; //!< Reserved data rate
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
