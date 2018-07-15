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
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

class RingNetwork;
class RadioNetwork;

/**
 * \ingroup svelteInfra
 * This class extends the EpcHelper to create and configure the SVELTE
 * infrastructure: the LTE radio network and the OpenFlow backhaul network.
 */
class SvelteEpcHelper : public EpcHelper
{
public:
  SvelteEpcHelper ();          //!< Default constructor.
  virtual ~SvelteEpcHelper (); //!< Dummy destructor, see DoDispose.

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

  /**
   * \name IPv4 address assign methods.
   * Assign IPv$ address to LTE devices at different segments.
   * \param devices The set of devices.
   * \return The interface container.
   */
  //\{
  Ipv4InterfaceContainer AssignHtcUeIpv4Address (NetDeviceContainer devices);
  Ipv4InterfaceContainer AssignMtcUeIpv4Address (NetDeviceContainer devices);
  //\}

  static const Ipv4Address      m_ueAddr;           //!< UE network address.
  static const Ipv4Address      m_htcAddr;          //!< HTC UE network address.
  static const Ipv4Address      m_mtcAddr;          //!< MTC UE network address.
  static const Ipv4Mask         m_ueMask;           //!< UE network mask.
  static const Ipv4Mask         m_htcMask;          //!< HTC UE network mask.
  static const Ipv4Mask         m_mtcMask;          //!< MTC UE network mask.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  // IP address helpers for UEs. // FIXME Isso deveria ser independente por slice.
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.

  // FIXME This should be independent per slice.
  Ipv4Address                   m_pgwAddr;          //!< P-GW gateway addr.

  Ptr<RingNetwork>              m_backhaul;         //!< The backhaul network.
  Ptr<RadioNetwork>             m_lteRan;           //!< The LTE RAN network.
};

} // namespace ns3
#endif  // SVELTE_EPC_HELPER_H
