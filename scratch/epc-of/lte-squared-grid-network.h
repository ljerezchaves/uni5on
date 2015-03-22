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

#ifndef LTE_SQUARED_GRID_NETWORK_H
#define LTE_SQUARED_GRID_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/buildings-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * LTE radio network layed out on a squared grid. This class generates a
 * squared grid topology, placing a eNodeB at the centre of each square. UEs
 * attached to this node may be fixed at same position of eNB or scattered
 * randomly around the eNB. See figure 18.63 of ns-3-model-library v3.19). 
 */
class LteSquaredGridNetwork : public Object
{
public:
  LteSquaredGridNetwork ();           //!< Default constructor
  virtual ~LteSquaredGridNetwork ();  //!< Dummy destructor, see DoDipose

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

  /** Enable LTE ascii traces */
  void EnableTraces ();
   
  /**
   * Creates the LTE radio tolopoly.
   * \param epcHelper The EpcHelper used to create the LTE EPC core
   * \param nUes A vector containing the number of UEs for each eNB.
   * \return Ptr<LteHelper> The LteHelper used to create this LTE network.
   */
  Ptr<LteHelper> CreateTopology (Ptr<EpcHelper> epcHelper, 
      std::vector<uint32_t> nUes); 

private:
  /** Configure default values for LTE radio network. */
  void ConfigureLteParameters ();

  /** Set eNBs and UEs positions */
  void SetLteNodePositions ();

  /** Install the LTE protocol stack into each eNB and UE */
  void InstallProtocolStack ();

  uint32_t  m_nEnbs;      //!< Number of eNBs
  double    m_enbHeight;  //!< eNB height
  double    m_ueHeight;   //!< UE height
  double    m_roomLength; //!< The x,y distances between eNBs
  bool      m_fixedUes;   //!< True to place all UEs at same eNB position

  NodeContainer               m_enbNodes;       //!< eNB nodes
  NodeContainer               m_ueNodes;        //!< UE nodes
  NetDeviceContainer          m_enbDevices;     //!< eNB devices
  NetDeviceContainer          m_ueDevices;      //!< UE devices
  std::vector<NodeContainer>  m_ueNodesPerEnb;  //!< UE nodes for each eNB
  std::vector<uint32_t>       m_nUesPerEnb;     //!< Number of UEs for each eNB 

  Ptr<LteHelper> m_lteHelper; //!< LteHelper used to create the radio network
  Ptr<EpcHelper> m_epcHelper; //!< EpcHelper used to create the EPC network
};

};  // namespace ns3
#endif  // LTE_SQUARED_GRID_NETWORK_H

