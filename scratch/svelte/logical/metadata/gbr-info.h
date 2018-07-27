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

#ifndef GBR_INFO_H
#define GBR_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

class RoutingInfo;

/**
 * \ingroup svelteLogical
 * Metadata associated to GBR bearers.
 */
class GbrInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   */
  GbrInfo (Ptr<RoutingInfo> rInfo);
  virtual ~GbrInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  bool IsReserved (void) const;
  uint64_t GetDownBitRate (void) const;
  uint64_t GetUpBitRate (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /** \name Private member accessors. */
  //\{
  void SetReserved (bool value);
  //\}

private:
  bool              m_isReserved;   //!< True when resources are reserved.
  bool              m_hasDown;      //!< True for downlink reserve.
  bool              m_hasUp;        //!< True for uplink reserve.
  uint64_t          m_downBitRate;  //!< Downlink reserved bit rate.
  uint64_t          m_upBitRate;    //!< Uplink reserved bit rate.
};

} // namespace ns3
#endif // GBR_INFO_H
