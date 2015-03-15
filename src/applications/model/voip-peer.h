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

#ifndef VOIP_PEER_H_
#define VOIP_PEER_H_

#include <ns3/application.h>
#include <ns3/event-id.h>
#include <ns3/ptr.h>
#include <ns3/ipv4-address.h>
#include <ns3/seq-ts-header.h>
#include "ns3/data-rate.h"
#include "qos-stats-calculator.h"

using namespace std;

namespace ns3 {

class Socket;
class Packet;

/**
 * This class implements both the VoIP client and server sides, sending UDP
 * datagrams following VoIP traffic pattern to another VoipPeer application.
 */
class VoipPeer : public Application
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId
   */ 
  static TypeId GetTypeId (void);
  
  VoipPeer ();             //!< Default constructor
  virtual ~VoipPeer ();    //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the IPv4 destination address and port of the outbound packets.
   * \param ip The IPv4 address to use.
   * \return port The port number to use.
   */
  void SetPeerAddress (Ipv4Address ip, uint16_t port);

  /**
   * \brief Set the VoIP peer application. 
   * \param peer The pointer to peer application. 
   */
  void SetPeerApp (Ptr<VoipPeer> peer);

  /**
   * \brief Assign a fixed random variable stream number to the random
   * variables used by this model.
   * \param stream first stream index to use.
   * \return the number of stream indices assigned by this model.
   */
  void SetStreams (int64_t stream);

  /**
   * \brief Get the VoIP peer application. 
   * \return The pointer to peer application. 
   */
  Ptr<VoipPeer> GetPeerApp ();
  
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
  Ipv4Address       m_peerAddress;      //!< Outbound peer IPv4 address
  uint16_t          m_peerPort;         //!< Outbound peer port
  uint16_t          m_localPort;        //!< Inbound local port
  Ptr<VoipPeer>     m_peerApp;          //!< Peer VoIP application
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
#endif /* VOIP_PEER_H_ */
