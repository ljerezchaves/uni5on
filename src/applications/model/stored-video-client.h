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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "stored-video-server.h"
#include "epc-application.h"
#include "qos-stats-calculator.h"

namespace ns3 {

class StoredVideoServer;

/**
 * \ingroup applications
 * This is the client side of a stored video traffic generator. The client
 * establishes a TCP connection with the server and sends a HTTP request for
 * the main video object. After receiving the entire video, the client closes
 * the connection.
 */
class StoredVideoClient : public EpcApplication
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StoredVideoClient ();   //!< Default constructor
  ~StoredVideoClient ();  //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the server application.
   * \param server The pointer to server application.
   * \param serverAddress The IPv4 address of the server.
   * \param serverPort The port number on the server
   */
  void SetServer (Ptr<StoredVideoServer> server, Ipv4Address serverAddress,
                  uint16_t serverPort);

  /**
   * \brief Get the server application.
   * \return The pointer to server application.
   */
  Ptr<StoredVideoServer> GetServerApp ();

  /**
   * Set the server trace file attribute.
   * \param filename Name of file to load a trace from.
   */
  void SetTraceFilename (std::string filename);

  // Inherited from EpcApplication
  void Start (void);

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  /**
   * \brief Open the TCP connection between this client and the server.
   */
  void OpenSocket ();

  /**
   * \brief Close the TCP connection between this client and the server.
   */
  void CloseSocket ();

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
   * \brief Send the request to server side.
   * \param socket Socket that sends requests.
   * \param url The URL of the object requested.
   */
  void SendRequest (Ptr<Socket> socket, std::string url);

  /**
   * \brief Receive method.
   * \param socket The socket that receives packets from server.
   */
  void HandleReceive (Ptr<Socket> socket);

  Ptr<Socket>             m_socket;         //!< Local socket.
  Ipv4Address             m_serverAddress;  //!< Server address.
  uint16_t                m_serverPort;     //!< Server port.
  Ptr<StoredVideoServer>  m_serverApp;      //!< Server application.
  EventId                 m_forceStop;      //!< Maximum duration stop event.
};

} // namespace ns3
#endif /* STORED_VIDEO_CLIENT_H */
