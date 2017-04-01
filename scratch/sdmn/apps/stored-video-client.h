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

#ifndef STORED_VIDEO_CLIENT_H
#define STORED_VIDEO_CLIENT_H

#include "sdmn-client-app.h"

namespace ns3 {

/**
 * \ingroup sdmnApps
 * This is the client side of a stored video traffic generator. The client
 * establishes a TCP connection with the server and sends a HTTP request for
 * the main video object. After receiving all video chunks, the client closes
 * the connection.
 */
class StoredVideoClient : public SdmnClientApp
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  StoredVideoClient ();           //!< Default constructor.
  virtual ~StoredVideoClient ();  //!< Dummy destructor, see DoDispose.

  // Inherited from SdmnClientApp.
  void Start ();

protected:
  // Inherited from Object.
  virtual void DoDispose (void);

  // Inherited from SdmnClientApp.
  void ForceStop ();
  void NotifyStop (bool withError);

private:
  /**
   * Callback for a connection successfully established.
   * \param socket The connected socket.
   */
  void NotifyConnectionSucceeded (Ptr<Socket> socket);

  /**
   * Callback for a connection failed.
   * \param socket The connected socket.
   */
  void NotifyConnectionFailed (Ptr<Socket> socket);

  /**
   * Callback for in-order bytes available in receive buffer.
   * \param socket The connected socket.
   */
  void DataReceived (Ptr<Socket> socket);

  /**
   * \brief Send the request to server.
   * \param socket The connected socket.
   * \param url The URL of the object requested.
   */
  void SendRequest (Ptr<Socket> socket, std::string url);

  EventId                 m_errorEvent;         //!< Error timeout.
  Ptr<Packet>             m_rxPacket;           //!< RX packet.
  uint32_t                m_pendingBytes;       //!< Pending bytes.
  uint32_t                m_pendingObjects;     //!< Pending video chunks.
};

} // Namespace ns3
#endif /* STORED_VIDEO_CLIENT_H */
