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

#include <ns3/csma-module.h>
#include "epc-network.h"

namespace ns3 {

/**
 * \ingroup epcof
 * This class generates and simple n-switch OpenFlow ring topology
 * controlled by RingController, which will be used by S1-U and
 * X2 EPC interfaces.
 */
class RingNetwork : public EpcNetwork
{
public:
  RingNetwork ();   //!< Default constructor
  ~RingNetwork ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  // Inherited from EpcNetwork
  Ptr<NetDevice> S1Attach (Ptr<Node> node, uint16_t cellId);
  NetDeviceContainer X2Attach (Ptr<Node> enb1, Ptr<Node> enb2);
  void EnablePcap (std::string prefix, bool promiscuous = false);

protected:
  /** Destructor implementation */
  void DoDispose ();

  // Inherited from ObjectBase
  void NotifyConstructionCompleted (void);

  // Inherited from EpcNetwork
  void CreateTopology ();

private:
  DataRate          m_swLinkRate;     //!< Switch link data rate
  Time              m_swLinkDelay;    //!< Switch link delay
  DataRate          m_gwLinkRate;     //!< Gateway link data rate
  Time              m_gwLinkDelay;    //!< Gateway link delay
  uint16_t          m_linkMtu;        //!< Link mtu
  uint16_t          m_numNodes;       //!< Number of switches
  Ipv4AddressHelper m_s1uAddrHelper;  //!< S1 address helper
  Ipv4AddressHelper m_x2AddrHelper;   //!< X2 address helper
  CsmaHelper        m_swHelper;       //!< Csma switch connection helper
  CsmaHelper        m_gwHelper;       //!< Csma gateway connection helper

}; // class RingNetwork

}; // namespace ns3
#endif // RING_NETWORK_H
