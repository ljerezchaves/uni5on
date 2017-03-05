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

#ifndef EPC_CONTROLLER_H
#define EPC_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "info/routing-info.h"
#include "info/connection-info.h"
#include "info/meter-info.h"
#include "info/gbr-info.h"
#include "info/ue-info.h"
#include "info/enb-info.h"
#include "stats/admission-stats-calculator.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * Create an OpenFlow EPC controller. This is an abstract base class which
 * should be extended in accordance to OpenFlow network topology. It implements
 * the logic for traffic routing and engineering within the backhaul network
 * and the P-GW control plane.
 */
class EpcController : public OFSwitch13Controller
{
  friend class MemberEpcS11SapSgw<EpcController>;

public:
  EpcController ();           //!< Default constructor
  virtual ~EpcController ();  //!< Dummy destructor, see DoDispose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Request a new dedicated EPS bearer. This is used to check for necessary
   * resources in the network (mainly available data rate for GBR bearers).
   * When returning false, it aborts the bearer creation process.
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
   * Release a dedicated EPS bearer.
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

  /** \name Methods for network topology and LTE EPC monitoring. */
  //\{
  /**
   * Notify this controller of the P-GW and Internet Web server connection over
   * the SGi interface. This function will save the IP address / MAC address
   * from this new device for further ARP resolution, and will configure the
   * P-GW datapath.
   * \param pgwSwDev The OpenFlow P-GW switch device.
   * \param pgwSgiDev The SGi device attached to the OpenFlow P-GW switch.
   * \param pgwSgiAddr The IPv4 address assigned to the SGi device.
   * \param pgwSgiPort The OpenFlow port number for the SGi device.
   * \param pgwS5Port The OpenFlow port number for the S5 device.
   * \param webAddr The IPv4 address assignet to the Internet Web server.
   */
  virtual void NewSgiAttach (Ptr<OFSwitch13Device> pgwSwDev,
    Ptr<NetDevice> pgwSgiDev, Ipv4Address pgwSgiAddr, uint32_t pgwSgiPort,
    uint32_t pgwS5Port, Ipv4Address webAddr);

  /**
   * Notify this controller of a new S-GW or P-GW device connected to the S5
   * OpenFlow network over some switch port. This function will save the IP
   * address / MAC address from this new device for further ARP resolution, and
   * will configure local port delivery.
   * \param nodeDev The device connected to the OpenFlow switch (this is not
   * the one added as port to switch, instead, this is the 'other' end of this
   * connection, associated with the S-GW or P-GW node).
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchDev The OpenFlow switch device.
   * \param swtchIdx The OpenFlow switch index.
   * \param swtchPort The port number for nodeDev at OpenFlow switch.
   */
  virtual void NewS5Attach (Ptr<NetDevice> nodeDev, Ipv4Address nodeIp,
    Ptr<OFSwitch13Device> swtchDev, uint16_t swtchIdx, uint32_t swtchPort);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow network. The user is supposed to connect this function as trace
   * sink for EpcNetwork::NewSwitchConnection trace source.
   * \param cInfo The connection information and metadata.
   */
  virtual void NewSwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this controller that all connection between switches have already
   * been configure and the topology is finished. The user is supposed to
   * connect this function as trace sink for EpcNetwork::TopologyBuilt trace
   * source.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void TopologyBuilt (OFSwitch13DeviceContainer devices);

  /**
   * Notify this controller when the MME receives a context created response
   * message. This is used to notify this controller with the list of bearers
   * context created. With this information, the controller can configure the
   * switches for GTP routing. The user is supposed to connect this function as
   * trace sink for EpcMme::SessionCreated trace source.
   * \see 3GPP TS 29.274 7.2.1 for CreateSessionRequest message format.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param pgwAddr The P-GW IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  virtual void NotifySessionCreated (uint64_t imsi, uint16_t cellId,
    Ipv4Address enbAddr, Ipv4Address pgwAddr, BearerList_t bearerList);

  /**
   * Notify this controller when the Non-GBR allowed bit rate in any network
   * connection is adjusted. This is used to update Non-GBR meters bands based
   * on GBR resource reservation.
   * \param cInfo The connection information
   */
  virtual void NonGbrAdjusted (Ptr<ConnectionInfo> cInfo);
  //\}

  /**
   * Retrieve stored information for a specific bearer.
   * \param teid The GTP tunnel ID.
   * \return The EpsBearer information for this teid.
   */
  static EpsBearer GetEpsBearer (uint32_t teid);

  /**
   * Retrieve stored mapped value for a specific EPS QCI.
   * \param qci The EPS bearer QCI.
   * \return The IP DSCP mapped value for this QCI.
   */
  static uint16_t GetDscpMappedValue (EpsBearer::Qci qci);

  /**
   * TracedCallback signature for new bearer request.
   * \param ok True when the bearer request/release processes succeeds.
   * \param rInfo The routing information for this bearer tunnel.
   */
  typedef void (*BearerTracedCallback)(bool ok, Ptr<const RoutingInfo> rInfo);

  /**
   * TracedCallback signature for session created trace source.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param pgwAddr The P-GW IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  typedef void (*SessionCreatedTracedCallback)(uint64_t imsi, uint16_t cellId,
      Ipv4Address enbAddr, Ipv4Address pgwAddr, BearerList_t bearerList);

  /** \name Methods for the P-GW control plane. */
  //\{
  // FIXME Acho que essas funções vão pro S-GW.
  /**
   * Set the MME side of the S11 SAP.
   * \param s the MME side of the S11 SAP.
   */
  void SetS11SapMme (EpcS11SapMme * s);

