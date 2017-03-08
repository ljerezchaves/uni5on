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

#ifndef SDMN_BACKHAUL_STATS_CALCULATOR_H
#define SDMN_BACKHAUL_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-device-container.h>
#include "../info/connection-info.h"

namespace ns3 {

class ConnectionInfo;

/**
 * \ingroup sdmnStats
 * This class monitors the backhaul OpenFlow network and dump bandwidth usage
 * and resource reservation statistics on links between OpenFlow switches.
 */
class BackhaulStatsCalculator : public Object
{
public:
  BackhaulStatsCalculator ();          //!< Default constructor.
  virtual ~BackhaulStatsCalculator (); //!< Dummy destructor, see DoDispose.

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
  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  /**
   * Reset internal counters.
   */
  void ResetCounters ();

  ConnInfoList_t            m_connections;    //!< Switch connections.
  Time                      m_lastResetTime;  //!< Last reset time.
  std::string               m_regFilename;    //!< RegStats filename.
  Ptr<OutputStreamWrapper>  m_regWrapper;     //!< RegStats file wrapper.
  std::string               m_renFilename;    //!< RenStats filename.
  Ptr<OutputStreamWrapper>  m_renWrapper;     //!< RenStats file wrapper.
  std::string               m_bwbFilename;    //!< BwbStats filename.
  Ptr<OutputStreamWrapper>  m_bwbWrapper;     //!< BwbStats file wrapper.
  std::string               m_bwgFilename;    //!< BwgStats filename.
  Ptr<OutputStreamWrapper>  m_bwgWrapper;     //!< BwgStats file wrapper.
  std::string               m_bwnFilename;    //!< BwnStats filename.
  Ptr<OutputStreamWrapper>  m_bwnWrapper;     //!< BwnStats file wrapper.
};

} // namespace ns3
#endif /* SDMN_BACKHAUL_STATS_CALCULATOR_H */
