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

#ifndef LTE_SIMPLE_HANDOVER_NETWORK_H
#define LTE_SIMPLE_HANDOVER_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include <ns3/mobility-module.h>
#include <ns3/buildings-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * Sample LTE network for a X2-based handover, as in lena-x2-handover example.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB and triggers
 * a handover of the UE towards the 'target' eNB at 2 seconds of simulation.
 */
class LteSimpleHandoverNetwork : public Object
{
public:
  LteSimpleHandoverNetwork ();
  virtual ~LteSimpleHandoverNetwork ();

  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  NodeContainer GetEnbNodes ();
  NodeContainer GetUeNodes ();

  void EnableTraces ();
  void CreateTopology (Ptr<EpcHelper> epcHelper); 
  Ptr<LteHelper> GetLteHelper ();
  NetDeviceContainer GetUeDevices ();

private:
  uint32_t m_nEnbs;
  uint32_t m_nUes;

  // Attributes
  double m_enbDistance;

  // Containers
  NodeContainer m_enbNodes;
  NodeContainer m_ueNodes;
  NetDeviceContainer m_enbDevices;
  NetDeviceContainer m_ueDevices;

  // Helpers
  Ptr<LteHelper> m_lteHelper;
  Ptr<EpcHelper> m_epcHelper;

  void SetLteNodePositions ();
  void SetHandoverConfiguration ();
  void InstallProtocolStack ();
};

};  // namespace ns3
#endif  // LTE_SIMPLE_HANDOVER_NETWORK_H

