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

#include <ns3/application.h>
#include <ns3/event-id.h>
#include <ns3/ptr.h>
#include <ns3/ipv4-address.h>
#include <ns3/seq-ts-header.h>
#include <ns3/random-variable-stream.h>
#include "ns3/data-rate.h"
#include "qos-stats-calculator.h"

using namespace std;

namespace ns3 {

class Socket;
class Packet;
class VoipClient;

/**
 * This class implements the VoIP server side, sending and receiving UDP
 * datagrams following VoIP traffic pattern imposed by VoIP client application.
 * This VoIP server don't use start/stop callbacks, and the start/stop events
 * are called by the client application. 
 */
class VoipServer : public Application
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId
   */ 
  static TypeId GetTypeId (void);
  
  VoipServer ();             //!< Default constructor
  virtual ~VoipServer ();    //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the IPv4 destination address and port of the outbound packets.
   * \param ip The IPv4 address to use.
   * \return port The port number to use.
   */
  void SetClientAddress (Ipv4Address ip, uint16_t port);

  /**
   * \brief Set the VoIP client application. 
   * \param client The pointer to client application. 
   */
  void SetClientApp (Ptr<VoipClient> client);

  /**
   * \brief Get the VoIP client application. 
   * \return The pointer to client application. 
   */
  Ptr<VoipClient> GetClientApp ();
  
  /** 
   * Reset the QoS statistics
   */
  void ResetQosStats ();

  /**
   * \brief Start an "ON" period
   */
  void StartSending ();
  
  /**
   * \brief Start an "OFF" period
   */
  void StopSending ();

  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator 
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents ();

  /**
   * \brief Handle a connection succeed event.
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  
  /**
   * \brief Handle a connection failed event.
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  /**
   * \brief Handle a packet transmission.
   */
  void SendPacket ();

  /**
   * \brief Handle a packet reception.
   * \param socket the socket the packet was received to.
   */
  void ReadPacket (Ptr<Socket> socket);

  Time              m_interval;         //!< Interval between packets
  uint32_t          m_pktSize;          //!< Packet size
  uint32_t          m_pktSent;          //!< Number of TX packets
  Ipv4Address       m_clientAddress;    //!< Outbound client IPv4 address
  uint16_t          m_clientPort;       //!< Outbound client port
  uint16_t          m_localPort;        //!< Inbound local port
  Ptr<VoipClient>   m_clientApp;        //!< VoIP client application
  Ptr<Socket>       m_txSocket;         //!< Outbound TX socket
  Ptr<Socket>       m_rxSocket;         //!< Inbound RX socket
  bool              m_connected;        //!< True if outbound connected
  EventId           m_startStopEvent;   //!< Event id for next start or stop event
  EventId           m_sendEvent;        //!< Event id of pending 'send packet' event
  Ptr<QosStatsCalculator>   m_qosStats; //!< QoS statistics
};

} // namespace ns3
#endif /* VOIP_SERVER_H_ */
