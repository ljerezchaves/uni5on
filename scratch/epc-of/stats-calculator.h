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
  
class BearerRequestStats;

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
   * Set the default statistics dump interval.
   * \param timeout The timeout value.
   */
  void SetDumpTimeout (Time timeout);

  /**
   * Dump admission control statistics.
   */
  void DumpStatistics ();

  /** 
   * TracedCallback signature for AdmissionStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (* AdmTracedCallback)(Ptr<const AdmissionStatsCalculator> stats);

  /** 
   * TracedCallback signature for BearerRequestStats.
   * \param stats The statistics.
   */
  typedef void (* BrqTracedCallback)(Ptr<const BearerRequestStats> stats);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  /**
   * Trace sink fired at every new bearer request to OpenFlow controller.
   * \param accepted True when the bearer is accepted into network.
   * \param rInfo The bearer routing information.
   */
  void BearerRequest (bool accepted, Ptr<const RoutingInfo> rInfo);

  /** 
   * Reset all internal counters. 
   */
  void ResetCounters ();
 
  /** The cummulative bearer request trace source fired regularlly at DumpStatistics. */
  TracedCallback<Ptr<const AdmissionStatsCalculator> > m_admTrace;

  /** The bearer request trace source fired for every request at BearerRequest. */
  TracedCallback<Ptr<const BearerRequestStats> > m_brqTrace;


  uint32_t          m_nonRequests;      //!< number of non-GBR requests
  uint32_t          m_nonAccepted;      //!< number of non-GBR accepted 
  uint32_t          m_nonBlocked;       //!< number of non-GBR blocked
  uint32_t          m_gbrRequests;      //!< Number of GBR requests
  uint32_t          m_gbrAccepted;      //!< Number of GBR accepted
  uint32_t          m_gbrBlocked;       //!< Number of GBR blocked
  Time              m_lastResetTime;    //!< Last reset time
  Time              m_dumpTimeout;      //!< Dump stats timeout.
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * 
 * This class stores bearer request information. 
 */
class BearerRequestStats : public SimpleRefCount<BearerRequestStats>
{
  friend class AdmissionStatsCalculator;

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
