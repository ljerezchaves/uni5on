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

#ifndef BACKHAUL_STATS_CALCULATOR_H
#define BACKHAUL_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-device-container.h>
#include "../metadata/link-info.h"

namespace ns3 {

/**
 * \ingroup svelteStats
 * This class monitors the OpenFlow backhaul network and dump bandwidth usage
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
  /** Metadata associated to a network slice. */
  struct SliceStats
  {
    std::vector<uint64_t>     fwdBytes;     //!< FWD TX bytes per connetion.
    std::vector<uint64_t>     bwdBytes;     //!< BWD TX bytes per connetion.

    Ptr<OutputStreamWrapper>  nonWrapper;   //!< NonStats file wrapper.
    Ptr<OutputStreamWrapper>  resWrapper;   //!< RegStats file wrapper.
    Ptr<OutputStreamWrapper>  thpWrapper;   //!< ThpStats file wrapper.
    Ptr<OutputStreamWrapper>  useWrapper;   //!< UseStats file wrapper.
  };

  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  LinkInfoList_t            m_linkList;     //!< Backhaul connections.
  Time                      m_lastUpdate;   //!< Last update time.

  std::string               m_prefix;       //!< Common filename prefix.
  std::string               m_nonSuffix;    //!< Non filename suffix.
  std::string               m_resSuffix;    //!< Res filename suffix.
  std::string               m_thpSuffix;    //!< Thp filename suffix.
  std::string               m_useSuffix;    //!< Use filename suffix.

  std::string               m_shrFilename;  //!< Shared non stats filename.
  Ptr<OutputStreamWrapper>  m_shrWrapper;   //!< Shared non stats file wrapper.

  SliceStats                m_slices [N_SLICES_ALL];  //!< Slice metadata.
};

} // namespace ns3
#endif /* BACKHAUL_STATS_CALCULATOR_H */
