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

#ifndef SDMN_EPC_HELPER_H
#define SDMN_EPC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * Create an EPC network connected through CSMA devices to an user-defined
 * backhaul network. This Helper will create an EPC network topology comprising
 * of a single node that implements both the SGW and PGW functionality, and an
 * MME node. The S1 and X2 interfaces are realized over CSMA devices
 * connected to an user-defined backhaul network.
 */
class SdmnEpcHelper : public EpcHelper
{
public:
  SdmnEpcHelper ();           //!< Default constructor
  virtual ~SdmnEpcHelper ();  //!< Default destructor

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from EpcHelper
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId);
  virtual void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  virtual uint8_t ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer);
  virtual Ptr<Node> GetPgwNode ();
  virtual Ipv4InterfaceContainer AssignUeIpv4Address (NetDeviceContainer ueDevices);
  virtual Ipv4Address GetUeDefaultGatewayAddress ();

  /**
   * Get a pointer to the MME element.
   * \return The MME element.
   */
  Ptr<EpcMme> GetMmeElement ();

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

} // namespace ns3
#endif // SDMN_EPC_HELPER_H

