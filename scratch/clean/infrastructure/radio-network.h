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

#ifndef RADIO_NETWORK_H
#define RADIO_NETWORK_H

#include <ns3/buildings-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>

namespace ns3 {

class SvelteHelper;
class BackhaulNetwork;

/**
 * \ingroup svelteInfra
 * LTE radio access network with eNBs grouped in three-sector sites layed out
 * on an hexagonal grid. UEs are randomly distributed around the sites and
 * attach to the network automatically using idle mode cell selection.
 */
class RadioNetwork : public Object
{
public:
  /**
   * Complete constructor.
   * \param helper The SVELTE helper (EpcHelper).
   * \param backhaul The backhaul network.
   */
  RadioNetwork (Ptr<SvelteHelper> helper,
                Ptr<BackhaulNetwork> backhaul);
  virtual ~RadioNetwork ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  Ptr<LteHelper>      GetLteHelper    (void) const;
  NodeContainer       GetEnbNodes     (void) const;
  NodeContainer       GetHtcUeNodes   (void) const;
  NodeContainer       GetMtcUeNodes   (void) const;
  NetDeviceContainer  GetHtcUeDevices (void) const;
  NetDeviceContainer  GetMtcUeDevices (void) const;
  //\}

  /**
   * Enable PCAP traces on SDRAN clouds.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

protected:
  /** Destructor implementation. */
  void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /** Configure the helpers. */
  void ConfigureHelpers ();

  /** Configure the eNBs. */
  void ConfigureEnbs ();

  /** Configure the UEs. */
  void ConfigureUes ();

  /** Print LTE radio environment map. */
  void PrintRadioEnvironmentMap ();

  uint32_t            m_nSites;         //!< Number of cell sites.
  uint32_t            m_nEnbs;          //!< Number of eNBs.
  uint32_t            m_nHtcUes;        //!< Number of HTC UEs.
  uint32_t            m_nMtcUes;        //!< Number of MTC UEs.
  double              m_enbMargin;      //!< eNB coverage margin.
  double              m_ueHeight;       //!< UE height.
  bool                m_lteTrace;       //!< Enable LTE ASCII traces.
  bool                m_lteRem;         //!< Print the LTE REM.
  bool                m_htcUeMobility;  //!< Enable HTC UE mobility.
  bool                m_mtcUeMobility;  //!< Enable MTC UE mobility.
  std::string         m_remFilename;    //!< LTE REM filename.
  NodeContainer       m_enbNodes;       //!< eNB nodes.
  NodeContainer       m_htcUeNodes;     //!< HTC UE nodes.
  NodeContainer       m_mtcUeNodes;     //!< MTC UE nodes.
  NetDeviceContainer  m_enbDevices;     //!< eNB devices.
  NetDeviceContainer  m_htcUeDevices;   //!< HTC UE devices.
  NetDeviceContainer  m_mtcUeDevices;   //!< MTC UE devices.
  Rectangle           m_coverageArea;   //!< LTE radio coverage area.

  Ptr<LteHexGridEnbTopologyHelper> m_topoHelper;    //!< Grid topology helper.
  Ptr<RadioEnvironmentMapHelper>   m_remHelper;     //!< Radio map helper.
  Ptr<LteHelper>                   m_lteHelper;     //!< LTE radio helper.
  Ptr<SvelteHelper>                m_svelteHelper;  //!< SVELTE (EPC) helper.
  Ptr<BackhaulNetwork>             m_backhaul;      //!< Backhaul network.
};

} // namespace ns3
#endif  // RADIO_NETWORK_H

