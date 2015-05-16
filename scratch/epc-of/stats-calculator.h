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
 * This class monitors bearer management statistics. It counts the number of
 * bearer requests, including those accepted or blocked by network. 
 */
class AdmissionStatsCalculator : public SimpleRefCount<AdmissionStatsCalculator>
{
public:
  AdmissionStatsCalculator ();  //!< Default constructor
  virtual ~AdmissionStatsCalculator (); //!< Default destructor
  
  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();
  
  /**
   * Notify a new GBR bearer request accepted by network.
   * \param rInfo The bearer request information.
   */
  void NotifyAcceptedRequest (Ptr<const RoutingInfo> rInfo);

  /**
   * Notify a new GBR bearer request blocked by network.
   * \param rInfo The bearer request information.
   */
  void NotifyBlockedRequest (Ptr<const RoutingInfo> rInfo);
  
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
   * TracedCallback signature for AdmissionStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (* AdmTracedCallback)(Ptr<const AdmissionStatsCalculator> stats);

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
 * This class stores bearer request information. 
 */
class BearerRequestStats : public SimpleRefCount<BearerRequestStats>
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  BearerRequestStats ();  //!< Default constructor
  virtual ~BearerRequestStats (); //!< Default destructor
  
  /**
   * Get statistics.
   * \return The statistic value.
   */
  //\{
  uint32_t    GetTeid         (void) const;
  bool        IsAccepted      (void) const;
  DataRate    GetDownDataRate (void) const;
  DataRate    GetUpDataRate   (void) const;
  std::string GetDescription  (void) const;
  std::string GetRoutingPaths (void) const;
  //\}

  /** 
   * TracedCallback signature for BearerRequestStats.
   * \param stats The statistics.
   */
  typedef void (* BrqTracedCallback)(Ptr<const BearerRequestStats> stats);

private:
  uint32_t    m_teid;           //!< GTP TEID
  bool        m_accepted;       //!< True for accepted bearer
  DataRate    m_downDataRate;   //!< Downlink reserved data rate
  DataRate    m_upDataRate;     //!< Uplink reserved data rate
  std::string m_trafficDesc;    //!< Traffic description
  std::string m_routingPaths;   //!< Routing paths description
};


} // namespace ns3
#endif /* EPCOF_STATS_CALCULATOR_H */
