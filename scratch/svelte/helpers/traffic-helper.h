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
#include <ns3/internet-module.h>
#include "../applications/svelte-app-helper.h"

namespace ns3 {

class TrafficManager;

/**
 * \ingroup svelte
 * Traffic helper which installs client and server applications for all
 * applications into UEs and WebServer. This helper creates and aggregates a
 * traffic manager for each UE.
 */
class TrafficHelper : public Object
{
public:
  /**
   * Complete constructor.
   * \param webNode The Internet web server node.
   * \param lteHelper The LTE helper.
   * \param ueNodes The UE nodes.
   * \param ueDevices The UE network devices.
   */
  TrafficHelper (Ptr<Node> webNode, Ptr<LteHelper> lteHelper,
                 NodeContainer ueNodes, NetDeviceContainer ueDevices);

  TrafficHelper ();           //!< Default constructor.
  virtual ~TrafficHelper ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  void NotifyConstructionCompleted (void);

private:
  /**
   * Install applications and traffic manager into each UE. It creates
   * the client/server application pair, and install them in the respective
   * nodes. It also configure the TFT and EPS bearers.
   * \attention The QCIs used here for each application are strongly related to
   *     the DSCP mapping, which will reflect on the priority queues used by
   *     both OpenFlow switches and traffic control module. Be careful if you
   *     intend to change it.
   * \internal Some notes about internal GbrQosInformation usage:
   * \li The Maximum Bit Rate field is used by controller to install meter
   *     rules for this traffic. When this value is left to 0, no meter rules
   *     will be installed.
   * \li The Guaranteed Bit Rate field is used by the controller to reserve the
   *     requested bandwidth in OpenFlow EPC network (only for GBR beares).
   */
  void InstallApplications ();

  /**
   * Get the next port number available for use.
   * \return The port number to use.
   */
  static uint16_t GetNextPortNo ();

  /**
   * Get complete filename for video trace files.
   * \param idx The video index (see movies folder).
   * \return The complete file path.
   */
  static const std::string GetVideoFilename (uint8_t idx);

  /**
   * Get the GBR data rate for video trace files.
   * \param idx The video index (see movies folder).
   * \return The gbr video data rate.
   */
  static const DataRate GetVideoGbr (uint8_t idx);

  /**
   * Get the MBR data rate for video trace files.
   * \param idx The video index (see movies folder).
   * \return The mbr video data rate.
   */
  static const DataRate GetVideoMbr (uint8_t idx);

  /**
   * UDP uplink auto-pilot traffic.
   * This auto-pilot model is adapted from the MTC application model indicate
   * on the "Machine-to-Machine Communications: Architectures, Technology,
   * Standards, and Applications" book, chapter 3: "M2M traffic and models".
   * \param bearer The configured EPS bearer for this application.
   */
  void InstallAutoPilot (EpsBearer bearer);

  /**
   * TCP bidirectional buffered video streaming.
   * This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
   * \param bearer The configured EPS bearer for this application.
   * \param name The video filename for this application.
   */
  void InstallBufferedVideo (EpsBearer bearer, std::string name);

  /**
   * TCP bidirectional HTTP traffic.
   * This HTTP model is based on the distributions indicated in
   * the paper 'An HTTP Web Traffic Model Based on the Top One Million Visited
   * Web Pages' by Rastin Pries et. al. Each client will send a get request to
   * the server and will get the page content back including inline content.
   * These requests repeats after a reading time period, until MaxPages are
   * loaded or MaxReadingTime is reached.
   * \param bearer The configured EPS bearer for this application.
   */
  void InstallHttp (EpsBearer bearer);

  /**
   * UDP downlink live video streaming.
   * This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
   * \param bearer The configured EPS bearer for this application.
   * \param name The video filename for this application.
   */
  void InstallLiveVideo (EpsBearer bearer, std::string name);

  /**
   * UDP bidirectional VoIP traffic.
   * This VoIP traffic simulates the G.729 codec (~8.0 kbps for payload).
   * Check http://goo.gl/iChPGQ for bandwidth calculation and discussion.
   * \param bearer The configured EPS bearer for this application.
   */
  void InstallVoip (EpsBearer bearer);

  ObjectFactory              m_factory;     //!< Traffic manager factory.
  Ptr<TrafficManager>        m_manager;     //!< Traffic manager instance.
  Ptr<RandomVariableStream>  m_poissonRng;  //!< Inter-arrival traffic.
  bool                       m_restartApps; //!< Restart apps.

  Ptr<LteHelper>      m_lteHelper;          //!< The LTE helper.

  Ptr<Node>           m_webNode;            //!< Server node.
  Ipv4Address         m_webAddr;            //!< Server address.
  Ipv4Mask            m_webMask;            //!< Server address mask.

  Ptr<Node>           m_ueNode;             //!< Client node.
  Ptr<NetDevice>      m_ueDev;              //!< Client dev.
  Ipv4Address         m_ueAddr;             //!< Client address.
  Ipv4Mask            m_ueMask;             //!< Client address mask.

  NodeContainer       m_ueNodes;
  NetDeviceContainer  m_ueDevices;

  bool                m_gbrAutoPilot;       //!< GBR auto-pilot enable.
  bool                m_gbrLiveVideo;       //!< GBR live video enable.
  bool                m_gbrVoip;            //!< GBR VoIP enable.
  bool                m_nonGbrAutoPilot;    //!< Non-GBR auto-pilot enable.
  bool                m_nonGbrBuffVideo;    //!< Non-GBR buffered video enable.
  bool                m_nonGbrHttp;         //!< Non-GBR HTTP enable.
  bool                m_nonGbrLiveVideo;    //!< Non-GBR live video enable.

  SvelteAppHelper     m_autoPilotHelper;    //!< Auto-pilot app helper.
  SvelteAppHelper     m_buffVideoHelper;    //!< Buffered video app helper.
  SvelteAppHelper     m_httpHelper;         //!< HTTP app helper.
  SvelteAppHelper     m_liveVideoHelper;    //!< Live video app helper.
  SvelteAppHelper     m_voipHelper;         //!< Voip app helper.

  static uint16_t             m_port;           //!< Port numbers for apps.

  Ptr<UniformRandomVariable>  m_videoRng;       //!< Random video selection.
  static const std::string    m_videoDir;       //!< Video trace directory.
  static const std::string    m_videoTrace [];  //!< Video trace filenames.
  static const uint64_t       m_gbrBitRate [];  //!< Video gbr bit rate.
  static const uint64_t       m_mbrBitRate [];  //!< Video max bit rate.
};

} // namespace ns3
#endif // TRAFFIC_HELPER_H
