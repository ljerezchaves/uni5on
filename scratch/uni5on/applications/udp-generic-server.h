/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#ifndef UDP_GENERIC_SERVER_H
#define UDP_GENERIC_SERVER_H

#include "uni5on-server.h"

namespace ns3 {

/**
 * \ingroup uni5onApps
 * This is the server side of a generic UDP traffic generator, sending and
 * receiving UDP datagrams following the configured traffic pattern.
 */
class UdpGenericServer : public Uni5onServer
{
public:
  /**
   * \brief Register this type.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  UdpGenericServer ();             //!< Default constructor.
  virtual ~UdpGenericServer ();    //!< Dummy destructor, see DoDispose.

protected:
  // Inherited from Object.
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // Inherited from Uni5onServer.
  void NotifyStart ();
  void NotifyForceStop ();

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

  Ptr<RandomVariableStream>   m_pktInterRng;  //!< Pkt inter-arrival time.
  Ptr<RandomVariableStream>   m_pktSizeRng;   //!< Pkt size.
  EventId                     m_sendEvent;    //!< SendPacket event.
};

} // namespace ns3
#endif /* UDP_GENERIC_SERVER_H */
