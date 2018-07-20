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
 * Enumeration of LTE logical interfaces.
 */
typedef enum
{
  S1U  = 0,   //!< S1-U interface connecting eNB to S-GW.
  S5   = 1,   //!< S5 interface connecting S-GW to P-GW.
  X2   = 2,   //!< X2 interface connecting eNB to eNB.
  SGI  = 3    //!< SGi interface connecting P-GW to Internet.
} LteInterface;

/**
 * Get the LTE logical interface name.
 * \param iface The LTE logical interface.
 * \return The string with the interface name.
 */
std::string LteInterfaceStr (LteInterface iface);

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
public:
  BackhaulNetwork ();          //!< Default constructor.
  virtual ~BackhaulNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces on the OpenFlow backhaul network (user and control
   * planes), and on LTE EPC devices of S1, S5, and X2 interfaces.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Attach the EPC node to the OpenFlow backhaul network.
   * \param epcNode The eNB node.
   * \param swIdx The switch index at which the EPC node should be connected.
   * \param iface The LTE logical interface for this connection.
   * \return The network device created at the EPC node.
   */
  virtual Ptr<CsmaNetDevice> AttachEpcNode (Ptr<Node> enbNode, uint16_t swIdx,
                                            LteInterface iface);

  /**
   * Set the devices names identifying the connection between the nodes.
   * \param srcDev The network device in the source node.
   * \param dstDev The network device in the destination node.
   * \param desc The string describing this connection.
   */
  static void SetDeviceNames (Ptr<NetDevice> srcDev, Ptr<NetDevice> dstDev,
                              std::string desc);

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

private:
  // CSMA helper and attributes for EPC interfaces.
  CsmaHelper                    m_csmaHelper;     //!< EPC connection helper.
  DataRate                      m_linkRate;       //!< EPC link data rate.
  Time                          m_linkDelay;      //!< EPC link delay.

  // IPv4 address helpers for EPC interfaces.
  Ipv4AddressHelper             m_s1uAddrHelper;  //!< S1-U address helper.
  Ipv4AddressHelper             m_s5AddrHelper;   //!< S5 address helper.
  Ipv4AddressHelper             m_x2AddrHelper;   //!< X2 address helper.
  NetDeviceContainer            m_epcDevices;     //!< EPC devices.
};

} // namespace ns3
#endif  // BACKHAUL_NETWORK_H
