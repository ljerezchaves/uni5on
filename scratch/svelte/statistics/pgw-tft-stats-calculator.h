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

#ifndef PGW_TFT_STATS_CALCULATOR_H
#define PGW_TFT_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

class PgwInfo;

/**
 * \ingroup svelteStats
 * This class monitors the P-GW TFT adaptive mechanism.
 */
class PgwTftStatsCalculator : public Object
{
public:
  PgwTftStatsCalculator ();          //!< Default constructor.
  virtual ~PgwTftStatsCalculator (); //!< Dummy destructor, see DoDispose.

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
   * Notify the statistics for P-GW adaptive mechanism.
   * \param context Trace source context.
   * \param pgwInfo The P-GW metadata.
   * \param nextLevel The mechanism level for next cycle.
   * \param bearersMoved The number of bearers moved.
   */
  void NotifyPgwTftStats (std::string context, Ptr<const PgwInfo> pgwInfo,
                          uint32_t nextLevel, uint32_t bearersMoved);

private:
  std::string               m_tftFilename;    //!< TftStats filename.
  Ptr<OutputStreamWrapper>  m_tftWrapper;     //!< TftStats file wrapper.
};

} // namespace ns3
#endif /* PGW_TFT_STATS_CALCULATOR_H */
