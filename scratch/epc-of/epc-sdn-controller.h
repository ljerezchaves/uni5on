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

namespace ns3 {

/**
 * OpenFlow EPC controller.
 */
class EpcSdnController : public OFSwitch13Controller
{
public:
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
   * AddBearer callback, used to prepare the OpenFlow network for bearer
   * traffic routing. 
   * \param imsi
   * \param tft
   * \param bearer
   * \return //FIXME
   */
  uint8_t AddBearer (uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer);

  /**
   * Save a Dpctl command to be executed just after the conection stablishment
   * between switch and controller. 
   * \param textCmd The Dpctl command.
   * \param device The Switch OFSwitch13NetDevice pointer.
   */
  void ScheduleCommand (Ptr<OFSwitch13NetDevice> device, 
                        const std::string textCmd);

  /**
   * Notify the controller of a new device connected to the OpenFlow network.
   * This function will save the IP address / MAC address wich will be further
   * used in ARP resolution. 
   * \attention This dev is not the one added as port to switch. Insted, this
   * is the 'other' end of this connection, associated with a eNB or SgwPgw
   * node.
   * \param dev The device connected to the OpenFlow network.
   * \param ip The IPv4 address assigned to this device.
   */
  void NotifyNewIpDevice (Ptr<NetDevice> dev, Ipv4Address ip);

  /**
   * Install flow table entry for local delivery when a new IP device is
   * connected to the OpenFlow network.  This entry will match both MAC address
   * and IP address for the device in order to output packets on device port.
   * \attention This device is not the one added as port to switch. Insted,
   * this is the 'other' end of this connection, associated with a eNB or
   * SgwPgw node.
   * \param swtch The Switch OFSwitch13NetDevice pointer.
   * \param device The device connected to the OpenFlow network.
   * \param deviceIp The IPv4 address assigned to this device.
   * \param devicePort The number of the switch port this device is attached to.
   */
  void ConfigurePortDelivery (Ptr<OFSwitch13NetDevice> swtch, 
                              Ptr<NetDevice> device, Ipv4Address deviceIp, 
                              uint32_t devicePort);   

  /**
   * Handle packet-in messages sent from switch to this controller. Look for L2
   * switching information, update the structures and send a packet-out back.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandlePacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                          uint32_t xid);

  /**
   * Handle flow removed messages sent from switch to this controller. 
   * \param msg The flow removed message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, 
                             uint32_t xid);

private:
  /**
   * Callback fired when the switch / controller connection is sucssefully
   * stablished. This method will configure the switch, install table-miss
   * entry and execute all dpctl scheduled commands for this switch.
   * \param swtch The switch information.
   */
  void ConnectionStarted (SwitchInfo swtch);

  /**
   * Perform an ARP resolution
   * \param ip The Ipv4Address to search.
   * \return The MAC address for this ip.
   */
  Mac48Address ArpLookup (Ipv4Address ip);

  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (uint32_t oxm_of, ofl_match* match);

  /**
   * Create a Packet with an ARP reply, encapsulated inside of an Ehternet
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
   * Handle packet-in messages sent from switch with arp message.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, 
                             uint32_t xid);

  /** Multimap saving pair <pointer to device / dpctl command str> */
  typedef std::multimap<Ptr<OFSwitch13NetDevice>, std::string> DevCmdMap_t; 
  
  /** Map saving pair <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  
  DevCmdMap_t   m_schedCommands;  //!< Scheduled commands for execution
  IpMacMap_t    m_arpTable;       //!< ARP resolution table
  //\}
};

};  // namespace ns3
#endif // EPC_SDN_CONTROLLER_H

