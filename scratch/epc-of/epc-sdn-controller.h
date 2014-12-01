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

#ifndef EPC_SDN_CONTROLLER_H
#define EPC_SDN_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/epc-s11-sap.h>
#include "openflow-epc-network.h"

namespace ns3 {

/**
 * OpenFlow EPC controller.
 */
class EpcSdnController : public OFSwitch13Controller
{
public:
  /** List of created context bearers */
  typedef std::list<EpcS11SapMme::BearerContextCreated> ContextBearers_t;

  /** LTE context information for controller usage */
  struct ContextInfo {
    uint64_t imsi;                //!< UE IMSI
    uint16_t cellId;              //!< eNB Cell ID
    uint16_t enbIdx;              //!< eNB switch index
    uint16_t sgwIdx;              //!< Gateway switch index
    Ipv4Address enbAddr;          //!< eNB IPv4 addr
    Ipv4Address sgwAddr;          //!< Gateway IPv4 addr
    ContextBearers_t bearerList;  //!< List of bearers
  };

  EpcSdnController ();          //!< Default constructor
  virtual ~EpcSdnController (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();


  /**
   * Set the OpenFlowEpcNetwork object used to create the network.
   * \param ptr The object pointer.
   */
  void SetOpenFlowNetwork (Ptr<OpenFlowEpcNetwork> ptr);

  /**
   * Notify this controller of a new IP device connected to the OpenFlow
   * network over some switch port. This function will save the IP address /
   * MAC address from this IP device for further ARP resolution. 
   * \attention This dev is not the one added as port to switch. Instead, this
   * is the 'other' end of this connection, associated with a eNB or SgwPgw
   * node.
   * \param dev The device connected to the OpenFlow network.
   * \param ip The IPv4 address assigned to this device.
   * \param switchIdx The switch index this device is attached to.
   */
  virtual void 
  NotifyNewIpDevice (Ptr<NetDevice> dev, Ipv4Address ip, uint16_t switchIdx);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow network. 
   * \param conInfo The connection information and metadata.
   */ 
  virtual void 
  NotifyNewSwitchConnection (ConnectionInfo connInfo);
  
  /** 
   * Callback fired before creating new dedicated EPC bearers. This is used to
   * check for necessary resources in the network (mainly available data rate
   * for GBR bearers. When returning false, it aborts the bearer creation
   * process, and all traffic will be routed over default bearer.
   * traffic routing. 
   * \param uint64_t IMSI UE identifier.
   * \param uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param Ptr<EpcTft> tft traffic flow template of the bearer.
   * \param EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful (the bearer creation process will proceed),
   * false otherwise (the bearer creation process will abort).
   */
  virtual bool 
  RequestNewDedicatedBearer (uint64_t imsi, uint16_t cellId, Ptr<EpcTft> tft, 
                             EpsBearer bearer);

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
   * \param bearerContextList The list of bearers to be created.
   */
  virtual void 
  NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
                           Ipv4Address enbAddr, Ipv4Address sgwAddr,
                           ContextBearers_t bearerContextList);
  
  // virtual void NotifyContextModified ();

//  /**
//   * Notify this controller of a new application configured to send traffic
//   * from source to destination node.
//   * \param app The application pointer.
//   * \param source The source node, generating the traffic.
//   * \param destination The destination node, consuming the traffic.
//   */ 
//  virtual void 
//  NotifyNewApp (Ptr<Application> app, Ptr<Node> source, Ptr<Node> destination);

  /**
   * Notify this controller of a application starting sending traffic over EPC
   * OpenFlow network. This method expects that this application has a
   * TrafficFlowTemplate aggregated to it, since it uses the TFT to search for
   * bearer information.
   * \param app The application pointer.
   */ 
  virtual void 
  NotifyAppStart (Ptr<Application> app);

  /**
   * Install flow table entry for local delivery when a new IP device is
   * connected to the OpenFlow network.  This entry will match both MAC address
   * and IP address for the device in order to output packets on device port.
   * \attention This device is not the one added as port to switch. Instead,
   * this is the 'other' end of this connection, associated with a eNB or
   * SgwPgw node.
   * \param swtch The Switch OFSwitch13NetDevice pointer.
   * \param device The device connected to the OpenFlow network.
   * \param deviceIp The IPv4 address assigned to this device.
   * \param devicePort The number of switch port this device is attached to.
   */
  virtual void 
  ConfigurePortDelivery (Ptr<OFSwitch13NetDevice> swtch, Ptr<NetDevice> device, 
                         Ipv4Address deviceIp, uint32_t devicePort);   
  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  virtual void CreateSpanningTree ();

protected:
//  /** Pair identifying application endpoints */
//  typedef std::pair<Ptr<Node>, Ptr<Node> > AppEndpoint_t;
  
  /**
   * Search for connection information between two switches.
   * \param sw1 First switch index.
   * \param sw2 Second switch index.
   * \return Pointer to connection info saved.
   */
  ConnectionInfo* GetConnectionInfo (uint16_t sw1, uint16_t sw2);
  
//   /**
//    * Get the application source and destiation nodes.
//    * \param app The application pointer.
//    * \return The pair of endpoint nodes.
//    */
//   AppEndpoint_t GetAppEndpoint (Ptr<Application> app);

  /**
   * Get the OFSwitch13NetDevice of a specific switch.
   * \param index The switch index.
   * \return The pointer to the switch OFSwitch13NetDevice.
   */
  Ptr<OFSwitch13NetDevice> GetSwitchDevice (uint16_t index);

