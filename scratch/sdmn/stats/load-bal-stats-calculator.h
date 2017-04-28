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

#ifndef LOAD_BALANCING_STATS_CALCULATOR_H
#define LOAD_BALANCING_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "../info/routing-info.h"

namespace ns3 {

/**
 * \ingroup sdmnStats
 * This class monitors the EPC P-GW load balancing mechanisms.
 */
class LoadBalStatsCalculator : public Object
{
public:
  LoadBalStatsCalculator ();          //!< Default constructor.
  virtual ~LoadBalStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Notify a successful P-GW load balancing procedure.
   * \param context Trace source context.
   * \param status True when enabling the P-GW load balancing, false otherwise.
   * \param bearerList The list of bearers moved to a new P-GW TFT switch.
   */
  void NotifyLoadBalFinished (std::string context, bool status,
                              RoutingInfoList_t bearerList);

private:
  std::string               m_lbmFilename;    //!< LbmStats filename.
  Ptr<OutputStreamWrapper>  m_lbmWrapper;     //!< LbmStats file wrapper.
};

} // namespace ns3
#endif /* LOAD_BALANCING_STATS_CALCULATOR_H */
