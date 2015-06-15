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
 * \ingroup epcof
 * This class generates and simple n-switch OpenFlow ring topology
 * controlled by RingController, which will be used by S1-U and
 * X2 EPC interfaces.
 */
class RingNetwork : public OpenFlowEpcNetwork
{
public:
  RingNetwork ();   //!< Default constructor
  ~RingNetwork ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  void DoDispose ();

  // Inherited from OpenFlowEpcNetwork
  Ptr<NetDevice> AttachToS1u (Ptr<Node> node, uint16_t cellId);
  Ptr<NetDevice> AttachToX2  (Ptr<Node> node);
  void CreateTopology (Ptr<OpenFlowEpcController> controller,
                       std::vector<uint16_t> eNbSwitches);

private:
  DataRate              m_switchLinkDataRate; //!< Switch link data rate
  Time                  m_switchLinkDelay;    //!< Switch link delay
  DataRate              m_epcLinkDataRate;    //!< EPC link data rate
  Time                  m_epcLinkDelay;       //!< EPC link delay
  uint16_t              m_linkMtu;            //!< Link mtu
  uint16_t              m_nodes;              //!< Number of switches in the ring

  /** Helper to assign addresses to S1-U NetDevices */
  Ipv4AddressHelper m_s1uIpv4AddressHelper;

  /** Helper to assign addresses to X2 NetDevices */
  Ipv4AddressHelper m_x2Ipv4AddressHelper;

}; // class RingNetwork

}; // namespace ns3
#endif // RING_NETWORK_H
