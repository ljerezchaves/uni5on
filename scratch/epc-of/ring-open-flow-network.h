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

#include <ns3/core-module.h>
#include "open-flow-epc-network.h"

namespace ns3 {

/** 
 * This class generates and simple n-switch OpenFlow ring topology
 * controlled by EpcSdnController, which will be used by S1-U and
 * X2 EPC interfaces.
 *
 */
class RingOpenFlowNetwork : public OpenFlowEpcNetwork
{
public:
  RingOpenFlowNetwork ();
  ~RingOpenFlowNetwork ();

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  void DoDispose ();

  // Inherited from OpenFlowEpcNetowork
  Ptr<NetDevice> AttachToS1u (Ptr<Node> node);
  Ptr<NetDevice> AttachToX2  (Ptr<Node> node);
  void CreateInternalTopology ();
  
private:
  uint16_t m_nodes; //!< Number of switches in the ring
  
  std::map<uint32_t, uint8_t> m_nodeSwitchMap; //!< NodeId/SwitchIndex map.

  /**
   * Register the Node at the map <node, switch> for use in AttachToX2.
   * \param swtch The switch index.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterNodeAtSwitch (uint8_t swtch, Ptr<Node> node);

}; // class RingOpenFlowNetwork
}; // namespace ns3
