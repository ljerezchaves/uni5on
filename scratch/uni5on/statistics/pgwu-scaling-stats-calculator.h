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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef PGWU_SCALING_STATS_CALCULATOR_H
#define PGWU_SCALING_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class PgwInfo;
class PgwuScaling;

/**
 * \ingroup uni5onStats
 * This class monitors the P-GW TFT scaling mechanism.
 */
class PgwuScalingStatsCalculator : public Object
{
public:
  PgwuScalingStatsCalculator ();          //!< Default constructor.
  virtual ~PgwuScalingStatsCalculator (); //!< Dummy destructor, see DoDispose.

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
   * Notify the statistics for the P-GW scaling mechanism.
   * \param context Trace source context.
   * \param pgwInfo The P-GW scaling application.
   * \param nextLevel The level for the next cycle.
   * \param bearersMoved The number of bearers being moved.
   */
  void NotifyScalingStats (
    std::string context, Ptr<const PgwuScaling> scalingApp,
    uint32_t nextLevel, uint32_t bearersMoved);

private:
  /** Metadata associated to a network slice. */
  struct SliceMetadata
  {
    Ptr<OutputStreamWrapper> tftWrapper;  //!< AdmStats file wrapper.
  };

  /** Metadata for each network slice. */
  SliceMetadata             m_slices [N_SLICE_IDS];
  std::string               m_tftFilename;    //!< TftStats filename.
};

} // namespace ns3
#endif /* PGWU_SCALING_STATS_CALCULATOR_H */
