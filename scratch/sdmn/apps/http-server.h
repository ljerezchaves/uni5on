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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include "sdmn-server-app.h"

namespace ns3 {

/**
 * \ingroup sdmn
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
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  HttpServer ();          //!< Default constructor
  virtual ~HttpServer (); //!< Dummy destructor, see DoDispose

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

  bool                           m_connected;              //!< Connected state
  uint32_t                       m_pendingBytes;           //!< Pending bytes
  Ptr<WeibullRandomVariable>     m_mainObjectSizeStream;   //!< Main obj size
  Ptr<ExponentialRandomVariable> m_numOfInlineObjStream;   //!< Num inline obj
  Ptr<LogNormalRandomVariable>   m_inlineObjectSizeStream; //!< Inline obj size
};

} // Namespace ns3
#endif /* HTTP_SERVER_H_ */
