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

#ifndef INTERNET_NETWORK_H
#define INTERNET_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/csma-module.h>

namespace ns3 {

class LinkQueuesStatsCalculator;

/**
 * \ingroup epcof
 * Create an Internet network, connecting a Web server
 * to the LTE EPC Packet Gateway.
 */
class InternetNetwork : public Object
{
public:
  InternetNetwork ();           //!< Default constructor
  virtual ~InternetNetwork ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Creates the internet infrastructure.
   * \param pgw The gateway EPC node which will connect to the Internet.
   * \return Ptr<Node> for the WebHost.
   */
  Ptr<Node> CreateTopology (Ptr<Node> pgw);

  /**
   * Get the pointer to the Internet server node created by the topology.
   * \return The pointer to the server node.
   */
  Ptr<Node> GetServerNode ();

  /**
   * Enable pcap on Internet links.
   * \param prefix Filename prefix to use for pcap files.
   */
  void EnablePcap (std::string prefix);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

private:
  NodeContainer      m_webNodes;      //!< Internet nodes (server and gateway)
  NetDeviceContainer m_webDevices;    //!< Internet devices
  CsmaHelper         m_csmaHelper;    //!< Internet csma link helper
  DataRate           m_linkDataRate;  //!< Internet link data rate
  Time               m_linkDelay;     //!< Internet link delay
  uint16_t           m_linkMtu;       //!< Internet link MTU

  Ptr<LinkQueuesStatsCalculator>  m_internetStats;  //!< Web queues statistics
};

};  // namespace ns3
#endif  // INTERNET_NETWORK_H

