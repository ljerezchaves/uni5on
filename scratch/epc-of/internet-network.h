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
#include <ns3/point-to-point-module.h>

namespace ns3 {

/** Create an Internet network. */
class InternetNetwork : public Object
{
public:
  InternetNetwork ();
  virtual ~InternetNetwork ();

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   * Creates the internet infrastructure.
   * \param pgw The gateway EPC node which will connect to the Internet.
   * \return Ptr<Node> for the WebHost.
   */
  Ptr<Node> CreateTopology (Ptr<Node> pgw);

  /** 
   * Enable pcap on Internet links. 
   */
  void EnablePcap (std::string prefix);

private:
  NodeContainer m_webNodes;
  NetDeviceContainer m_webDevices;

  PointToPointHelper m_p2pHeler;

  DataRate m_LinkDataRate;
  Time     m_LinkDelay;
  uint16_t m_LinkMtu;
};

};  // namespace ns3
#endif  // INTERNET_NETWORK_H

