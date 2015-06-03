/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#ifndef TRAFFIC_HELPER_H
#define TRAFFIC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/applications-module.h>
#include <ns3/internet-module.h>
#include "traffic-manager.h"

namespace ns3 {

/** 
 * \ingroup epcof
 * Traffic helper which installs client and server applications for all
 * applications into UEs and WebServer. This helper creates and aggregates a
 * traffic manager for each UE.
 */
class TrafficHelper
{
public:
  /** 
   * Complete constructor.
   * \param server The server node.
   * \param helper The helper pointer.
   * \param controller The Epc controller.
   * \param network The Epc network.
   */
  TrafficHelper (Ptr<Node> server, Ptr<LteHelper> helper, 
                 Ptr<OpenFlowEpcController> controller,
                 Ptr<OpenFlowEpcNetwork> network);
  
  ~TrafficHelper (); //!< Default destructor.

  /**
   * Record an attribute to be set in each traffic manager.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetTfcManagerAttribute (std::string name, const AttributeValue &value);

  /**
   * Install applications and traffic manager into each UE. It creates the
   * client/server application pair, and install them in the respective
   * nodes. It also configure the TFT and EPS bearers.
   * \param ueNodes The UE Nodes container.
   * \param ueDevices The UE NetDevices container.
   */
  void Install (NodeContainer ueNodes, NetDeviceContainer ueDevices);

private:
  /** 
   * VoIP/UDP bidirectional traffic over dedicated GBR EPS bearer (QCI 1). 
   * This QCI is typically associated with conversational voice.
   *
   * \internal This VoIP traffic simulates the G.729 codec (~8.5 kbps for
   * payload). Check http://goo.gl/iChPGQ for bandwidth calculation and
   * discussion. 
   */
  void InstallVoip ();

  /**
   * UDP real-time video streaming over dedicated GBR EPS bearer (QCI 4).
   * This QCI is typically associated with non-conversational buffered video.
   *
   * \internal This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf. 
   */
  void InstallRealTimeVideo ();
  
  /**
   * TCP stored video streaming over dedicated Non-GBR EPS bearer (QCI 6).
   * This QCI 8 could be used for priorization of non real-time data of MPS
   * subscribers.
   *
   * \internal This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf. The video
   * is stored in server and is downloaded by the client.
   */
  void InstallStoredVideo ();

  /** 
   * HTTP/TCP traffic over dedicated Non-GBR EPS bearer (QCI 8).
   * This QCI 8 could be used for a dedicated 'premium bearer' for any
   * subscriber, or could be used for the default bearer of a for 'premium
   * subscribers'.
   *
   * \internal This HTTP model is based on the distributions indicated in the
   * paper 'An HTTP Web Traffic Model Based on the Top One Million Visited Web
   * Pages' by Rastin Pries et. al. Each client will send a get request to the
   * server and will get the page content back including inline content. These
   * requests repeats after a reading time period, until MaxPages are loaded. 
   */
  void InstallHttp ();

  /**
   * Get complete filename for stored video trace files.
   * \param idx The trace index.
   * \return Complete path.
   */
  static const std::string GetVideoFilename (uint8_t idx);
  
  ObjectFactory       m_managerFactory; //!< Traffic manager factory

  Ptr<LteHelper>      m_lteHelper;      //!< LteHelper pointer
  Ptr<Node>           m_webNode;        //!< Server node
  Ipv4Address         m_webAddr;        //!< Server address
  Ipv4Mask            m_webMask;        //!< Server address mask

  Ptr<Node>           m_ueNode;         //!< Current client node
  Ptr<NetDevice>      m_ueDev;          //!< Current client dev
  Ipv4Address         m_ueAddr;         //!< Current client address
  Ipv4Mask            m_ueMask;         //!< Current client address mask
  Ptr<TrafficManager> m_ueManager;      //!< Current client traffic manager

  HttpHelper          m_httpHelper;     //!< HTTP application helper
  StoredVideoHelper   m_stVideoHelper;  //!< Stored video application helper
  RealTimeVideoHelper m_rtVideoHelper;  //!< Real-time video application helper
  VoipHelper          m_voipHelper;     //!< Voip application helper

  Ptr<UniformRandomVariable> m_stVideoRng;    //!< Random stored video 
  static const std::string   m_videoDir;      //!< Video trace directory
  static const std::string   m_videoTrace []; //!< Stored video trace filenames
  static const uint64_t      m_avgBitRate []; //!< Stored video trace avg bitrate
  static const uint64_t      m_maxBitRate []; //!< Stored video trace max bitrate
};

};  // namespace ns3
#endif // TRAFFIC_HELPER_H

