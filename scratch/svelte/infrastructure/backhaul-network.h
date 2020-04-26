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

#ifndef BACKHAUL_NETWORK_H
#define BACKHAUL_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../svelte-common.h"

namespace ns3 {

class BackhaulController;

/**
 * \ingroup svelte
 * \defgroup svelteInfra Infrastructure
 * SVELTE architecture infrastructure.
 */

/**
 * \ingroup svelteInfra
 * This is the abstract base class for the OpenFlow backhaul network, which
 * should be extended in accordance to the desired backhaul network topology.
 * SVELTE EPC entities (eNB, S-GW, and P-GW) are connected to the OpenFlow
 * switches through CSMA devices.
 */
class BackhaulNetwork : public Object
{
  friend class BackhaulController;
  friend class RingController;

public:
  BackhaulNetwork ();          //!< Default constructor.
  virtual ~BackhaulNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces files on the OpenFlow backhaul network.
   * \param prefix Filename prefix to use for PCAP files.
   * \param promiscuous If true, enable PCAP promiscuous traces.
   * \param ofchannel If true, enable PCAP on backhaul OpenFlow channels.
   * \param epcDevices If true, enable PCAP on EPC S1, S5, and X2 interfaces.
   * \param swtDevices If true, enable PCAP on backhaul switches.
   */
  void EnablePcap (std::string prefix, bool promiscuous, bool ofchannel,
                   bool epcDevices, bool swtDevices);

  /**
   * Attach the EPC node to the OpenFlow backhaul network.
   * \param epcNode The eNB node.
   * \param swIdx The switch index at which the EPC node should be connected.
   * \param iface The LTE logical interface for this connection.
   * \param ifaceStr Custom name for this LTE logical interface. When this
   *        string is empty, the default name is used.
   * \return The pair with the network device created at the EPC node and the
   *         port device create at the backhaul switch.
   */
  virtual std::pair<Ptr<CsmaNetDevice>, Ptr<OFSwitch13Port> >
  AttachEpcNode (Ptr<Node> enbNode, uint16_t swIdx, LteIface iface,
                 std::string ifaceStr = std::string ());

  /**
   * Get the backahul switch index at which the given eNB should be connected.
   * \param cellId The eNB cell ID.
   * \return The backhaul switch index.
   */
  virtual uint16_t GetEnbSwIdx (uint16_t cellId) const = 0;

  /**
   * Get the total number of OpenFlow switches in the backhaul network.
   * \return The number of OpenFlow switches.
   */
  uint32_t GetNSwitches (void) const;

  /**
   * Get the OpenFlow backhaul network controller.
   * \return The OpenFlow controller.
   */
  Ptr<BackhaulController> GetControllerApp (void) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Create the controller application and switch devices for the OpenFlow
   * backhaul network, connecting them accordingly to the desired topology.
   */
  virtual void CreateTopology (void) = 0;

  // Backhaul controller.
  Ptr<BackhaulController>       m_controllerApp;  //!< Controller app.
  Ptr<Node>                     m_controllerNode; //!< Controller node.

  // OpenFlow switches, helper and connection attribute.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;   //!< Switch helper.
  NodeContainer                 m_switchNodes;    //!< Switch nodes.
  OFSwitch13DeviceContainer     m_switchDevices;  //!< Switch devices.
  uint16_t                      m_linkMtu;        //!< Link MTU.

  // Network addresses.
  static const Ipv4Address      m_s1Addr;         //!< S1-U network address.
  static const Ipv4Address      m_s5Addr;         //!< S5 network address.
  static const Ipv4Address      m_x2Addr;         //!< X2 network address.
  static const Ipv4Mask         m_s1Mask;         //!< S1-U network mask.
  static const Ipv4Mask         m_s5Mask;         //!< S5 network mask.
  static const Ipv4Mask         m_x2Mask;         //!< X2 network mask.

private:
  // CSMA helper and attributes for EPC interfaces.
  CsmaHelper                    m_csmaHelper;     //!< EPC connection helper.
  DataRate                      m_linkRate;       //!< EPC link data rate.
  Time                          m_linkDelay;      //!< EPC link delay.

  // Switch datapath configuration.
  DataRate                      m_cpuCapacity;    //!< CPU capacity.
  uint32_t                      m_flowTableSize;  //!< Flow table size.
  uint32_t                      m_groupTableSize; //!< Group table size.
  uint32_t                      m_meterTableSize; //!< Meter table size.

  // IPv4 address helpers for EPC interfaces.
  Ipv4AddressHelper             m_s1AddrHelper;   //!< S1-U address helper.
  Ipv4AddressHelper             m_s5AddrHelper;   //!< S5 address helper.
  Ipv4AddressHelper             m_x2AddrHelper;   //!< X2 address helper.
  NetDeviceContainer            m_epcDevices;     //!< EPC devices.
};

} // namespace ns3
#endif  // BACKHAUL_NETWORK_H
