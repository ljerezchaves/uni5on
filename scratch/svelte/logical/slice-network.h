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
 * Enumeration of available SVELTE logical slices.
 */
typedef enum
{
  HTC  = 0,   //!< Slice for HTC UEs.
  MTC  = 1    //!< Slice for MTC UEs.
} LogicalSlice;

/**
 * Get the logical slice name.
 * \param slice The logical slice identification.
 * \return The string with the slice name.
 */
std::string LogicalSliceStr (LogicalSlice slice);

/**
 * \ingroup svelte
 * \defgroup svelteLogical Logical
 * SVELTE architecture logical network.
 */

/**
 * \ingroup svelteLogical
 * This is the class for a logical LTE network slice, sharing the common
 * OpenFlow backhaul and radio networks.
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
   * control planes), and on the SGi interface for the Internet network.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /** \name Private member accessors. */
  //\{
  uint32_t GetPgwTftNumNodes (void) const;
  void SetBackhaulNetwork (Ptr<BackhaulNetwork> value);
  void SetRadioNetwork (Ptr<RadioNetwork> value);
  void SetPgwTftNumNodes (uint32_t value);
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Create the P-GW using OpenFlow switches, connecting it to the Internet web
   * server and to the OpenFlow backhaul network.
   */
  void CreatePgw (void);

  /**
   * Create the S-GWs using OpenFlow switches, connecting them to the OpenFlow
   * backhaul network.
   */
  void CreateSgws (void);

  /**
   * Create the UEs, connecting them to the LTE radio infrastructure network.
   */
  void CreateUes (void);

  // Slice identification.
  LogicalSlice                  m_sliceId;          //!< Logical slice ID.
  std::string                   m_sliceIdStr;       //!< Slice ID string.

  // Infrastructure interface.
  Ptr<BackhaulNetwork>          m_backhaul;         //!< OpenFlow backhaul.
  Ptr<RadioNetwork>             m_radio;            //!< LTE radio network.

  // OpenFlow network configuration.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.
  Ptr<SliceController>          m_controllerApp;    //!< Controller app.
  Ptr<Node>                     m_controllerNode;   //!< Controller node.

  // UEs network.
  uint32_t                      m_nUes;             //!< Number of UEs.
  bool                          m_ueMobility;       //!< Enable UE mobility.
  NodeContainer                 m_ueNodes;          //!< UE nodes.
  NetDeviceContainer            m_ueDevices;        //!< UE devices.
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.
  Ipv4Address                   m_ueAddr;           //!< UE network address.
  Ipv4Mask                      m_ueMask;           //!< UE network mask.

  // Internet network.
  Ptr<Node>                     m_webNode;          //!< Web server node.
  NetDeviceContainer            m_webDevices;       //!< Web SGi devices.
  Ipv4AddressHelper             m_webAddrHelper;    //!< Web address helper.
  Ipv4Address                   m_webAddr;          //!< Web network address.
  Ipv4Mask                      m_webMask;          //!< Web network mask.
  DataRate                      m_webLinkRate;      //!< Web link data rate.
  Time                          m_webLinkDelay;     //!< Web link delay.

  // P-GW user plane.
  Ipv4Address                   m_pgwAddress;       //!< P-GW S5 address.
  NodeContainer                 m_pgwNodes;         //!< P-GW switch nodes.
  OFSwitch13DeviceContainer     m_pgwDevices;       //!< P-GW switch devices.
  NetDeviceContainer            m_pgwIntDevices;    //!< P-GW int port devices.
  DataRate                      m_pgwLinkRate;      //!< P-GW link data rate.
  Time                          m_pgwLinkDelay;     //!< P-GW link delay.
  uint16_t                      m_pgwSwitchIdx;     //!< Backhaul switch index.
  uint16_t                      m_nTftNodes;        //!< Number of TFT nodes.
  DataRate                      m_tftPipeCapacity;  //!< TFT switch capacity.
  uint32_t                      m_tftTableSize;     //!< TFT switch table size.

  // S-GW user planes.
  uint32_t                      m_nSgws;
  NodeContainer                 m_sgwNodes;         //!< S-GW switch nodes.
  OFSwitch13DeviceContainer     m_sgwDevices;       //!< S-GW switch devices.

  // Helper and attributes for CSMA interface.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  uint16_t                      m_linkMtu;          //!< Link MTU.
};

} // namespace ns3
#endif  // SLICE_NETWORK_H
