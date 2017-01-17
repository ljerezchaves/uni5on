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

#include "sdmn-server-app.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * This is the server side of a real-time video traffic generator, sending UDP
 * datagrams following a MPEG video pattern with random video length.
 */
class RealTimeVideoServer : public SdmnServerApp
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RealTimeVideoServer ();           //!< Default constructor
  virtual ~RealTimeVideoServer ();  //!< Dummy destructor, see DoDispose

  /**
   * \brief Set the trace file to be used by the application
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void SetTraceFile (std::string filename);

protected:
  // Inherited from Object
  virtual void DoDispose (void);

private:
  // Inherited from Application
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // Inherited from SdmnServerApp
  void NotifyStart ();
  void NotifyForceStop ();

  /**
   * \brief Load a trace file.
   * \param filename The trace file path.
   */
  void LoadTrace (std::string filename);

  /**
   * \brief Load the default trace.
   */
  void LoadDefaultTrace (void);

  /**
   * \brief Start sending the video.
   */
  void SendStream (void);

  /**
   * \brief Handle a packet transmission.
   * \param size The packet size.
   */
  void SendPacket (uint32_t size);

  /**
   * Trace entry, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Relative time to send the frame (ms)
    uint32_t packetSize;  //!< Size of the frame
    char frameType;       //!< Frame type (I, P or B)
  };

  uint16_t                        m_pktSize;          //!< Maximum packet size
  EventId                         m_sendEvent;        //!< SendPacket event
  uint32_t                        m_currentEntry;     //!< Current entry index
  static struct TraceEntry        g_defaultEntries[]; //!< Default trace
  std::vector<struct TraceEntry>  m_entries;          //!< Entries in the trace
};

} // namespace ns3
#endif /* REAL_TIME_VIDEO_SERVER_H */
