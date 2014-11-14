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

#include "epc-sdn-controller.h"

NS_LOG_COMPONENT_DEFINE ("EpcSdnController");

namespace ns3 {

EpcSdnController::EpcSdnController ()
{
  NS_LOG_FUNCTION (this);
  SetConnectionCallback (MakeCallback (&EpcSdnController::ConnectionStarted, this));
}

EpcSdnController::~EpcSdnController ()
{
  NS_LOG_FUNCTION (this);
}

void
EpcSdnController::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_schedCommands.clear ();
  m_arpTable.clear ();
}

TypeId 
EpcSdnController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcSdnController")
    .SetParent (OFSwitch13Controller::GetTypeId ())
  ;
  return tid;
}

uint8_t
EpcSdnController::AddBearer (uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  return 0;
}

void
EpcSdnController::ScheduleCommand (Ptr<OFSwitch13NetDevice> device, 
                                   const std::string textCmd)
{
  NS_ASSERT (device);
  std::pair<Ptr<OFSwitch13NetDevice>,std::string> entry (device, textCmd);
  m_schedCommands.insert (entry);
}

void 
EpcSdnController::NotifyNewIpDevice (Ptr<NetDevice> dev, Ipv4Address ip)
{
  Mac48Address macAddr = Mac48Address::ConvertFrom (dev->GetAddress ());
  std::pair<Ipv4Address, Mac48Address> entry (ip, macAddr);
  std::pair <IpMacMap_t::iterator, bool> ret;
  ret = m_arpTable.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New ARP entry: " << ip << " - " << macAddr);
      return;
    }
  NS_FATAL_ERROR ("This IP already existis in this network.");
}


ofl_err
EpcSdnController::HandlePacketIn (ofl_msg_packet_in *msg, 
                                  SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (swtch.ipv4 << xid);

  char *m = ofl_structs_match_to_string ((struct ofl_match_header*)msg->match, NULL);
  NS_LOG_DEBUG ("Packet in match: " << m);
  free (m);

  enum ofp_packet_in_reason reason = msg->reason;
  if (reason == OFPR_NO_MATCH)
    {
      // Get input port
      uint32_t inPort;
      ofl_match_tlv *inPortTlv = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
      memcpy (&inPort, inPortTlv->value, OXM_LENGTH (OXM_OF_IN_PORT));
 
      // Just for testing, let's always send the packet in clockwise direction
      ofl_action_group *action = (ofl_action_group*)xmalloc (sizeof (ofl_action_group));
      action->header.type = OFPAT_GROUP;
      action->group_id = 1;

      // Create the OpenFlow PacketOut message
      ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = msg->buffer_id;
      reply.in_port = inPort;
      reply.data_length = 0;
      reply.data = 0;
      reply.actions_num = 1;
      reply.actions = (ofl_action_header**)&action;
      if (msg->buffer_id == OFP_NO_BUFFER)
        {
          // No packet buffer. Send data back to switch
          reply.data_length = msg->data_length;
          reply.data = msg->data;
        }
    
      SendToSwitch (&swtch, (ofl_msg_header*)&reply, xid);
      free (action);
    }
  else if (reason == OFPR_ACTION)
    {
      // Get Ethernet frame type 
      uint16_t ethType;
      ofl_match_tlv *ethTypeTlv = oxm_match_lookup (OXM_OF_ETH_TYPE, (ofl_match*)msg->match);
      memcpy (&ethType, ethTypeTlv->value, OXM_LENGTH (OXM_OF_ETH_TYPE));

      // Check for ARP packet
      if (ethType == ArpL3Protocol::PROT_NUMBER)
        {
          return HandleArpPacketIn (msg, swtch, xid);
        }
      NS_LOG_WARN ("Packet sent to controller, but we can't handle it.");
    }
  else if (reason == OFPR_INVALID_TTL)
    {
      NS_LOG_WARN ("Discarding packet sent to controller with invalid TTL.");
    }
  else
    {
      NS_FATAL_ERROR ("Error handling PacketIn message.");
    }
  
  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*dp->exp*/);
  return 0;
}

ofl_err
EpcSdnController::HandleFlowRemoved (ofl_msg_flow_removed *msg, 
                                     SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (swtch.ipv4 << xid);
  NS_LOG_DEBUG ( "Flow removed.");

  // All handlers must free the message when everything is ok
  ofl_msg_free_flow_removed (msg, true, NULL);
  return 0;
}


/********** Private methods **********/
void
EpcSdnController::ConnectionStarted (SwitchInfo swtch)
{
  NS_LOG_FUNCTION (this << swtch.ipv4);
  
  // Set the switch to buffer packets and send only the first 128 bytes
  DpctlCommand (swtch, "set-config miss=128");

  // After a successfull handshake, let's install some default entries 
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");  // Table miss
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=65500 eth_type=0x0806 apply:output=ctrl");  // Arp handling

  // Executing any scheduled commands for this switch
  std::pair <DevCmdMap_t::iterator, DevCmdMap_t::iterator> ret;
  ret = m_schedCommands.equal_range (swtch.netdev);
  for (DevCmdMap_t::iterator it = ret.first; it != ret.second; it++)
    {
      DpctlCommand (swtch, it->second);
    }
  m_schedCommands.erase (ret.first, ret.second);
}

