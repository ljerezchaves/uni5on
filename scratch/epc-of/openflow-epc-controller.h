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

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/**
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
   * Return the block radio statistics.
   * \return The GBR bearer block ratio
   */
  double DumpGbrBlockStatistics ();

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
   * TracedCallback signature for Application QoS dump. 
   * \param description String describing this traffic.
   * \param teid Bearer TEID.
   * \param duration Duration of this traffic.
   * \param lossRatio Loss packet ratio at application level.
   * \param delay Packet average delay at application level.
   * \param jitter Packet average jitter at application level.
   * \param bytes The total of bytes received at application level.
   * \param goodput Traffic throguhput at application level.
   */
  typedef void (* AppQosTracedCallback)
    (std::string description, uint32_t teid, Time duration, double lossRatio,
     Time delay, Time jitter, uint32_t bytes, DataRate goodput);

  /** 
   * TracedCallback signature for GBR block ratio statistics.
   * \param requests The number of GBR requests.
   * \param blocks The number of GBR requests blockd.
   * \param ratio The block ratio.
   */
  typedef void (* GbrBlockTracedCallback)
    (uint32_t requests, uint32_t blocks, double ratio);

protected:
  /** 
   * Pure virtual functions used to reserve and release GBR bearers resources,
   * and to install OpenFlow flow rules into switches. 
   */
  //\{
  /**
   * Process the GBR bearer resource allocation.
   * \param rInfo The routing information to process.
   * \return True when the GBR resources request are satisfied.
   */
  virtual bool GbrBearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the GBR bearer resource release.
   * \param rInfo The routing information to process.
   * \return True when the GBR resources are successfully released.
   */
  virtual bool GbrBearerRelease (Ptr<RoutingInfo> rInfo) = 0;

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
  //\}

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
   * Retrieve the switch index for the SgwPgw gateway
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForGateway ();

  /**
   * Retrieve the switch index for the switch device
   * \param dev The OpenFlow device pointer.
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxFromDevice (Ptr<OFSwitch13NetDevice> dev);

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
   * Increase by on the number of GBR bearer requests in network.
   */
  void IncreaseGbrRequest ();

  /**
   * Increase by on the number of GBR bearer requests that were blocked by
   * network.
   */
  void IncreaseGbrBlocks ();

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
  TracedCallback<std::string, uint32_t, Time, double, Time, 
      Time, uint32_t, DataRate> m_appQosTrace;

  /** The GBR block ratio trace source, fired at GetBlockRatioStatistics. */
  TracedCallback<uint32_t, uint32_t, double> m_gbrBlockTrace;

  
  /** A pair of switches index */
  typedef std::pair<uint16_t, uint16_t> SwitchPair_t; 

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

  IpMacMap_t        m_arpTable;         //!< ARP resolution table.
  IpSwitchMap_t     m_ipSwitchTable;    //!< eNB IP / Switch Index table.
  ConnInfoMap_t     m_connections;      //!< Connections between switches.
  ContextInfoList_t m_contexts;         //!< List of created contexts.
  TeidRoutingMap_t  m_routes;           //!< TEID routing informations.
    
  uint32_t          m_gbrBearers;       //!< Requests for GBR bearers
  uint32_t          m_gbrBlocks;        //!< Blocked GBR requests
  
  Ptr<OpenFlowEpcNetwork> m_ofNetwork;  //!< Pointer to OpenFlow network
};

};  // namespace ns3
#endif // OPENFLOW_EPC_CONTROLLER_H

