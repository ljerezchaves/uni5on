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

/** Bandwitdh stats between two switches */
typedef std::pair<SwitchPair_t, double> BandwidthStats_t;

/**
 * \ingroup epcof
 * Metadata associated to a connection between
 * two any switches in the OpenFlow network.
 */
class ConnectionInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;
  friend class RingNetwork;

public:
  ConnectionInfo ();           //!< Default constructor
  virtual ~ConnectionInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the pair of switch indices for this connection, in increasing order.
   * \param The pair of switch indices.
   */
  SwitchPair_t GetSwitchIndexPair (void) const;

  /**
   * Get the availabe bandwitdh between these two switches.
   * \return True available DataRate.
   */
  DataRate GetAvailableDataRate (void) const;

  /**
   * Get the availabe bandwitdh between these two switches, considering a
   * saving reserve factor.
   * \param bwFactor The bandwidth saving factor.
   * \return True available DataRate.
   */
  DataRate GetAvailableDataRate (double bwFactor) const;

  /**
   * Return the bandwidth usage ratio, ignoring the saving reserve factor.
   * \return The usage ratio.
   */
  double GetUsageRatio (void) const;

  /**
   * TracedCallback signature for bandwidth usage ratio.
   * \param swIdx1 The first switch index.
   * \param swIdx2 The second switch index.
   * \param ratio The bandwidth usage ratio.
   */
  typedef void (*UsageTracedCallback)
    (uint16_t swIdx1, uint16_t swIdx2, double ratio);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Reserve some bandwith between these two switches.
   * \param dr The DataRate to reserve.
   * \return True if everything is ok, false otherwise.
   */
  bool ReserveDataRate (DataRate dr);

  /**
   * Release some bandwith between these two switches.
   * \param dr The DataRate to release.
   * \return True if everything is ok, false otherwise.
   */
  bool ReleaseDataRate (DataRate dr);

  /** Information associated to the first switch */
  //\{
  uint16_t m_switchIdx1;                  //!< Switch index
  Ptr<OFSwitch13NetDevice> m_switchDev1;  //!< OpenFlow device
  Ptr<CsmaNetDevice> m_portDev1;          //!< OpenFlow csma port device
  uint32_t m_portNum1;                    //!< OpenFlow port number
  //\}

  /** Information associated to the second switch */
  //\{
  uint16_t m_switchIdx2;                  //!< Switch index
  Ptr<OFSwitch13NetDevice> m_switchDev2;  //!< OpenFlow device
  Ptr<CsmaNetDevice> m_portDev2;          //!< OpenFlow csma port device
  uint32_t m_portNum2;                    //!< OpenFlow port number
  //\}

  /** Information associated to the connection between these two switches */
  //\{
  DataRate m_maxDataRate;         //!< Maximum nominal bandwidth
  DataRate m_reservedDataRate;    //!< Reserved bandwitdth
  //\}

private:
  /** The usage ratio trace source, fired when reserving/releasing DataRate. */
  TracedCallback<uint16_t, uint16_t, double> m_usageTrace;
};

};  // namespace ns3
#endif  // CONNECTION_INFO_H