  /**
   * Get the S-GW side of the S11 SAP.
   * \return the SGW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw ();

  /**
   * Set the address of a previously added UE.
   * \param imsi The unique identifier of the UE.
   * \param ueAddr The IPv4 address of the UE.
   */
  void SetUeAddress (uint64_t imsi, Ipv4Address ueAddr);
  //\}


protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

  /** \name Topology-dependent functions
   * This virtual functions must be implemented by subclasses, as they are
   * dependent on network topology.
   */
  //\{
  /**
   * Configure the switches with OpenFlow commands for TEID routing, based on
   * network topology information.
   * \attention This function must increase the routing priority before
   * installing the rules, to avoid conflicts with old entries.
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

  /**
   * Configure the P-GW with OpenFlow rules for downlink TFT packet filtering.
   * \attention To avoid conflicts with old entries, increase the routing
   * priority before installing P-GW TFT rules.
   * \param rInfo The routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   */
  void InstallPgwTftRules (Ptr<RoutingInfo> rInfo,
    uint32_t buffer = OFP_NO_BUFFER);

  // Inherited from OFSwitch13Controller
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  virtual ofl_err HandlePacketIn (
    ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);

  /**
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param index The switch index in device colection.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDatapathId (uint16_t index) const;

  /**
   * Get the OpenFlow datapath ID for the P-GW switch.
   * \return The P-GW datapath ID.
   */
  uint64_t GetPgwDatapathId () const;

private:
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
   * Install flow table entry for S5 port when a new IP device is connected to
   * the OpenFlow network. This entry will match both MAC address and IP
   * address for the S5 device in order to output packets on respective device
   * port. It will also match input port for packet classification and
   * routing.
   * \param swtchDev The Switch OFSwitch13Device pointer.
   * \param nodeDev The device connected to the OpenFlow network.
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchPort The number of switch port this device is attached to.
   */
  void ConfigureS5PortRules (Ptr<OFSwitch13Device> swtchDev,
    Ptr<NetDevice> nodeDev, Ipv4Address nodeIp, uint32_t swtchPort);

  /**
   * Install flow table entry for SGi packet forwarding.
   * \param pgwDev The P-GW switch OFSwitch13Device pointer.
   * \param pgwSgiPort The SGi interface port number on P-GW switch.
   * \param pgwS5Port The S5 interface port number on P-GW switch.
   * \param webAddr The IPv4 address assignet to the Internet Web server.
   */
  void ConfigurePgwRules (Ptr<OFSwitch13Device> pgwDev, uint32_t pgwSgiPort,
    uint32_t pgwS5Port, Ipv4Address webAddr);

  /**
   * Handle packet-in messages sent from switch with ARP message.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (ofl_msg_packet_in *msg,
    Ptr<const RemoteSwitch> swtch, uint32_t xid);

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

  /** \name Methods for the P-GW control plane. */
  //\{
  // S11 SAP SGW methods
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoModifyBearerRequest (EpcS11SapSgw::ModifyBearerRequestMessage msg);
  void DoDeleteBearerCommand (EpcS11SapSgw::DeleteBearerCommandMessage req);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req);
  //\}

  /**
   * EpcController inner friend utility class
   * used to initialize static DSCP map table.
   */
  class QciDscpInitializer
  {
  public:
    /** Initializer function. */
    QciDscpInitializer ();
  };
  friend class QciDscpInitializer;

  /**
   * Static instance of Initializer. When this is created, its constructor
   * initializes the EpcController s' static DSCP map table.
   */
  static QciDscpInitializer initializer;

// Member variables
protected:
  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, Ipv4Address, Ipv4Address, BearerList_t>
    m_sessionCreatedTrace;

  bool m_voipQos;                         //!< Enable VoIP QoS with queues.
  bool m_nonGbrCoexistence;               //!< Enable Non-GBR coexistence.

  static const uint16_t m_dedicatedTmo;   //!< Timeout for dedicated bearers

private:
  OFSwitch13DeviceContainer      m_ofDevices;       //!< OpenFlow devices.
  Ptr<AdmissionStatsCalculator>  m_admissionStats;  //!< Admission statistics.
  uint32_t                       m_pgwDpId;         //!< P-GW datapath ID.
  uint32_t                       m_pgwS5Port;       //!< P-GW S5 port no.

  /** Map saving <IPv4 address / Switch index > */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;
  IpSwitchMap_t       m_ipSwitchTable;    //!< eNB IP / Switch Index table.

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  IpMacMap_t          m_arpTable;         //!< ARP resolution table.

  /** Map saving <EpsBearer::Qci / IP Dscp value> */
  typedef std::map<EpsBearer::Qci, uint16_t> QciDscpMap_t;
  static QciDscpMap_t m_qciDscpTable;     //!< DSCP mapped values.

  /**
   * TEID counter, used to allocate new GTP-U TEID values.
   * \internal This counter is initialized at 0x0000000F, reserving the first
   *           values for controller usage.
   */
  uint32_t m_teidCount;

  EpcS11SapMme* m_s11SapMme;      //!< MME side of the S11 SAP
  EpcS11SapSgw* m_s11SapSgw;      //!< SGW side of the S11 SAP
};

};  // namespace ns3
#endif // EPC_CONTROLLER_H

