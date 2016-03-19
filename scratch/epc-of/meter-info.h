/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#ifndef METER_INFO_H
#define METER_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

class RoutingInfo;

/**
 * \ingroup epcof
 * Metadata associated to GTP tunnel meter rules.
 */
class MeterInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  MeterInfo ();          //!< Default constructor
  virtual ~MeterInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This MeterInfo object must be aggregated to rInfo.
   */
  MeterInfo (Ptr<RoutingInfo> rInfo);

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
  bool IsInstalled (void) const;
  bool HasDown (void) const;
  bool HasUp (void) const;
  //\}

  /**
   * \name Dpctl commands to add or delete meter rules
   * \return The requested command.
   */
  //\{
  std::string GetDownAddCmd (void) const;
  std::string GetUpAddCmd (void) const;
  std::string GetDelCmd (void) const;
  //\}

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /**
   * Set the internal installed flag.
   * \param installed The value to set.
   */
  void SetInstalled (bool installed);

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isInstalled;   //!< True when this meter is installed
  bool     m_hasDown;       //!< True for downlink meter
  bool     m_hasUp;         //!< True for uplink meter
  uint64_t m_downBitRate;   //!< Downlink meter drop bit rate (bps)
  uint64_t m_upBitRate;     //!< Uplink meter drop bit rate (bps)
  Ptr<RoutingInfo> m_rInfo; //!< Routing information
};

};  // namespace ns3
#endif // METER_INFO_H

