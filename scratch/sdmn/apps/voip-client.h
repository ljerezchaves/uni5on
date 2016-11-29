/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2014 University of Campinas (Unicamp)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef VOIP_CLIENT_H_
#define VOIP_CLIENT_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "voip-server.h"
#include "sdmn-client-app.h"
#include "qos-stats-calculator.h"

namespace ns3 {

class VoipServer;

/**
 * \ingroup applications
 * This is the client side of a voip traffic generator. This client sends and
 * receives UDP datagrams following voip traffic pattern. This client control
 * start/stop events on the server application. 
 */
class VoipClient : public SdmnClientApp
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId
   */ 
  static TypeId GetTypeId (void);
  
  VoipClient ();             //!< Default constructor
  virtual ~VoipClient ();    //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the server application.
   * \param server The pointer to server application.
   * \param serverAddress The IPv4 address of the server.
   * \param serverPort The port number on the server
   */
  void SetServer (Ptr<VoipServer> server, Ipv4Address serverAddress,
                  uint16_t serverPort);

  /**
   * \brief Get the VoIP server application. 
   * \return The pointer to server application. 
   */
  Ptr<VoipServer> GetServerApp ();
 
  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetServerQosStats (void) const;

  /** 
   * Reset the QoS statistics
   */
  void ResetQosStats ();

  /**
   * \brief Callback invoked when server stops sending traffic.
   * \param pkts The total number of packets transmitted by the server.
   */
  void ServerTrafficEnd (uint32_t pkts);

  // Inherited from SdmnClientApp
  void Start (void);

protected:
  /** Destructor implementation */
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
   * \brief Start the streaming.
   */
  void StartSending ();
  
  /**
   * \brief Stop the streaming.
   */
  void StopSending ();

  /**
   * \brief Handle a packet transmission.
   */
  void SendPacket ();

  /**
   * \brief Handle a packet reception.
   * \param socket the socket the packet was received to.
   */
  void ReadPacket (Ptr<Socket> socket);

  Time                      m_interval;       //!< Interval between packets
  uint32_t                  m_pktSize;        //!< Packet size
  uint32_t                  m_pktSent;        //!< Number of TX packets
  Ipv4Address               m_serverAddress;  //!< Server address
  uint16_t                  m_serverPort;     //!< Server port
  Ptr<VoipServer>           m_serverApp;      //!< Server application
  uint16_t                  m_localPort;      //!< Local port
  Ptr<Socket>               m_socket;         //!< Local socket
  EventId                   m_sendEvent;      //!< SendPacket event
};

} // namespace ns3
#endif /* VOIP_CLIENT_H_ */
