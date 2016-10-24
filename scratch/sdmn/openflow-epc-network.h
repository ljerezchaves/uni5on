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

#ifndef OPENFLOW_EPC_NETWORK_H
#define OPENFLOW_EPC_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class ConnectionInfo;
class InternetNetwork;
class LinkQueuesStatsCalculator;
class NetworkStatsCalculator;
class OpenFlowEpcController;
class SdmnEpcHelper;

/**
 * \ingroup epcof
 * Create an OpenFlow EPC S1-U network infrastructure. This is an abstract base
 * class which should be extended to create any desired network topology. For
 * each subclass, a corresponding topology-aware controller must be
 * implemented, extending the generig OpenFlowEpcController.
 */
class OpenFlowEpcNetwork : public Object
{
public:
  OpenFlowEpcNetwork ();          //!< Default constructor
  virtual ~OpenFlowEpcNetwork (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

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
   * Retrieve the gateway node pointer.
   * \return The gateway node pointer.
   */
  Ptr<Node> GetGatewayNode ();

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
  Ptr<OpenFlowEpcController> GetControllerApp ();

  /**
   * Retrieve the OpenFlow EPC helper used for LTE configuration.
   * \return The OpenFlow EPC helper.
   */
  Ptr<SdmnEpcHelper> GetEpcHelper ();

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
  void InstallController (Ptr<OpenFlowEpcController> controller);

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
  Ptr<Node>                       m_ofCtrlNode;     //!< Controller node.
  Ptr<OpenFlowEpcController>      m_ofCtrlApp;      //!< Controller app.
  Ptr<SdmnEpcHelper>              m_ofEpcHelper;    //!< Helper for LTE EPC.
  Ptr<InternetNetwork>            m_webNetwork;     //!< Internet network.
  Ptr<NetworkStatsCalculator>     m_networkStats;   //!< Network statistics.

  /** Map saving Node / Switch indexes. */
  typedef std::map<Ptr<Node>,uint16_t> NodeSwitchMap_t;
  NodeSwitchMap_t     m_nodeSwitchMap;    //!< Registered nodes per switch idx.
};

};  // namespace ns3
#endif  // OPENFLOW_EPC_NETWORK_H
