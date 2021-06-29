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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
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

/**
 * \ingroup uni5onInfra
 * LTE radio access network with eNBs grouped in three-sector sites layed out
 * on an hexagonal grid. UEs are randomly distributed around the sites and
 * attach to the network automatically using idle mode cell selection.
 */
class RadioNetwork : public Object
{
public:
  /**
   * Complete constructor.
   * \param helper The scenario helper (EpcHelper).
   */
  RadioNetwork (Ptr<EpcHelper> helper);
  virtual ~RadioNetwork ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enables automatic attachment of a set of UE devices to a suitable cell
   * using idle mode initial cell selection procedure.
   * \param ueDevices The set of UE devices to be attached.
   */
  void AttachUeDevices (NetDeviceContainer ueDevices);

  /**
   * Configure the given nodes as UEs.
   * \param ueNodes The set of nodes.
   * \param mobilityHelper The mobility helper for UEs.
   * \return The container with the newly created UE devices.
   */
  NetDeviceContainer InstallUeDevices (NodeContainer ueNodes,
                                       MobilityHelper mobilityHelper);

  /**
   * Get the RAN coverage area of a specific cell site or the entire RAN
   * network, considering the EnbMargin attribute.
   * \param cellSiteId The ID of any cell site (starting at 1). When left
   *        to 0, the coverage area of the entire RAN network will be returned.
   * \return The coverage area.
   */
  Rectangle GetCoverageArea (uint16_t cellSiteId = 0) const;

  /**
   * Get the LTE helper used to configure this radio network.
   * \return The LTE helper.
   */
  Ptr<LteHelper> GetLteHelper (void) const;

  /**
   * Create a random box position allocator that randomly spreads UE nodes
   * within the coverage area of a specific cell site ID or the entire RAN
   * network.
   * \param cellSiteId The ID of any cell site (starting at 1). When left
   *        to 0, the coverage area of the entire RAN network will be used.
   * \return The position allocator.
   */
  Ptr<PositionAllocator> GetRandomPositionAllocator (
    uint16_t cellSiteId = 0) const;

  /**
   * Print the radio environment map.
   */
  void PrintRadioEnvironmentMap (void);

protected:
  /** Destructor implementation. */
  void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  uint32_t            m_nSites;         //!< Number of cell sites.
  double              m_enbMargin;      //!< eNB coverage margin.
  double              m_ueHeight;       //!< UE height.
  bool                m_handover;       //!< Enable UE handover.
  bool                m_lteTrace;       //!< Enable LTE ASCII traces.
  std::string         m_remFilename;    //!< REM filename.
  NodeContainer       m_enbNodes;       //!< eNB nodes.
  NetDeviceContainer  m_enbDevices;     //!< eNB devices.
  NodeContainer       m_ueNodes;        //!< UE nodes.
  NetDeviceContainer  m_ueDevices;      //!< UE devices.
  Rectangle           m_ranCoverArea;   //!< Radio coverage area.

  Ptr<LteHexGridEnbTopologyHelper> m_topoHelper;    //!< Grid topology helper.
  Ptr<RadioEnvironmentMapHelper>   m_remHelper;     //!< Radio map helper.
  Ptr<LteHelper>                   m_lteHelper;     //!< LTE helper.
  Ptr<EpcHelper>                   m_epcHelper;     //!< EPC helper.
};

} // namespace ns3
#endif  // RADIO_NETWORK_H
