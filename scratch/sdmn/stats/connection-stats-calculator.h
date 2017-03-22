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

#ifndef SDMN_CONNECTION_STATS_CALCULATOR_H
#define SDMN_CONNECTION_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

/**
 * \ingroup sdmnStats
 * This class monitors the SDN EPC bearer admission control and dump bearer
 * requests and blocking statistics.
 */
class ConnectionStatsCalculator : public Object
{
public:
  ConnectionStatsCalculator ();          //!< Default constructor.
  virtual ~ConnectionStatsCalculator (); //!< Dummy destructor, see DoDispose.

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
   * Notify a successful initial cell selection procedure.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   */
  void NotifyInitialCellSelectionEndOk (
    std::string context, uint64_t imsi, uint16_t cellId);

  /**
   * Notify a failed initial cell selection procedure.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   */
  void NotifyInitialCellSelectionEndError (
    std::string context, uint64_t imsi, uint16_t cellId);

  /**
   * Notify a successful RRC connection establishment.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   */
  void NotifyConnectionEstablished (
    std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);

  /**
   * Notify a timeout RRC connection establishment because of T300.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   */
  void NotifyConnectionTimeout (
    std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);

  /**
   * Notify a RRC connection reconfiguration.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   */
  void NotifyConnectionReconfiguration (
    std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);

  /**
   * Notify the start of a handover procedure.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param srcCellId The current serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   * \param dstCellId The target eNB cell ID.
   */
  void NotifyHandoverStart (
    std::string context, uint64_t imsi, uint16_t srcCellId, uint16_t rnti,
    uint16_t dstCellId);

  /**
   * Notify a successful termination of a handover procedure.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   */
  void NotifyHandoverEndOk (
    std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);

  /**
   * Notify a failure of a handover procedure.
   * \param context Trace source context.
   * \param imsi The UE IMSI.
   * \param cellId The serving eNB cell ID.
   * \param rnti The Cell Radio Network Temporary Identifier.
   */
  void NotifyHandoverEndError (
    std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);

private:
  std::string               m_rrcFilename;    //!< RrcStats filename.
  Ptr<OutputStreamWrapper>  m_rrcWrapper;     //!< RrcStats file wrapper.
};

} // namespace ns3
#endif /* SDMN_CONNECTION_STATS_CALCULATOR_H */
