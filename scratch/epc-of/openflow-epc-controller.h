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

#ifndef OPENFLOW_EPC_CONTROLLER_H
#define OPENFLOW_EPC_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "routing-info.h"
#include "connection-info.h"

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/**
 * \ingroup epcof
 * Create an OpenFlow EPC controller. This is an abstract base class which
 * should be extended in accordance to OpenFlow network topology.
 */
class OpenFlowEpcController : public OFSwitch13Controller
{
public:
  OpenFlowEpcController ();           //!< Default constructor
  virtual ~OpenFlowEpcController ();  //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Request a new dedicated EPC bearer. This is used to check for necessary
   * resources in the network (mainly available data rate for GBR bearers).
   * When returning false, it aborts the bearer creation process
   * \internal
   * Current implementation assumes that each application traffic flow is
   * associated with a unique bearer/tunnel. Because of that, we can use only
   * the teid for the tunnel to prepare and install route. If we would like to
   * aggregate traffic from several applications into same bearer we will need
   * to revise this.
   * \param teid The teid for this bearer, if already defined.
   * \param imsi uint64_t IMSI UE identifier.
   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful (the bearer creation process will proceed),
   * false otherwise (the bearer creation process will abort).
   */
  virtual bool RequestDedicatedBearer (EpsBearer bearer, uint64_t imsi,
                                       uint16_t cellId, uint32_t teid);

  /**
   * Release a dedicated EPC bearer.
   * \internal
   * Current implementation assumes that each application traffic flow is
   * associated with a unique bearer/tunnel. Because of that, we can use only
   * the teid for the tunnel to prepare and install route. If we would like to
   * aggregate traffic from several applications into same bearer we will need
   * to revise this.
   * \param teid The teid for this bearer, if already defined.
   * \param imsi uint64_t IMSI UE identifier.
   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful, false otherwise.
   */
  virtual bool ReleaseDedicatedBearer (EpsBearer bearer, uint64_t imsi,
                                       uint16_t cellId, uint32_t teid);

  /**
   * Retrieve stored information for a specific GTP teid.
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  Ptr<const RoutingInfo> GetConstRoutingInfo (uint32_t teid) const;

  /**
   * Retrieve stored information for a specific bearer.
   * \param teid The GTP tunnel ID.
   * \return The EpsBearer information for this teid.
   */
  static EpsBearer GetEpsBearer (uint32_t teid);

  /**
   * TracedCallback signature for new bearer request.
   * \param ok True when the bearer request/release processes succeeds.
   * \param rInfo The routing information for this bearer tunnel.
   */
  typedef void (*BearerTracedCallback)(bool ok, Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  /** \name Trace sinks for network topology and LTE EPC monitoring. */
  //\{
  /**
   * Notify this controller of a new eNB or SgwPgw device connected to the
   * OpenFlow network over some switch port. This function will save the IP
   * address / MAC address from this new device for further ARP resolution, and
   * will configure local port delivery. The user is supposed to connect this
   * function as trace sink for OpenFlowEpcNetwork::NewEpcAttach trace source.
   * \param nodeDev The device connected to the OpenFlow switch.
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchDev The OpenFlow switch device.
   * \param swtchIdx The OpenFlow switch index.
   * \param swtchPort The port number for nodeDev at OpenFlow switch.
   */
  virtual void NotifyNewEpcAttach (Ptr<NetDevice> nodeDev, Ipv4Address nodeIp,
                                   Ptr<OFSwitch13NetDevice> swtchDev, 
                                   uint16_t swtchIdx, uint32_t swtchPort);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow network. The user is supposed to connect this function as trace
   * sink for OpenFlowEpcNetwork::NewSwitchConnection trace source.
   * \param cInfo The connection information and metadata.
   */
  virtual void NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this controller that all connection between switches have already
   * been configure and the topology is finished. The user is supposed to
   * connect this function as trace sink for OpenFlowEpcNetwork::TopologyBuilt
   * trace source.
   * \param devices The NetDeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyTopologyBuilt (NetDeviceContainer devices);

  /**
   * Notify this controller when the SgwPgw gateway is handling a
   * CreateSessionRequest message. This is used to notify this controller with
   * the list of bearers context created (this list will be sent back to the
   * MME over S11 interface in the CreateSessionResponde message). With this
   * information, the controller can configure the switches for GTP routing.
   * The user is supposed to connect this function as trace sink for
   * SgwPgwApplication::ContextCreated trace source.
   * \see 3GPP TS 29.274 7.2.1 for CreateSessionRequest message format.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param sgwAddr The SgwPgw IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  virtual void NotifyContextCreated (uint64_t imsi, uint16_t cellId,
                                     Ipv4Address enbAddr, Ipv4Address sgwAddr, 
                                     BearerList_t bearerList);

  /**
   * Notify this controller when the Non-GBR allowed bit rate in any network
   * connection is adjusted. This is used to update Non-GBR meters bands based
   * on GBR resource reservation.
   * \param cInfo The connection information
   */
  virtual void NotifyNonGbrAdjusted (Ptr<ConnectionInfo> cInfo);
  //\}

  /** \name Topology-dependent functions
   * This virtual functions must be implemented by subclasses, as they are
   * dependent on network topology.
   */
  //\{
  /**
   * Configure the switches with OpenFlow commands for TEID routing, based on
   * network topology information.
   * \attention This function must increase the priority before installing the
   * rules, to avoid conflicts with old entries.
   * \param rInfo The routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   * \return True if configuration succeeded, false otherwise.
   */
  virtual bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo,
                                       uint32_t buffer = OFP_NO_BUFFER) = 0;

