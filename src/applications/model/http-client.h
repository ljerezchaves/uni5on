/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
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
 */

#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/http-header.h"
#include "http-server.h"
#include "ns3/data-rate.h"
#include "ns3/core-module.h"
#include "qos-stats-calculator.h"

using namespace std;
namespace ns3 {

class HttpServer;

/**
 * \ingroup applications
 *
 * This is the client side of a HTTP Traffic Generator. The client
 * establishes a TCP connection with the server and sends a request
 * for the main object of a given web page. When client gets the main
 * object, it process the message and start to request the inline objects
 * of the given web page. After receiving all inline objects, the client
 * waits an interval (reading time) before it requests a new main object
 * of a new web page. The implementation of this application is simplistic
 * and it does not support pipelining in this current version. The
 * model used is based on the distributions indicated in the paper
 * "An HTTP Web Traffic Model Based on the Top One Million Visited
 * Web Pages" by Rastin Pries et. al. This simplistic approach was
 * taken since this traffic generator was developed primarily to help
 * users evaluate their proposed algorithm in other modules of NS-3.
 * To allow deeper studies about the HTTP Protocol it needs some improvements.
 */
class HttpClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return type ID
   */
  static TypeId GetTypeId (void);

  HttpClient ();          //!< Default constructor
  virtual ~HttpClient (); //!< Dummy destructor, see DoDipose

  /**
   * \brief Set the HttpServer application. 
   * \param server The pointer to server application. 
   */
  void SetServerApp (Ptr<HttpServer> server);
  
  /**
   * \brief Get the HttpServer application. 
   * \return The pointer to server application. 
   */
  Ptr<HttpServer> GetServerApp ();

  /** 
   * Reset the QoS statistics
   */
  void ResetQosStats ();

  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator 
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

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
   * \param socket socket that sends requests.
   * \param url URL of the object requested.
   */
  void SendRequest (Ptr<Socket> socket, string url);

  /**
   * \brief Receive method.
   * \param socket socket that receives packets from server.
   */
  void HandleReceive (Ptr<Socket> socket);

  /**
   * \brief Set a reading time before requesting a new main object.
   * \param socket socket that sends requests.
   */
  void SetReadingTime (Ptr<Socket> socket);

  Ptr<Socket>     m_socket;             //!< Local socket.
  Ipv4Address     m_peerAddress;        //!< Address of the server.
  uint16_t        m_peerPort;           //!< Remote port in the server.
  HttpHeader      m_httpHeader;         //!< HTTP Header.
  uint32_t        m_contentLength;      //!< Content-Length header line.
  string          m_contentType;        //!< Content-Type header line.
  uint32_t        m_numOfInlineObjects; //!< Number-of-Inline-Objects header line.
  uint32_t        m_bytesReceived;      //!< Number of bytes received from server.
  uint32_t        m_inlineObjLoaded;    //!< Number of inline objects already loaded.
  Time            m_tcpTimeout;         //!< TCP connection timeout.
  Ipv4Address     m_clientAddress;      //!< Client Address.
  Ptr<HttpServer> m_serverApp;          //!< HttpServer application
  Ptr<QosStatsCalculator> m_qosStats;   //!< QoS statistics
  Ptr<LogNormalRandomVariable> m_readingTimeStream; //!< Random Variable Stream for reading time.
  Ptr<UniformRandomVariable> m_readingTimeAdjust; //!< Reading time adjustment for lower values.
  Ptr<RandomVariableStream> m_delayTime; //!< Delay time for first request and blocked attempts.
};

}

#endif /* HTTP_CLIENT_H_ */
