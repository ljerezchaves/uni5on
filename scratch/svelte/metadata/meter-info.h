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
 * \ingroup svelteMeta
 * Metadata associated to GTP tunnel meter rules.
 */
class MeterInfo : public Object
{
  friend class SliceController;

public:
  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   */
  MeterInfo (Ptr<RoutingInfo> rInfo);
  virtual ~MeterInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  bool IsDownInstalled (void) const;
  bool IsUpInstalled (void) const;
  bool HasDown (void) const;
  bool HasUp (void) const;
  //\}

  /**
   * \name Dpctl commands to add or delete meter rules.
   * \return The requested command.
   */
  //\{
  std::string GetDownAddCmd (void) const;
  std::string GetUpAddCmd (void) const;
  std::string GetDelCmd (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /** \name Private member accessors. */
  //\{
  void SetDownInstalled (bool value);
  void SetUpInstalled (bool value);
  //\}

private:
  uint32_t          m_teid;            //!< GTP TEID.
  bool              m_isDownInstalled; //!< True when this meter is installed.
  bool              m_isUpInstalled;   //!< True when this meter is installed.
  bool              m_hasDown;         //!< True for downlink meter.
  bool              m_hasUp;           //!< True for uplink meter.
  uint64_t          m_downBitRate;     //!< Downlink meter drop bit rate (bps).
  uint64_t          m_upBitRate;       //!< Uplink meter drop bit rate (bps).
};

} // namespace ns3
#endif // METER_INFO_H
