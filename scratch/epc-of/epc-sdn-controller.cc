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
  NS_LOG_FUNCTION_NOARGS ();
  SetConnectionCallback (MakeCallback (&EpcSdnController::ConnectionStarted, this));
}

EpcSdnController::~EpcSdnController ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
EpcSdnController::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_schedCommands.clear ();
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
EpcSdnController::ScheduleCommand (Ptr<OFSwitch13NetDevice> device, const std::string textCmd)
{
  NS_ASSERT (device);
  m_schedCommands.insert (std::pair<Ptr<OFSwitch13NetDevice>,std::string> (device, textCmd));
}

ofl_err
EpcSdnController::HandlePacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (swtch.ipv4 << xid);

  char *m = ofl_structs_match_to_string ((struct ofl_match_header*)msg->match, NULL);
  NS_LOG_DEBUG ("Packet in match: " << m);
  free (m);

  enum ofp_packet_in_reason reason = msg->reason;
  if (reason == OFPR_NO_MATCH)
    {
      // What is this frame carrying ? 
      uint16_t ethType;
      size_t ethTypeLen = OXM_LENGTH (OXM_OF_ETH_TYPE);
      ofl_match_tlv *ethTypeTlv = oxm_match_lookup (OXM_OF_ETH_TYPE, (ofl_match*)msg->match);
      memcpy (&ethType, ethTypeTlv->value, ethTypeLen);

      if (ethType == ArpL3Protocol::PROT_NUMBER)
        {
          return HandleArpPacketIn (msg, swtch, xid); 
        }

      // Let's get necessary information (input port)
      uint32_t inPort;
      size_t portLen = OXM_LENGTH (OXM_OF_IN_PORT); // (Always 4 bytes)
      ofl_match_tlv *input = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
      memcpy (&inPort, input->value, portLen);

      // Lets send the packet out to switch.
      ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = msg->buffer_id;
      reply.in_port = inPort;
      reply.data_length = 0;
      reply.data = 0;

      if (msg->buffer_id == NO_BUFFER)
        {
          // No packet buffer. Send data back to switch
          reply.data_length = msg->data_length;
          reply.data = msg->data;
        }

      // ofl_action_output *a = (ofl_action_output*)xmalloc (sizeof (ofl_action_output));
      // a->header.type = OFPAT_OUTPUT;
      // a->port = outPort;
      // a->max_len = 0;

      // Just for testing, let's always send to clockwise direction
      ofl_action_group *a = (ofl_action_group*)xmalloc (sizeof (ofl_action_group));
      a->header.type = OFPAT_GROUP;
      a->group_id = 1;

      reply.actions_num = 1;
      reply.actions = (ofl_action_header**)&a;

      SendToSwitch (&swtch, (ofl_msg_header*)&reply, xid);
      free (a);
    }
  else
    {
      NS_LOG_WARN ("This packet was sent to controller for a reason that we can't handle.");
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*dp->exp*/);
  return 0;
}

ofl_err
EpcSdnController::HandleFlowRemoved (ofl_msg_flow_removed *msg, SwitchInfo swtch, uint32_t xid)
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

  DpctlCommand (swtch, "port-desc");

  // After a successfull handshake, let's install some default entries 
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=1 eth_dst=ff:ff:ff:ff:ff:ff apply:output=flood");

  // Executing any scheduled commands for this switch
  std::pair <DevCmdMap_t::iterator, DevCmdMap_t::iterator> ret;
  ret = m_schedCommands.equal_range (swtch.netdev);
  for (DevCmdMap_t::iterator it = ret.first; it != ret.second; it++)
    {
      DpctlCommand (swtch, it->second);
    }
  m_schedCommands.erase (ret.first, ret.second);
}

ofl_err
EpcSdnController::HandleArpPacketIn (ofl_msg_packet_in *msg, SwitchInfo swtch, uint32_t xid)
{
  // Get L2Table for this datapath
 // uint64_t dpId = swtch.netdev->GetDatapathId ();
 // DatapathMap_t::iterator it = m_learnedInfo.find (dpId);
 // if (it != m_learnedInfo.end ())
 //   {
 //     L2Table_t *l2Table = &it->second;
 //   }

  // Is this a request or a response?
  uint16_t arpOp;
  size_t arpOpLen = OXM_LENGTH (OXM_OF_ARP_OP);
  ofl_match_tlv *arpOpTlv = oxm_match_lookup (OXM_OF_ARP_OP, (ofl_match*)msg->match);
  memcpy (&arpOp, arpOpTlv->value, arpOpLen);

  //if (arpOp == ArpHeader::ARP_TYPE_REQUEST)
    {
      uint32_t inPort;
      size_t inPortLen = OXM_LENGTH (OXM_OF_IN_PORT);
      ofl_match_tlv *inPortTlv = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
      memcpy (&inPort, inPortTlv->value, inPortLen);

      Mac48Address ethSrc;
      ofl_match_tlv *ethSrcTlv = oxm_match_lookup (OXM_OF_ETH_DST, (ofl_match*)msg->match);
      ethSrc.CopyFrom (ethSrcTlv->value);

      Mac48Address ethDst;
      ofl_match_tlv *ethDstTlv = oxm_match_lookup (OXM_OF_ETH_DST, (ofl_match*)msg->match);
      ethDst.CopyFrom (ethDstTlv->value);
      //NS_ASSERT_MSG (ethDst.IsBroadcast (), "Invalid broadcast address for arp request.");

      // Let's flood this request over switch ports
      ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = msg->buffer_id;
      reply.in_port = inPort;
      reply.data_length = 0;
      reply.data = 0;

      if (msg->buffer_id == NO_BUFFER)
        {
          // No packet buffer. Send data back to switch
          reply.data_length = msg->data_length;
          reply.data = msg->data;
        }

      ofl_action_output *a = (ofl_action_output*)xmalloc (sizeof (ofl_action_output));
      a->header.type = OFPAT_OUTPUT;
      a->port = OFPP_FLOOD;
      a->max_len = 0;

      reply.actions_num = 1;
      reply.actions = (ofl_action_header**)&a;

      int error = SendToSwitch (&swtch, (ofl_msg_header*)&reply, xid);
      free (a);
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

