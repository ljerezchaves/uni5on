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

#ifndef SDMN_SERVER_APP_H
#define SDMN_SERVER_APP_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include "qos-stats-calculator.h"

namespace ns3 {

class SdmnClientApp;

/**
 * \ingroup sdmnApps
 * This class extends the Application class to proper work with SDMN
 * architecture. Only server applications (those which will be installed into
 * web server node) extends this class.
 */
class SdmnServerApp : public Application
{
  friend class SdmnClientApp;

public:
  SdmnServerApp ();            //!< Default constructor
  virtual ~SdmnServerApp ();   //!< Dummy destructor, see DoDispose

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
   * Get QoS statistics
   * \return Get the constant pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

  /**
   * \brief Set the client application.
   * \param clientApp The pointer to client application.
   * \param clientAddress The IPv4 address of the client.
   * \param clientPort The port number on the client.
   */
  void SetClient (Ptr<SdmnClientApp> clientApp, Ipv4Address clientAddress,
                  uint16_t clientPort);

  /**
   * \brief Get the client application.
   * \return The pointer to client application.
   */
  Ptr<SdmnClientApp> GetClientApp ();

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Notify this server of a start event on the client application. The server
   * must reset internal counters and start generating traffic (when
   * applicable).
   */
  virtual void NotifyStart ();

  /**
   * Notify this server of a stop event on the client application. The server
   * must close the socket (when applicable). This function is expected to be
   * called only after application traffic is completely stopped (no pending
   * bytes for transmission and no in-transit packets).
   */
  virtual void NotifyStop ();

  /**
   * Notify this server of a force stop event on the client application. The
   * server must stop generating traffic (when applicable), and be prepared to
   * the upcoming stop event on the client side.
   */
  virtual void NotifyForceStop ();

  /**
   * Update TX counter for a new transmitted packet on client stats calculator.
   * \param txBytes The total number of bytes in this packet.
   * \return The next TX sequence number to use.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counter for a new received packet on server stats calculator.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  /**
   * Reset the QoS statistics
   */
  void ResetQosStats ();

  Ptr<QosStatsCalculator> m_qosStats;         //!< QoS statistics.
  Ptr<Socket>             m_socket;           //!< Local socket.
  uint16_t                m_localPort;        //!< Local port.
  Ipv4Address             m_clientAddress;    //!< Client address.
  uint16_t                m_clientPort;       //!< Client port.
  Ptr<SdmnClientApp>      m_clientApp;        //!< Client application.

private:
  bool                    m_active;           //!< Active state.
  bool                    m_forceStopFlag;    //!< Force stop flag.
};

} // namespace ns3
#endif /* SDMN_SERVER_APP_H */
