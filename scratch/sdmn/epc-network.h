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

/**
 * \ingroup sdmn
 * This class extends the EpcHelper to create an OpenFlow EPC S5 backhaul
 * network infrastructure, where EPC S5 entities (P-GW and S-GW) are connected
 * through CSMA devices to the user-defined backhaul network. This is an
 * abstract base class which should be extended to create any desired network
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

  // Inherited from EpcHelper. These methods will be called from the LteHelper
  // to notify the EPC about topology configuration.
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
   * Enable pcap on LTE EPC network devices (S1-U and X2), and OpenFlow control
   * and user planes.
   * \param prefix The file prefix.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Set an attribute for ns3::OFSwitch13Device
   * \param n1 the name of the attribute to set.
   * \param v1 the value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \return Number of switches in the network.
   */
  uint16_t GetNSwitches (void) const;

  /**
   * Get the pointer to the Internet Web server node created by this class.
   * \return The pointer to the web node.
   */
  Ptr<Node> GetWebNode () const;

  /**
   * Get the Internet Web server IPv4 address assigned by this class.
   * \return The web server IP address.
   */
  Ipv4Address GetWebIpAddress () const;

  /**
   * Retrieve the controller node pointer.
   * \return The OpenFlow controller node.
   */
  Ptr<Node> GetControllerNode () const;

  /**
   * Retrieve the controller application pointer.
   * \return The OpenFlow controller application.
   */
  Ptr<EpcController> GetControllerApp () const;

  /**
   * Get the OFSwitch13Device of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13Device.
   */
  Ptr<OFSwitch13Device> GetSwitchDevice (uint16_t index) const;

  /**
   * Retrieve the switch index for node pointer.
   * \param node Ptr<Node> The node pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForNode (Ptr<Node> node) const;

  /**
   * Retrieve the switch index for switch device.
   * \param dev The OpenFlow device pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForDevice (Ptr<OFSwitch13Device> dev) const;

  /**
   * Retrieve the switch index at which the P-GW is connected.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetGatewaySwitchIdx () const;

  /**
   * BoolTracedCallback signature for topology creation completed.
   * \param devices The NetDeviceContainer for OpenFlow switch devices.
   */
  typedef void (*TopologyTracedCallback)(NetDeviceContainer devices);

  /** Default GTP-U UDP port for tunnel sockets */
  static const uint16_t m_gtpuPort;

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

  /** Creates the OpenFlow network infrastructure topology with controller. */
  virtual void TopologyCreate () = 0;

  /**
   * Get the switch index in the backhaul network topology to attach the P-GW.
   * \return The switch index.
   */
  virtual uint16_t TopologyGetPgwIndex () = 0;

  /**
   * Get the switch index in the backhaul network topology to attach the given
   * eNB.
   * \param cellId The eNB cell id.
   * \return The switch index.
   */
  virtual uint16_t TopologyGetEnbIndex (uint16_t cellId) = 0;

  /**
   * Install the EPC controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<EpcController> controller);

  /** New connection between two switches trace source. */
  TracedCallback<Ptr<ConnectionInfo> > m_newConnTrace;

  /** Connections between switches finished trace source. */
  TracedCallback<OFSwitch13DeviceContainer> m_topoBuiltTrace;

  // Member variables used by derived topology classes
  NodeContainer                   m_ofSwitches;       //!< Switch nodes.
  OFSwitch13DeviceContainer       m_ofDevices;        //!< Switch devices.
  Ptr<OFSwitch13Helper>           m_ofSwitchHelper;   //!< Switch helper.
  uint16_t                        m_linkMtu;          //!< Link mtu.
  Ptr<EpcController>              m_epcCtrlApp;       //!< EPC controller app.

private:
  /**
   * Store the pair <node, switch index> for further use.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Store the switch index at which the P-GW is connected.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * This method will create the P-GW node, attach it to the backhaul network
   * and configure it as an OpenFlow switch. It will also create the Internet
   * web server and connect to the P-GW.
   */
  void ConfigureGatewayAndInternet ();

  // Connection attributes and helper
  CsmaHelper                      m_csmaHelper;       //!< Connection helper.
  DataRate                        m_linkRate;         //!< Link data rate.
  Time                            m_linkDelay;        //!< Link delay.

  Ptr<Node>                       m_epcCtrlNode;      //!< EPC controller node.

  // EPC user-plane device
  NetDeviceContainer              m_x2Devices;        //!< X2 devices.
  NetDeviceContainer              m_s5Devices;        //!< S5 devices.
  NetDeviceContainer              m_sgiDevices;       //!< SGi devices.

  // IP addresses
  Ipv4Address                     m_ueNetworkAddr;    //!< UE network address.
  Ipv4Address                     m_s5NetworkAddr;    //!< S5 network address.
  Ipv4Address                     m_x2NetworkAddr;    //!< X2 network address.
  Ipv4Address                     m_webNetworkAddr;   //!< Web network address.

  Ipv4AddressHelper               m_ueAddrHelper;     //!< UE address helper.
  Ipv4AddressHelper               m_s5AddrHelper;     //!< S5 address helper.
  Ipv4AddressHelper               m_x2AddrHelper;     //!< X2 address helper.
  Ipv4AddressHelper               m_webAddrHelper;    //!< Web address helper.

  // P-GW
  Ptr<Node>                       m_pgwNode;          //!< P-GW node.
  Ptr<OFSwitch13Device>           m_pgwSwitchDev;     //!< P-GW switch device.
  uint16_t                        m_pgwSwIdx;         //!< P-GW switch index.
  Ipv4Address                     m_pgwSgiAddr;       //!< P-GW SGi IP addr.
  Ipv4Address                     m_pgwS5Addr;        //!< P-GW S5 IP addr.
  Ipv4Address                     m_pgwGwAddr;        //!< P-GW gateway addr.

  // Internet network (web server)
  Ptr<Node>                       m_webNode;          //!< Web server node.
  Ipv4Address                     m_webSgiIpAddr;     //!< Web server IP addr.
  // Statistics
  Ptr<BackhaulStatsCalculator>    m_epcStats;         //!< Backhaul statistics.

  /** Map saving Node / Switch indexes. */
  typedef std::map<Ptr<Node>,uint16_t> NodeSwitchMap_t;
  NodeSwitchMap_t     m_nodeSwitchMap;  //!< Registered nodes per switch idx.


  // FIXME
  // Colocando aqui todas as coisas do sdmn-epc-heper... organizar depois.
  //
public:
  /** FIXME Isso aqui vai pro controlador!
   * Get a pointer to the MME element.
   * \return The MME element.
   */
  Ptr<EpcMme> GetMmeElement ();

private:
  /**
   * Retrieve the S-GW IP address at the S1-U interface.
   * \return The Ipv4Addrress of the S-GW S1-U interface.
   */
  virtual Ipv4Address GetSgwS1uAddress ();

  /**
   * Retrieve the S-GW IP address at the S5 interface.
   * \return The Ipv4Addrress of the S-GW S5 interface.
   */
  virtual Ipv4Address GetSgwS5Address ();

  /**
   * Retrieve the IP address for device, set by the OpenFlow network.
   * \param device NetDevice connected to the OpenFlow network.
   * \return The Ipv4Addrress of the EPC interface for the specific NetDevice
   * connected to the OpenFlow network.
   */
  virtual Ipv4Address GetAddressForDevice (Ptr<NetDevice> device);
};

};  // namespace ns3
#endif  // EPC_NETWORK_H
