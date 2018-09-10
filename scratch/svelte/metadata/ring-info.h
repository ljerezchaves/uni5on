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

#ifndef RING_INFO_H
#define RING_INFO_H

#include <ns3/core-module.h>
#include "../svelte-common.h"
#include "link-info.h"

namespace ns3 {

class RoutingInfo;

/**
 * \ingroup svelteMeta
 * Metadata associated to the routing path for a single EPS bearer among the
 * switches in the OpenFlow ring backhaul network.
 */
class RingInfo : public Object
{
  friend class RingController;

public:
  /** Routing direction in the ring. */
  enum RingPath
  {
    LOCAL = 0,
    CLOCK = 1,
    COUNTER = 2
  };

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   */
  RingInfo (Ptr<RoutingInfo> rInfo);
  virtual ~RingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  Ptr<RoutingInfo> GetRoutingInfo (void) const;
  std::string GetTeidHex (void) const;
  SliceId GetSliceId (void) const;
  RingPath GetDownPath (LteIface iface) const;
  RingPath GetUpPath (LteIface iface) const;
  bool IsDefaultPath (LteIface iface) const;
  bool IsLocalPath (LteIface iface) const;
  bool IsLocalPath (void) const;
  std::string GetPathStr (LteIface iface) const;
  uint16_t GetEnbInfraSwIdx (void) const;
  uint16_t GetPgwInfraSwIdx (void) const;
  uint16_t GetSgwInfraSwIdx (void) const;
  //\}

  /**
   * Invert the given routing path.
   * \param path The original routing path.
   * \return The inverted routing path.
   */
  static RingPath Invert (RingPath path);

  /**
   * Map the link direction to the corresponding ring routing path.
   * \param dir The link direction.
   * \return The ring routing path.
   * \attention This works only for links created in clockwise direction.
   */
  static RingPath LinkDirToRingPath (LinkInfo::Direction dir);

  /**
   * Map the ring routing path to the corresponding link direction.
   * \param path The ring routing path.
   * \return The link direction.
   * \attention This works only for links created in clockwise direction.
   */
  static LinkInfo::Direction RingPathToLinkDir (RingPath path);

  /**
   * Get the string representing the routing path.
   * \param path The routing path.
   * \return The routing path string.
   */
  static std::string RingPathStr (RingPath path);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Set the default downlink routing path for S1-U or S5 interface.
   * The uplink path will always be the same, but with inverted direction.
   * \param downPath The downlink default path.
   * \param iface The LTE logical interface for this path.
   */
  void SetDefaultPath (RingPath downPath, LteIface iface);

  /**
   * Invert the S1-U or S5 routing path, only if different from LOCAL.
   * \param iface The LTE logical interface for this path.
   */
  void InvertPath (LteIface iface);

  /**
   * Reset both routing paths (S1-U and S5) to the default values.
   */
  void ResetToDefaults ();

  Ptr<RoutingInfo> m_rInfo;              //!< Routing metadata.
  RingPath         m_downPath [2];       //!< Downlink routing path.
  bool             m_isDefaultPath [2];  //!< True for default path.
  bool             m_isLocalPath [2];    //!< True for local path.
};

} // namespace ns3
#endif // RING_INFO_H
