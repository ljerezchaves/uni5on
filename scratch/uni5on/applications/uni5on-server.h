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

#ifndef UNI5ON_SERVER_H
#define UNI5ON_SERVER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

class Uni5onClient;

/**
 * \ingroup uni5onApps
 * This class extends the Application class to proper work with the UNI5ON
 * architecture. Only server applications (those which will be installed into
 * web server node) should extend this class.
 */
class Uni5onServer : public Application
{
  friend class Uni5onClient;

public:
  Uni5onServer ();            //!< Default constructor.
  virtual ~Uni5onServer ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested value.
   */
  //\{
  std::string       GetAppName    (void) const;
  Ptr<Uni5onClient> GetClientApp  (void) const;
  std::string       GetTeidHex    (void) const;
  bool              IsActive      (void) const;
  bool              IsForceStop   (void) const;
  //\}

  /**
   * \brief Set the client application.
   * \param clientApp The pointer to client application.
   * \param clientAddress The Inet socket address of the client.
   */
  void SetClient (Ptr<Uni5onClient> clientApp, Address clientAddress);

  /**
   * Get the uplink goodput for this application.
   * \return The requested goodput.
   */
  DataRate GetUlGoodput (void) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  /**
   * Notify this server of a start event on the client application. Update
   * internal members and start traffic when applicable.
   */
  virtual void NotifyStart ();

  /**
   * Notify this server of a stop event on the client application. Update
   * internal members.
   */
  virtual void NotifyStop ();

  /**
   * Notify this server of a force stop event on the client application. Update
   * internal members and stop traffic when applicable.
   */
  virtual void NotifyForceStop ();

  /**
   * Update RX counter for new bytes received by this application.
   * \param bytes The number of bytes received.
   */
  void NotifyRx (uint32_t bytes);

  Ptr<Socket>               m_socket;           //!< Local socket.
  uint16_t                  m_localPort;        //!< Local port.
  Address                   m_clientAddress;    //!< Client address.
  Ptr<Uni5onClient>         m_clientApp;        //!< Client application.

  // Traffic statistics.
  uint64_t                  m_rxBytes;        //!< Number of RX bytes.
  Time                      m_startTime;      //!< App start time.
  Time                      m_stopTime;       //!< App stop time.
};

} // namespace ns3
#endif /* UNI5ON_SERVER_H */
