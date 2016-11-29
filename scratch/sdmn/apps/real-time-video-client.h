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

#ifndef REAL_TIME_VIDEO_CLIENT_H
#define REAL_TIME_VIDEO_CLIENT_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "real-time-video-server.h"
#include "sdmn-client-app.h"
#include "qos-stats-calculator.h"

namespace ns3 {

class RealTimeVideoServer;

/**
 * \ingroup applications
 * This is the client side of a real-time video traffic generator. The client
 * start the transmission at the server (using a direct member function call),
 * and receives UDP datagrams from server to measures statistiscs.
 */
class RealTimeVideoClient : public SdmnClientApp
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RealTimeVideoClient ();   //!< Default constructor
  ~RealTimeVideoClient ();  //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the server application.
   * \param server The pointer to server application.
   */
  void SetServer (Ptr<RealTimeVideoServer> server);

  /**
   * \brief Get the server application.
   * \return The pointer to server application.
   */
  Ptr<RealTimeVideoServer> GetServerApp ();

  /**
   * Set the server trace file attribute.
   * \param filename Name of file to load a trace from.
   */
  void SetTraceFilename (std::string filename);

  /**
   * \brief Invoked when server stops sending traffic.
   * \param pkts The total number of packets transmitted by the server.
   */
  void ServerTrafficEnd (uint32_t pkts);

  // Inherited from SdmnClientApp
  void Start (void);

protected:
  virtual void DoDispose (void);

  /**
   * \brief Dump application statistcs and fire stop callback.
   */
  void NotifyStop ();

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Handle a packet reception.
   * \param socket the socket the packet was received to.
   */
  void ReadPacket (Ptr<Socket> socket);
  
  uint16_t                  m_localPort;  //!< Local port
  Ptr<Socket>               m_socket;     //!< Local socket
  Ptr<RealTimeVideoServer>  m_serverApp;  //!< Server application
};

} // namespace ns3
#endif /* REAL_TIME_VIDEO_CLIENT_H */
