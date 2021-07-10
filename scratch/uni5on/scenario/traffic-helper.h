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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef TRAFFIC_HELPER_H
#define TRAFFIC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../applications/application-helper.h"
#include "../uni5on-common.h"

namespace ns3 {

class UeInfo;
class RadioNetwork;
class SliceController;
class SliceNetwork;
class TrafficManager;

/**
 * \ingroup uni5on
 * The helper to create and configure client and server applications into UEs
 * and web server nodes. This helper also creates and aggregates a traffic
 * manager object to each UE.
 */
class TrafficHelper : public Object
{
public:
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
   * Configure application helpers for different traffic patterns.
   */
  void ConfigureHelpers ();

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
   * Install a traffic manager into each UE and configure the EPS bearers and
   * TFT packet filters for enable applications
   * \attention The QCIs used here for each application are strongly related
   *     to the DSCP mapping, which will reflect on the queues used by both
   *     OpenFlow switches and traffic control module. Be careful if you
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
   * Create the pair of client/server applications and install them,
   * configuring a dedicated EPS bearer for this traffic according to bearer
   * and packet filter parameters.
   * \param ueInfo The UE metadata.
   * \param helper The reference to the application helper.
   * \param bearer The reference to the EPS bearer.
   * \param filter The reference to the packet filter.
   */
  void InstallAppDedicated (
    Ptr<UeInfo> ueInfo, ApplicationHelper& helper,
    EpsBearer& bearer, EpcTft::PacketFilter& filter);

  /**
   * Create the pair of client/server applications and install them,
   * using the default EPS bearer for this traffic.
   * \param ueInfo The UE metadata.
   * \param helper The reference to the application helper.
   */
  void InstallAppDefault (
    Ptr<UeInfo> ueInfo, ApplicationHelper& helper);

  // Traffic helper.
  SliceId                     m_sliceId;          //!< Logical slice ID.
  Ptr<RadioNetwork>           m_radio;            //!< Radio network.
  Ptr<SliceNetwork>           m_slice;            //!< Slice network.
  Ptr<SliceController>        m_controller;       //!< Slice controller.
  static uint16_t             m_port;             //!< Port numbers for apps.
  bool                        m_useOnlyDefault;   //!< Use only default bearer.

  // Traffic manager.
  Time                        m_fullProbAt;       //!< Time to 100% requests.
  Time                        m_halfProbAt;       //!< Time to 50% requests.
  Time                        m_zeroProbAt;       //!< Time to 0% requests.
  double                      m_initialProb;      //!< Initial start probab.
  ObjectFactory               m_managerFac;       //!< Traffic manager factory.
  Ptr<RandomVariableStream>   m_poissonRng;       //!< Inter-arrival traffic.
  bool                        m_restartApps;      //!< Continuous restart apps.
  Time                        m_startAppsAt;      //!< Time to start apps.
  Time                        m_stopAppsAt;       //!< Time to stop apps.

  // Application helpers.
  ApplicationHelper           m_autPilotHelper;   //!< Auto-pilot helper.
  ApplicationHelper           m_bikeRaceHelper;   //!< Bicycle race helper.
  ApplicationHelper           m_gameOpenHelper;   //!< Open Arena helper.
  ApplicationHelper           m_gameTeamHelper;   //!< Team Fortress helper.
  ApplicationHelper           m_gpsTrackHelper;   //!< GPS tracking helper.
  ApplicationHelper           m_httpPageHelper;   //!< HTTP page helper.
  ApplicationHelper           m_livVideoHelper;   //!< Live video helper.
  ApplicationHelper           m_recVideoHelper;   //!< Recorded video helper.
  ApplicationHelper           m_voipCallHelper;   //!< VoIP call helper.

  // Web server.
  Ptr<Node>                   m_webNode;          //!< Server node.
  Ipv4Address                 m_webAddr;          //!< Server address.
  Ipv4Mask                    m_webMask;          //!< Server address mask.

  // Radio network.
  Ptr<LteHelper>              m_lteHelper;        //!< LTE helper.

  // Video traces.
  Ptr<UniformRandomVariable>  m_gbrVidRng;        //!< GBR random live video.
  Ptr<UniformRandomVariable>  m_nonVidRng;        //!< Non-GBR random video.
  static const std::string    m_videoDir;         //!< Video trace directory.
  static const std::string    m_videoTrace [];    //!< Video trace filenames.
  static const uint64_t       m_gbrBitRate [];    //!< Video gbr bit rate.
  static const uint64_t       m_mbrBitRate [];    //!< Video max bit rate.
};

} // namespace ns3
#endif // TRAFFIC_HELPER_H
