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
#include <ns3/applications-module.h>
#include "openflow-epc-network.h"
#include "routing-info.h"
#include "stats-calculator.h"

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/**
 * \ingroup epcof
 * The abstract base OpenFlow EPC controller, which should be extend in
 * accordance to network topology.
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

  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Get the default timeout for dedicated bearers
   * \return The default idle timeout.
   */
  static const Time GetDedicatedTimeout ();

  /**
   * Set the pointer to OpenFlow network controlled by this app.
   * \param network The OpenFlowEpcNetwork pointer.
   */
  void SetOfNetwork (Ptr<OpenFlowEpcNetwork> network);

  /**
   * Set the default statistics dump interval.
   * \param timeout The timeout value.
   */
  void SetDumpTimeout (Time timeout);

  /**
   * Notify this controller of a new eNB or SgwPgw device connected to the
   * OpenFlow network over some switch port. This function will save the IP
   * address / MAC address from this new device for further ARP resolution, and
   * will configure local port delivery. 
   * \attention This nodeDev is not the one added as port to switch. Instead,
   * this is the 'other' end of this connection, associated with a eNB or
   * SgwPgw node.
   * \param nodeDev The device connected to the OpenFlow switch.
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchDev The OpenFlow switch device.
   * \param swtchIdx The OpenFlow switch index.
   * \param swtchPort The port number for nodeDev at OpenFlow switch.
   */
  virtual void NotifyNewAttachToSwitch (Ptr<NetDevice> nodeDev, 
      Ipv4Address nodeIp, Ptr<OFSwitch13NetDevice> swtchDev, uint16_t swtchIdx, 
      uint32_t swtchPort);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow network. 
   * \param conInfo The connection information and metadata.
   */ 
  virtual void NotifyNewConnBtwnSwitches (const Ptr<ConnectionInfo> connInfo);

  /**
   * Notify this controller that all connection between switches have already
   * been configure and the topology is finished.
   */ 
  virtual void NotifyConnBtwnSwitchesOk ();
  
  /** 
   * Callback fired before creating new dedicated EPC bearers. This is used to
   * check for necessary resources in the network (mainly available data rate
   * for GBR bearers. When returning false, it aborts the bearer creation
   * process, and all traffic will be routed over default bearer.
   * \param imsi uint64_t IMSI UE identifier.
   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param tft Ptr<EpcTft> TFT traffic flow template of the bearer.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful (the bearer creation process will proceed),
   * false otherwise (the bearer creation process will abort).
   */
  virtual bool RequestNewDedicatedBearer (uint64_t imsi, uint16_t cellId,
      Ptr<EpcTft> tft, EpsBearer bearer);

  /** 
   * Callback fired when the SgwPgw gateway is handling a CreateSessionRequest
   * message. This is used to notify this controller with the list of bearers
   * context created (this list will be sent back to the MME over S11 interface
   * in the CreateSessionResponde message). With this information, the
   * controller can configure the switches for GTP routing.
   * \see 3GPP TS 29.274 7.2.1 for CreateSessionRequest message format.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param sgwAddr The SgwPgw IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  virtual void NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
      Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList);
  
  // virtual void NotifyContextModified ();
  // virtual void NotifyBearerDeactivated ();

  /**
   * Notify this controller of an application starts sending traffic over EPC
   * OpenFlow network. This method expects that this application has a
   * TrafficFlowTemplate aggregated to it, since it uses the TFT to search for
   * bearer information.
   * \param app The application pointer.
   * \return true to allow the app to start the traffic, false otherwise.
   */ 
  virtual bool NotifyAppStart (Ptr<Application> app);

  /**
   * Notify this controller of an application stops sending traffic over EPC
   * OpenFlow network. This method expects that this application has a
   * TrafficFlowTemplate aggregated to it, since it uses the TFT to search for
   * bearer information.
   * \param app The application pointer.
   * \return true.
   */ 
  virtual bool NotifyAppStop (Ptr<Application> app);

  /**
   * Connect all trace sinks used by this controller to monitor the network.
   */
  void ConnectTraceSinks ();

  /**
   * Trace sink for packets entering the EPC. The packet will get tagged for
   * QoS monitoring.
   * \param context Node name.
   * \param packet The packet.
   */
  void EpcInputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets leaving the EPC. The tag will be read from packet,
   * and QoS stats updated.
   * \param context Node name.
   * \param packet The packet.
   */
  void EpcOutputPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets traversing the EPC packet gateway from the
   * Internet to the EPC.
   * \param direction The traffic direction.
   * \param packet The packet.
   */
  void PgwTraffic (std::string direction, Ptr<const Packet> packet);

  /**
   * Trace sink for packets dropped by meter bands. The tag will be read
   * from packet, and QoS stats updated.
   * \param context Output switch index.
   * \param packet The dropped packet.
   */
  void MeterDropPacket (std::string context, Ptr<const Packet> packet);

  /**
   * Trace sink for packets dropped by queues. 
   * \param context The queue context location.
   * \param packet The dropped packet.
   */
  void QueueDropPacket (std::string context, Ptr<const Packet> packet);

  /** 
   * TracedCallback signature for QoS dump. 
   * \param description String describing this traffic.
   * \param teid Bearer TEID.
   * \param stats The QoS statistics.
   */
  typedef void (* QosTracedCallback)(std::string description, uint32_t teid, 
                                     Ptr<const QosStatsCalculator> stats);

  /** 
   * TracedCallback signature for Pgw traffic statistics.
   * \param downTraffic The average downlink traffic for last interval.
   * \param upTraffic The average uplink traffic for last interval.
   */
  typedef void (* PgwTracedCallback)(DataRate downTraffic, DataRate upTraffic);

  /** 
   * TracedCallback signature for switch Flow table rules statistics.
   * \param teid The number of TEID routing flow rules at each switch.
   */
  typedef void (* SwtTracedCallback)(std::vector<uint32_t> teid);


