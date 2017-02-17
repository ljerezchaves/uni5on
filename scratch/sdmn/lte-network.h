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

#ifndef LTE_NETWORK_H
#define LTE_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/buildings-module.h>
#include <ns3/internet-module.h>
#include "sdran-cloud-container.h"

namespace ns3 {

class EpcNetwork;

/**
 * \ingroup sdmn
 * LTE radio network topology with eNBs grouped in three-sector sites layed out
 * on an hexagonal grid. UEs are randomly distributed around the sites and
 * attach to the network automatically using Idle mode cell selection.
 */
class LteNetwork : public Object
{
public:
  /**
   * Complete constructor.
   * \param epcNetwork The OpenFlow EPC network.
   */
  LteNetwork (Ptr<EpcNetwork> epcNetwork);

  LteNetwork ();           //!< Default constructor.
  virtual ~LteNetwork ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \return the eNBs node container */
  NodeContainer GetEnbNodes ();

  /** \return the UEs node container */
  NodeContainer GetUeNodes ();

  /** \return the UEs NetDevice container */
  NetDeviceContainer GetUeDevices ();

  /** \return the LteHelper used to create this LTE network */
  Ptr<LteHelper> GetLteHelper ();

protected:
  /** Destructor implementation */
  void DoDispose ();

  // Inherited from ObjectBase
  void NotifyConstructionCompleted (void);

private:
  /** Create and configure the helpers objects. */
  void ConfigureHelpers ();

  /** Create the LTE radio topology. */
  void CreateTopology ();

  /** Identify the LTE radio coverage area based on eNB node positions. */
  void IdentifyCoverageArea ();

  /** Print LTE radio environment map. */
  void PrintRadioEnvironmentMap ();

  uint32_t            m_nSdrans;      //!< Number of SDRAN clouds
  uint32_t            m_nUes;         //!< Number of UEs
  double              m_enbMargin;    //!< eNB coverage margin
  double              m_ueHeight;     //!< UE height
  bool                m_lteTrace;     //!< Enable LTE ASCII traces
  bool                m_lteRem;       //!< Print the LTE REM
  bool                m_ueMobility;   //!< Enable UE mobility
  std::string         m_remFilename;  //!< LTE REM filename
  SdranCloudContainer m_sdranClouds;  //!< SDRAN clouds
  NodeContainer       m_enbNodes;     //!< eNB nodes
  NetDeviceContainer  m_enbDevices;   //!< eNB devices
  NodeContainer       m_ueNodes;      //!< UE nodes
  NetDeviceContainer  m_ueDevices;    //!< UE devices
  Rectangle           m_coverageArea; //!< LTE radio coverage area

  Ptr<LteHexGridEnbTopologyHelper> m_topoHelper;  //!< Grid topology helper
  Ptr<RadioEnvironmentMapHelper>   m_remHelper;   //!< Radio map helper
  Ptr<LteHelper>                   m_lteHelper;   //!< LTE radio helper
  Ptr<EpcNetwork>                  m_epcNetwork;  //!< EPC network
};

};  // namespace ns3
#endif  // LTE_NETWORK_H

