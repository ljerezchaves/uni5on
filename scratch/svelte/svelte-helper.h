/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#ifndef SVELTE_EPC_HELPER_H
#define SVELTE_EPC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class RadioNetwork;
class RingNetwork;
class SliceController;
class SliceNetwork;
class SvelteMme;

/**
 * \ingroup svelte
 * This class creates and configures the SVELTE architecture, including the
 * shared infrastructure and logical networks.
 */
class SvelteHelper : public EpcHelper
{
public:
  SvelteHelper ();          //!< Default constructor.
  virtual ~SvelteHelper (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces on the SVELTE infrastructure.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Print the LTE radio environment map.
   */
  void PrintLteRem (void);

  // Inherited from EpcHelper.
  uint8_t ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi,
                             Ptr<EpcTft> tft, EpsBearer bearer);
  void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice,
               uint16_t cellId);
  void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  Ptr<Node> GetPgwNode ();
  Ipv4InterfaceContainer AssignUeIpv4Address (NetDeviceContainer ueDevices);
  Ipv4Address GetUeDefaultGatewayAddress ();
  // Inherited from EpcHelper.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  Ptr<RingNetwork>          m_backhaul;         //!< The backhaul network.
  Ptr<RadioNetwork>         m_radio;            //!< The LTE RAN network.
  Ptr<SvelteMme>            m_mme;              //!< SVELTE MME entity.

  ObjectFactory             m_htcNetFactory;    //!< HTC network factory.
  ObjectFactory             m_htcCtrlFactory;   //!< HTC controller factory.
  Ptr<SliceNetwork>         m_htcNetwork;       //!< HTC slice network.
  Ptr<SliceController>      m_htcController;    //!< HTC slice controller.

  ObjectFactory             m_mtcNetFactory;    //!< MTC network factory.
  ObjectFactory             m_mtcCtrlFactory;   //!< MTC controller factory.
  Ptr<SliceNetwork>         m_mtcNetwork;       //!< MTC slice network.
  Ptr<SliceController>      m_mtcController;    //!< MTC slice controller.
};

} // namespace ns3
#endif  // SVELTE_EPC_HELPER_H
