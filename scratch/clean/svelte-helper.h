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

/**
 * \ingroup svelte
 * This class extends the EpcHelper to configure IP addresses and how EPC S5
 * entities (P-GW and S-GW) are connected through CSMA devices to the OpenFlow
 * backhaul network.
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

  // Inherited from EpcHelper.
  virtual uint8_t ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi,
                                     Ptr<EpcTft> tft, EpsBearer bearer);
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice,
                       uint16_t cellId);
  virtual void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  virtual Ptr<Node> GetPgwNode ();
  virtual Ipv4InterfaceContainer AssignUeIpv4Address (
    NetDeviceContainer ueDevices);
  virtual Ipv4Address GetUeDefaultGatewayAddress ();
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
  Ipv4InterfaceContainer AssignS1Ipv4Address    (NetDeviceContainer devices);
  Ipv4InterfaceContainer AssignS5Ipv4Address    (NetDeviceContainer devices);
  Ipv4InterfaceContainer AssignSgiIpv4Address   (NetDeviceContainer devices);
  Ipv4InterfaceContainer AssignX2Ipv4Address    (NetDeviceContainer devices);
  //\}

  /**
   * Get the IPv4 address assigned to a given device.
   * \param device The network device.
   * \return The IPv4 address.
   */
  static Ipv4Address GetIpv4Addr (Ptr<const NetDevice> device);

  /**
   * Get the IPv4 mask assigned to a given device.
   * \param device The network device.
   * \return The IPv4 mask.
   */
  static Ipv4Mask GetIpv4Mask (Ptr<const NetDevice> device);

  static const uint16_t         m_gtpuPort;         //!< GTP-U UDP port.
  static const Ipv4Address      m_htcAddr;          //!< HTC UE network address.
  static const Ipv4Address      m_mtcAddr;          //!< MTC UE network address.
  static const Ipv4Address      m_s1uAddr;          //!< S1-U network address.
  static const Ipv4Address      m_s5Addr;           //!< S5 network address.
  static const Ipv4Address      m_sgiAddr;          //!< Web network address.
  static const Ipv4Address      m_ueAddr;           //!< UE network address.
  static const Ipv4Address      m_x2Addr;           //!< X2 network address.
  static const Ipv4Mask         m_htcMask;          //!< HTC UE network mask.
  static const Ipv4Mask         m_mtcMask;          //!< MTC UE network mask.
  static const Ipv4Mask         m_s1uMask;          //!< S1-U network mask.
  static const Ipv4Mask         m_s5Mask;           //!< S5 network mask.
  static const Ipv4Mask         m_sgiMask;          //!< Web network mask.
  static const Ipv4Mask         m_ueMask;           //!< UE network mask.
  static const Ipv4Mask         m_x2Mask;           //!< X2 network mask.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  // IP address helpers for interfaces.
  Ipv4AddressHelper             m_htcUeAddrHelper;  //!< HTC UE address helper.
  Ipv4AddressHelper             m_mtcUeAddrHelper;  //!< MTC UE address helper.
  Ipv4AddressHelper             m_s1uAddrHelper;    //!< S1U address helper.
  Ipv4AddressHelper             m_s5AddrHelper;     //!< S5 address helper.
  Ipv4AddressHelper             m_sgiAddrHelper;    //!< Web address helper.
  Ipv4AddressHelper             m_x2AddrHelper;     //!< X2 address helper.

  // FIXME This should be independent per slice.
  Ipv4Address                   m_pgwAddr;          //!< P-GW gateway addr.
};

} // namespace ns3
#endif  // SVELTE_EPC_HELPER_H
