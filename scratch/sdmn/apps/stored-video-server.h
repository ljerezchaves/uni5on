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

#ifndef STORED_VIDEO_SERVER_H
#define STORED_VIDEO_SERVER_H

#include "sdmn-server-app.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * This is the server side of a stored video traffic generator. The server
 * listen for client video requests, and send data as fast as possible up to
 * random video length duration over TCP connection.
 */
class StoredVideoServer : public SdmnServerApp
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StoredVideoServer ();           //!< Default constructor
  virtual ~StoredVideoServer ();  //!< Dummy destructor, see DoDispose

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

  /**
   * \brief Processes the request of client to establish a TCP connection.
   * \param socket Socket that receives the TCP request for connection.
   */
  bool HandleRequest (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Handle the acceptance or denial of the TCP connection.
   * \param socket Socket for the TCP connection.
   * \param address Address of the client.
   */
  void HandleAccept (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Handle an connection close.
   * \param socket The connected socket.
   */
  void HandlePeerClose (Ptr<Socket> socket);

  /**
   * \brief Handle an connection error.
   * \param socket The connected socket.
   */
  void HandlePeerError (Ptr<Socket> socket);

  /**
   * \brief Socket receive callback.
   * \param socket Socket with data available to be read.
   */
  void ReceiveData (Ptr<Socket> socket);

  /**
   * \brief Socket send callback.
   * \param socket The pointer to the socket with space in the transmit buffer.
   * \param available The number of bytes available for writing into buffer.
   */
  void SendData (Ptr<Socket> socket, uint32_t available);

  /**
   * Process the Http request message, sending back the response.
   * \param socket The connected socket.
   * \param header The HTTP request header.
   */
  void ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header);

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
   * Compute the bideo size based on given length.
   * \param length The video length.
   * \return The size in bytes.
   */
  uint32_t GetVideoBytes (Time length);

  /**
   * Trace entry, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Relative time to send the frame (ms)
    uint32_t packetSize;  //!< Size of the frame
    char frameType;       //!< Frame type (I, P or B)
  };

  bool                            m_connected;        //!< True if connected
  uint32_t                        m_pendingBytes;     //!< Pending TX bytes
  Ptr<RandomVariableStream>       m_lengthRng;        //!< Length generator
  static struct TraceEntry        g_defaultEntries[]; //!< Default trace
  std::vector<struct TraceEntry>  m_entries;          //!< Trace entries
};

} // Namespace ns3
#endif /* STORED_VIDEO_SERVER_H */
