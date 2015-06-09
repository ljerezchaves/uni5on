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

#ifndef EPC_APPLICATION_H
#define EPC_APPLICATION_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/epc-tft.h"
#include "ns3/eps-bearer.h"
#include "ns3/traced-callback.h"
#include "qos-stats-calculator.h"

namespace ns3 {

/**
 * \ingroup applications
 * This class extends the Application class to proper work ith OpenFlow + EPC
 * simulations.  Only clients applications should use this EpcApplication as
 * superclass. It includes a QosStatsCalculator for traffic statistics, and a
 * stop callback to notify when the traffic stops. For LTE EPC, each
 * application is associated with an EPS bearer, and traffic is sent over GTP
 * tunnels. These info are also saved here for further usage.
 */
class EpcApplication : public Application
{
  friend class TrafficHelper;
  friend class TrafficManager;

public:
  EpcApplication ();            //!< Default constructor
  virtual ~EpcApplication ();   //!< Dummy destructor, see DoDipose
  
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
   * \return The tft for this application.
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
   * Create the description string for this application, including name and UE
   * ISMI identifier.
   * \return The description string.
   */
  std::string GetDescription (void) const;

  /**
   * Get the application name.
   * \return The application name.
   */
  virtual std::string GetAppName (void) const;

  /**
   * Start this application at any time.
   */
  virtual void Start (void);
  
  /**
   * Stop callback signature. 
   */
  typedef Callback<void, Ptr<EpcApplication> > StopCb_t;

  /** 
   * TracedCallback signature for EpcApplication QoS stats.
   * \param description String describing this application.
   * \param teid GTP TEID.
   * \param stats The QoS statistics.
   */
  typedef void (* AppStatsTracedCallback)(std::string description, uint32_t teid, 
                                          Ptr<const QosStatsCalculator> stats);

  /**
   * TracedCallback signature for EpcApplication.
   * \param app The EpcApplication.
   */
  typedef void (* EpcAppTracedCallback)(Ptr<const EpcApplication> app);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Set the stop callback
   * \param cb The callback to invoke when traffic stops.
   */
  void SetStopCallback (StopCb_t cb);

  /**
   * Reset the QoS statistics
   */
  virtual void ResetQosStats ();

  /**
   * Dump application statistics. By default, only statistics for this app will
   * be dumped to m_appTrace trace source. Specialized applications can
   * override this method to dump additional information (like stats from
   * server).
   */
  virtual void DumpAppStatistics (void) const;

  Ptr<QosStatsCalculator> m_qosStats;   //!< QoS statistics
  StopCb_t                m_stopCb;     //!< Stop callback

  /** The Application QoS trace source, fired when application stops. */
  TracedCallback<std::string, uint32_t, Ptr<const QosStatsCalculator> > m_appTrace;

private:
  // LTE EPC metadata 
  Ptr<EpcTft>   m_tft;        //!< Traffic flow template for this app
  EpsBearer     m_bearer;     //!< EPS bearer info
  uint32_t      m_teid;       //!< GTP TEID associated with this app
  std::string   m_desc;       //!< UE@eNB description set by traffic manager
};

} // namespace ns3
#endif /* EPC_APPLICATION_H */
