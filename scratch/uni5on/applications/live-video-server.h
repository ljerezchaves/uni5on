/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef LIVE_VIDEO_SERVER_H
#define LIVE_VIDEO_SERVER_H

#include "base-server.h"

namespace ns3 {

/**
 * \ingroup uni5onApps
 * The server side of a live video traffic generator, sending and receiving UDP
 * datagrams following a MPEG video pattern with random video length.
 */
class LiveVideoServer : public BaseServer
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  LiveVideoServer ();           //!< Default constructor.
  virtual ~LiveVideoServer ();  //!< Dummy destructor, see DoDispose.

protected:
  // Inherited from Object.
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // Inherited from BaseServer.
  void NotifyStart ();
  void NotifyForceStop ();

  /**
   * \brief Load the trace file to be used by the application.
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void LoadTrace (std::string filename);

  /**
   * \brief Socket receive callback.
   * \param socket Socket with data available to be read.
   */
  void ReadPacket (Ptr<Socket> socket);

  /**
   * \brief Handle a packet transmission.
   * \param size The packet size.
   */
  void SendPacket (uint32_t size);

  /**
   * \brief Start sending the video.
   */
  void SendStream (void);

  /**
   * Trace entry, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Relative time to send the frame (ms)
    uint32_t packetSize;  //!< Size of the frame
    char     frameType;   //!< Frame type (I, P or B)
  };

  uint16_t                        m_pktSize;          //!< Packet size.
  uint32_t                        m_currentEntry;     //!< Current entry.
  std::vector<struct TraceEntry>  m_entries;          //!< Trace entries.
  EventId                         m_sendEvent;        //!< SendPacket event.
};

} // namespace ns3
#endif /* LIVE_VIDEO_SERVER_H */
