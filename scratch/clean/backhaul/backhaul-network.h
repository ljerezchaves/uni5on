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
class ConnectionInfo;

/**
 * \ingroup svelte
 * \defgroup svelteBackhaul Backhal
 * OpenFlow backhaul network for SVELTE architecture.
 */

/**
 * \ingroup svelteBackhaul
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
   * Set an attribute for ns3::OFSwitch13Device factory.
   * \param n1 The name of the attribute to set.
   * \param v1 The value of the attribute to set.
   */
  void SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * Enable PCAP traces on the OpenFlow backhaul network (user and control
   * planes), and on LTE EPC devices of S1, S5, and X2 interfaces.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Create the controller application and switch devices for the OpenFlow
   * backhaul network, connecting them accordingly to the desired topology.
   */
  virtual void TopologyCreate (void) = 0;

  /**
   * Install the OpenFlow backhaul controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<BackhaulController> controller);

  // Backhaul controller.
  Ptr<BackhaulController>       m_controllerApp;  //!< Controller app.
  Ptr<Node>                     m_controllerNode; //!< Controller node.

  // OpenFlow switches, helper and connection attribute.
  NodeContainer                 m_switchNodes;    //!< Switch nodes.
  OFSwitch13DeviceContainer     m_switchDevices;  //!< Switch devices.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;   //!< Switch helper.
  uint16_t                      m_linkMtu;        //!< Link MTU.

private:
  // Helper and attributes for EPC interfaces.
  CsmaHelper                    m_csmaHelper;     //!< Connection helper.
  DataRate                      m_s1LinkRate;     //!< S1 link data rate.
  Time                          m_s1LinkDelay;    //!< S1 link delay.
  DataRate                      m_s5LinkRate;     //!< S5 link data rate.
  Time                          m_s5LinkDelay;    //!< S5 link delay.
  DataRate                      m_x2LinkRate;     //!< X2 link data rate.
  Time                          m_x2LinkDelay;    //!< X2 link delay.

  // EPC user-plane devices.
  NetDeviceContainer            m_s1Devices;      //!< S1 devices.
  NetDeviceContainer            m_s5Devices;      //!< S5 devices.
  NetDeviceContainer            m_x2Devices;      //!< X2 devices.
};

} // namespace ns3
#endif  // BACKHAUL_NETWORK_H
