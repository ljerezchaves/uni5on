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


#ifndef VIDEO_CLIENT_H
#define VIDEO_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/core-module.h"
#include "udp-server.h"
#include <vector>

namespace ns3 {

class Socket;
class Packet;

/**
 * This class implements the same functionality of UdpTraceClient, sending UDP
 * datagrams following Video traffic pattern, but alternates between ON/OFF
 * periods in the same way VoIP does.
 */
class VideoClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  VideoClient ();       //!< Default constructor
  ~VideoClient ();      //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the IPv4 destination address and port of the outbound packets.
   * \param ip The IPv4 address to use.
   * \return port The port number to use.
   */
  void SetRemote (Ipv4Address ip, uint16_t port);

  /**
   * \brief Set the trace file to be used by the application
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void SetTraceFile (std::string filename);
 
  /**
   * \brief Set the maximum packet size
   * \param maxPacketSize The maximum packet size
   */
  void SetMaxPacketSize (uint16_t maxPacketSize);

  /**
   * \brief Set the UdpServer application. 
   * \param server The pointer to server application. 
   */
  void SetServerApp (Ptr<UdpServer> server);
  
  /**
   * \brief Get the UdpServer application. 
   * \return The pointer to server application. 
   */
  Ptr<UdpServer> GetServerApp ();

  /**
   * \brief Return the maximum packet size
   * \return the maximum packet size
   */
  uint16_t GetMaxPacketSize (void);
  
  /** 
   * \brief Reset counter and statistics 
   */
  void ResetCounters ();
  
  /**
   * \brief Get application statistics.
   * \return The statistic value.
   */
  //\{
  uint32_t  GetTxPackets  (void)  const;
  uint32_t  GetTxBytes    (void)  const;
  Time      GetActiveTime (void)  const;
  //\}

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Load a trace file.
   * \param filename The trace file path.
   */
  void LoadTrace (std::string filename);
  
  /**
   * \brief Load the default trace
   */
  void LoadDefaultTrace (void);
  
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
   * \brief Start sending the video stream.
   */
  void SendStream (void);

  /**
   * \brief Handle a packet transmission.
   * \param size The packet size
   */
  void SendPacket (uint32_t size);

  /**
   * \brief Entry to send.
   * Each entry represents an MPEG frame
   */
  struct TraceEntry
  {
    uint32_t timeToSend; //!< Time to send the frame
    uint32_t packetSize; //!< Size of the frame
    char frameType; //!< Frame type (I, P or B)
  };

  uint32_t          m_sent;             //!< Counter for sent packets
  uint32_t          m_txBytes;          //!< Number of TX bytes
  Ptr<Socket>       m_socket;           //!< Socket
  Ipv4Address       m_peerAddress;      //!< Remote peer address
  uint16_t          m_peerPort;         //!< Remote peer port
  EventId           m_startStopEvent;   //!< Event id for next start or stop event
  EventId           m_sendEvent;        //!< Event id of pending 'send packet' event
  uint16_t          m_maxPacketSize;    //!< Maximum packet size to send (including the SeqTsHeader)
  bool              m_connected;        //!< True if connected
  Time              m_lastStartTime;    //!< Last start time
  uint32_t          m_currentEntry;     //!< Current entry index
  Ptr<UdpServer>    m_serverApp;        //!< UdpServer application
  Ptr<RandomVariableStream>       m_onTime;           //!< rng for On Time
  Ptr<RandomVariableStream>       m_offTime;          //!< rng for Off Time
  std::vector<struct TraceEntry>  m_entries;          //!< Entries in the trace to send
  static struct TraceEntry        g_defaultEntries[]; //!< Default trace to send
};

} // namespace ns3

#endif /* VIDEO_CLIENT_H */
