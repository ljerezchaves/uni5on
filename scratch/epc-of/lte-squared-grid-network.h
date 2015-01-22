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
 * \brief Squared grid LTE network (without EPC that should be created apart)
 * 
 * This class generates a squared grid topology, placing a eNodeB at the
 * centre of each square. UEs attached to this node are scattered randomly
 * across the square (using a random uniform distribution along X and Y
 * axis). See figure 18.63 of ns-3-model-library v3.19). 
 */
class LteSquaredGridNetwork : public Object
{
public:
  LteSquaredGridNetwork ();
  virtual ~LteSquaredGridNetwork ();

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  NodeContainer GetEnbNodes ();
  NodeContainer GetUeNodes ();

  void EnableTraces ();
  void CreateTopology (Ptr<EpcHelper> epcHelper, std::vector<uint32_t> nUes); 
  Ptr<LteHelper> GetLteHelper ();
  NetDeviceContainer GetUeDevices ();

private:
  // Attributes
  uint32_t m_nEnbs;
  double m_enbHeight;
  double m_ueHeight;
  double m_roomLength;

  // Containers
  NodeContainer m_enbNodes;
  NodeContainer m_ueNodes;
  NetDeviceContainer m_enbDevices;
  NetDeviceContainer m_ueDevices;
  std::vector<NodeContainer> m_ueNodesPerEnb;
  std::vector<uint32_t> m_nUesPerEnb;


  // Helpers
  Ptr<LteHelper> m_lteHelper;
  Ptr<EpcHelper> m_epcHelper;

  void SetLteNodePositions ();
  void InstallProtocolStack ();
};

};  // namespace ns3
#endif  // LTE_SQUARED_GRID_NETWORK_H

