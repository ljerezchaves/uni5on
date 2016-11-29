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

#ifndef SDMN_CLIENT_APP_H
#define SDMN_CLIENT_APP_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/epc-tft.h"
#include "ns3/eps-bearer.h"
#include "ns3/traced-callback.h"
#include "qos-stats-calculator.h"
#include "ns3/string.h"

namespace ns3 {

/**
 * \ingroup applications
 * This class extends the Application class to proper work with the SDMN
 * architecture. Only clients applications (those which will be installed into
 * UEs) should extends this class. It includes a QosStatsCalculator for traffic
 * statistics, and start/stop callbacks to notify the SDMN controller when the
 * traffic stats/stops. Each application is associated with an EPS bearer, and
 * application traffic is sent within GTP tunnels over EPC interfaces. These
 * informations are also saved here for further usage.
 */
class SdmnClientApp : public Application
{
  friend class TrafficHelper;
  friend class TrafficManager;

public:
  SdmnClientApp ();            //!< Default constructor
  virtual ~SdmnClientApp ();   //!< Dummy destructor, see DoDipose

  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

  /**
   * \return The pgw tft for this application.
   */
  Ptr<EpcTft> GetTft (void) const;

  /**
   * \return The EpsBearer for this application.
   */
  EpsBearer GetEpsBearer (void) const;

  /**
   * \return The teid for this application.
   */
  uint32_t GetTeid (void) const;

  /**
   * Get the application name.
   * \return The application name.
   */
  std::string GetAppName (void) const;

  /**
   * Start this application at any time.
   */
  virtual void Start (void);

  /**
   * Get the active state for this application.
   * \return true if the application is active, false otherwise.
   */
  bool IsActive (void) const;

  /**
   * TracedCallback signature for SdmnClientApp.
   * \param app The SdmnClientApp.
   */
  typedef void (*EpcAppTracedCallback)(Ptr<const SdmnClientApp> app);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Reset the QoS statistics
   */
  virtual void ResetQosStats ();

  Ptr<QosStatsCalculator> m_qosStats;   //!< QoS statistics

  Time m_maxDurationTime;   //!< Application max duration time
  bool m_active;            //!< Active state for this application
  std::string m_name;       //!< Name for this application

  /** Application start trace source, fired when application start. */
  TracedCallback<Ptr<const SdmnClientApp> > m_appStartTrace;

  /** Application stop trace source, fired when application stopts. */
  TracedCallback<Ptr<const SdmnClientApp> > m_appStopTrace;

private:
  // LTE EPC metadata
  Ptr<EpcTft>   m_tft;        //!< Traffic flow template for this app
  EpsBearer     m_bearer;     //!< EPS bearer info
  uint32_t      m_teid;       //!< GTP TEID associated with this app
};

} // namespace ns3
#endif /* SDMN_CLIENT_APP_H */
