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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef TRANSPORT_NETWORK_H
#define TRANSPORT_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class TransportController;
class SwitchHelper;
class EnbInfo;

/**
 * \ingroup uni5onInfra
 * Abstract base class for the OpenFlow transport network, which should be
 * extended to configure the desired network topology.
 */
class TransportNetwork : public Object
{
public:
  TransportNetwork ();          //!< Default constructor.
  virtual ~TransportNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces files on the OpenFlow transport network.
   * \param prefix Filename prefix to use for PCAP files.
   * \param promiscuous If true, enable PCAP promiscuous traces.
   * \param ofchannel If true, enable PCAP on OpenFlow transport channels.
   * \param epcDevices If true, enable PCAP on EPC S1, S5, and X2 interfaces.
   * \param swtDevices If true, enable PCAP on transport switches.
   */
  void EnablePcap (std::string prefix, bool promiscuous, bool ofchannel,
                   bool epcDevices, bool swtDevices);

  /**
   * Attach the EPC node to the OpenFlow transport network.
   * \param epcNode The EPC node.
   * \param swIdx The switch index at which the EPC node should be connected.
   * \param iface The logical interface for this connection.
   * \param ifaceStr Custom name for this logical interface. When this
   *        string is empty, the default name is used.
   * \return The pair with the network device created at the EPC node and the
   *         port device create at the transport switch.
   */
  virtual std::pair<Ptr<CsmaNetDevice>, Ptr<OFSwitch13Port>>
  AttachEpcNode (Ptr<Node> epcNode, uint16_t swIdx, EpsIface iface,
                 std::string ifaceStr = std::string ());

  /**
   * Interconnect OpenFlow switches and controllers.
   */
  void CreateOpenFlowChannels (void);

  /**
   * Configure this eNB as an OpenFlow switch and connect it to the transport
   * network via S1-U interface.
   * \param enbNode The eNB node.
   * \param cellId Tne eNB cell ID.
   * \return The virtual device for communication between eNB application and
   *         the OpenFlow switch.
   */
  virtual Ptr<VirtualNetDevice> ConfigureEnb (Ptr<Node> enbNode,
                                              uint16_t cellId);

  /**
   * Get the transport switch index at which the given eNB should be connected.
   * \param cellId The eNB cell ID.
   * \return The transport switch index.
   */
  virtual uint16_t GetEnbSwIdx (uint16_t cellId) const = 0;

  /**
   * Get the total number of OpenFlow switches in the transport network.
   * \return The number of OpenFlow switches.
   */
  uint32_t GetNSwitches (void) const;

  /**
   * Get the OpenFlow transport network controller.
   * \return The OpenFlow controller.
   */
  Ptr<TransportController> GetControllerApp (void) const;

  // Network addresses.
  static const Ipv4Address      m_s1Addr;         //!< S1-U network address.
  static const Ipv4Address      m_s5Addr;         //!< S5 network address.
  static const Ipv4Address      m_x2Addr;         //!< X2 network address.
  static const Ipv4Mask         m_s1Mask;         //!< S1-U network mask.
  static const Ipv4Mask         m_s5Mask;         //!< S5 network mask.
  static const Ipv4Mask         m_x2Mask;         //!< X2 network mask.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Create the controller application and switch devices for the OpenFlow
   * transport network, connecting them accordingly to the desired topology.
   */
  virtual void CreateTopology (void) = 0;

  // Transport network controller.
  Ptr<TransportController>      m_controllerApp;  //!< Controller app.
  Ptr<Node>                     m_controllerNode; //!< Controller node.

  // OpenFlow switches and helper.
  Ptr<SwitchHelper>             m_switchHelper;   //!< Switch helper.
  NodeContainer                 m_switchNodes;    //!< Transport switch nodes.
  OFSwitch13DeviceContainer     m_switchDevices;  //!< Transport switch devices.
  OFSwitch13DeviceContainer     m_enbDevices;     //!< eNB switch devices.

  // CSMA helper and attributes for transport links.
  CsmaHelper                    m_csmaHelper;     //!< Connection helper.
  DataRate                      m_linkRate;       //!< Link data rate.
  Time                          m_linkDelay;      //!< Link delay.
  uint16_t                      m_linkMtu;        //!< Link MTU.

private:
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
#endif  // TRANSPORT_NETWORK_H