  uint16_t GetSwitchIdxFromIp (Ipv4Address addr);

//  /**
//   * Retrieve the switch index for a cell ID
//   * \param cellId The eNB cell ID .
//   * \return The switch index in m_ofSwitches.
//   */
//  uint16_t GetSwitchIdxForCellId (uint16_t cellId);
//
//  /**
//   * Retrieve the switch index for an UE IMSI
//   * \param imsi The UE IMSI..
//   * \return The switch index in m_ofSwitches.
//   */
//  uint16_t GetSwitchIdxForImsi (uint64_t imsi);
//
//  /**
//   * Retrieve the switch index for a node.
//   * \param node The node.
//   * \return The switch index in m_ofSwitches.
//   */
//  uint16_t GetSwitchIdxForNode (Ptr<Node> node);

  /**
   * Retrieve the switch index for the SgwPgw gateway
   * \return The switch index in m_ofSwitches.
   */
  uint16_t GetSwitchIdxForGateway ();

  /**
   * \return Number of switches in the network.
   */
  uint16_t GetNSwitches ();

  /** 
   * Iterate over the context bearers map looking for the bearer information
   * for a specific traffic flow template.
   * \param tft The traffic flow template.
   * \return The bearer info for this tft.
   */ 
  EpcS11SapMme::BearerContextCreated GetBearerFromTft (Ptr<EpcTft> tft);

  /**
   * Save a Dpctl command to be executed just after the connection
   * establishment between switch and controller. 
   * \param textCmd The Dpctl command.
   * \param device The Switch OFSwitch13NetDevice pointer.
   */
  void 
  ScheduleCommand (Ptr<OFSwitch13NetDevice> device, const std::string textCmd);

  /**
   * Handle packet-in messages sent from switch to this controller. Look for L2
   * switching information, update the structures and send a packet-out back.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err 
  HandlePacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid);

  /**
   * Handle flow removed messages sent from switch to this controller. 
   * \param msg The flow removed message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  virtual ofl_err 
  HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, 
                     uint32_t xid);

  /**
   * Handle packet-in messages sent from switch with unknown TEID routing.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \param teid The GTPU TEID identifier.
   * \return 0 if everything's ok, otherwise an error number.
   */
  virtual ofl_err 
  HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                          uint32_t xid, uint32_t teid);

private:
  /**
   * Callback fired when the switch / controller connection is successfully
   * established. This method will configure the switch, install table-miss
   * entry and execute all dpctl scheduled commands for this switch.
   * \param swtch The switch information.
   */
  void NotifyConnectionStarted (SwitchInfo swtch);

  /**
   * Handle packet-in messages sent from switch with arp message.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err 
  HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid);

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

  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (uint32_t oxm_of, ofl_match* match);

  /** Multimap saving pair <pointer to device / dpctl command str> */
  typedef std::multimap<Ptr<OFSwitch13NetDevice>, std::string> DevCmdMap_t; 

  /** Map saving pair <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;

  /** Map saving pair <IPv4 address / Switch index > */
  typedef std::map<Ipv4Address, uint16_t> IpSwitchMap_t;

  /** Key identifying a pair of switches */
  typedef std::pair<uint16_t, uint16_t> ConnectionKey_t; 
  
  /** Map saving pair of switch indexes / connection information */
  typedef std::map<ConnectionKey_t, ConnectionInfo> ConnInfoMap_t; 

  /** List of context info */
  typedef std::vector<ContextInfo> ContextInfoList_t;
  
  
//  /** Key indentifying pair IMSI/CellId */
//  typedef std::pair<uint64_t, uint16_t> ContextKey_t;
//  
//  /** Map saving pair ContextKey_t / ContextBearers_t */
//  typedef std::map<ContextKey_t, ContextBearers_t> ContextMap_t;
//
////  /** Map saving application / endpoints */
////  typedef std::map<Ptr<Application>, AppEndpoint_t> AppMap_t;
//
//  /** Map saving pair IMSI / Switch index */
//  typedef std::map<uint64_t, uint16_t> ImsiMap_t;
//
//  /** Pair of IPv4 TEID endpoints */
//  typedef std::pair<Ipv4Address, Ipv4Address> PgwEnbAddr_t;
//
//  /** Map saving TEID endpoints */
//  typedef std::map<uint32_t, PgwEnbAddr_t> TeidAddrMap_t;
  
  DevCmdMap_t       m_schedCommands;  //!< Scheduled commands for execution
  IpMacMap_t        m_arpTable;       //!< ARP resolution table
  IpSwitchMap_t     m_ipSwitchTable;  //!< IP / switch table
  ConnInfoMap_t     m_connections;    //!< Connections between switches.
  ContextInfoList_t m_contexts;       //!< List of contexts

//  ContextMap_t  m_createdBearers; //!< Created bearers in network.
//  AppMap_t      m_appEndpoints;   //!< Application IP endpoints.
//  TeidAddrMap_t m_teidEndpoints;  //!< TEID IPv4 endpoints.
//  ImsiMap_t     m_imsiSwitch;     //!< Mapping IMSI to switch index.
//  Ipv4Address   m_webNetwork;     //!< Internet IP address network.
//  Ipv4Address   m_ueNetwork;      //!< UEs IP address network.

  Ptr<OpenFlowEpcNetwork> m_ofNetwork;    //!< Pointer to OpenFlow network
};

};  // namespace ns3
#endif // EPC_SDN_CONTROLLER_H

