/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2015 University of Campinas (Unicamp)
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
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/http-header.h"
#include "sdmn-server-app.h"
#include "http-client.h"
#include <string>
#include <sstream>

namespace ns3 {

class HttpClient;

/**
 * \ingroup applications
 * This is the server side of a HTTP Traffic Generator. The server listen for
 * client object requests. The implementation of this application is simplistic
 * and it does not support pipelining in this current version. The model used
 * is based on the distributions indicated in the paper "An HTTP Web Traffic
 * Model Based on the Top One Million Visited Web Pages" by Rastin Pries et.
 * al. This simplistic approach was taken since this traffic generator was
 * developed primarily to help users evaluate their proposed algorithm in other
 * modules of ns-3. To allow deeper studies about the HTTP Protocol it needs
 * some improvements.
 */
class HttpServer : public SdmnServerApp
{
public:
  /**
   * \brief Get the type ID.
   * \return type ID
   */
  static TypeId GetTypeId (void);

  HttpServer ();          //!< Default constructor
  virtual ~HttpServer (); //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the client application.
   * \param client The pointer to client application.
   */
  void SetClient (Ptr<HttpClient> client);

  /**
   * \brief Get the client application.
   * \return The pointer to client application.
   */
  Ptr<HttpClient> GetClientApp ();

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
   * \brief Start sending the object. Callback for socket SetSendCallback.
   * \param socket The pointer to the socket.
   * \param available The number of bytes available for writing into the buffer
   */
  void SendObject (Ptr<Socket> socket, uint32_t available);

  Ptr<Socket>     m_socket;             //!< Local socket.
  uint16_t        m_localPort;          //!< Local port.
  bool            m_connected;          //!< True if connected
  Ptr<HttpClient> m_clientApp;          //!< Client application.
  uint32_t        m_pendingBytes;       //!< Pending TX bytes

  /** Random variable for main object size. */
  Ptr<WeibullRandomVariable>      m_mainObjectSizeStream;

  /** Random variable for number of inline objects. */
  Ptr<ExponentialRandomVariable>  m_numOfInlineObjStream;

  /** Random variable for inline object size. */
  Ptr<LogNormalRandomVariable>    m_inlineObjectSizeStream;
};

}
#endif /* HTTP_SERVER_H_ */
