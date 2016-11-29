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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "sdmn-server-app.h"
#include "stored-video-client.h"

namespace ns3 {

class StoredVideoClient;

/**
 * \ingroup applications
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

  StoredVideoServer ();   //!< Default constructor
  ~StoredVideoServer ();  //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the client application.
   * \param client The pointer to client application.
   */
  void SetClient (Ptr<StoredVideoClient> client);

  /**
   * \brief Get the client application.
   * \return The pointer to client application.
   */
  Ptr<StoredVideoClient> GetClientApp ();

  /**
   * \brief Set the trace file to be used by the application
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void SetTraceFile (std::string filename);

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * Process the Http request message, sending back the response.
   * \param socket The TCP socket.
   * \param header The HTTP request header.
   */
  void ProccessHttpRequest (Ptr<Socket> socket, HttpHeader header);

  /**
   * \brief Processes the request of client to establish a TCP connection.
   * \param socket Socket that receives the TCP request for connection.
   */
  bool HandleRequest (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Handle the acceptance or denial of the TCP connection.
   * \param socket Socket for the TCP connection.
   * \param address Address of the client
   */
  void HandleAccept (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Receive method.
   * \param socket Socket that receives packets from client.
   */
  void HandleReceive (Ptr<Socket> socket);

  /**
   * \brief Handle an connection close
   * \param socket The connected socket
   */
  void HandlePeerClose (Ptr<Socket> socket);

  /**
   * \brief Handle an connection error
   * \param socket The connected socket
   */
  void HandlePeerError (Ptr<Socket> socket);

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
   * Compute the bideo size based on given length.
   * \param length The video length.
   * \return The size in bytes.
   */
  uint32_t GetVideoBytes (Time length);

  /**
   * \brief Start sending the video. Callback for socket SetSendCallback.
   * \param socket The pointer to the socket.
   * \param available The number of bytes available for writing into the buffer
   */
  void SendStream (Ptr<Socket> socket, uint32_t available);

  /**
   * Trac eentry to send, representing a MPEG frame.
   */
  struct TraceEntry
  {
    uint32_t timeToSend;  //!< Time to send the frame
    uint32_t packetSize;  //!< Size of the frame
    char frameType;       //!< Frame type (I, P or B)
  };

  static struct TraceEntry        g_defaultEntries[]; //!< Default trace
  std::vector<struct TraceEntry>  m_entries;          //!< Trace entries
  Ptr<Socket>                     m_socket;           //!< Local socket
  uint16_t                        m_localPort;        //!< Local port
  bool                            m_connected;        //!< True if connected
  Ptr<StoredVideoClient>          m_clientApp;        //!< Client application
  Ptr<RandomVariableStream>       m_lengthRng;        //!< Length generator
  uint32_t                        m_pendingBytes;     //!< Pending TX bytes
  Ptr<Packet>                     m_rxPacket;         //!< RX packet.
};

} // namespace ns3

#endif /* STORED_VIDEO_SERVER_H */
