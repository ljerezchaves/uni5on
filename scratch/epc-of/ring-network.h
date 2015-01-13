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

#ifndef RING_NETWORK_H
#define RING_NETWORK_H

#include <ns3/core-module.h>
#include "openflow-epc-network.h"
#include "ring-controller.h"

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/** 
 * This class generates and simple n-switch OpenFlow ring topology
 * controlled by RingController, which will be used by S1-U and
 * X2 EPC interfaces.
 *
 */
class RingOpenFlowNetwork : public OpenFlowEpcNetwork
{
public:
  RingOpenFlowNetwork ();
  ~RingOpenFlowNetwork ();

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  void DoDispose ();

  // Inherited from OpenFlowEpcNetowork
  Ptr<NetDevice> AttachToS1u (Ptr<Node> node, uint16_t cellId);
  Ptr<NetDevice> AttachToX2  (Ptr<Node> node);
  void CreateInternalTopology ();
  
private:
  Ptr<RingController>   m_ringCtrlApp;  //!< Casted controller app pointer
  uint16_t              m_nodes;        //!< Number of switches in the ring
  
  DataRate              m_LinkDataRate; //!< Link data rate
  Time                  m_LinkDelay;    //!< Link delay
  uint16_t              m_LinkMtu;      //!< Link mtu

  /** Helper to assign addresses to S1-U NetDevices */
  Ipv4AddressHelper m_s1uIpv4AddressHelper;
  
  /** Helper to assign addresses to X2 NetDevices */
  Ipv4AddressHelper m_x2Ipv4AddressHelper;

}; // class RingOpenFlowNetwork

}; // namespace ns3
#endif // RING_NETWORK_H
