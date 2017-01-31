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

#ifndef RING_ROUTING_INFO_H
#define RING_ROUTING_INFO_H

#include <ns3/core-module.h>

namespace ns3 {

class RingController;
class RoutingInfo;

/**
 * \ingroup sdmnInfo
 * Metadata associated to a ring routing path between
 * two any switches in the OpenFlow ring network.
 */
class RingRoutingInfo : public Object
{
  friend class RingController;

public:
  /** Routing direction in the ring. */
  enum RoutingPath
  {
    LOCAL = 0,
    CLOCK = 1,
    COUNTER = 2
  };

  RingRoutingInfo ();          //!< Default constructor.
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This RingRoutingInfo object must be aggregated to rInfo.
   */
  RingRoutingInfo (Ptr<RoutingInfo> rInfo);

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
  uint32_t    GetTeid       (void) const;
  bool        IsDefaultPath (void) const;
  bool        IsLocalPath   (void) const;
  uint16_t    GetPgwSwIdx   (void) const;
  uint16_t    GetEnbSwIdx   (void) const;
  RoutingPath GetDownPath   (void) const;
  RoutingPath GetUpPath     (void) const;
  //\}

  /**
   * Invert the given routing path.
   * \param path The original routing path.
   * \return The inverted routing path.
   */
  static RoutingPath InvertPath (RoutingPath path);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

private:
  /**
   * Set the default routing paths.
   * \param downPath The downlink default path.
   * \param upPath The uplink default path.
   */
  void SetDefaultPaths (RoutingPath downPath, RoutingPath upPath);

  /** Invert both routing paths, only if different from LOCAL. */
  void InvertBothPaths ();

  /** Reset both routing paths to default values. */
  void ResetToDefaultPaths ();

  Ptr<RoutingInfo> m_rInfo;         //!< Routing information
  RoutingPath      m_downPath;      //!< Downlink routing path
  RoutingPath      m_upPath;        //!< Uplink routing path
  bool             m_isDefaultPath; //!< True when paths are default
  bool             m_isLocalPath;   //!< True when routing path is local
};

};  // namespace ns3
#endif // RING_ROUTING_INFO_H

