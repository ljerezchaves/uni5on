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
class VoipServer;

/**
 * This class implements the VoIP client side, sending and receiving UDP
 * datagrams following VoIP traffic pattern to a VoipServer application.  This
 * VoIP client is bounded to start/stop callbacks, and control start/stop
 * events on the server application. 
 */
class VoipClient : public Application
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
   * \brief Assign a fixed random variable stream number to the random
   * variables used by this model.
   * \param stream first stream index to use.
   * \return the number of stream indices assigned by this model.
   */
  void SetStreams (int64_t stream);

  /**
   * \brief Get the VoIP server application. 
   * \return The pointer to server application. 
   */
  Ptr<VoipServer> GetServerApp ();
  
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
   * \brief Start an "ON" period
   */
  void StartSending ();
  
  /**
   * \brief Start an "OFF" period
   */
  void StopSending ();

  /**
   * \brief Schedules the event to start sending data (switch to "ON" state).
   */
  void ScheduleStartEvent ();
  
  /**
   * \brief Schedules the event to stop sending data (switch to "OFF" state).
   */
  void ScheduleStopEvent ();

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
  Ipv4Address       m_serverAddress;    //!< Outbound server IPv4 address
  uint16_t          m_serverPort;       //!< Outbound server port
  uint16_t          m_localPort;        //!< Inbound local port
  Ptr<VoipServer>   m_serverApp;        //!< VoIP server application
  Ptr<Socket>       m_txSocket;         //!< Outbound TX socket
  Ptr<Socket>       m_rxSocket;         //!< Inbound RX socket
  bool              m_connected;        //!< True if outbound connected
  EventId           m_startStopEvent;   //!< Event id for next start or stop event
  EventId           m_sendEvent;        //!< Event id of pending 'send packet' event
  Ptr<QosStatsCalculator>   m_qosStats; //!< QoS statistics
  Ptr<RandomVariableStream> m_onTime;   //!< Random variable for ON Time
  Ptr<RandomVariableStream> m_offTime;  //!< Random variable for OFF Time
};

} // namespace ns3
#endif /* VOIP_CLIENT_H_ */
