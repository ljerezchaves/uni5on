/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#ifndef RING_CONTROLLER_H
#define RING_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "epc-sdn-controller.h"
//#include "ring-openflow-network.h"

namespace ns3 {

/**
 * OpenFlow EPC controller for ring network.
 */
class RingController : public EpcSdnController
{
public:
  RingController ();          //!< Default constructor
  virtual ~RingController (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Notify this ring controller of a new connection between two switches in
   * the ring. 
   * \param conInfo The connection information and metadata.
   */ 
  void NotifyNewSwitchConnection (ConnectionInfo connInfo);

  /**
   * Populate the internal NetDeviceContainer with switch devices at  ring
   * network.
   * \param devs The NetDeviceContainer with switch devices.
   */
  void SetSwitchDevices (NetDeviceContainer devs);

  /**
   * Let's configure one single link to drop packets when flooding over ports
   * (OFPP_FLOOD).  Here we are disabling the farthest gateway link,
   * configuring its ports to OFPPC_NO_FWD flag (0x20).
   */
  void CreateSpanningTree ();

private:
  static uint16_t     m_flowPrio;     //!< Flow-mod priority
  DataRate            m_LinkDataRate; //!< Link data rate
};

};  // namespace ns3
#endif // RING_CONTROLLER_H

