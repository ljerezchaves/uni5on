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
class BackhaulStatsCalculator;
class EpcController;
class SdranCloud;

/**
 * \ingroup sdmn
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
  EpcNetwork ();          //!< Default constructor
  virtual ~EpcNetwork (); //!< Dummy destructor, see DoDispose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \return The number of switches in the backhaul network. */
  uint16_t GetNSwitches (void) const;

  /** \return The pointer to the Internet web node. */
  Ptr<Node> GetWebNode (void) const;

  /** \return The Internet web server IP address. */
  Ipv4Address GetWebIpAddress (void) const;

  /** \return The pointer to the OpenFlow EPC controller node. */
  Ptr<Node> GetControllerNode (void) const;

  /** \return The pointer to the OpenFlow EPC controller application. */
  Ptr<EpcController> GetControllerApp (void) const;

  /**
   * Get the OpenFlow switch node for a given OpenFlow switch datapath ID.
   * \param dpId The switch datapath ID.
   * \return The pointer to the switch node.
   */
  Ptr<Node> GetSwitchNode (uint64_t dpId) const;

  /**
   * Get the switch datapath ID for a given node attached to the network.
   * \param node The pointer to the node.
   * \return The switch datapath ID.
   */
  uint64_t GetDpIdForAttachedNode (Ptr<Node> node) const;

  /**
   * Set an attribute for ns3::OFSwitch13Device factory.
   * \param n1 The name of the attribute to set.
   * \param v1 The value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * Enable PCAP traces on LTE EPC network devices, OpenFlow channel
   * (control-plane) and switch ports (data-plane).
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Configure and connect the S-GW node from the SDRAN cloud to the X5
   * interface over the backhaul network infrastructure.
   * \param sdranCloud The SDRAN cloud pointer.
   */
  virtual void AddSdranCloud (Ptr<SdranCloud> sdranCloud);

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

  // FIXME The following attach functions will be moved to the SDRAN cloud.
  /**
   * Connect the eNBs to the S1-U interface over the backhaul network
   * infrastructure.
   * \param node The eNB node pointer.
   * \param cellId The eNB cell ID.
   * \return A pointer to the device created at the eNB.
   */
  Ptr<NetDevice> S1EnbAttach (Ptr<Node> node, uint16_t cellId);

  /**
   * Connect the eNBs nodes to the X2 interface over the backhaul network
   * infrastructure.
   * \param node The 1st eNB node pointer.
   * \param node The 2nd eNB node pointer.
   * \return The container with devices created at each eNB.
   */
  NetDeviceContainer X2Attach (Ptr<Node> enb1, Ptr<Node> enb2);

  /**
   * TracedCallback signature for topology creation completed.
   * \param devices The container of OpenFlow switch devices.
   */
  typedef void (*TopologyTracedCallback)(NetDeviceContainer devices);

  /** Default GTP-U UDP port for tunnel sockets. */
  static const uint16_t m_gtpuPort;

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Create the OpenFlow EPC controller application and switch devices for the
   * OpenFlow network infrastructure, connecting them accordingly to the
   * desired topology.
   */
  virtual void TopologyCreate () = 0;

  /**
   * Get the switch datapath ID at which the P-GW node should be connected.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetPgwSwitch () = 0;

  /**
   * Get the switch datapath ID at which the given eNB should be connected.
   * \param cellId The eNB cell id.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetEnbSwitch (uint16_t cellId) = 0;

  /**
   * Install the OpenFlow EPC controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<EpcController> controller);

  /** Trace source for new connection between two switches. */
  TracedCallback<Ptr<ConnectionInfo> > m_newConnTrace;

  /** Trace source for connections between switches finished. */
  TracedCallback<OFSwitch13DeviceContainer> m_topoBuiltTrace;

  // EPC controller
  Ptr<EpcController>            m_epcCtrlApp;       //!< EPC controller app.
  Ptr<Node>                     m_epcCtrlNode;      //!< EPC controller node.

  // OpenFlow switches, helper and connection attribute
  NodeContainer                 m_ofSwitches;       //!< Switch nodes.
  OFSwitch13DeviceContainer     m_ofDevices;        //!< Switch devices.
  Ptr<OFSwitch13InternalHelper> m_ofSwitchHelper;   //!< Switch helper.
  uint16_t                      m_linkMtu;          //!< Link MTU.

private:
  /**
   * Save the pair node / switch datapath ID.
   * \param dpId The switch datapath ID.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterAttachToSwitch (uint64_t dpId, Ptr<Node> node);

  /**
   * Create the P-GW node, configure it as an OpenFlow switch and attach it to
   * the backhaul network infrastructure vir S5 interface. Then, create the
   * Internet web server node and connect it to the P-GW via SGi interface.
   */
  void ConfigurePgwAndInternet ();

  /**
   * Get a pointer to the MME element. FIXME
   * \return The MME element.
   */
  Ptr<EpcMme> GetMmeElement ();

  /**
   * Retrieve the S-GW IP address at the S1-U interface. FIXME
   * \return The S-GW IP address at S1-U interface.
   */
  virtual Ipv4Address GetSgwS1uAddress ();

  /**
   * Get the IP address for a given device.
   * \param device The network device.
   * \return The IP address assigned to this device.
   */
  Ipv4Address GetAddressForDevice (Ptr<NetDevice> device);

  // Helper and connection attributes
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  DataRate                      m_linkRate;         //!< Link data rate.
  Time                          m_linkDelay;        //!< Link delay.

  // EPC user-plane devices
  NetDeviceContainer            m_x2Devices;        //!< X2 devices.
  NetDeviceContainer            m_s5Devices;        //!< S5 devices.
  NetDeviceContainer            m_sgiDevices;       //!< SGi devices.

  // IP addresses
  Ipv4Address                   m_ueNetworkAddr;    //!< UE network address.
  Ipv4Address                   m_s5NetworkAddr;    //!< S5 network address.
  Ipv4Address                   m_x2NetworkAddr;    //!< X2 network address.
  Ipv4Address                   m_webNetworkAddr;   //!< Web network address.

  // IP address helpers
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.
  Ipv4AddressHelper             m_s5AddrHelper;     //!< S5 address helper.
  Ipv4AddressHelper             m_x2AddrHelper;     //!< X2 address helper.
  Ipv4AddressHelper             m_webAddrHelper;    //!< Web address helper.

  // P-GW
  Ptr<Node>                     m_pgwNode;          //!< P-GW node.
  Ptr<OFSwitch13Device>         m_pgwSwitchDev;     //!< P-GW switch device.
  Ipv4Address                   m_pgwSgiAddr;       //!< P-GW SGi IP addr.
  Ipv4Address                   m_pgwS5Addr;        //!< P-GW S5 IP addr.
  Ipv4Address                   m_pgwGwAddr;        //!< P-GW gateway addr.

  // Internet web server
  Ptr<Node>                     m_webNode;          //!< Web server node.
  Ipv4Address                   m_webSgiIpAddr;     //!< Web server IP addr.

  // Statistics calculator
  Ptr<BackhaulStatsCalculator>  m_epcStats;         //!< Backhaul statistics.

  /** Map saving node / switch datapath ID. */
  typedef std::map<Ptr<Node>, uint64_t> NodeSwitchMap_t;
  NodeSwitchMap_t m_nodeSwitchMap;  //!< Registered nodes by switch ID.
};

};  // namespace ns3
#endif  // EPC_NETWORK_H
