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

#ifndef EPC_NETWORK_H
#define EPC_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

class ConnectionInfo;
class EpcController;
class SdranCloud;

/**
 * \ingroup sdmn
 * \defgroup sdmnEpc EPC
 * Software-Defined EPC for the SDMN architecture.
 */

/**
 * \ingroup sdmnEpc
 * This class extends the EpcHelper to create an OpenFlow EPC S5 backhaul
 * network infrastructure, where EPC S5 entities (P-GW and S-GW) are connected
 * through CSMA devices to the OpenFlow backhaul network. This is an abstract
 * base class which should be extended to create any desired backhaul network
 * topology. For each subclass, a corresponding topology-aware controller must
 * be implemented, extending the generig EpcController.
 */
class EpcNetwork : public EpcHelper
{
public:
  EpcNetwork ();          //!< Default constructor.
  virtual ~EpcNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the Internet web server node.
   * \return The pointer to the web node.
   */
  Ptr<Node> GetWebNode (void) const;

  /**
   * Get the OpenFlow switch node for a given OpenFlow switch datapath ID.
   * \param dpId The switch datapath ID.
   * \return The pointer to the switch node.
   */
  Ptr<Node> GetSwitchNode (uint64_t dpId) const;

  /**
   * Set an attribute for ns3::OFSwitch13Device factory.
   * \param n1 The name of the attribute to set.
   * \param v1 The value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * Enable PCAP traces on the OpenFlow backhaul network (user and control
   * planes), and on LTE EPC devices of S5, SGi and X2 interfaces.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Configure and connect the S-GW node from the SDRAN cloud to the X5
   * interface over the backhaul network infrastructure.
   * \param sdranCloud The SDRAN cloud pointer.
   */
  virtual void AttachSdranCloud (Ptr<SdranCloud> sdranCloud);

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
   * Get the IPv4 address assigned to a given device.
   * \param device The network device.
   * \return The IPv4 address.
   */
  static Ipv4Address GetIpv4Addr (Ptr<NetDevice> device);

  /**
   * Get the IPv4 mask assigned to a given device.
   * \param device The network device.
   * \return The IPv4 mask.
   */
  static Ipv4Mask GetIpv4Mask (Ptr<NetDevice> device);

  static const uint16_t     m_gtpuPort;  //!< GTP-U UDP port.
  static const Ipv4Address  m_ueAddr;    //!< UE network address.
  static const Ipv4Address  m_sgiAddr;   //!< Web network address.
  static const Ipv4Address  m_s5Addr;    //!< S5 network address.
  static const Ipv4Address  m_s1uAddr;   //!< S1-U network address.
  static const Ipv4Address  m_x2Addr;    //!< X2 network address.
  static const Ipv4Mask     m_ueMask;    //!< UE network mask.
  static const Ipv4Mask     m_sgiMask;   //!< Web network mask.
  static const Ipv4Mask     m_s5Mask;    //!< S5 network mask.
  static const Ipv4Mask     m_s1uMask;   //!< S1-U network mask.
  static const Ipv4Mask     m_x2Mask;    //!< X2 network mask.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /** \name Topology methods.
   * These virtual methods must be implemented by topology subclasses, as they
   * are dependent on the backhaul OpenFlow network topology.
   */
  //\{
  /**
   * Create the OpenFlow EPC controller application and switch devices for the
   * OpenFlow network infrastructure, connecting them accordingly to the
   * desired topology.
   */
  virtual void TopologyCreate (void) = 0;

  /**
   * Get the switch datapath ID at which the P-GW node should be connected.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetPgwSwitch (void) = 0;

  /**
   * Get the switch datapath ID at which the S-GW node from the SDRAN cloud
   * should be connected.
   * \param sdran The SDRAN cloud pointer.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetSgwSwitch (Ptr<SdranCloud> sdran) = 0;

  /**
   * Get the switch datapath ID at which the given eNB should be connected.
   * \param cellId The eNB cell id.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetEnbSwitch (uint16_t cellId) = 0;
  //\}

  /**
   * Get the number of P-GW TFT switch nodes available on this topology.
   * \return The number of TFT nodes.
   */
  uint32_t GetNTftNodes (void) const;

  /**
   * Install the OpenFlow EPC controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<EpcController> controller);

  // EPC controller.
  Ptr<EpcController>            m_epcCtrlApp;       //!< EPC controller app.
  Ptr<Node>                     m_epcCtrlNode;      //!< EPC controller node.

  // OpenFlow switches, helper and connection attribute.
  NodeContainer                 m_backNodes;        //!< Backhaul nodes.
  OFSwitch13DeviceContainer     m_backOfDevices;    //!< Backhaul switch devs.
  Ptr<OFSwitch13InternalHelper> m_ofSwitchHelper;   //!< Switch helper.
  uint16_t                      m_linkMtu;          //!< Link MTU.

private:
  /**
   * Create the Internet network composed of a single node where server
   * applications will be installed.
   */
  void InternetCreate (void);

  /**
   * Create the P-GW user-plane network composed of OpenFlow switches managed
   * by the EPC controller. This function will also attach the P-GW to the S5
   * and SGi interfaces.
   */
  void PgwCreate (void);

  // Helper and attributes for S5 interface.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  DataRate                      m_s5LinkRate;       //!< Link data rate.
  Time                          m_s5LinkDelay;      //!< Link delay.

  // EPC user-plane devices.
  NetDeviceContainer            m_x2Devices;        //!< X2 devices.
  NetDeviceContainer            m_s5Devices;        //!< S5 devices.
  NetDeviceContainer            m_sgiDevices;       //!< SGi devices.

  // IP address helpers for interfaces.
  Ipv4AddressHelper             m_sgiAddrHelper;    //!< Web address helper.
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.
  Ipv4AddressHelper             m_s5AddrHelper;     //!< S5 address helper.
  Ipv4AddressHelper             m_x2AddrHelper;     //!< X2 address helper.

  // Internet web server.
  Ptr<Node>                     m_webNode;          //!< Web server node.
  DataRate                      m_sgiLinkRate;      //!< Link data rate.
  Time                          m_sgiLinkDelay;     //!< Link delay.

  // P-GW user plane.
  Ipv4Address                   m_pgwAddr;          //!< P-GW gateway addr.
  NodeContainer                 m_pgwNodes;         //!< P-GW user-plane nodes.
  OFSwitch13DeviceContainer     m_pgwOfDevices;     //!< P-GW switch devices.
  NetDeviceContainer            m_pgwIntDevices;    //!< P-GW int port devices.
  uint16_t                      m_pgwNumNodes;      //!< Num of P-GW nodes.
  DataRate                      m_tftPipeCapacity;  //!< P-GW TFT capacity.
  uint32_t                      m_tftTableSize;     //!< P-GW TFT table size.
};

};  // namespace ns3
#endif  // EPC_NETWORK_H
