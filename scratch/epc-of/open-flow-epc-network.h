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

/**
 * Create an OpenFlow network infrastrutcure to be used by
 * OpenFlowEpcHelper on LTE networks.
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
  
  /** Destructor implementation */
  virtual void DoDispose ();
 
  /**
   * Function called by OpenFlowEpcHelper to proper connect the SgwPgw
   * and eNBs to the S1-U OpenFlow network insfrastructure.
   *
   * \param node The SgwPgw or eNB node pointer.
   * \return A pointer to the NetDevice created at node.
   */
  virtual Ptr<NetDevice> AttachToS1u (Ptr<Node> node) = 0;
  
  /**
   * Function called by OpenFlowEpcHelper to proper connect the eNBs
   * nodes to the X2 OpenFlow network insfrastructure.
   *
   * \param node The eNB node pointer.
   * \return A pointer to the NetDevice created at the eNB.
   */
  virtual Ptr<NetDevice> AttachToX2 (Ptr<Node> node) = 0;
  
  /** 
   * Creates the OpenFlow network infrastructure with existing OpenFlow
   * Controller application. 
   * \param controller The OpenFlow controller application for this network.
   */
  virtual void CreateTopology (Ptr<OFSwitch13Controller> controller);

  /** 
   * Enable pcap on OpenFlow links. 
   * \param prefix The file prefix.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /** 
   * Enable pcap on OpenFlow controller channel. 
   * \param prefix The file prefix.
   */
  void EnableOpenFlowPcap (std::string prefix);

  /** 
   * \return The CsmaHelper used to create the OpenFlow network. 
   */
  CsmaHelper GetCsmaHelper ();

  /** 
   * \return The NodeContainer with all OpenFlow switches nodes. 
   */
  NodeContainer GetSwitchNodes ();

  /** 
   * \return The NetDeviceContainer with all OFSwitch13NetDevice devices. 
   */
  NetDeviceContainer GetSwitchDevices ();

  /** 
   * \return The OpenFlow controller application.
   */
  Ptr<OFSwitch13Controller> GetControllerApp ();

  /** 
   * \return The OpenFlow controller node.
   */
  Ptr<Node> GetControllerNode ();

  /**
   * Set an attribute for ns3::OFSwitch13NetDevice
   * \param n1 the name of the attribute to set.
   * \param v1 the value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

protected:

  Ptr<OFSwitch13Controller> m_ofCtrlApp;      //!< Controller application
  Ptr<Node>                 m_ofCtrlNode;     //!< Controller node
  OFSwitch13Helper          m_ofHelper;       //!< OpenFlow helper
  NodeContainer             m_ofSwitches;     //!< OpenFlow switch nodes
  NetDeviceContainer        m_ofDevices;      //!< OpenFlow OFSwitch13NetDevice devices
  CsmaHelper                m_ofCsmaHelper;   //!< Csma helper
  DataRate                  m_LinkDataRate;   //!< Csma link data rate
  Time                      m_LinkDelay;      //!< Csma link delay
  uint16_t                  m_LinkMtu;        //!< Csma link mtu

  /**
   * Create a CSMA link between m_ofSwitches[switchIdx] and Ptr<Node> node,
   * adding a new port to the switch and a new NetDevice to the node.
   *
   * \param switchIdx The switch index in m_ofSwitches.
   * \param Ptr<Node> The Node pointer.
   * \return The NetDevice created at the Node.
   */
  virtual Ptr<NetDevice> SwitchAttach (uint8_t switchIdx, Ptr<Node> node);

  /** 
   * Creates the OpenFlow internal network infrastructure. 
   */
  virtual void CreateInternalTopology () = 0;

private:
  /** 
   * Install the OpenFlow controller application into controller node.
   * \param controller The controller app.
   * \return The new controller node.
   */
  Ptr<Node> InstallControllerApp (Ptr<OFSwitch13Controller> controller);
};

};  // namespace ns3
#endif  // OPENFLOW_EPC_NETWORK_H

