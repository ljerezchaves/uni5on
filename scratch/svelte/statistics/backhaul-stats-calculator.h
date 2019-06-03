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
#include "flow-stats-calculator.h"
#include "../logical/epc-gtpu-tag.h"
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
  /**
   * Dump statistics into file.
   * \param nextDump The interval before next dump.
   */
  void DumpStatistics (Time nextDump);

  /**
   * Trace sink fired when a packet is dropped while exceeding pipeline load
   * capacity.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void OverloadDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when a packets is dropped by meter band.
   * \param context Context information.
   * \param packet The dropped packet.
   * \param meterId The meter ID that dropped the packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet,
                        uint32_t meterId);

  /**
   * Trace sink fired when a packet is dropped by OpenFlow port queues.
   * \param context Context information.
   * \param packet The dropped packet.
   */
  void QueueDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when a packet enters the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcInputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink fired when a packet leaves the EPC.
   * \param context Context information.
   * \param packet The packet.
   */
  void EpcOutputPacket (std::string context, Ptr<const Packet> packet);

  /** Metadata associated to a network slice. */
  struct SliceMetadata
  {
    Ptr<OutputStreamWrapper>  bwdWrapper;       //!< BwdStats file wrapper.
    Ptr<OutputStreamWrapper>  tffWrapper;       //!< FlwStats file wrapper.

    /** Flow stats calculator for each traffic direction and QoS type. */
    Ptr<FlowStatsCalculator>  flowStats [N_DIRECTIONS][N_QOS_TYPES_BOTH];
  };

  /** Metadata for each network slice. */
  SliceMetadata             m_slices [N_SLICE_IDS_ALL];
  std::string               m_bwdFilename;  //!< BwdStats filename.
  std::string               m_tffFilename;  //!< TffStats filename.
};

} // namespace ns3
#endif /* BACKHAUL_STATS_CALCULATOR_H */
