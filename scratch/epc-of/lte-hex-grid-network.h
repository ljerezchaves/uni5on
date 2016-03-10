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

#ifndef LTE_HEX_GRID_NETWORK_H
#define LTE_HEX_GRID_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/buildings-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * \ingroup epcof
 * LTE radio network topology with eNBs grouped in three-sector sites layed out
 * on an hexagonal grid. UEs are randomly distributed around the sites and
 * attach to the network automatically using Idle mode cell selection.
 */
class LteHexGridNetwork : public Object
{
public:
  LteHexGridNetwork ();           //!< Default constructor
  virtual ~LteHexGridNetwork ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return the eNBs node container */
  NodeContainer GetEnbNodes ();

  /** \return the UEs node container */
  NodeContainer GetUeNodes ();

  /** \return the UEs NetDevice container */
  NetDeviceContainer GetUeDevices ();

  /** \return the LteHelper used to create this LTE network */
  Ptr<LteHelper> GetLteHelper ();

  /** Enable LTE ascii traces. */
  void EnableTraces ();

  /**
   * Creates the LTE radio topology.
   * \param epcHelper The EpcHelper used to create the LTE EPC core
   * \return Ptr<LteHelper> The LteHelper used to create this LTE network.
   */
  Ptr<LteHelper> CreateTopology (Ptr<EpcHelper> epcHelper);

private:
  /**
   * Set the number of macro eNB sites, and adjust the total eNBs accordingly.
   * \param sites The number of sites.
   */
  void SetNumSites (uint32_t sites);

  /**
   * Identify the LTE radio coverage area considering the eNB positions.
   * \return The retangle with the coverage area.
   */
  Rectangle IdentifyEnbsCoverageArea ();

  /**
   * Print LTE radio environment map.
   * \param filename The filename to use.
   */
  void PrintRadioEnvironmentMap (std::string filename);

  /**
   * Print buildings boundarites to gnuplot format.
   * \param filename The filename to use.
   */
  void PrintBuildingListToFile (std::string filename);

  /**
   * Print UE positions to gnuplot format.
   * \param filename The filename to use.
   */
  void PrintUeListToFile (std::string filename);

  /**
   * Print eNB positions to gnuplot format.
   * \param filename The filename to use.
   */
  void PrintEnbListToFile (std::string filename);

  uint32_t            m_nSites;       //!< Number of sites
  uint32_t            m_nEnbs;        //!< Number of eNBs (3 * m_nSites)
  uint32_t            m_nUes;         //!< Number of UEs
  uint32_t            m_enbMargin;    //!< eNB coverage margin
  double              m_ueHeight;     //!< UE height
  bool                m_lteRem;       //!< Print the LTE REM
  bool                m_ueMobility;   //!< Enable UE mobility
  std::string         m_remFilename;  //!< LTE REM filename
  std::string         m_bldsFilename; //!< Building position filename
  std::string         m_uesFilename;  //!< UE position filename
  std::string         m_enbsFilename; //!< eNB position filename
  NodeContainer       m_enbNodes;     //!< eNB nodes
  NetDeviceContainer  m_enbDevices;   //!< eNB devices
  NodeContainer       m_ueNodes;      //!< UE nodes
  NetDeviceContainer  m_ueDevices;    //!< UE devices
  Rectangle           m_coverageArea; //!< LTE radio coverage area

  Ptr<LteHexGridEnbTopologyHelper> m_topoHelper;  //!< Grid topology helper
  Ptr<RadioEnvironmentMapHelper>   m_remHelper;   //!< Radio map helper
  Ptr<LteHelper>                   m_lteHelper;   //!< Lte radio helper
  Ptr<EpcHelper>                   m_epcHelper;   //!< Lte epc helper
};

};  // namespace ns3
#endif  // LTE_HEX_GRID_NETWORK_H

