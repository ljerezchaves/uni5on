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

namespace ns3 {

class BackhaulController;

/**
 * \ingroup svelte
 * \defgroup svelteInfra Infrastructure
 * SVELTE architecture infrastructure.
 */

/**
 * \ingroup svelteBackhaul
 * This is the abstract base class for the OpenFlow backhaul network, which
 * should be extended in accordance to the desired backhaul network topology.
 * SVELTE EPC entities (eNB, S-GW, and P-GW) are connected to the OpenFlow
 * switches through CSMA devices.
 */
class BackhaulNetwork : public Object
{
public:
  BackhaulNetwork ();          //!< Default constructor.
  virtual ~BackhaulNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set an attribute for ns3::OFSwitch13Device factory.
   * \param n1 The name of the attribute to set.
   * \param v1 The value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * Enable PCAP traces on the OpenFlow backhaul network (user and control
   * planes), and on LTE EPC devices of S1, S5, and X2 interfaces.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Attach the eNB node to the OpenFlow backhaul network.
   * \param enbNode The eNB node.
   * \param cellId The eNB cell ID.
   * \return The eNB S1-U IP address.
   */
  virtual Ipv4Address AttachEnb (Ptr<Node> enbNode, uint16_t cellId);

  static const uint16_t         m_gtpuPort;         //!< GTP-U UDP port.
  static const Ipv4Address      m_s1uAddr;          //!< S1-U network address.
  static const Ipv4Address      m_s5Addr;           //!< S5 network address.
  static const Ipv4Address      m_x2Addr;           //!< X2 network address.
  static const Ipv4Mask         m_s1uMask;          //!< S1-U network mask.
  static const Ipv4Mask         m_s5Mask;           //!< S5 network mask.
  static const Ipv4Mask         m_x2Mask;           //!< X2 network mask.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * \name Topology methods.
   * These virtual methods must be implemented by subclasses, as they
   * are dependent on the OpenFlow backhaul network topology.
   */
  //\{
  /**
   * Create the controller application and switch devices for the OpenFlow
   * backhaul network, connecting them accordingly to the desired topology.
   */
  virtual void TopologyCreate (void) = 0;

  /**
   * Get the switch datapath ID at which the P-GW node should be connected.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetPgwSwitch (void) = 0;

  /**
   * Get the switch datapath ID at which the given eNB should be connected.
   * \param cellId The eNB cell ID.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetEnbSwitch (uint16_t cellId) = 0;
  //\}

  /**
   * Install the OpenFlow backhaul controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<BackhaulController> controller);

  // Backhaul controller.
  Ptr<BackhaulController>       m_controllerApp;  //!< Controller app.
  Ptr<Node>                     m_controllerNode; //!< Controller node.

  // OpenFlow switches, helper and connection attribute.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;   //!< Switch helper.
  NodeContainer                 m_switchNodes;    //!< Switch nodes.
  OFSwitch13DeviceContainer     m_switchDevices;  //!< Switch devices.
  uint16_t                      m_linkMtu;        //!< Link MTU.

private:
  // CSMA helper and attributes for EPC interfaces.
  CsmaHelper                    m_csmaHelper;     //!< EPC connection helper.
  DataRate                      m_linkRate;       //!< EPC link data rate.
  Time                          m_linkDelay;      //!< EPC link delay.

  // IPv4 address helpers for EPC interfaces.
  Ipv4AddressHelper             m_s1uAddrHelper;  //!< S1-U address helper.
  Ipv4AddressHelper             m_s5AddrHelper;   //!< S5 address helper.
  Ipv4AddressHelper             m_x2AddrHelper;   //!< X2 address helper.

  // EPC user-plane devices.
  NetDeviceContainer            m_s1uDevices;     //!< S1-U devices.
  NetDeviceContainer            m_s5Devices;      //!< S5 devices.
  NetDeviceContainer            m_x2Devices;      //!< X2 devices.
};

} // namespace ns3
#endif  // BACKHAUL_NETWORK_H