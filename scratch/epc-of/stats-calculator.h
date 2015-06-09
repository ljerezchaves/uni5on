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

#ifndef EPCOF_STATS_CALCULATOR_H
#define EPCOF_STATS_CALCULATOR_H

#include "ns3/nstime.h"
#include "routing-info.h"

namespace ns3 {
  
/**
 * \ingroup epcof
 *
 * This class monitors bearer request statistics. It counts the number of
 * bearer requests, including those accepted or blocked by network. 
 */
class AdmissionStatsCalculator : public Object
{
public:
  AdmissionStatsCalculator ();  //!< Default constructor
  virtual ~AdmissionStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime       (void) const;
  uint32_t  GetNonGbrRequests   (void) const;
  uint32_t  GetNonGbrAccepted   (void) const;
  uint32_t  GetNonGbrBlocked    (void) const;
  double    GetNonGbrBlockRatio (void) const;
  uint32_t  GetGbrRequests      (void) const;
  uint32_t  GetGbrAccepted      (void) const;
  uint32_t  GetGbrBlocked       (void) const;
  double    GetGbrBlockRatio    (void) const;
  uint32_t  GetTotalRequests    (void) const;
  uint32_t  GetTotalAccepted    (void) const;
  uint32_t  GetTotalBlocked     (void) const;
  //\}

  /**
   * Notify new bearer request.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void NotifyRequest (bool accepted, Ptr<const RoutingInfo> rInfo);

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

  /** 
   * TracedCallback signature for AdmissionStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (* AdmTracedCallback)
    (Ptr<const AdmissionStatsCalculator> stats);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  uint32_t          m_nonRequests;      //!< number of non-GBR requests
  uint32_t          m_nonAccepted;      //!< number of non-GBR accepted 
  uint32_t          m_nonBlocked;       //!< number of non-GBR blocked
  uint32_t          m_gbrRequests;      //!< Number of GBR requests
  uint32_t          m_gbrAccepted;      //!< Number of GBR accepted
  uint32_t          m_gbrBlocked;       //!< Number of GBR blocked
  Time              m_lastResetTime;    //!< Last reset time
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 *
 * This class monitors gateway bandwidth statistics. 
 */
class GatewayStatsCalculator : public Object
{
public:
  GatewayStatsCalculator ();  //!< Default constructor
  virtual ~GatewayStatsCalculator (); //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  Time      GetActiveTime       (void) const;
  DataRate  GetDownDataRate     (void) const;
  DataRate  GetUpDataRate       (void) const;
  //\}

  /**
   * Notify gateway traffic.
   * \param direction Traffic direction.
   * \param packet The packet.
   */
  void NotifyTraffic (std::string direction, Ptr<const Packet> packet);

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();

  /** 
   * TracedCallback signature for GatewayStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (* PgwTracedCallback)
    (Ptr<const GatewayStatsCalculator> stats);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  uint32_t  m_pgwDownBytes;   //!< Pgw traffic downlink bytes.
  uint32_t  m_pgwUpBytes;     //!< Pgw traffic uplink bytes.
  Time      m_lastResetTime;  //!< Last reset time
};


} // namespace ns3
#endif /* EPCOF_STATS_CALCULATOR_H */
