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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef SLICE_NETWORK_H
#define SLICE_NETWORK_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class BackhaulNetwork;
class RadioNetwork;
class SliceController;

/**
 * \ingroup uni5on
 * \defgroup uni5onLogical Logical
 * eEPC architecture logical network.
 */

/**
 * \ingroup uni5onLogical
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
   * Enable PCAP traces on the logical slice network.
   * \param prefix Filename prefix to use for PCAP files.
   * \param promiscuous If true, enable PCAP promiscuous traces.
   * \param ofchannel If true, enable PCAP on backhaul OpenFlow channels.
   * \param sgiDevices If true, enable PCAP on SGi interfaces (Internet).
   * \param pgwDevices If true, enable PCAP on P-GW switches.
   */
  void EnablePcap (std::string prefix, bool promiscuous, bool ofchannel,
                   bool sgiDevices, bool pgwDevices);

  /**
   * Get the UE nodes.
   * \return The UE node container.
   */
  NodeContainer GetUeNodes (void) const;

  /**
   * Get the UE LTE network devices.
   * \return The UE network device container.
   */
  NetDeviceContainer GetUeDevices (void) const;

  /**
   * Get the Internet web server node.
   * \return The pointer to the web node.
   */
  Ptr<Node> GetWebNode (void) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Get the number of P-GW TFT switch nodes available on this topology.
   * \return The number of P-GW TFT nodes.
   */
  uint32_t GetPgwTftNumNodes (void) const;

  /**
   * Set the number of P-GW TFT switch nodes available on this topology.
   * \param value The number of P-GW TFT nodes.
   */
  void SetPgwTftNumNodes (uint32_t value);

private:
  /**
   * Create the P-GW using OpenFlow switches, connecting it to the
   * Internet web server and to the OpenFlow backhaul network.
   */
  void CreatePgw (void);

  /**
   * Create the S-GW using an OpenFlow switch, connecting it
   * to the OpenFlow backhaul network.
   */
  void CreateSgw (void);

  /**
   * Create the UEs, connecting them to the LTE RAN network.
   */
  void CreateUes (void);

  // Slice identification.
  SliceId                       m_sliceId;          //!< Logical slice ID.
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
  Ptr<RandomVariableStream>     m_ueMobPause;       //!< UE random mob pause.
  Ptr<RandomVariableStream>     m_ueMobSpeed;       //!< UE random mob speed.
  NodeContainer                 m_ueNodes;          //!< UE nodes.
  NetDeviceContainer            m_ueDevices;        //!< UE devices.
  Ipv4AddressHelper             m_ueAddrHelper;     //!< UE address helper.
  Ipv4Address                   m_ueAddr;           //!< UE network address.
  Ipv4Mask                      m_ueMask;           //!< UE network mask.
  uint16_t                      m_ueCellSiteCover;  //!< UE cell site coverage.

  // Internet network.
  Ptr<Node>                     m_webNode;          //!< Web server node.
  NetDeviceContainer            m_webDevices;       //!< Web SGi devices.
  Ipv4AddressHelper             m_webAddrHelper;    //!< Web address helper.
  Ipv4Address                   m_webAddr;          //!< Web network address.
  Ipv4Mask                      m_webMask;          //!< Web network mask.
  DataRate                      m_webLinkRate;      //!< Web link data rate.
  Time                          m_webLinkDelay;     //!< Web link delay.

  // P-GW user plane.
  Ptr<PgwInfo>                  m_pgwInfo;          //!< P-GW metadata.
  Ipv4Address                   m_pgwAddress;       //!< P-GW logical address.
  NodeContainer                 m_pgwNodes;         //!< P-GW switch nodes.
  OFSwitch13DeviceContainer     m_pgwDevices;       //!< P-GW switch devices.
  NetDeviceContainer            m_pgwIntDevices;    //!< P-GW int port devices.
  DataRate                      m_pgwLinkRate;      //!< P-GW link data rate.
  Time                          m_pgwLinkDelay;     //!< P-GW link delay.
  uint16_t                      m_pgwInfraSwIdx;    //!< Backhaul switch index.
  DataRate                      m_mainCpuCapacity;  //!< Main CPU capacity.
  uint32_t                      m_mainFlowSize;     //!< Main flow table size.
  uint16_t                      m_nTfts;            //!< Number of TFT nodes.
  DataRate                      m_tftCpuCapacity;   //!< TFT CPU capacity.
  uint32_t                      m_tftFlowSize;      //!< TFT flow table size.
  uint32_t                      m_tftMeterSize;     //!< TFT meter table size.
  Time                          m_tftTcamDelay;     //!< TFT TCAM delay.

  // S-GW user planes.
  Ptr<SgwInfo>                  m_sgwInfo;          //!< S-GW metadata.
  Ptr<Node>                     m_sgwNode;          //!< S-GW switch node.
  Ptr<OFSwitch13Device>         m_sgwDevice;        //!< S-GW switch device.
  uint16_t                      m_sgwInfraSwIdx;    //!< Backhaul switch idx.
  DataRate                      m_sgwCpuCapacity;   //!< S-GW CPU capacity.
  uint32_t                      m_sgwFlowSize;      //!< S-GW flow table size.
  uint32_t                      m_sgwMeterSize;     //!< S-GW meter table size.

  // Helper and attributes for CSMA interface.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  uint16_t                      m_linkMtu;          //!< Link MTU.
};

} // namespace ns3
#endif  // SLICE_NETWORK_H
