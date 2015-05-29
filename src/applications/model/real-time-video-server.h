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

#ifndef REAL_TIME_VIDEO_SERVER_H
#define REAL_TIME_VIDEO_SERVER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "real-time-video-client.h"

namespace ns3 {

class RealTimeVideoClient;

/**
 * \ingroup applications
 * This is the server side of a real-time video traffic generator. The server
 * send UDP datagrams to a client following a MPEG video pattern with random
 * video length.
 */
class RealTimeVideoServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RealTimeVideoServer ();   //!< Default constructor
  ~RealTimeVideoServer ();  //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the trace file to be used by the application
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void SetTraceFile (std::string filename);

  /**
   * \brief Set the maximum packet size.
   * \param maxPacketSize The maximum packet size.
   */
  void SetMaxPacketSize (uint16_t maxPacketSize);

  /**
   * \brief Return the maximum packet size.
   * \return the maximum packet size.
   */
  uint16_t GetMaxPacketSize (void) const;

  /**
   * \brief Set the client application.
   * \param client The pointer to client application.
   * \param clientAddress The IPv4 address of the client.
   * \param clientPort The port number on the client.
   */
  void SetClientApp (Ptr<RealTimeVideoClient> client, 
                     Ipv4Address clientAddress, uint16_t clientPort);

  /**
   * \brief Get the client application.
   * \return The pointer to client application.
   */
  Ptr<RealTimeVideoClient> GetClientApp ();

  /**
   * \brief Start the real-time streaming.
   */
  void StartSending ();
  
  /**
   * \brief Stop the real-time streaming.
   */
  void StopSending ();

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
   * \brief Start sending the video. Callback for socket SetSendCallback.
   * \param socket The pointer to the socket.
   * \param size The number of bytes available for writing into the buffer
   */
  void SendStream (void);

  /**
   * \brief Handle a packet transmission.
   * \param size The packet size.
   */
  void SendPacket (uint32_t size);

  /**
   * Trac eentry to send, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Time to send the frame
    uint32_t packetSize;  //!< Size of the frame
    char frameType;       //!< Frame type (I, P or B)
  };

  std::vector<struct TraceEntry>  m_entries;          //!< Entries in the trace to send
  static struct TraceEntry        g_defaultEntries[]; //!< Default trace to send
  uint32_t                        m_currentEntry;     //!< Current entry index
  uint32_t                        m_sent;             //!< Counter for sent packets
  EventId                         m_sendEvent;        //!< Event id of pending 'send packet' event
  uint16_t                        m_maxPacketSize;    //!< Maximum packet size to send
  Ptr<Socket>                     m_socket;           //!< Local socket
  Ipv4Address                     m_clientAddress;    //!< Client address
  uint16_t                        m_clientPort;       //!< Client UDP port
  Ptr<RealTimeVideoClient>        m_clientApp;        //!< Client application
  Ptr<RandomVariableStream>       m_lengthRng;        //!< Random video length generator
  Time                            m_lengthTime;       //!< Current video length
  Time                            m_elapsed;          //!< Elapsed video length
};

} // namespace ns3

#endif /* REAL_TIME_VIDEO_SERVER_H */