Mac48Address 
EpcSdnController::ArpLookup (Ipv4Address ip)
{
  IpMacMap_t::iterator ret;
  ret = m_arpTable.find (ip);
  if (ret != m_arpTable.end ())
    {
      NS_LOG_DEBUG ("Found ARP entry: " << ip << " - " << ret->second);
      return ret->second;
    }
  NS_FATAL_ERROR ("No ARP information for this IP.");
}

Ipv4Address 
EpcSdnController::ExtractIpv4Address (uint32_t oxm_of, ofl_match* match)
{
  switch (oxm_of)
    {
      case OXM_OF_ARP_SPA:
      case OXM_OF_ARP_TPA:
      case OXM_OF_IPV4_DST:
      case OXM_OF_IPV4_SRC:
        {
          uint32_t ip;
          int size = OXM_LENGTH (oxm_of);
          ofl_match_tlv *ipTlv = oxm_match_lookup (oxm_of, match);
          memcpy (&ip, ipTlv->value, size);
          return Ipv4Address (ntohl (ip));
        }
      default:
        NS_FATAL_ERROR ("Invalid IP field.");
    }
}

Ptr<Packet> 
EpcSdnController::CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp, 
                                  Mac48Address dstMac, Ipv4Address dstIp)
{
  Ptr<Packet> packet = Create<Packet> ();
  
  // ARP header
  ArpHeader arp;
  arp.SetReply (srcMac, srcIp, dstMac, dstIp);
  packet->AddHeader (arp);
  
  // Ethernet header
  EthernetHeader eth (false);
  eth.SetSource (srcMac);
  eth.SetDestination (dstMac);
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }
  eth.SetLengthType (ArpL3Protocol::PROT_NUMBER);
  packet->AddHeader (eth);

  // Ethernet trailer
  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);
  
  return packet;
}

ofl_err
EpcSdnController::HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid)
{
  // Get ARP operation
  uint16_t arpOp;
  ofl_match_tlv *arpOpTlv = oxm_match_lookup (OXM_OF_ARP_OP, (ofl_match*)msg->match);
  memcpy (&arpOp, arpOpTlv->value, OXM_LENGTH (OXM_OF_ARP_OP));
 
  // Get input port
  uint32_t inPort;
  ofl_match_tlv *inPortTlv = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
  memcpy (&inPort, inPortTlv->value, OXM_LENGTH (OXM_OF_IN_PORT));

  // Check for ARP request
  if (arpOp == ArpHeader::ARP_TYPE_REQUEST)
    {
      // Get target IP address
      Ipv4Address dstIp = ExtractIpv4Address (OXM_OF_ARP_TPA, (ofl_match*)msg->match);
      
      // Get target MAC address from ARP table
      Mac48Address dstMac = ArpLookup (dstIp);
      NS_LOG_UNCOND ("Got ARP request for IP " << dstIp << ", resolved to " << dstMac);

      // Get source IP address
      Ipv4Address srcIp = ExtractIpv4Address (OXM_OF_ARP_SPA, (ofl_match*)msg->match);

      // Get Source MAC address
      Mac48Address srcMac;
      ofl_match_tlv *ethSrcTlv = oxm_match_lookup (OXM_OF_ARP_SHA, (ofl_match*)msg->match);
      srcMac.CopyFrom (ethSrcTlv->value);

      // Create the ARP reply packet
      Ptr<Packet> pkt = CreateArpReply (dstMac, dstIp, srcMac, srcIp);
      uint8_t pktData[pkt->GetSize ()];
      pkt->CopyData (pktData, pkt->GetSize ());

      // Send the ARP reply within an OpenFlow PacketOut message
      ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = OFP_NO_BUFFER;
      reply.in_port = inPort;
      reply.data_length = pkt->GetSize ();
      reply.data = &pktData[0]; 

      // Send the ARP replay back to the input port
      ofl_action_output *action = (ofl_action_output*)xmalloc (sizeof (ofl_action_output));
      action->header.type = OFPAT_OUTPUT;
      action->port = OFPP_IN_PORT;
      action->max_len = 0;
      
      reply.actions_num = 1;
      reply.actions = (ofl_action_header**)&action;

      int error = SendToSwitch (&swtch, (ofl_msg_header*)&reply, xid);
      free (action);
      if (error)
        {
          NS_LOG_ERROR ("Error sending packet out with arp request"); 
        }
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*dp->exp*/);
  return 0;
}

};  // namespace ns3

