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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef BASE_CLIENT_H
#define BASE_CLIENT_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

class BaseServer;

/**
 * \ingroup uni5on
 * \defgroup uni5onApps Applications
 * Applications prepared to work with the UNI5ON architecture.
 */
/**
 * \ingroup uni5onApps
 * This base class extends the Application class to proper work with the UNI5ON
 * architecture. Clients applications that will be installed into UEs should
 * extend this class.
 */
class BaseClient : public Application
{
  friend class BaseServer;

public:
  BaseClient ();            //!< Default constructor.
  virtual ~BaseClient ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested value.
   */
  //\{
  std::string       GetAppName        (void) const;
  EpsBearer         GetEpsBearer      (void) const;
  uint8_t           GetEpsBearerId    (void) const;
  Time              GetMaxOnTime      (void) const;
  std::string       GetNameTeid       (void) const;
  Ptr<BaseServer> GetServerApp      (void) const;
  uint32_t          GetTeid           (void) const;
  std::string       GetTeidHex        (void) const;
  bool              IsActive          (void) const;
  bool              IsForceStop       (void) const;
  //\}

  /**
   * \name Private member modifiers.
   * \param value The value to set.
   */
  //\{
  void              SetEpsBearer      (EpsBearer  value);
  void              SetEpsBearerId    (uint8_t    value);
  void              SetTeid           (uint32_t   value);
  //\}

  /**
   * \brief Set the server application.
   * \param serverApp The pointer to server application.
   * \param serverAddress The Inet socket address of the server.
   */
  void SetServer (Ptr<BaseServer> serverApp, Address serverAddress);

  /**
   * Start this application. Update internal members, notify the server
   * application, fire the start trace source, and start traffic generation.
   */
  virtual void Start ();

  /**
   * Get the downlink goodput for this application.
   * \return The requested goodput.
   */
  DataRate GetDlGoodput (void) const;

  /**
   * Get the uplink goodput for this application.
   * \return The requested goodput.
   */
  DataRate GetUlGoodput (void) const;

  /**
   * TracedCallback signature for Ptr<Uni5onClient>.
   * \param app The client application.
   */
  typedef void (*AppTracedCallback)(Ptr<BaseClient> app);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Force this application to stop. This function will interrupt traffic
   * generation, allowing on-transit packets to reach the destination before
   * closing sockets and notifying the stop event.
   */
  virtual void ForceStop ();

  /**
   * Get the random traffic length for this application.
   * \return The random traffic length.
   */
  Time GetTrafficLength ();

  /**
   * Notify the stop event on this client application. This function is
   * expected to be called only after application traffic is completely stopped
   * (no pending bytes for transmission, no in-transit packets, and closed
   * sockets). It will fire stop trace source.
   * \param withError If true indicates an event with erros, false otherwise.
   */
  virtual void NotifyStop (bool withError = false);

  /**
   * Update RX counter for new bytes received by this application.
   * \param bytes The number of bytes received.
   */
  void NotifyRx (uint32_t bytes);

  Ptr<Socket>               m_socket;           //!< Local socket.
  uint16_t                  m_localPort;        //!< Local port.
  Address                   m_serverAddress;    //!< Server address.
  Ptr<BaseServer>         m_serverApp;        //!< Server application.

  /** Trace source fired when application start. */
  TracedCallback<Ptr<BaseClient>> m_appStartTrace;

  /** Trace source fired when application stops. */
  TracedCallback<Ptr<BaseClient>> m_appStopTrace;

  /** Trace source fired when application reports an error. */
  TracedCallback<Ptr<BaseClient>> m_appErrorTrace;

private:
  std::string               m_name;           //!< Application name.
  bool                      m_active;         //!< Active state.
  Ptr<RandomVariableStream> m_lengthRng;      //!< Random traffic length.
  Time                      m_maxOnTime;      //!< Max duration time.
  EventId                   m_forceStop;      //!< Max duration stop event.
  bool                      m_forceStopFlag;  //!< Force stop flag.

  // Traffic statistics.
  uint64_t                  m_rxBytes;        //!< Number of RX bytes.
  Time                      m_startTime;      //!< App start time.
  Time                      m_stopTime;       //!< App stop time.

  // EPS bearer metadata.
  EpsBearer                 m_bearer;         //!< EPS bearer info.
  uint8_t                   m_bearerId;       //!< EPS bearer ID.
  uint32_t                  m_teid;           //!< GTP TEID.
};

} // namespace ns3
#endif /* BASE_CLIENT_H */
