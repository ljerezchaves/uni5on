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
 * \ingroup uni5onTraffic
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

  /**
   * Slice ID accessor.
   * \return The slice ID.
   */
  SliceId GetSliceId (void) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  void NotifyConstructionCompleted (void);

  /**
   * Configure application helpers for different traffic patterns.
   */
  virtual void ConfigureHelpers () = 0;

  /**
   * Configure the UE and install applications for different traffic patterns.
   * \param ueInfo The UE metadata.
   */
  virtual void ConfigureUeTraffic (Ptr<UeInfo> ueInfo) = 0;

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

private:
  /**
   * Get the next port number available for use.
   * \return The port number to use.
   */
  static uint16_t GetNextPortNo ();

  // Traffic helper.
  SliceId                     m_sliceId;          //!< Logical slice ID.
  Ptr<RadioNetwork>           m_radio;            //!< Radio network.
  Ptr<SliceNetwork>           m_slice;            //!< Slice network.
  Ptr<SliceController>        m_controller;       //!< Slice controller.
  static uint16_t             m_port;             //!< Port numbers for apps.
  bool                        m_useOnlyDefault;   //!< Use only default bearer.

  // Traffic manager.
  ObjectFactory               m_managerFac;       //!< Traffic manager factory.
  Ptr<RandomVariableStream>   m_poissonRng;       //!< Inter-arrival traffic.
  bool                        m_restartApps;      //!< Continuous restart apps.
  double                      m_initialProb;      //!< Initial start probab.
  Time                        m_startAppsAt;      //!< Time to start apps.
  Time                        m_stopAppsAt;       //!< Time to stop apps.

  // Web server.
  Ptr<Node>                   m_webNode;          //!< Server node.
  Ipv4Address                 m_webAddr;          //!< Server address.
  Ipv4Mask                    m_webMask;          //!< Server address mask.

  // Radio network.
  Ptr<LteHelper>              m_lteHelper;        //!< LTE helper.
};

} // namespace ns3
#endif // TRAFFIC_HELPER_H
