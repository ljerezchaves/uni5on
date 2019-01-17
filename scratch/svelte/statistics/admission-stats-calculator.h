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

#ifndef ADMISSION_STATS_CALCULATOR_H
#define ADMISSION_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include "../svelte-common.h"

namespace ns3 {

class RoutingInfo;

/**
 * \ingroup svelteStats
 * This class monitors the SDN EPC bearer admission control and dump bearer
 * requests and blocking statistics.
 */
class AdmissionStatsCalculator : public Object
{
public:
  AdmissionStatsCalculator ();          //!< Default constructor.
  virtual ~AdmissionStatsCalculator (); //!< Dummy destructor, see DoDispose.

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

private:
  /** Metadata associated to a network slice. */
  struct SliceMetadata
  {
    uint32_t releases;      //!< Number of releases.
    uint32_t requests;      //!< Number of requests.
    uint32_t accepted;      //!< Number of requests accepted.
    uint32_t blocked;       //!< Number of requests blocked.
    uint32_t aggregated;    //!< Number of requests aggregated.
    uint32_t activeBearers; //!< Number of active bearers.
    uint32_t instalBearers; //!< Number of installed bearers.
    uint32_t aggregBearers; //!< Number of aggregated bearers.
    Ptr<OutputStreamWrapper> admWrapper;  //!< AdmStats file wrapper.
  };

  /**
   * Notify a new bearer request.
   * \param rInfo The bearer routing information.
   */
  void NotifyBearerRequest (Ptr<const RoutingInfo> rInfo);

  /**
   * Notify a new bearer release.
   * \param rInfo The bearer routing information.
   */
  void NotifyBearerRelease (Ptr<const RoutingInfo> rInfo);

  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  /**
   * Reset internal counters.
   * \param slData The slice metadata to clear.
   */
  void ResetCounters (SliceMetadata &slData);

  /** Metadata for each network slice. */
  SliceMetadata            m_slices [N_SLICE_IDS];
  std::string              m_admFilename;   //!< AdmStats filename.
  std::string              m_brqFilename;   //!< BrqStats filename.
  Ptr<OutputStreamWrapper> m_brqWrapper;    //!< BrqStats file wrapper.
};

} // namespace ns3
#endif /* ADMISSION_STATS_CALCULATOR_H */
