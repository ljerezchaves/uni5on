/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef S5_AGGREGATION_INFO_H
#define S5_AGGREGATION_INFO_H

#include <ns3/core-module.h>

namespace ns3 {

class RoutingInfo;

/**
 * \ingroup sdmnInfo
 * Metadata associated to the S5 traffic aggregation mechanism.
 */
class S5AggregationInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   */
  S5AggregationInfo (Ptr<RoutingInfo> rInfo);
  virtual ~S5AggregationInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  bool IsAggregated (void) const;

  void SetAggregated (bool value);
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  bool             m_isAggregated;  //!< Traffic aggregated over S5 bearer.
};

};  // namespace ns3
#endif // S5_AGGREGATION_INFO_H

