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

#ifndef OPENFLOW_EPC_HELPER_H
#define OPENFLOW_EPC_HELPER_H

#include <ns3/object.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/data-rate.h>
#include <ns3/epc-tft.h>
#include <ns3/eps-bearer.h>
#include <ns3/epc-helper.h>
#include <ns3/trace-helper.h>
#include <ns3/net-device-container.h>
#include <ns3/epc-mme.h>
#include <ns3/epc-sgw-pgw-application.h>

namespace ns3 {

class Node;
class NetDevice;
class VirtualNetDevice;
class EpcSgwPgwApplication;
class EpcX2;
class EpcMme;

/**
 * \brief Create an EPC network connected to an OpenFlow network.
 *
 * This Helper will create an EPC network topology comprising of a single node
 * that implements both the SGW and PGW functionality, and an MME node. The S1
 * and X2 interfaces are realized over an OpenFlow network.
 */
class OpenFlowEpcHelper : public EpcHelper, public PcapHelperForDevice
{
public:
  
  /** Default constructor. Initialize EPC structure. */
  OpenFlowEpcHelper ();

  /** Dummy destructor, see DoDispose. */
  virtual ~OpenFlowEpcHelper ();
  
  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation. */
  virtual void DoDispose ();

  // inherited from EpcHelper
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId);
  virtual void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  virtual void ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer);
  virtual Ptr<Node> GetPgwNode ();
  virtual Ipv4InterfaceContainer AssignUeIpv4Address (NetDeviceContainer ueDevices);
  virtual Ipv4Address GetUeDefaultGatewayAddress ();

 
  /**
    * Enable Pcap output on all S1-U devices connected to OpenFlow network.
    * \param prefix Filename prefix to use for pcap files.
    * \param promiscuous If true capture all packets available at the devices.
    * \param explicitFilename Treat the prefix as an explicit filename if true.
    */
  void EnablePcapS1u (std::string prefix, bool promiscuous = false, 
                      bool explicitFilename = false);

  /**
    * Enable Pcap output on all X2-U devices connected to OpenFlow network.
    * \param prefix Filename prefix to use for pcap files.
    * \param promiscuous If true capture all packets available at the devices.
    * \param explicitFilename Treat the prefix as an explicit filename if true.
    */
  void EnablePcapX2 (std::string prefix, bool promiscuous = false, 
                     bool explicitFilename = false);

  /**
   * S1U attach callback signature.
   * \param Ptr<Node> node to attach.
   * \param uint16_t eNB cell ID.
   * \returns Ptr<NetDevice> the device created at the Node.
   */
  typedef Callback <Ptr<NetDevice>, Ptr<Node>, uint16_t > S1uConnectCallback_t;

  /**
   * X2 attach callback signature.
   * \param Ptr<Node> node to attach 
   * \returns Ptr<NetDevice> the device created at the Node
   */
  typedef Callback <Ptr<NetDevice>, Ptr<Node> > X2ConnectCallback_t;
 
  /**
   * Callback signature for adding a new EPS bearer.
   * \param uint64_t IMSI UE identifier.
   * \param uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param Ptr<EpcTft> tft traffic flow template of the bearer.
   * \param EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful (proceed with bearer creation), false
   * otherwise (abort).
   */
  typedef Callback<bool, uint64_t, uint16_t, Ptr<EpcTft>, EpsBearer> 
      AddBearerCallback_t;

  /**
    * \brief Specify callbacks to allow the caller to proper connect the EPC
    * nodes (SgwPgw and eNBs) to the S1-U OpenFlow network insfrastructure.
    * \param cb Callback invoked during AddEnb procedure to proper connect
    * the eNB Node to the OpenFlow network. This is also called by
    * SetS1uConnectCallback to proper connect the SgwPgw Node to the OpenFlow
    * network.
    */
  void SetS1uConnectCallback (S1uConnectCallback_t cb);

  /**
    * Specify callbacks to allow the caller to proper connect the eNB nodes to
    * the X2 OpenFlow network insfrastructure.
    * \param cb Callback invoked during AddX2Interface procedure to proper
    * connect the eNB Node to the OpenFlow network. 
    */
  void SetX2ConnectCallback (X2ConnectCallback_t cb);

  /**
    * Callback used to query the OpenFlow controller for the necessary
    * resources before creating a new dedicated EPS bearer. Depending on the
    * return of this callback, the bearer creation will proceed or abort.
    * \param cb Callback invoked by ActivateEpsBearer before creating a new
    * dedicated bearer. The bearer will be created only if the callback returns
    * true.
    */
  void SetAddBearerCallback (AddBearerCallback_t cb);

  /**
   * Callback used to notify the OpenFlow controller with the list of bearers
   * context created. 
   * \param cb Callback invoked by EpcSgwPgwApplication::DoCreateSessionRequest
   * before sending back the CreateSessionResponse message. 
   */
  void SetCreateSessionRequestCallback (
      EpcSgwPgwApplication::CreateSessionRequestCallback_t cb);
 
private:
  /**
   * \brief Enable pcap output on the indicated net device.
   * \internal This is the same implementation from CsmaNetDevice class, as the
   * OpenFlow uses Csma devices.
   * \param prefix Filename prefix to use for pcap files.
   * \param nd Net device for which you want to enable tracing.
   * \param promiscuous If true capture all packets available at the device.
   * \param explicitFilename Treat the prefix as an explicit filename if true.
   */
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, 
                                   bool promiscuous, bool explicitFilename);

  /**
   * Retrieve SgwPgw OpenFlow address.
   * \return The SgwPgw Ipv4Addrress of the S1-U interface connected to the
   * OpenFlow network.
   */
  virtual Ipv4Address GetSgwS1uAddress ();

  /**
   * Retrieve eNB OpenFlow address for device.
   * \param device NetDevice connected to the OpenFlow network.
   * \return The Ipv4Addrress of the S1-U or X2 interface for the specific
   * NetDevice connected to the OpenFlow network.
   */
  virtual Ipv4Address GetAddressForDevice (Ptr<NetDevice> device);

  /** Callback to connect nodes to S1-U OpenFlow network. */
  S1uConnectCallback_t m_s1uConnect;

  /** Callback to connect nodes to X2 OpenFlow network. */
  X2ConnectCallback_t m_x2Connect;

  /** AddBearer callback */
  AddBearerCallback_t m_addBearerCallback;

  /** A collection of S1-U NetDevice */
  NetDeviceContainer m_s1uDevices;
  
  /** A collection of X2 NetDevice */
  NetDeviceContainer m_x2Devices;

  /** The SgwPgw NetDevice connected to the S1-U OpenFlow network switch */
  Ptr<NetDevice> m_sgwS1uDev;

  /** SgwPgw network element */
  Ptr<Node> m_sgwPgw; 

  /** SgwPgw application */
  Ptr<EpcSgwPgwApplication> m_sgwPgwApp;

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

#endif // OPENFLOW_EPC_HELPER_H

