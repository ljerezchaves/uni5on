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
#include "stored-video-client.h"

namespace ns3 {

class StoredVideoClient;

/**
 * \ingroup applications
 *
 * This class implements a stored video server application, sending stored
 * video stream pattern to client over TCP connection.
 */
class StoredVideoServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StoredVideoServer ();       //!< Default constructor
  ~StoredVideoServer ();      //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the trace file to be used by the application
   * \param filename a path to an MPEG4 trace file formatted as follows:
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  Frame No Frametype   Time[ms]    Length [byte]
   *  ...
   */
  void SetTraceFile (std::string filename);

  /**
   * \brief Set the StoredVideoClient application.
   * \param server The pointer to client application.
   */
  void SetClientApp (Ptr<StoredVideoClient> client);

  /**
   * \brief Get the StoredVideoClient application.
   * \return The pointer to client application.
   */
  Ptr<StoredVideoClient> GetClientApp ();

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Processes the request of client to establish a TCP connection.
   * \param socket socket that receives the TCP request for connection.
   */
  bool HandleRequest (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Handle the acceptance or denial of the TCP connection.
   * \param socket socket for the TCP connection.
   * \param address address of the client
   */
  void HandleAccept (Ptr<Socket> socket, const Address& address);

  /**
   * \brief Receive method.
   * \param socket socket that receives packets from client.
   */
  void HandleReceive (Ptr<Socket> socket);

  /**
   * \brief Handle an connection close
   * \param socket the connected socket
   */
  void HandlePeerClose (Ptr<Socket> socket);
  
  /**
   * \brief Handle an connection error
   * \param socket the connected socket
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
   * \return The size (in bytes) of the video
   */
  uint32_t GetVideoBytes (void);

  /**
   * \brief Start sending the video stream.
   * Callback for socket's SetSendCallback.
   * \param socket The pointer to the socket.
   * \param size The number of bytes available for writing into the buffer
   */
  void SendStream (Ptr<Socket> socket, uint32_t size);

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

  std::vector<struct TraceEntry>  m_entries;          //!< Entries in the trace to send
  static struct TraceEntry        g_defaultEntries[]; //!< Default trace to send
  uint32_t                        m_currentEntry;     //!< Current entry index

  Ptr<Socket>                     m_socket;           //!< Local socket
  uint16_t                        m_port;             //!< Local port
  bool                            m_connected;        //!< True if connected
  Ptr<StoredVideoClient>          m_clientApp;        //!< Client application

  Ptr<RandomVariableStream>       m_lengthRng;        //!< Random video length generator
  Time                            m_lengthTime;       //!< Current video length
  Time                            m_elapsed;          //!< Elapsed video length
};

} // namespace ns3

#endif /* STORED_VIDEO_SERVER_H */
