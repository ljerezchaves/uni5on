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

#ifndef TRAFFIC_MANAGER_H
#define TRAFFIC_MANAGER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class SliceController;
class Uni5onClient;

/**
 * \ingroup uni5onLogical
 * Traffic manager which handles UNI5ON client applications start/stop events.
 * It interacts with the UNI5ON architecture to request and release bearers.
 * Each LteUeNetDevice has one TrafficManager object aggregated to it.
 */
class TrafficManager : public Object
{
public:
  TrafficManager ();          //!< Default constructor.
  virtual ~TrafficManager (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Add a new application to this manager.
   * \param app The application pointer.
   */
  void AddUni5onClient (Ptr<Uni5onClient> app);

  /**
   * Notify this manager when a new session is created in the controller.
   * This will be used to assign the teid to each application.
   * \param imsi The IMSI UE identifier.
   * \param bearerList The list of bearer contexts created.
   */
  void NotifySessionCreated (uint64_t imsi, BearerCreatedList_t bearerList);

  /**
   * Set the OpenFlow slice controller.
   * \param controller The slice controller.
   */
  void SetController (Ptr<SliceController> controller);

  /**
   * Set the IMSI attribute.
   * \param imsi The ISMI value.
   */
  void SetImsi (uint64_t imsi);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Attempt to (re)start this application. This method will request for bearer
   * resources to the controller before starting the application. If the
   * controller accept the request, the application starts.
   * \internal The teid approach only works because we currently have a single
   * application associated with each bearer/tunnel.
   * \param app The application pointer.
   */
  void AppStartTry (Ptr<Uni5onClient> app);

  /**
   * Member function called by applications to notify this manager when traffic
   * stops. This method will fire network statistics (EPC) and schedule
   * application restart attempt.
   * \param app The application pointer.
   */
  void NotifyAppStop (Ptr<Uni5onClient> app);

  /**
   * Set the time for the next attempt to start the application.
   * \internal We will use this interval to limit the current traffic duration
   * to avoid overlapping traffic. This is necessary to respect inter-arrival
   * times for the Poisson process and reuse apps and bearers along the
   * simulation.
   * \param app The application pointer.
   */
  void SetNextAppStartTry (Ptr<Uni5onClient> app);

  /**
   * Get the absolute time for the next attemp to start the application.
   * \param app The application pointer.
   */
  Time GetNextAppStartTry (Ptr<Uni5onClient> app) const;

  Ptr<RandomVariableStream> m_interArrivalRng;  //!< Inter-arrival random time.
  bool                      m_restartApps;      //!< Restart apps after stop.
  double                    m_startProb;        //!< Probability to start apps.
  Ptr<RandomVariableStream> m_startProbRng;     //!< Start random probability.
  Time                      m_startTime;        //!< Time to start apps.
  Time                      m_stopTime;         //!< Time to stop apps.
  Ptr<SliceController>      m_ctrlApp;          //!< OpenFlow slice controller.
  uint64_t                  m_imsi;             //!< UE IMSI identifier.
  uint32_t                  m_defaultTeid;      //!< Default UE tunnel TEID.

  /** Map saving application pointer / next start time. */
  typedef std::map<Ptr<Uni5onClient>, Time> AppTimeMap_t;
  AppTimeMap_t              m_timeByApp;        //!< Application map.
};

} // namespace ns3
#endif // TRAFFIC_MANAGER_H
