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
#include <ns3/applications-module.h>
#include <ns3/internet-module.h>
#include "openflow-epc-controller.h"

namespace ns3 {

/** 
 * \ingroup epcof
 * Traffic manager which handles UE client applications start/stop events, and
 * interacts with the epc-openflow-controller to request/release EPS bearers.
 * Each UE has one TrafficManager object aggregated to it.
 */
class TrafficManager : public Object
{
public:
  TrafficManager ();          //!< Default constructor
  virtual ~TrafficManager (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  void SetLteUeDevice (Ptr<LteUeNetDevice> ueDevice);

  /**
   * Add a new application for this manager.
   * \param app The application pointer.
   */ 
  void AddEpcApplication (Ptr<EpcApplication> app);

  /**
   * Notify this manager when an application stops sending traffic.
   * \param app The application pointer.
   */ 
  void NotifyAppStop (Ptr<EpcApplication> app);

  /**
   * Attempt to start this application. This method will request for bearer
   * resources to the controller before startgin the application. If the
   * controller deny the request, this method reschedule the application start
   * attempt.
   * \param app The application pointer.
   */ 
  void AppStartTry (Ptr<EpcApplication> app);

  /** 
   * TracedCallback signature for QoS dump. 
   * \param description String describing this traffic.
   * \param teid Bearer TEID.
   * \param stats The QoS statistics.
   */
  typedef void (* QosTracedCallback)(std::string description, uint32_t teid, 
                                     Ptr<const QosStatsCalculator> stats);

private:
  /**
   * Dump application statistics.
   * \param app The application pointer.
   */
  void DumpAppStatistics (Ptr<EpcApplication> app);

  /**
   * Create the description string for this application.
   * \param app The application pointer.
   * \return The description string.
   */
  std::string GetAppDescription (Ptr<const EpcApplication> app);

  bool m_httpEnable;     //!< HTTP traffic enable
  bool m_voipEnable;     //!< Voip traffic enable
  bool m_stVideoEnable;  //!< Stored video traffic enable
  bool m_rtVideoEnable;  //!< Real-time video traffic enable

  Ptr<RandomVariableStream>         m_idleRng;    //!< Idle random time generator
  Ptr<RandomVariableStream>         m_startRng;   //!< Start random time generator
  Ptr<OpenFlowEpcController>        m_controller; //!< OpenFLow controller
  Ptr<OpenFlowEpcNetwork>           m_network;    //!< OpenFLow network
  std::vector<Ptr<EpcApplication> > m_apps;       //!< Application list

  Ptr<LteUeNetDevice>               m_ueDevice;   //!< UE LTE device

  /** The Application QoS trace source, fired at DumpAppStatistics. */
  TracedCallback<std::string, uint32_t, Ptr<const QosStatsCalculator> > m_appTrace;
};

};  // namespace ns3
#endif // TRAFFIC_MANAGER_H