  /**
   * Remove TEID routing rules from switches.
   * \param rInfo The routing information to remove.
   * \return True if remove succeeded, false otherwise.
   */
  virtual bool TopologyRemoveRouting (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer resource request and bit rate allocation based on
   * network topology information.
   * \param rInfo The routing information to process.
   * \return True when the request is satisfied.
   */
  virtual bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer and bit rate release based on network topology
   * information.
   * \param rInfo The routing information to process.
   * \return True when the resources are successfully released.
   */
  virtual bool TopologyBearerRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  virtual void TopologyCreateSpanningTree () = 0;
  //\}

  // Inherited from OFSwitch13Controller
  virtual void ConnectionStarted (SwitchInfo);
  virtual ofl_err HandlePacketIn (ofl_msg_packet_in *, SwitchInfo, uint32_t);
  virtual ofl_err HandleFlowRemoved (ofl_msg_flow_removed *,
                                     SwitchInfo, uint32_t);

  /**
   * Get the OFSwitch13NetDevice for a specific switch index.
   * \param index The switch index in device colection.
   * \return The pointer to the switch OFSwitch13NetDevice.
   */
  Ptr<OFSwitch13NetDevice> GetSwitchDevice (uint16_t index);

private:
  /**
   * Save the RoutingInfo metadata for further usage.
   * \param rInfo The routing information to save.
   */
  void SaveRoutingInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Retrieve stored information for a specific GTP teid.
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  Ptr<RoutingInfo> GetRoutingInfo (uint32_t teid);

  /**
   * Save the pair IP / Switch index in switch table.
   * \param ipAddr The IPv4 address.
   * \param index The switch index in devices collection..
   */
  void SaveSwitchIndex (Ipv4Address ipAddr, uint16_t index);

  /**
   * Retrieve the switch index for EPC entity attached to OpenFlow network.
   * \param addr The eNB or SgwPgw IPv4 address.
   * \return The switch index in devices collection.
   */
  uint16_t GetSwitchIndex (Ipv4Address addr);

  /**
   * Save the pair IP / MAC address in ARP table.
   * \param ipAddr The IPv4 address.
   * \param macAddr The MAC address.
   */
  void SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr);

  /**
   * Perform an ARP resolution
   * \param ip The Ipv4Address to search.
   * \return The MAC address for this ip.
   */
  Mac48Address GetArpEntry (Ipv4Address ip);

  /**
   * Install flow table entry for local port when a new IP device is connected
   * to the OpenFlow network. This entry will match both MAC address and IP
   * address for the local device in order to output packets on respective
   * device port. It will also match input port for packet classification and
   * routing. 
   * \param swtchDev The Switch OFSwitch13NetDevice pointer.
   * \param nodeDev The device connected to the OpenFlow network.
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchPort The number of switch port this device is attached to.
   */
  void ConfigureLocalPortRules (Ptr<OFSwitch13NetDevice> swtchDev,
                                Ptr<NetDevice> nodeDev, Ipv4Address nodeIp, 
                                uint32_t swtchPort);

  /**
   * Handle packet-in messages sent from switch with unknown TEID routing.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \param teid The GTPU TEID identifier.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch,
                                  uint32_t xid, uint32_t teid);

  /**
   * Handle packet-in messages sent from switch with ARP message.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch,
                             uint32_t xid);

  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (uint32_t oxm_of, ofl_match* match);

  /**
   * Create a Packet with an ARP reply, encapsulated inside of an Ethernet
   * frame (with header and trailer.
   * \param srcMac Source MAC address.
   * \param srcIP Source IP address.
   * \param dstMac Destination MAC address.
   * \param dstMac Destination IP address.
   * \return The ns3 Ptr<Packet> with the ARP reply.
   */
  Ptr<Packet> CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp,
                              Mac48Address dstMac, Ipv4Address dstIp);

  /**
   * Insert a new bearer entry in global bearer map.
   * \param teid The GTP tunnel ID.
   * \param bearer The bearer information.
   */
  static void RegisterBearer (uint32_t teid, EpsBearer bearer);

  /**
   * Remove a bearer entry from global bearer map.
   * \param teid The GTP tunnel ID.
   */
  static void UnregisterBearer (uint32_t teid);

// Member variables
protected:
  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  static const int m_dedicatedTmo;        //!< Timeout for dedicated bearers

private:
  NetDeviceContainer  m_ofDevices;        //!< OpenFlow switch devices.

  /** Map saving <TEID / Routing information > */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;
  TeidRoutingMap_t    m_routes;           //!< TEID routing informations.

  /** Map saving <IPv4 address / Switch index > */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;
  IpSwitchMap_t       m_ipSwitchTable;    //!< eNB IP / Switch Index table.

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  IpMacMap_t          m_arpTable;         //!< ARP resolution table.

  /** Map saving <TEID / EpsBearer > */
  typedef std::map<uint32_t, EpsBearer> TeidBearerMap_t;
  static TeidBearerMap_t m_bearersTable;  //!< TEID bearers table.
};

};  // namespace ns3
#endif // OPENFLOW_EPC_CONTROLLER_H

