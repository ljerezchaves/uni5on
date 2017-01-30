/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include "qos-stats-calculator.h"

namespace ns3 {

class SdmnServerApp;

/**
 * \ingroup sdmnApps
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
  friend class SdmnServerApp;
  friend class TrafficHelper;
  friend class TrafficManager;

public:
  SdmnClientApp ();            //!< Default constructor
  virtual ~SdmnClientApp ();   //!< Dummy destructor, see DoDispose

  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Get the active state for this application.
   * \return true if the application is active, false otherwise.
   */
  bool IsActive (void) const;

  /**
   * Get the force stop state for this application.
   * \return true if the application is in force stop state, false otherwise.
   */
  bool IsForceStop (void) const;

  /**
   * Get the application name.
   * \return The application name.
   */
  std::string GetAppName (void) const;

  /**
   * Get QoS statistics.
   * \return Get the constant pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

  /**
   * Get QoS statistics from server.
   * \return Get the constant pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetServerQosStats (void) const;

  /**
   * \return The P-GW TFT for this application.
   */
  Ptr<EpcTft> GetTft (void) const;

  /**
   * \return The EpsBearer for this application.
   */
  EpsBearer GetEpsBearer (void) const;

  /**
   * \return The TEID for this application.
   */
  uint32_t GetTeid (void) const;

  /**
   * \brief Set the server application.
   * \param serverApp The pointer to server application.
   * \param serverAddress The IPv4 address of the server.
   * \param serverPort The port number on the server.
   */
  void SetServer (Ptr<SdmnServerApp> serverApp, Ipv4Address serverAddress,
                  uint16_t serverPort);

  /**
   * \brief Get the server application.
   * \return The pointer to server application.
   */
  Ptr<SdmnServerApp> GetServerApp ();

  /**
   * Start this application. Start traffic generation,
   * reset internal counters and fire the start trace.
   */
  virtual void Start ();

  /**
   * TracedCallback signature for SdmnClientApp.
   * \param app The client application.
   */
  typedef void (*EpcAppTracedCallback)(Ptr<const SdmnClientApp> app);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Stop this application. Close the socket and fire the stop trace (which
   * will trigger statistics dumps). This function is expected to be called
   * only after application traffic is completely stopped (no pending bytes
   * for transmission and no in-transit packets).
   */
  virtual void Stop ();

  /**
   * Force this application to stop. This function will stops the traffic
   * generation, allowing on-transit packets to reach the destination before
   * effectively stopping the application.
   */
  virtual void ForceStop ();

  /**
   * Update TX counter for a new transmitted packet on server stats calculator.
   * \param txBytes The total number of bytes in this packet.
   * \return The next TX sequence number to use.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counter for a new received packet on client stats calculator.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  Ptr<QosStatsCalculator> m_qosStats;         //!< QoS statistics.
  Ptr<Socket>             m_socket;           //!< Local socket.
  uint16_t                m_localPort;        //!< Local port.
  Ipv4Address             m_serverAddress;    //!< Server address.
  uint16_t                m_serverPort;       //!< Server port.
  Ptr<SdmnServerApp>      m_serverApp;        //!< Server application.

  /** Application start trace source, fired when application start. */
  TracedCallback<Ptr<const SdmnClientApp> > m_appStartTrace;

  /** Application stop trace source, fired when application stops. */
  TracedCallback<Ptr<const SdmnClientApp> > m_appStopTrace;

private:
  /**
   * Reset the QoS statistics
   */
  virtual void ResetQosStats ();

  std::string   m_name;           //!< Application name.
  bool          m_active;         //!< Active state.
  Time          m_maxOnTime;      //!< Max duration time.
  EventId       m_forceStop;      //!< Max duration stop event.
  bool          m_forceStopFlag;  //!< Force stop flag.

  // LTE EPC metadata
  Ptr<EpcTft>   m_tft;            //!< Traffic flow template for this app.
  EpsBearer     m_bearer;         //!< EPS bearer info.
  uint32_t      m_teid;           //!< GTP TEID associated with this app.
};

} // namespace ns3
#endif /* SDMN_CLIENT_APP_H */
