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
#include "apps/sdmn-application.h"
#include "routing-info.h"

namespace ns3 {

class EpcController;

/**
 * \ingroup sdmn
 * Traffic manager which handles SDMN client applications start/stop events. It
 * interacts with the OpenFlow EPC network and controller to dump statistics
 * and request/release EPS bearers. Each LteUeNetDevice has one TrafficManager
 * object aggregated to it.
 */
class TrafficManager : public Object
{
  friend class TrafficHelper;

public:
  TrafficManager ();          //!< Default constructor
  virtual ~TrafficManager (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Add a new application to this manager.
   * \param app The application pointer.
   */
  void AddSdmnApplication (Ptr<SdmnApplication> app);

  /**
   * Attempt to (re)start this application. This method will request for bearer
   * resources to the controller before starting the application. If the
   * controller accept the request, this starts the application. Otherwise, it
   * reschedule the (re)start attempt.
   * \internal
   * The teid approach only works because we currently have a single
   * application associated with each bearer/tunnel. If we would like to
   * aggregate traffic from several applications into same bearer we will need
   * to revise this.
   * \param app The application pointer.
   */
  void AppStartTry (Ptr<SdmnApplication> app);

  /**
   * Member function called by applications to notify this manager when traffic
   * stops. This method will fire network statistics (EPC) and schedule
   * application restart attempt.
   * \param app The application pointer.
   */
  void NotifyAppStop (Ptr<const SdmnApplication> app);

  /**
   * Trace sink notified when new session is created. This will be used to get
   * the teid for each bearer created.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param sgwAddr The SgwPgw IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  void SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
                               Ipv4Address enbAddr, Ipv4Address sgwAddr,
                               BearerList_t bearerList);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  Ptr<ExponentialRandomVariable>      m_poissonRng; //!< Inter-arrival traffic
  Ptr<EpcController>                  m_controller; //!< OpenFlow controller
  std::vector<Ptr<SdmnApplication> >  m_apps;       //!< Application list

  uint64_t    m_imsi;         //!< UE IMSI identifier
  uint16_t    m_cellId;       //!< Current eNB cellId
  uint32_t    m_defaultTeid;  //!< TEID for default UE tunnel
};

};  // namespace ns3
#endif // TRAFFIC_MANAGER_H