protected:
  /**
   * Process the bearer resource and bandwidth allocation.
   * \param rInfo The routing information to process.
   * \return True when the request is satisfied.
   */
  virtual bool BearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer and bandwidth release.
   * \param rInfo The routing information to process.
   * \return True when the resources are successfully released.
   */
  virtual bool BearerRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Configure the switches with OpenFlow commands for TEID routing.
   * \attention This function must increase the priority before installing the
   * rules, to avoid conflicts with old entries.
   * \param rInfo The routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   * \return True if configuration succeeded, false otherwise.
   */
  virtual bool InstallTeidRouting (Ptr<RoutingInfo> rInfo, 
      uint32_t buffer = OFP_NO_BUFFER) = 0;

  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  virtual void CreateSpanningTree () = 0;   

  /**
   * \return Number of switches in the network.
   */
  uint16_t GetNSwitches ();

  /**
   * Get the OFSwitch13NetDevice of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13NetDevice.
   */
  Ptr<OFSwitch13NetDevice> GetSwitchDevice (uint16_t index);

  /**
   * Retrieve the switch index for EPC entity attached to OpenFlow network.
   * \param addr The eNB or SgwPgw address.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxFromIp (Ipv4Address addr);

  /**
   * Retrieve the LTE context information from the GTP tunnel id
   * \param teid The GTP tunnel id.
   * \return The context info for this TEID.
   */
  Ptr<const ContextInfo> GetContextFromTeid (uint32_t teid);

  /**
   * Retrieve the TEID identifier for traffic generated by application app.
   * \param app The application.
   * \return The TEID identifier.
   */
  uint32_t GetTeidFromApplication (Ptr<Application> app);

  /**
   * Search for connection information between two switches.
   * \param sw1 First switch index.
   * \param sw2 Second switch index.
   * \return Pointer to connection info saved.
   */
  Ptr<ConnectionInfo> GetConnectionInfo (uint16_t sw1, uint16_t sw2);
 
  /**
   * Retrieve stored information for a specific GTP tunnel
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  Ptr<RoutingInfo> GetTeidRoutingInfo (uint32_t teid);

  /**
   * Dump bearer request statistics.
   */
  void DumpGbrStatistics ();

  /**
   * Dump EPC Pgw downlink/uplink traffic statistics.
   */
  void DumpPgwStatistics ();
  
  /**
   * Dump Opwnflow switch table statistics.
   */
  void DumpSwtStatistics ();

  /**
   * Dump network bandwidth usage.
   */
  void DumpBwdStatistics ();

  /**
   * Dump application statistics.
   * \param app The application pointer.
   */
  void DumpAppStatistics (Ptr<Application> app);

  /**
   * Reset application statistics.
   * \param app The application pointer.
   */
  void ResetAppStatistics (Ptr<Application> app);
 
  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (uint32_t oxm_of, ofl_match* match);

  // Inherited from OFSwitch13Controller
  virtual void ConnectionStarted (SwitchInfo);
  virtual ofl_err HandlePacketIn (ofl_msg_packet_in*, SwitchInfo, uint32_t);
  virtual ofl_err HandleFlowRemoved (ofl_msg_flow_removed*, SwitchInfo, uint32_t);

  /** Timeout values */
  //\{ 
  static const int m_defaultTmo;    //!< Timeout for default bearers
  static const int m_dedicatedTmo;  //!< Timeout for dedicated bearers
  //\}

  /** Priority values */
  //\{
  // Table 0
  static const int m_t0ArpPrio;           //!< ARP handling
  static const int m_t0GotoT1Prio;        //!< GTP TEID handling (goto table 1)

  // Table 1
  static const int m_t1LocalDeliverPrio;  //!< Local delivery (to eNB/SgwPgw)
  static const int m_t1DedicatedStartPrio;//!< Dedicated bearer (start value) 
  static const int m_t1DefaultPrio;       //!< Default bearer
  static const int m_t1RingPrio;          //!< Ring forward
  //\}

