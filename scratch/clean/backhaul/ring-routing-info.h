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

class RoutingInfo;

/**
 * \ingroup svelteInfo
 * Metadata associated to the routing path for a single EPS bearer among the
 * switches in the OpenFlow ring backhaul network.
 */
class RingRoutingInfo : public Object
{
public:
  /** Routing direction in the ring. */
  enum RoutingPath
  {
    LOCAL = 0,
    CLOCK = 1,
    COUNTER = 2
  };

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   */
  // RingRoutingInfo (Ptr<RoutingInfo> rInfo);
  RingRoutingInfo ();
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

//  /** \name Private member accessors. */
//  //\{
//  RoutingPath GetDownPath (void) const;
//  RoutingPath GetUpPath (void) const;
//  uint16_t GetPgwSwIdx (void) const;
//  uint16_t GetSgwSwIdx (void) const;
//  uint64_t GetPgwSwDpId (void) const;
//  uint64_t GetSgwSwDpId (void) const;
//  bool IsDefaultPath (void) const;
//  bool IsLocalPath (void) const;
//  std::string GetPathStr (void) const;
//
//  void SetPgwSwIdx (uint16_t value);
//  void SetSgwSwIdx (uint16_t value);
//  void SetPgwSwDpId (uint64_t value);
//  void SetSgwSwDpId (uint64_t value);
//  //\}
//
//  /**
//   * Set the default downlink routing path.
//   * The uplink path will always be the same, but with inverted direction.
//   * \param downPath The downlink default path.
//   */
//  void SetDefaultPath (RoutingPath downPath);
//
//  /**
//   * Invert routing path, only if different from LOCAL.
//   */
//  void InvertPath ();
//
//  /**
//   * Reset routing path to default values.
//   */
//  void ResetPath ();

  /**
   * Invert the given routing path.
   * \param path The original routing path.
   * \return The inverted routing path.
   */
  static RoutingPath Invert (RoutingPath path);

  /**
   * Get the string representing the routing path.
   * \param path The routing path.
   * \return The routing path string.
   */
  static std::string RoutingPathStr (RoutingPath path);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
//  RoutingPath      m_downPath;      //!< Downlink routing path.
//  uint16_t         m_pgwIdx;        //!< Switch index attached to P-GW.
//  uint16_t         m_sgwIdx;        //!< Switch index attached to S-GW.
//  uint64_t         m_pgwDpId;       //!< Switch dp id attached to the P-GW.
//  uint64_t         m_sgwDpId;       //!< Switch dp id attached to the S-GW.
//  bool             m_isDefaultPath; //!< True if path is default (!inverted).
//  bool             m_isLocalPath;   //!< True if path is local.
};

};  // namespace ns3
#endif // RING_ROUTING_INFO_H

