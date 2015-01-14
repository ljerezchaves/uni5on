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
#include <ns3/csma-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class OpenFlowEpcController;

/**
 * Metadata associated to a connection between two any switches in the OpenFlow
 * network.
 */
class ConnectionInfo : public SimpleRefCount<ConnectionInfo>
{
  friend class OpenFlowEpcNetwork;
  friend class RingNetwork;
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  DataRate GetAvailableDataRate ();     //!< Get available bandwitdth 
  bool ReserveDataRate (DataRate dr);   //!< Reserve bandwitdth
  bool ReleaseDataRate (DataRate dr);   //!< Release reserved bandwitdth

protected:
  uint16_t switchIdx1;                  //!< Switch index 1
  uint16_t switchIdx2;                  //!< Switch index 2
  Ptr<OFSwitch13NetDevice> switchDev1;  //!< Switch OpenFlow device 1
  Ptr<OFSwitch13NetDevice> switchDev2;  //!< Switch OpenFlow device 2
  Ptr<CsmaNetDevice> portDev1;          //!< OpenFlow port csma device 1
  Ptr<CsmaNetDevice> portDev2;          //!< OpenFlow port csma device 1
  uint32_t portNum1;                    //!< OpenFlow port number 1
  uint32_t portNum2;                    //!< OpenFlow port number 2
  DataRate maxDataRate;                 //!< Maximum nominal bandwidth
  DataRate reservedDataRate;            //!< Reserved bandwitdth
};

/**
 * Create an OpenFlow network infrastrutcure to be used by
 * OpenFlowEpcHelper on LTE networks.
 */
class OpenFlowEpcNetwork : public Object
{

friend class OpenFlowEpcController;

public:
  OpenFlowEpcNetwork ();          //!< Default constructor
  virtual ~OpenFlowEpcNetwork (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  
  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Called by OpenFlowEpcHelper to proper connect the SgwPgw and eNBs to the
   * S1-U OpenFlow network insfrastructure.
   * \internal This method must create the NetDevice at node and assign an IPv4
   * address to it.
   * \param node The SgwPgw or eNB node pointer.
   * \param cellId The eNB cell ID.
   * \return A pointer to the NetDevice created at node.
   */
  virtual Ptr<NetDevice> AttachToS1u (Ptr<Node> node, uint16_t cellId) = 0;
  
  /**
   * Called by OpenFlowEpcHelper to proper connect the eNBs nodes to the X2
   * OpenFlow network insfrastructure.
   * \internal This method must create the NetDevice at node and assign an IPv4
   * address to it.
   * \param node The eNB node pointer.
   * \return A pointer to the NetDevice created at node.
   */
  virtual Ptr<NetDevice> AttachToX2 (Ptr<Node> node) = 0;
  
  /** 
   * Creates the OpenFlow network infrastructure with existing OpenFlow
   * Controller application.
   */
  virtual void CreateTopology () = 0;

  /** 
   * Enable pcap on switch data ports.
   * \param prefix The file prefix.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnableDataPcap (std::string prefix, bool promiscuous = false);

  /** 
   * Enable pcap on OpenFlow channel. 
   * \param prefix The file prefix.
   */
  void EnableOpenFlowPcap (std::string prefix);

  /** 
   * Enable internal ofsoftswitch13 logging.
   * \param level tring representing library logging level.
   */
  void EnableDatapathLogs (std::string level = "all");

  /**
   * Set an attribute for ns3::OFSwitch13NetDevice
   * \param n1 the name of the attribute to set.
   * \param v1 the value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /** \return The CsmaHelper used to create the OpenFlow network. */
  CsmaHelper GetCsmaHelper ();

  /** \return The NodeContainer with all OpenFlow switches nodes. */
  NodeContainer GetSwitchNodes ();

  /** \return The NetDeviceContainer with all OFSwitch13NetDevice devices. */
  NetDeviceContainer GetSwitchDevices ();

  /** \return The OpenFlow controller application. */
  Ptr<OFSwitch13Controller> GetControllerApp ();

  /** \return The OpenFlow controller node. */
  Ptr<Node> GetControllerNode ();
  
  /** \return Number of switches in the network. */
  uint16_t GetNSwitches ();

protected:
  /**
   * Get the OFSwitch13NetDevice of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13NetDevice.
   */
  Ptr<OFSwitch13NetDevice> GetSwitchDevice (uint16_t index);

  /**
   * Store the pair <node, switch index> for further use.
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The node pointer.
   */
  void RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node);

  /**
   * Store the switch index at which the gateway is connected.
   * \param switchIdx The switch index in m_ofSwitches.
   */
  void RegisterGatewayAtSwitch (uint16_t switchIdx);

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
  uint16_t GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev);
  
  /**
   * Retrieve the switch index at which the gateway is connected.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForGateway ();

  /**
   * Set the OpenFlow controller for this network.
   * \param controller The controller application.
   */ 
  void SetController (Ptr<OpenFlowEpcController> controller);
  
  Ptr<OpenFlowEpcController>  m_ofCtrlApp;      //!< Controller application.
  Ptr<Node>                   m_ofCtrlNode;     //!< Controller node.
  NodeContainer               m_ofSwitches;     //!< Switch nodes.
  NetDeviceContainer          m_ofDevices;      //!< Switch devices.
  OFSwitch13Helper            m_ofHelper;       //!< OpenFlow helper.
  CsmaHelper                  m_ofCsmaHelper;   //!< Csma helper.

private:
  uint16_t                    m_gatewaySwitch;  //!< Gateway switch index

  /** Map saving Node / Switch indexes. */
  typedef std::map<Ptr<Node>,uint16_t> NodeSwitchMap_t;  
  NodeSwitchMap_t     m_nodeSwitchMap;    //!< Registered nodes per switch idx.
};

};  // namespace ns3
#endif  // OPENFLOW_EPC_NETWORK_H
