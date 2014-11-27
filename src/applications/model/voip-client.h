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

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/seq-ts-header.h"

using namespace std;

namespace ns3 {

class Socket;
class Packet;

/**
 * This class is similar to an OnOffApplication, which send packets following
 * VoIP traffic pattern togheter with an UdpServer to receive the packets.
 */
class VoipClient : public Application
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId
   */ 
  static TypeId GetTypeId (void);
  
  VoipClient ();
  virtual ~VoipClient ();

  void SetRemote (Ipv4Address ip, uint16_t port);
  void SetRemote (Ipv6Address ip, uint16_t port);
  void SetRemote (Address ip, uint16_t port);

 /**
  * \brief Assign a fixed random variable stream number to the random variables
  * used by this model.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  void AssignStreams (int64_t stream);

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
   * \brief Start an On period
   */
  void StartSending ();
  
  /**
   * \brief Start an Off period
   */
  void StopSending ();
  
  /**
   * \brief Send a packet
   */
  void SendPacket ();

  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx ();
  
  /**
   * \brief Schedule the next On period start
   */
  void ScheduleStartEvent ();
  
  /**
   * \brief Schedule the next Off period start
   */
  void ScheduleStopEvent ();

  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  Time        m_interval;               //!< Interval between packets
  uint32_t    m_size;                   //!< Packet size
  uint32_t    m_sent;                   //!< Number of packets send
  Ptr<Socket> m_socket;                 //!< Associated socket
  Address     m_peerAddress;            //!< Peer address
  uint16_t    m_peerPort;               //!< Peer port
  bool        m_connected;              //!< True if connected
  Time        m_lastStartTime;          //!< Time last packet sent
  uint32_t    m_totBytes;               //!< Total bytes sent so far
  EventId     m_startStopEvent;         //!< Event id for next start or stop event
  EventId     m_sendEvent;              //!< Event id of pending 'send packet' event
  Ptr<RandomVariableStream>  m_onTime;  //!< rng for On Time
  Ptr<RandomVariableStream>  m_offTime; //!< rng for Off Time
};

} // namespace ns3
#endif /* VOIP_CLIENT_H_ */
