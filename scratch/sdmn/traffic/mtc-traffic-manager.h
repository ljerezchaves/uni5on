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

#ifndef MTC_TRAFFIC_MANAGER_H
#define MTC_TRAFFIC_MANAGER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../apps/sdmn-client-app.h"
#include "../info/routing-info.h"

namespace ns3 {

class SdranController;

/**
 * \ingroup sdmn
 * Traffic manager which handles SDMN client applications start/stop events.
 * It interacts with the SDMN architecture to request and release EPS bearers.
 * Each MTC LteUeNetDevice has one MtcTrafficManager object aggregated to it.
 */
class MtcTrafficManager : public Object
{
public:
  MtcTrafficManager ();          //!< Default constructor.
  virtual ~MtcTrafficManager (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Add a new application to this manager.
   * \param app The application pointer.
   */
  void AddSdmnClientApp (Ptr<SdmnClientApp> app);

  /**
   * Trace sink notified when new session is created.
   * This will be used to get the teid for each bearer created.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param bearerList The list of context bearers created.
   */
  void SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
                               BearerContextList_t bearerList);

  /**
   * Set the IMSI attribute.
   * \param value The ISMI value.
   */
  void SetImsi (uint64_t value);

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
  void AppStartTry (Ptr<SdmnClientApp> app);

  /**
   * Member function called by applications to notify this manager when traffic
   * stops. This method will fire network statistics (EPC) and schedule
   * application restart attempt.
   * \param app The application pointer.
   */
  void NotifyAppStop (Ptr<SdmnClientApp> app);

  /** Set saving application pointers. */
  typedef std::set<Ptr<SdmnClientApp> > AppSet_t;

  Ptr<RandomVariableStream> m_poissonRng;     //!< Inter-arrival traffic.
  bool                      m_restartApps;    //!< Continuously restart apps.
  Ptr<SdranController>      m_ctrlApp;        //!< OpenFlow controller.
  AppSet_t                  m_appTable;       //!< Application set.
  uint64_t                  m_imsi;           //!< UE IMSI identifier.
  uint16_t                  m_cellId;         //!< Current eNB cellId.
  uint32_t                  m_defaultTeid;    //!< TEID for default UE tunnel.
};

};  // namespace ns3
#endif // MTC_TRAFFIC_MANAGER_H

