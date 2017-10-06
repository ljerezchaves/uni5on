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
#include "../epc/epc-controller.h"

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
  double GetLinkUsage (void) const;
  OperationMode GetOperationMode (void) const;
  std::string GetOperationModeStr (void) const;
  double GetThreshold (void) const;

  void SetLinkUsage (double value);
  void SetOperationMode (OperationMode value);
  void SetThreshold (double value);
  //\}

  /**
   * Check internal members and decide if this bearer has to be aggregated or
   * not over the S5 interface.
   * \return True when the bearer has to be aggregated.
   */
  bool IsAggregated (void) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  double           m_linkUsage;     //!< Link usage.
  OperationMode    m_mode;          //!< Operation mode.
  double           m_threshhold;    //!< Link usage threshold.
};

};  // namespace ns3
#endif // S5_AGGREGATION_INFO_H

