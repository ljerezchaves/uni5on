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
class InternetNetwork;
class LinkQueuesStatsCalculator;
class NetworkStatsCalculator;
class EpcController;

/**
 * \ingroup epcof
 * Create an OpenFlow EPC S1-U network infrastructure. This is an abstract base
 * class which should be extended to create any desired network topology. For
 * each subclass, a corresponding topology-aware controller must be
 * implemented, extending the generig EpcController.
 */
/**
 * Create an EPC network connected through CSMA devices to an user-defined
 * backhaul network. This Helper will create an EPC network topology comprising
 * of a single node that implements both the SGW and PGW functionality, and an
 * MME node. The S1 and X2 interfaces are realized over CSMA devices
 * connected to an user-defined backhaul network.
 */
class EpcNetwork : public EpcHelper
{
public:
  EpcNetwork ();          //!< Default constructor
  virtual ~EpcNetwork (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  // Inherited from EpcHelper
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
   * Called by SdmnEpcHelper to proper connect the SgwPgw and eNBs to the
   * S1-U interface over the OpenFlow network infrastructure.
   * \param node The SgwPgw/eNB node pointer.
   * \param cellId The eNB cell ID (0 for SgwPgw node).
   * \return A pointer to the device created at node.
   */
  virtual Ptr<NetDevice> S1Attach (Ptr<Node> node, uint16_t cellId) = 0;

  /**
   * Called by SdmnEpcHelper to proper connect the eNBs nodes to the X2
   * interface over the OpenFlow network infrastructure.
   * \param node The 1st eNB node pointer.
   * \param node The 2nd eNB node pointer.
   * \return The container with devices created at each eNB.
   */
  virtual NetDeviceContainer X2Attach (Ptr<Node> enb1, Ptr<Node> enb2) = 0;

  /**
   * Enable pcap on LTE EPC network, and OpenFlow control and user planes.
   * \param prefix The file prefix.
   * \param promiscuous If true, enable promisc trace.
   */
  virtual void EnablePcap (std::string prefix, bool promiscuous = false);

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
   * Get the pointer to the Internet server node created by the topology.
   * \return The pointer to the server node.
   */
  Ptr<Node> GetServerNode ();

  /**
   * Retrieve the controller node pointer.
   * \return The OpenFlow controller node.
   */
  Ptr<Node> GetControllerNode ();

  /**
   * Retrieve the controller application pointer.
   * \return The OpenFlow controller application.
   */
  Ptr<EpcController> GetControllerApp ();

  /**
   * BoolTracedCallback signature for topology creation completed.
   * \param devices The NetDeviceContainer for OpenFlow switch devices.
   */
  typedef void (*TopologyTracedCallback)(NetDeviceContainer devices);

  /**
  * AttachTracedCallback signature for new  EPC entity connected to OpenFlow
  * network.
  * \attention This nodeDev is not the one added as port to switch. Instead,
  * this is the 'other' end of this connection, associated with the EPC eNB or
  * SgwPgw node.
  * \param nodeDev The device connected to the OpenFlow switch.
  * \param nodeIp The IPv4 address assigned to this device.
  * \param swtchDev The OpenFlow switch device.
  * \param swtchIdx The OpenFlow switch index.
  * \param swtchPort The port number for nodeDev at OpenFlow switch.
  */
  typedef void (*AttachTracedCallback)(
    Ptr<NetDevice> nodeDev, Ipv4Address nodeIp, Ptr<OFSwitch13Device> swtchDev,
    uint16_t swtchIdx, uint32_t swtchPort);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

  /** Creates the OpenFlow network infrastructure topology with controller. */
  virtual void CreateTopology () = 0;

  /**
   * Store the pair <node, switch index> for further use.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Store the switch index at which the gateway is connected.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Get the OFSwitch13Device of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13Device.
   */
  Ptr<OFSwitch13Device> GetSwitchDevice (uint16_t index);

  /**
   * Retrieve the switch index for node pointer.
   * \param node Ptr<Node> The node pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForNode (Ptr<Node> node);

  /**
   * Retrieve the switch index for switch device.
   * \param dev The OpenFlow device pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForDevice (Ptr<OFSwitch13Device> dev);

  /**
   * Retrieve the switch index at which the gateway is connected.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetGatewaySwitchIdx ();

  /**
   * Install the OpenFlow controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<EpcController> controller);

  NodeContainer                  m_ofSwitches;     //!< Switch nodes.
  OFSwitch13DeviceContainer      m_ofDevices;      //!< Switch devices.
  Ptr<OFSwitch13Helper>          m_ofSwitchHelper; //!< OpenFlow switch helper.
  Ptr<LinkQueuesStatsCalculator> m_gatewayStats;   //!< Gateway statistics.

  /** New connection between two switches trace source. */
  TracedCallback<Ptr<ConnectionInfo> > m_newConnTrace;

  /** Connections between switches finished trace source. */
  TracedCallback<OFSwitch13DeviceContainer> m_topoBuiltTrace;

  /** New EPC entity connected to OpenFlow network trace source. */
  TracedCallback<Ptr<NetDevice>, Ipv4Address, Ptr<OFSwitch13Device>,
                 uint16_t, uint32_t> m_newAttachTrace;

private:
  uint16_t                        m_gatewaySwitch;  //!< Gateway switch index.
  Ptr<Node>                       m_controllerNode; //!< Controller node.
  Ptr<EpcController>              m_controllerApp;  //!< Controller app.
  Ptr<InternetNetwork>            m_webNetwork;     //!< Internet network.
  Ptr<NetworkStatsCalculator>     m_networkStats;   //!< Network statistics.
  
  /** Map saving Node / Switch indexes. */
  typedef std::map<Ptr<Node>,uint16_t> NodeSwitchMap_t;
  NodeSwitchMap_t     m_nodeSwitchMap;    //!< Registered nodes per switch idx.


  //
  // Colocando aqui todas as coisas do sdmn-epc-heper... organizar depois.
  //

public:
    /**
   * Get a pointer to the MME element.
   * \return The MME element.
   */
  Ptr<EpcMme> GetMmeElement ();


  void ConfigurePgw ();

  /**
    * Enable Pcap output on all S1-U devices connected to the backhaul network.
    * \param prefix Filename prefix to use for pcap files.
    * \param promiscuous If true capture all packets available at the devices.
    * \param explicitFilename Treat the prefix as an explicit filename if true.
    */
  void EnablePcapS1u (std::string prefix, bool promiscuous = false,
                      bool explicitFilename = false);

  /**
    * Enable Pcap output on all X2 devices connected to the backhaul network.
    * \param prefix Filename prefix to use for pcap files.
    * \param promiscuous If true capture all packets available at the devices.
    * \param explicitFilename Treat the prefix as an explicit filename if true.
    */
  void EnablePcapX2 (std::string prefix, bool promiscuous = false,
                     bool explicitFilename = false);

  /**
   * S1-U attach callback signature.
   * \param Ptr<Node> The EPC node to attach to the S1-U backhaul network.
   * \param uint16_t The eNB cell ID (0 for SgwPgw node).
   * \returns Ptr<NetDevice> the device created at the node.
   */
  typedef Callback <Ptr<NetDevice>, Ptr<Node>, uint16_t > S1uConnectCallback_t;

  /**
   * X2 attach callback signature.
   * \param Ptr<Node> The 1st eNB node.
   * \param Ptr<Node> The 2nd eNB node.
   * \returns NetDeviceContainer The devices created at each eNB.
   */
  typedef Callback <NetDeviceContainer, Ptr<Node>, Ptr<Node> > X2ConnectCallback_t;

  /**
    * Specify the callback to allow the user to proper connect the EPC nodes
    * (SgwPgw and eNBs) to the S1-U interface over the backhaul network.
    * \param cb Callback invoked during AddEnb procedure to proper connect the
    * eNB node to the backhaul network. This is also called by
    * SetS1uConnectCallback to proper connect the SgwPgw node to the backhaul
    * network.
    */
  void SetS1uConnectCallback (S1uConnectCallback_t cb);

  /**
    * Specify the callback to allow the user to proper connect two eNB nodes to
    * the X2 interface over the backhaul network.
    * \param cb Callback invoked during AddX2Interface procedure to proper
    * connect the pair of eNB nodes to the backhaul network.
    */
  void SetX2ConnectCallback (X2ConnectCallback_t cb);

private:
  /**
   * Retrieve the SgwPgw IP address, set by the OpenFlow network.
   * \return The SgwPgw Ipv4Addrress of the S1-U interface connected to the
   * OpenFlow network.
   */
  virtual Ipv4Address GetSgwS1uAddress ();

  /**
   * Retrieve the eNB IP address for device, set by the OpenFlow network.
   * \param device NetDevice connected to the OpenFlow network.
   * \return The Ipv4Addrress of the S1-U or X2 interface for the specific
   * NetDevice connected to the OpenFlow network.
   */
  virtual Ipv4Address GetAddressForDevice (Ptr<NetDevice> device);

  /** Callback to connect nodes to S1-U OpenFlow network. */
  S1uConnectCallback_t m_s1uConnect;

  /** Callback to connect nodes to X2 OpenFlow network. */
  X2ConnectCallback_t m_x2Connect;

  /** A collection of S1-U NetDevice */
  NetDeviceContainer m_s1uDevices;

  /** A collection of X2 NetDevice */
  NetDeviceContainer m_x2Devices;

  /** The SgwPgw NetDevice connected to the S1-U OpenFlow network switch */
  Ptr<NetDevice> m_sgwS1uDev;

  /** SgwPgw network element */
  Ptr<Node> m_sgwPgw;

  /** SgwPgw application */
  Ptr<EpcSgwPgwCtrlApplication> m_sgwPgwCtrlApp;
  Ptr<EpcSgwPgwUserApplication> m_sgwPgwUserApp;

  /** MME element */
  Ptr<EpcMme> m_mme;

  /** VirtualNetDevice for GTP tunneling implementation */
  Ptr<VirtualNetDevice> m_tunDevice;

  /** Helper to assign addresses to UE devices as well as to the TUN device of the SGW/PGW */
  Ipv4AddressHelper m_ueAddressHelper;

  /** UDP port where the GTP-U Socket is bound, fixed by the standard as 2152 */
  uint16_t m_gtpuUdpPort;

  /** Map storing for each IMSI the corresponding eNB NetDevice */
  std::map<uint64_t, Ptr<NetDevice> > m_imsiEnbDeviceMap;


};

};  // namespace ns3
#endif  // EPC_NETWORK_H