private:
  /**
   * Retrieve the LTE context information from the traffic flow template
   * associated with an application.
   * \param tft The Traffic Flow Template.
   * \return The context info for this TFT.
   */
  Ptr<const ContextInfo> GetContextFromTft (Ptr<EpcTft> tft);

  /** 
   * Iterate over the context bearers map looking for the bearer information
   * for a specific traffic flow template.
   * \param tft The traffic flow template.
   * \return The bearer info for this TFT.
   */ 
  ContextBearer_t GetBearerFromTft (Ptr<EpcTft> tft);

  /**
   * Retrieve the LTE EPC QoS statistics information for the GTP tunnel id.
   * When no information structure available, create it.
   * \param teid The GTP tunnel id.
   * \param isDown True for downlink stats, false for uplink.
   * \return The QoS information.
   */
  Ptr<QosStatsCalculator> GetQosStatsFromTeid (uint32_t teid, bool isDown);

  /**
   * Save the RoutingInfo metadata for further usage and reserve the bandwidth.
   * \param rInfo The routing information to save.
   */
  void SaveTeidRoutingInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Install flow table entry for local delivery when a new IP device is
   * connected to the OpenFlow network. This entry will match both MAC address
   * and IP address for the device in order to output packets on device port.
   * \param swtchDev The Switch OFSwitch13NetDevice pointer.
   * \param nodeDev The device connected to the OpenFlow network.
   * \param nodeIp The IPv4 address assigned to this device.
   * \param swtchPort The number of switch port this device is attached to.
   */
  void ConfigureLocalPortDelivery (Ptr<OFSwitch13NetDevice> swtchDev, 
    Ptr<NetDevice> nodeDev, Ipv4Address nodeIp, uint32_t swtchPort);  

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
   * Perform an ARP resolution
   * \param ip The Ipv4Address to search.
   * \return The MAC address for this ip.
   */
  Mac48Address ArpLookup (Ipv4Address ip);

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


  /** The Application QoS trace source, fired at DumpAppStatistics. */
  TracedCallback<std::string, uint32_t, 
    Ptr<const QosStatsCalculator> > m_appTrace;

  /** The LTE EPC QoS trace source, fired at DumpAppStatistics. */
  TracedCallback<std::string, uint32_t, 
    Ptr<const QosStatsCalculator> > m_epcTrace;

  /** The EPC Pgw traffic trace source, fired at DumpPgwStatistics. */
  TracedCallback<DataRate, DataRate> m_pgwTrace;

  /** The GBR block ratio trace source, fired at DumpGbrStatistics. */
  TracedCallback<Ptr<const BearerStatsCalculator> > m_gbrTrace;

  /** The switch flow table rules trace source, fired at DumpSwtStatistics. */
  TracedCallback<std::vector<uint32_t> > m_swtTrace;

  /** The network bandwidth usage, fired at DumpBwdStatistics. */
  TracedCallback<Ptr<const BandwidthStats> > m_bwdTrace;

  
  /** A pair of switches index */
  typedef std::pair<uint16_t, uint16_t> SwitchPair_t;

  /** A pair of QosStatsCalculator, for downlink and uplink statistics */
  typedef std::pair<Ptr<QosStatsCalculator>, 
    Ptr<QosStatsCalculator> > QosStatsPair_t;

  /** A list of context information */
  typedef std::vector<Ptr<ContextInfo> > ContextInfoList_t;

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;

  /** Map saving <IPv4 address / Switch index > */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;
   
  /** Map saving <Pair of switch indexes / Connection information */
  typedef std::map<SwitchPair_t, Ptr<ConnectionInfo> > ConnInfoMap_t; 

  /** Map saving <TEID / Routing information > */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;

  /** Map saving <TEID / QoS stats > */
  typedef std::map<uint32_t, QosStatsPair_t> TeidQosMap_t;

  IpMacMap_t        m_arpTable;         //!< ARP resolution table.
  IpSwitchMap_t     m_ipSwitchTable;    //!< eNB IP / Switch Index table.
  ConnInfoMap_t     m_connections;      //!< Connections between switches.
  ContextInfoList_t m_contexts;         //!< List of created contexts.
  TeidRoutingMap_t  m_routes;           //!< TEID routing informations.
  TeidQosMap_t      m_qosStats;         //!< TEID QoS statistics
  
  Time              m_dumpTimeout;      //!< Dump stats timeout.
  uint32_t          m_pgwDownBytes;     //!< Pgw traffic downlink bytes.
  uint32_t          m_pgwUpBytes;       //!< Pgw traffic uplink bytes.
  
  Ptr<BearerStatsCalculator> m_bearerStats;  //!< Bearer statistics. 
  Ptr<OpenFlowEpcNetwork> m_ofNetwork;  //!< Pointer to OpenFlow network.
};

};  // namespace ns3
#endif // OPENFLOW_EPC_CONTROLLER_H

