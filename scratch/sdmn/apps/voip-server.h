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

#ifndef VOIP_SERVER_H_
#define VOIP_SERVER_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "voip-client.h"
#include "qos-stats-calculator.h"

namespace ns3 {

class VoipClient;

/**
 * \ingroup applications
 * This is the server side of a voip traffic generator. This server sends and
 * receives UDP datagrams following voip traffic pattern. The start/stop events
 * are called by the client application. 
 */
class VoipServer : public Application
{
  friend class VoipClient;

public:
  /**
   * \brief Register this type.
   * \return the object TypeId
   */ 
  static TypeId GetTypeId (void);
  
  VoipServer ();             //!< Default constructor
  virtual ~VoipServer ();    //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the client application.
   * \param client The pointer to client application.
   * \param clientAddress The IPv4 address of the client.
   * \param clientPort The port number on the client.
   */
  void SetClient (Ptr<VoipClient> client, Ipv4Address clientAddress, 
                  uint16_t clientPort);

  /**
   * \brief Get the client application.
   * \return The pointer to client application.
   */
  Ptr<VoipClient> GetClientApp ();

  /** 
   * Reset the QoS statistics
   */
  void ResetQosStats ();

  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator 
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  
  /**
   * \brief Start the streaming.
   * \param maxDuration The maximum duration for this traffic.
   */
  void StartSending (Time maxDuration = Time ());
  
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
  Ipv4Address               m_clientAddress;  //!< Client address
  uint16_t                  m_clientPort;     //!< Client port
  Ptr<VoipClient>           m_clientApp;      //!< Client application
  uint16_t                  m_localPort;      //!< Local port
  Ptr<Socket>               m_socket;         //!< Local socket
  EventId                   m_sendEvent;      //!< SendPacket event
  Ptr<QosStatsCalculator>   m_qosStats;       //!< QoS statistics
  Ptr<RandomVariableStream> m_lengthRng;      //!< Random call length
};

} // namespace ns3
#endif /* VOIP_SERVER_H_ */
