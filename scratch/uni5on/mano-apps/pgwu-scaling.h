/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
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

#ifndef PGWU_SCALING_H
#define PGWU_SCALING_H

#include <ns3/core-module.h>
#include "../metadata/pgw-info.h"
#include "../slices/slice-controller.h"
#include "../uni5on-common.h"

namespace ns3 {

class PgwInfo;
class SliceController;

/**
 * \ingroup uni5onMano
 * The intra-slice P-GWu scaling application.
 */
class PgwuScaling : public Object
{
  friend class SliceController;

public:
  /**
   * Complete contructor
   * \param pgwInfo The P-GW metadata.
   * \param slcCtrl The slice controller.
   */
  PgwuScaling (Ptr<PgwInfo> pgwInfo, Ptr<SliceController> slcCtrl);
  virtual ~PgwuScaling ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for P-GW scaling metadata.
   * \return The requested information.
   */
  //\{
  Ptr<SliceController>  GetSliceCtrl    (void) const;
  Ptr<PgwInfo>          GetPgwInfo      (void) const;
  uint16_t              GetCurLevel     (void) const;
  uint16_t              GetCurTfts      (void) const;
  uint16_t              GetMaxLevel     (void) const;
  OpMode                GetScalingMode  (void) const;
  double                GetJoinThs      (void) const;
  double                GetSplitThs     (void) const;
  bool                  GetStartMax     (void) const;
  Time                  GetTimeout      (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW TFT active switches aggregated
   *       datapath information.
   * \param tableId The flow table ID. The default value of 0 can be used as
   *        P-GW TFT switches have only one pipeline table.
   * \return The requested information.
   * \internal These methods iterate only over active TFT switches for
   *           collecting statistics.
   */
  //\{
  uint32_t      GetTftAvgFlowTableCur   (uint8_t tableId = 0) const;
  uint32_t      GetTftAvgFlowTableMax   (uint8_t tableId = 0) const;
  double        GetTftAvgFlowTableUse   (uint8_t tableId = 0) const;
  DataRate      GetTftAvgEwmaCpuCur     (void) const;
  double        GetTftAvgEwmaCpuUse     (void) const;
  DataRate      GetTftAvgCpuMax         (void) const;
  uint32_t      GetTftMaxFlowTableCur   (uint8_t tableId = 0) const;
  uint32_t      GetTftMaxFlowTableMax   (uint8_t tableId = 0) const;
  double        GetTftMaxFlowTableUse   (uint8_t tableId = 0) const;
  DataRate      GetTftMaxEwmaCpuCur     (void) const;
  double        GetTftMaxEwmaCpuUse     (void) const;
  DataRate      GetTftMaxCpuMax         (void) const;
  //\}

  /**
   * Get the P-GW TFT index for a given traffic flow.
   * \param bInfo The bearer information.
   * \param nTfts The number of active P-GW TFT switches.
   * \return The P-GW TFT index.
   */
  uint16_t GetTftIdx (Ptr<const BearerInfo> bInfo, uint16_t nTfts) const;

  /**
   * TracedCallback signature for the P-GW TFT scaling trace source.
   * \param pgwScaling This P-GW scaling application.
   * \param nextLevel The scaling level for the next cycle.
   * \param bearersMoved The number of bearers being moved now.
   */
  typedef void (*PgwTftScalingTracedCallback)(
    Ptr<const PgwuScaling> pgwScaling, uint32_t nextLevel,
    uint32_t bearersMoved);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Notify this application of a new bearer context created.
   * \param bInfo The bearer information.
   */
  void NotifyBearerCreated (Ptr<BearerInfo> bInfo);

private:
  /**
   * Periodically check for the P-GW processing load and flow table usage to
   * scale the number of TFTs.
   */
  void PgwTftScaling (void);

  /** The P-GW TFT scaling trace source. */
  TracedCallback<Ptr<const PgwuScaling>, uint32_t, uint32_t> m_pgwScalingTrace;

  // P-GW metadata and TFT load balancing mechanism.
  Ptr<SliceController>      m_controller;     //!< Slice controller.
  Ptr<PgwInfo>              m_pgwInfo;        //!< P-GW metadata for slice.
  OpMode                    m_scalingMode;    //!< P-GW TFT load balancing.
  double                    m_joinThs;        //!< P-GW TFT join threshold.
  double                    m_splitThs;       //!< P-GW TFT split threshold.
  bool                      m_startMax;       //!< P-GW TFT start with maximum.
  Time                      m_timeout;        //!< P-GW TFT load bal timeout.
  uint8_t                   m_level;          //!< Current adaptive level.
};

} // namespace ns3
#endif // PGWU_SCALING_H
