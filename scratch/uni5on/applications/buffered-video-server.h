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

#ifndef BUFFERED_VIDEO_SERVER_H
#define BUFFERED_VIDEO_SERVER_H

#include "uni5on-server.h"

namespace ns3 {

/**
 * \ingroup uni5onApps
 * This is the server side of a buffered video traffic generator. The server
 * listen for a client requesting video chunks and send the chunks back as fast
 * as possible.
 */
class BufferedVideoServer : public Uni5onServer
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  BufferedVideoServer ();           //!< Default constructor.
  virtual ~BufferedVideoServer ();  //!< Dummy destructor, see DoDispose.

protected:
  // Inherited from Object.
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * Callback for connection request from peer.
   * \param socket The socket that receives the connection request.
   */
  bool NotifyConnectionRequest (Ptr<Socket> socket, const Address& address);

  /**
   * Callback for a new connection created.
   * \param socket The new (forked) socket for this connection.
   * \param address The peer address.
   */
  void NotifyNewConnectionCreated (Ptr<Socket> socket, const Address& address);

  /**
   * Callback for a connection gracefully closed.
   * \param socket The connected socket.
   */
  void NotifyNormalClose (Ptr<Socket> socket);

  /**
   * Callback for a connection abnormally closed.
   * \param socket The connected socket.
   */
  void NotifyErrorClose (Ptr<Socket> socket);

  /**
   * Callback for in-order bytes available in receive buffer.
   * \param socket The connected socket.
   */
  void DataReceived (Ptr<Socket> socket);

  /**
   * Callback for bytes available in transmission buffer.
   * \param socket The connected socket.
   * \param available The number of bytes available into tx buffer.
   */
  void SendData (Ptr<Socket> socket, uint32_t available);

  /**
   * Process the HTTP request message.
   * \param socket The connected socket.
   * \param header The HTTP request header.
   */
  void ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header);

  /**
   * \brief Load the trace file to be used by the application.
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void LoadTrace (std::string filename);

  /**
   * Get the number of chunks based on the given video length and chunk size.
   * \param length The video length.
   * \return The number of chunks for this video.
   */
  uint32_t GetVideoChunks (Time length);

  /**
   * Trace entry, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Relative time to send the frame (ms).
    uint32_t packetSize;  //!< Size of the frame.
    char     frameType;   //!< Frame type (I, P or B).
  };

  bool                            m_connected;        //!< Connected state.
  uint32_t                        m_pendingBytes;     //!< Pending bytes.
  uint32_t                        m_chunkSize;        //!< Chunk size.
  std::vector<struct TraceEntry>  m_entries;          //!< Trace entries.
};

} // Namespace ns3
#endif /* BUFFERED_VIDEO_SERVER_H */
