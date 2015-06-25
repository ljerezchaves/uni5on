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
 * Metadata associated to a switch, which will be used by ConnectionInfo.
 */
struct SwitchData
{
  uint16_t                  swIdx;    //!< Switch index
  Ptr<OFSwitch13NetDevice>  swDev;    //!< OpenFlow switch device
  Ptr<CsmaNetDevice>        portDev;  //!< OpenFlow csma port device
  uint32_t                  portNum;  //!< OpenFlow port number
};

/**
 * \ingroup epcof
 * Metadata associated to a connection between
 * two any switches in the OpenFlow network.
 */
class ConnectionInfo : public Object
{
  friend class RingController;

public:
  ConnectionInfo ();            //!< Default constructor
  virtual ~ConnectionInfo ();   //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param sw1 First switch metadata.
   * \param sw2 Second switch metadata.
   * \param linkSpeed Channel data rate in connection between switches.
   */
  ConnectionInfo (SwitchData sw1, SwitchData sw2, DataRate linkSpeed);

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
   * Return the bandwidth usage ratio, considering both forward and backward
   * paths for full-duplex channel. This method ignores the saving reserve
   * factor.
   * \return The usage ratio.
   */
  double GetUsageRatio (void) const;

  /**
   * Return the bandwidth usage ratio in forward direction, This method ignores
   * the saving reserve factor.
   * \return The usage ratio.
   */
  double GetFowardUsageRatio (void) const;

  /**
   * Return the bandwidth usage ratio in backward direction, This method
   * ignores the saving reserve factor.
   * \return The usage ratio.
   */
  double GetBackwardUsageRatio (void) const;

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
   * Get the available bandwidth between these two switches, considering a
   * saving reserve factor.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param bwFactor The bandwidth saving factor.
   * \return The available data rate from srcIdx to dstIdx.
   */
  DataRate GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx, 
                                 double bwFactor) const;

  /**
   * Reserve some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param dr The DataRate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr);

  /**
   * Release some bandwidth between these two switches.
   * \param srcIdx The source switch index.
   * \param dstIdx The destination switch index.
   * \param dr The DataRate to release.
   * \return True if everything is ok, false otherwise.
   */
  bool ReleaseDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr);

private:
  SwitchData m_sw1; //!< First switch (lowest index)
  SwitchData m_sw2; //!< Second switch (highest index)

  DataRate m_fwDataRate;  //!< Forward data rate (from sw1 to sw2)
  DataRate m_bwDataRate;  //!< Backward data rate (from sw2 to sw1)
  
  bool m_fullDuplex;      //!< Full duplex connection

  DataRate m_fwReserved;  //!< Forward reserved data rate
  DataRate m_bwReserved;  //!< Backward reserved data rate
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
