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
    LOCAL = 0,    //!< Local routing.
    CLOCK = 1,    //!< Clockwise routing.
    COUNT = 2     //!< Counterclockwise routing.
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

  /**
   * \name Private member accessors for bearer ring routing information.
   * \param iface The LTE logical interface.
   * \return The requested information.
   */
  //\{
  RingPath      GetDlPath     (LteIface iface) const;
  RingPath      GetUlPath     (LteIface iface) const;
  bool          IsShortPath   (LteIface iface) const;
  bool          IsLocalPath   (LteIface iface) const;
  //\}

  /**
   * Check whether both LTE S1-U and S5 interfaces are routed over local paths.
   * \return True for local paths, false otherwise.
   */
  bool AreLocalPaths (void) const;

  /**
   * Get the bearer routing information aggregated to this object.
   * \return The routing information.
   */
  Ptr<RoutingInfo> GetRoutingInfo (void) const;

  /**
   * Invert the given routing path.
   * \param path The original routing path.
   * \return The inverted routing path.
   */
  static RingPath InvertPath (RingPath path);

  /**
   * Map the link direction to the corresponding ring routing path.
   * \param dir The link direction.
   * \return The ring routing path.
   * \attention This works only for links created in clockwise direction.
   */
  static RingPath LinkDirToRingPath (LinkInfo::LinkDir dir);

  /**
   * Map the ring routing path to the corresponding link direction.
   * \param path The ring routing path.
   * \return The link direction.
   * \attention This works only for links created in clockwise direction.
   */
  static LinkInfo::LinkDir RingPathToLinkDir (RingPath path);

  /**
   * Get the string representing the routing path.
   * \param path The routing path.
   * \return The routing path string.
   */
  static std::string RingPathStr (RingPath path);

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Set the downlink shortest routing path for the given interface.
   * The uplink path will always be the same, but with inverted direction.
   * \param iface The LTE logical interface for this path.
   * \param path The downlink path.
   */
  void SetIfacePath (LteIface iface, RingPath path);

  /**
   * Invert the interface routing path.
   * \param iface The LTE logical interface.
   */
  void InvertIfacePath (LteIface iface);

  /**
   * Reset the interface routing path to the shortest one.
   * \param iface The LTE logical interface.
   */
  void ResetIfacePath (LteIface iface);

  RingPath         m_downPath [2];       //!< Downlink routing path.
  bool             m_isShortPath [2];    //!< True for short down path.
  bool             m_isLocalPath [2];    //!< True for local down path.
  Ptr<RoutingInfo> m_rInfo;              //!< Routing metadata.
};

/**
 * Print the ring routing metadata on an output stream.
 * \param os The output stream.
 * \param ringInfo The RingInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the RingInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const RingInfo &ringInfo);

} // namespace ns3
#endif // RING_INFO_H
