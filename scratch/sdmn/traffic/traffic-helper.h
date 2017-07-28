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
#include "../apps/sdmn-app-helper.h"

namespace ns3 {

class HtcTrafficManager;
class MtcTrafficManager;
class EpcController;
class LteNetwork;
class EpcNetwork;

/**
 * \ingroup sdmn
 * Traffic helper which installs client and server applications for all
 * applications into UEs and WebServer. This helper creates and aggregates a
 * traffic manager for each UE.
 */
class TrafficHelper : public Object
{
public:
  /**
   * Complete constructor.
   * \param lteNetwork The LTE network.
   * \param webNode The Internet web server node.
   */
  TrafficHelper (Ptr<LteNetwork> lteNetwork, Ptr<Node> webNode);

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
   * Install HTC applications and traffic manager into each HTC UE. It creates
   * the client/server application pair, and install them in the respective
   * nodes. It also configure the TFT and EPS bearers.
   * \param ueNodes The HTC UE Nodes container.
   * \param ueDevices The HTC UE NetDevices container.
   * \internal
   * Some notes about internal GbrQosInformation usage:
   * \li The Maximum Bit Rate field is used by controller to install meter
   *     rules for this traffic. When this value is left to 0, no meter rules
   *     will be installed.
   * \li The Guaranteed Bit Rate field is used by the controller to reserve the
   *     requested bandwidth in OpenFlow EPC network (only for GBR beares).
   */
  void InstallHtcApplications (NodeContainer ueNodes,
                               NetDeviceContainer ueDevices);

  /**
   * Install MTC applications and traffic manager into each MTC UE. It creates
   * the client/server application pair, and install them in the respective
   * nodes. It also configure the TFT and EPS bearers.
   * \param ueNodes The MTC UE Nodes container.
   * \param ueDevices The MTC UE NetDevices container.
   */
  void InstallMtcApplications (NodeContainer ueNodes,
                               NetDeviceContainer ueDevices);

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
   * Retrieve the LTE helper used by create the LTE network.
   * \return The LTE helper pointer.
   */
  Ptr<LteHelper> GetLteHelper ();

  /**
   * UDP bidirect VoIP traffic over dedicated GBR EPS bearer (QCI 1).
   * This QCI is typically associated with conversational voice. This VoIP
   * traffic simulates the G.729 codec (~8.0 kbps for payload). Check
   * http://goo.gl/iChPGQ for bandwidth calculation and discussion.
   */
  void InstallGbrVoip ();

  /**
   * UDP uplink auto-pilot traffic over dedicated GBR EPS bearer (QCI 3).
   * This QCI is typically associated with an operator controlled service,
   * i.e., a service where the data flow aggregate's uplink/downlink packet
   * filters are known at the point in time when the data flow aggregate is
   * authorized. This auto-pilot model is adapted from the MTC application
   * model indicate on the "Machine-to-Machine Communications: Architectures,
   * Technology, Standards, and Applications" book, chapter 3: "M2M traffic and
   * models".
   */
  void InstallGbrAutoPilot ();

  /**
   * UDP down live video streaming over dedicated GBR EPS bearer (QCI 4).
   * This QCI is typically associated with non-conversational video and live
   * streaming. This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
   */
  void InstallGbrLiveVideoStreaming ();

  /**
   * TCP bidirect buff video streaming over dedicated Non-GBR bearer (QCI 6).
   * This QCI could be used for priorization of non real-time data of MPS
   * subscribers. This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
   */
  void InstallNonGbrBufferedVideoStreaming ();

  /**
   * UDP down live video streaming over dedicated Non-GBR bearer (QCI 7).
   * This QCI is typically associated with voice, live video streaming and
   * interactive games. This video traffic is based on MPEG-4 video traces from
   * http://www-tkn.ee.tu-berlin.de/publications/papers/TKN0006.pdf.
   */
  void InstallNonGbrLiveVideoStreaming ();

  /**
   * TCP bidirect HTTP traffic over dedicated Non-GBR bearer (QCI 8).
   * This QCI could be used for a dedicated 'premium bearer' for any
   * subscriber, or could be used for the default bearer of a for 'premium
   * subscribers'. This HTTP model is based on the distributions indicated in
   * the paper 'An HTTP Web Traffic Model Based on the Top One Million Visited
   * Web Pages' by Rastin Pries et. al. Each client will send a get request to
   * the server and will get the page content back including inline content.
   * These requests repeats after a reading time period, until MaxPages are
   * loaded or MaxReadingTime is reached.
   */
  void InstallNonGbrHttp ();

  ObjectFactory               m_htcFactory;     //!< HTC manager factory.
  Ptr<HtcTrafficManager>      m_htcManager;     //!< HTC traffic manager.
  ObjectFactory               m_mtcFactory;     //!< MTC manager factory.
  Ptr<MtcTrafficManager>      m_mtcManager;     //!< MTC traffic manager.

  Ptr<LteNetwork>             m_lteNetwork;     //!< The LTE network.

  Ptr<Node>                   m_webNode;        //!< Server node.
  Ipv4Address                 m_webAddr;        //!< Server address.
  Ipv4Mask                    m_webMask;        //!< Server address mask.

  Ptr<Node>                   m_ueNode;         //!< Client node.
  Ptr<NetDevice>              m_ueDev;          //!< Client dev.
  Ipv4Address                 m_ueAddr;         //!< Client address.
  Ipv4Mask                    m_ueMask;         //!< Client address mask.

  bool                        m_httpEnable;     //!< HTTP enable.
  bool                        m_plotEnable;     //!< Auto-pilot enable.
  bool                        m_rtvgEnable;     //!< GBR live video enable.
  bool                        m_rtvnEnalbe;     //!< Non-GBR live video enable.
  bool                        m_stvdEnable;     //!< Buffered video enable.
  bool                        m_voipEnable;     //!< VoIP enable.

  SdmnAppHelper               m_httpHelper;     //!< HTTP app helper.
  SdmnAppHelper               m_plotHelper;     //!< Pilot app helper.
  SdmnAppHelper               m_rtvdHelper;     //!< Live video app helper.
  SdmnAppHelper               m_stvdHelper;     //!< Stored video app helper.
  SdmnAppHelper               m_voipHelper;     //!< Voip app helper.

  static uint16_t             m_port;           //!< Port numbers for apps.

  Ptr<UniformRandomVariable>  m_videoRng;       //!< Random video selection.
  static const std::string    m_videoDir;       //!< Video trace directory.
  static const std::string    m_videoTrace [];  //!< Video trace filenames.
  static const uint64_t       m_gbrBitRate [];  //!< Video gbr bit rate.
  static const uint64_t       m_mbrBitRate [];  //!< Video max bit rate.
};

};  // namespace ns3
#endif // TRAFFIC_HELPER_H
