/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#ifndef SLICE_NETWORK_H
#define SLICE_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class BackhaulNetwork;
class RadioNetwork;
class SliceController;

/**
 * \ingroup svelte
 * \defgroup svelteLogical Logical
 * SVELTE architecture logical network.
 */

/**
 * \ingroup svelteLogical
 * This is the abstract base class for a logical LTE network slice, which
 * should be extended in accordance to the desired network configuration. All
 * LTE network slices share the common OpenFlow backhaul and radio networks.
 */
class SliceNetwork : public Object
{
public:
  SliceNetwork ();          //!< Default constructor.
  virtual ~SliceNetwork (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces on the S/P-GW OpenFlow internal switches (user and
   * control planes), and on the SGi interface for web network.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /** \name Private/protected member accessors. */
  //\{
  uint32_t GetNumUes (void) const;
  Ipv4Address GetUeAddress (void) const;
  Ipv4Mask GetUeMask (void) const;
  Ipv4Address GetSgiAddress (void) const;
  Ipv4Mask GetSgiMask (void) const;
  NodeContainer GetUeNodes (void) const;
  NetDeviceContainer GetUeDevices (void) const;
  uint32_t GetPgwTftNumNodes (void) const;
  DataRate GetPgwTftPipeCapacity (void) const;
  uint32_t GetPgwTftTableSize (void) const;
  Ipv4Address GetPgwS5Address (void) const;
  Ptr<Node> GetWebNode (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /** \name Slice methods.
   * These virtual methods must be implemented by slice subclasses, as they
   * are dependent on the network configuration.
   */
  //\{
  /**
   * Create the LTE network slice.
   */
  virtual void SliceCreate (void) = 0;
  //\}

  /** \name Private/protected member accessors. */
  //\{
  void SetNumUes (uint32_t value);
  void SetUeAddress (Ipv4Address value);
  void SetUeMask (Ipv4Mask value);
  void SetSgiAddress (Ipv4Address value);
  void SetSgiMask (Ipv4Mask value);
  void SetPgwTftNumNodes (uint32_t value);
  void SetPgwTftPipeCapacity (DataRate value);
  void SetPgwTftTableSize (uint32_t value);
  //\}

  /**
   * Install the OpenFlow slice controller for this network.
   * \param controller The controller application.
   */
  void InstallController (Ptr<SliceController> controller);

  // Slice controller.
  Ptr<SliceController>          m_controllerApp;    //!< Controller app.
  Ptr<Node>                     m_controllerNode;   //!< Controller node.

  // OpenFlow switches, helper and connection attribute.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch   helper.
  uint16_t                      m_linkMtu;          //!< Link MTU.
  DataRate                      m_sgiLinkRate;      //!< SGi link data rate.
  Time                          m_sgiLinkDelay;     //!< SGi link delay.
  DataRate                      m_pgwLinkRate;      //!< P-GW link data rate.
  Time                          m_pgwLinkDelay;     //!< P-GW link delay.

  // IP address helpers.
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.
  Ipv4AddressHelper             m_sgiAddrHelper;    //!< Web address helper.

  // Attributes
  uint32_t                      m_nUes;             //!< Number of UEs.
  Ipv4Address                   m_ueAddr;           //!< UE network address.
  Ipv4Mask                      m_ueMask;           //!< UE network mask.
  Ipv4Address                   m_sgiAddr;          //!< SGi network address.
  Ipv4Mask                      m_sgiMask;          //!< SGi network mask.
  uint16_t                      m_tftNumNodes;      //!< Num P-GW TFT nodes.
  DataRate                      m_tftPipeCapacity;  //!< P-GW TFT capacity.
  uint32_t                      m_tftTableSize;     //!< P-GW TFT table size.

  NodeContainer                 m_ueNodes;          //!< UE nodes.
  NetDeviceContainer            m_ueDevices;        //!< UE devices.
  
  // SVELTE infrastructure.
  Ptr<BackhaulNetwork>          m_backhaul;         //!< OpenFlow backhaul.
  Ptr<RadioNetwork>             m_lteRan;           //!< LTE radio network.

  // Helper and attributes for CSMA interface.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.

  // P-GW user plane.
  Ipv4Address                   m_pgwAddress;       //!< P-GW S5 address.
  NodeContainer                 m_pgwNodes;         //!< P-GW switch nodes.
  OFSwitch13DeviceContainer     m_pgwDevices;       //!< P-GW switch devices.
  NetDeviceContainer            m_pgwIntDevices;    //!< P-GW int port devices.

  // Internet web server.
  Ptr<Node>                     m_webNode;          //!< Web server node.

  NetDeviceContainer            m_sgiDevices;       //!< SGi devices.

  /**
   * Create the P-GW user-plane network composed of OpenFlow switches managed
   * by the EPC controller. This function will also attach the P-GW to the S5
   * and SGi interfaces.
   */
  void PgwCreate (void);

private:

};

} // namespace ns3
#endif  // SLICE_NETWORK_H
