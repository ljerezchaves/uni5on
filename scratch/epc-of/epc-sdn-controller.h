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
   * AddBearer callback, used to prepare the OpenFlow network for bearer traffic.
   * \param 
   */
  uint8_t AddBearer (uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer);

  /**
   * Save a Dpctl command to be executed just after the conection stablishment
   * between switch and controller. 
   * \param textCmd The Dpctl command.
   * \param device The Switch OFSwitch13NetDevice pointer.
   */
  void ScheduleCommand (Ptr<OFSwitch13NetDevice> device, const std::string textCmd);

  /**
   * Handle packet-in messages sent from switch to this controller. Look for L2
   * switching information, update the structures and send a packet-out back.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandlePacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid);

  /**
   * Handle flow removed messages sent from switch to this controller. Look for L2
   * switching information and removes associated entry.
   * \param msg The flow removed message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, uint32_t xid);

private:
  /**
   * Callback fired when the switch / controller connection is sucssefully
   * stablished. This method will configure the switch, install table-miss
   * entry and execute all dpctl scheduled commands for this switch.
   * \param swtch The switch information.
   */
  void ConnectionStarted (SwitchInfo swtch);

  /**
   * Handle packet-in messages sent from switch to this controller with arp
   * protocol. For an arp request, flood the packet over switch ports.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid);

  /**
   * \name Scheduller structures (used by ScheduleCommand)
   */
  //\{
  /** Multimap between device pointer and dpctl command */
  typedef std::multimap<Ptr<OFSwitch13NetDevice>, std::string> DevCmdMap_t; 
  
  /** Map of scheduled dpctl commands to be executed. */
  DevCmdMap_t m_schedCommands; 
  //\}
  
  /**
   * \name L2 switching structures (used by ARP)
   */
  //\{
  typedef std::map<Mac48Address, uint32_t> L2Table_t;     //!< L2SwitchingTable: map MacAddress to port
  typedef std::map<uint64_t, L2Table_t>    DatapathMap_t; //!< Map datapathID to L2SwitchingTable
  DatapathMap_t                            m_learnedInfo; //!< Switching information for all dapataths
  //\}
};

};  // namespace ns3
#endif // EPC_SDN_CONTROLLER_H

