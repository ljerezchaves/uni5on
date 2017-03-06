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

  /**
   * Get The number of OpenFlow switches in the backhaul network.
   * \return The number of switches.
   */
  uint16_t GetNSwitches (void) const;

  /**
   * Get the Internet web server node.
   * \return The Internet node.
   */
  Ptr<Node> GetWebNode (void) const;

  /**
   * Get the Internet web server IP address.
   * \return The IP address.
   */
  Ipv4Address GetWebIpAddress (void) const;

  /**
   * Get the OpenFlow EPC controller node.
   * \return The controller node pointer.
   */
  Ptr<Node> GetControllerNode (void) const;

  /**
   * Get the OpenFlow EPC controller application.
   * \return The controller application pointer.
   */
  Ptr<EpcController> GetControllerApp (void) const;

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

  /**
   * Notify this helper that all SDRAN cloud objects are already connected to
   * the OpenFlow backhaul infrastructure via the S5 interface.
   */
  void SdranCloudsDone (void);

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
   * \param sgwDev The P-GW OpenFlow switch device.
   * \return The switch datapath ID.
   */
  virtual uint64_t TopologyGetPgwSwitch (Ptr<OFSwitch13Device> pgwDev) = 0;

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
   * Create the P-GW node, configure it as an OpenFlow switch and attach it to
   * the backhaul network infrastructure via S5 interface. Then, create the
   * Internet web server node and connect it to the P-GW via SGi interface.
   */
  void ConfigurePgwAndInternet ();

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
  Ipv4Address                   m_webSgiAddr;       //!< Web server IP addr.

  // Statistics calculator
  Ptr<BackhaulStatsCalculator>  m_epcStats;         //!< Backhaul statistics.
};

};  // namespace ns3
#endif  // EPC_NETWORK_H
