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

#include "openflow-epc-controller.h"

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcController);

OpenFlowEpcController::OpenFlowEpcController ()
{
  NS_LOG_FUNCTION (this);
}

OpenFlowEpcController::~OpenFlowEpcController ()
{
  NS_LOG_FUNCTION (this);
}

void
OpenFlowEpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ipSwitchTable.clear ();
  m_arpTable.clear ();
  m_connections.clear ();
  m_contexts.clear ();
  m_ofNetwork = 0;
}

TypeId 
OpenFlowEpcController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenFlowEpcController")
    .SetParent (OFSwitch13Controller::GetTypeId ())
    .AddAttribute ("OFNetwork",
                   "The OpenFlow network controlled by this app.",
                   PointerValue (),
                   MakePointerAccessor (&OpenFlowEpcController::m_ofNetwork),
                   MakePointerChecker<OpenFlowEpcNetwork> ())
  ;
  return tid;
}

void 
OpenFlowEpcController::NotifyNewIpDevice (Ptr<NetDevice> dev, Ipv4Address ip, 
    uint16_t switchIdx)
{
  { // Save the pair IP/MAC address in ARP table
    Mac48Address macAddr = Mac48Address::ConvertFrom (dev->GetAddress ());
    std::pair<Ipv4Address, Mac48Address> entry (ip, macAddr);
    std::pair <IpMacMap_t::iterator, bool> ret;
    ret = m_arpTable.insert (entry);
    if (ret.second == false)
      {
        NS_FATAL_ERROR ("This IP already exists in ARP table.");
      }
    NS_LOG_DEBUG ("New ARP entry: " << ip << " - " << macAddr);
  }

  { // Save the pair IP/Switch index in switch table
    std::pair<Ipv4Address, uint16_t> entry (ip, switchIdx);
    std::pair <IpSwitchMap_t::iterator, bool> ret;
    ret = m_ipSwitchTable.insert (entry);
    if (ret.second == false)
      {
        NS_FATAL_ERROR ("This IP already existis in switch index table.");
      }
    NS_LOG_DEBUG ("New IP/Switch entry: " << ip << " - " << switchIdx);
  }
}

void
OpenFlowEpcController::NotifyNewSwitchConnection (
    const Ptr<ConnectionInfo> connInfo)
{
  // Save this connection info
  SwitchPair_t key;
  key.first  = std::min (connInfo->switchIdx1, connInfo->switchIdx2);
  key.second = std::max (connInfo->switchIdx1, connInfo->switchIdx2);
  std::pair<SwitchPair_t, Ptr<ConnectionInfo> > entry (key, connInfo);
  std::pair<ConnInfoMap_t::iterator, bool> ret;
  ret = m_connections.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Error saving connection info.");
    }
  NS_LOG_DEBUG ("New connection info saved: switch " << key.first << 
                " (" << connInfo->portNum1 << ") -- switch " << key.second << 
                " (" << connInfo->portNum2 << ")");
}

bool
OpenFlowEpcController::RequestNewDedicatedBearer (uint64_t imsi, 
    uint16_t cellId, Ptr<EpcTft> tft, EpsBearer bearer)
{
  // Allowing any bearer creation
  return true;
}

void 
OpenFlowEpcController::NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
    Ipv4Address enbAddr, Ipv4Address sgwAddr, ContextBearers_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr);

  // Create context info and save in context list.
  Ptr<ContextInfo> info = Create<ContextInfo> ();
  info->imsi = imsi;
  info->cellId = cellId;
  info->enbIdx = GetSwitchIdxFromIp (enbAddr);
  info->sgwIdx = GetSwitchIdxFromIp (sgwAddr);
  info->enbAddr = enbAddr;
  info->sgwAddr = sgwAddr;
  info->bearerList = bearerList;

  m_contexts.push_back (info);
}

bool 
OpenFlowEpcController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);
  return true;
}

bool
OpenFlowEpcController::NotifyAppStop (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);
  return true;
}

void 
OpenFlowEpcController::ConfigurePortDelivery (Ptr<OFSwitch13NetDevice> swtch, 
    Ptr<NetDevice> device, Ipv4Address deviceIp, uint32_t devicePort)
{
  NS_LOG_FUNCTION (this << swtch << deviceIp << (int)devicePort);

  Mac48Address devMacAddr = Mac48Address::ConvertFrom (device->GetAddress ());
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=40000" <<
         " eth_type=0x800,eth_dst=" << devMacAddr <<
         ",ip_dst=" << deviceIp << " apply:output=" << devicePort;
  DpctlCommand (swtch, cmd.str ());
}

void
OpenFlowEpcController::CreateSpanningTree ()
{
  NS_LOG_WARN ("No Spanning Tree Protocol implemented here.");
}

Ptr<ConnectionInfo>
OpenFlowEpcController::GetConnectionInfo (uint16_t sw1, uint16_t sw2)
{
  SwitchPair_t key;
  key.first = std::min (sw1, sw2);
  key.second = std::max (sw1, sw2);
  ConnInfoMap_t::iterator it = m_connections.find (key);
  if (it != m_connections.end ())
    {
      return it->second;
    }
  NS_FATAL_ERROR ("No connection information available.");
}

uint16_t 
OpenFlowEpcController::GetNSwitches ()
{
  return m_ofNetwork->GetNSwitches ();
}

Ptr<OFSwitch13NetDevice> 
OpenFlowEpcController::GetSwitchDevice (uint16_t index)
{
  return m_ofNetwork->GetSwitchDevice (index);
}

uint16_t 
OpenFlowEpcController::GetSwitchIdxForGateway ()
{
  return m_ofNetwork->GetSwitchIdxForGateway ();
}

uint16_t
OpenFlowEpcController::GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev)
{
  return m_ofNetwork->GetSwitchIdxForDevice (dev);
}

uint16_t 
OpenFlowEpcController::GetSwitchIdxFromIp (Ipv4Address addr)
{
  IpSwitchMap_t::iterator ret;
  ret = m_ipSwitchTable.find (addr);
  if (ret != m_ipSwitchTable.end ())
    {
      uint16_t idx = ret->second;
      NS_LOG_DEBUG ("Found sw index " << idx << " for IP " << addr);
      return idx;
    }
  NS_FATAL_ERROR ("IP not registered in switch index table.");
}

Ptr<OpenFlowEpcController::ContextInfo>
OpenFlowEpcController::GetContextFromTft (Ptr<EpcTft> tft)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      ContextBearers_t::iterator blsIt = cInfo->bearerList.begin (); 
      for ( ; blsIt != cInfo->bearerList.end (); blsIt++)
        {
          if (blsIt->tft == tft)
            {
              NS_LOG_DEBUG ("Found context for tft " << tft);
              return cInfo;
            }
        }
    }
  NS_FATAL_ERROR ("Invalid tft.");
}

Ptr<OpenFlowEpcController::ContextInfo>
OpenFlowEpcController::GetContextFromTeid (uint32_t teid)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      ContextBearers_t::iterator blsIt = cInfo->bearerList.begin (); 
      for ( ; blsIt != cInfo->bearerList.end (); blsIt++)
        {
          if (blsIt->sgwFteid.teid == teid)
            {
              NS_LOG_DEBUG ("Found bearer for teid " << teid);
              return cInfo;
            }
        }
    }
  NS_FATAL_ERROR ("Invalid teid.");
}

EpcS11SapMme::BearerContextCreated 
OpenFlowEpcController::GetBearerFromTft (Ptr<EpcTft> tft)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      ContextBearers_t::iterator blsIt = cInfo->bearerList.begin (); 
      for ( ; blsIt != cInfo->bearerList.end (); blsIt++)
        {
          if (blsIt->tft == tft)
            {
              NS_LOG_DEBUG ("Found bearer for tft " << tft);
              return *blsIt;
            }
        }
    }
  NS_FATAL_ERROR ("Invalid tft.");
}

ofl_err
OpenFlowEpcController::HandlePacketIn (ofl_msg_packet_in *msg, 
    SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (swtch.ipv4 << xid);
  ofl_match_tlv *tlv;

  enum ofp_packet_in_reason reason = msg->reason;
  if (reason == OFPR_NO_MATCH)
    {
      char *m = ofl_structs_match_to_string (msg->match, 0);
      NS_LOG_DEBUG ("Packet in match: " << m);
      free (m);

      // (Table #1 is used only for GTP TEID routing)
      uint8_t tableId = msg->table_id;
      if (tableId == 1)
        {
          uint32_t teid;
          tlv = oxm_match_lookup (OXM_OF_GTPU_TEID, (ofl_match*)msg->match);
          memcpy (&teid, tlv->value, OXM_LENGTH (OXM_OF_GTPU_TEID));

          NS_LOG_DEBUG ("TEID routing miss packet: " << teid);
          return HandleGtpuTeidPacketIn (msg, swtch, xid, teid);
        }
    }
  else if (reason == OFPR_ACTION)
    {
      // Get Ethernet frame type 
      uint16_t ethType;
      tlv = oxm_match_lookup (OXM_OF_ETH_TYPE, (ofl_match*)msg->match);
      memcpy (&ethType, tlv->value, OXM_LENGTH (OXM_OF_ETH_TYPE));

      // Check for ARP packet
      if (ethType == ArpL3Protocol::PROT_NUMBER)
        {
          return HandleArpPacketIn (msg, swtch, xid);
        }
    }

  NS_LOG_WARN ("Ignoring packet sent to controller.");
  
  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

ofl_err
OpenFlowEpcController::HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
    SwitchInfo swtch, uint32_t xid, uint32_t teid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << teid);
  NS_LOG_WARN ("No handling implemented here.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

Ipv4Address 
OpenFlowEpcController::ExtractIpv4Address (uint32_t oxm_of, ofl_match* match)
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
          ofl_match_tlv *tlv = oxm_match_lookup (oxm_of, match);
          memcpy (&ip, tlv->value, size);
          return Ipv4Address (ntohl (ip));
        }
      default:
        NS_FATAL_ERROR ("Invalid IP field.");
    }
}

void
OpenFlowEpcController::ConnectionStarted (SwitchInfo swtch)
{
  NS_LOG_FUNCTION (this << swtch.ipv4);
  
  // Set the switch to buffer packets and send only the first 128 bytes
  DpctlCommand (swtch, "set-config miss=128");

  // After a successfull handshake, let's install some default entries: table
  // miss entry and arp handling entry.
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=1 eth_type=0x0806 "
                       "apply:output=ctrl");

  // Handling GTP tunnels at table #1
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=2 eth_type=0x800,"
                       "ip_proto=17,udp_src=2152,udp_dst=2152 goto:1");
  DpctlCommand (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");
}

ofl_err
OpenFlowEpcController::HandleArpPacketIn (ofl_msg_packet_in *msg, 
    SwitchInfo swtch, uint32_t xid)
{
  ofl_match_tlv *tlv;

  // Get ARP operation
  uint16_t arpOp;
  tlv = oxm_match_lookup (OXM_OF_ARP_OP, (ofl_match*)msg->match);
  memcpy (&arpOp, tlv->value, OXM_LENGTH (OXM_OF_ARP_OP));
 
  // Get input port
  uint32_t inPort;
  tlv = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
  memcpy (&inPort, tlv->value, OXM_LENGTH (OXM_OF_IN_PORT));

  // Check for ARP request
  if (arpOp == ArpHeader::ARP_TYPE_REQUEST)
    {
      // Get target IP address
      Ipv4Address dstIp;
      dstIp = ExtractIpv4Address (OXM_OF_ARP_TPA, (ofl_match*)msg->match);
      
      // Get target MAC address from ARP table
      Mac48Address dstMac = ArpLookup (dstIp);
      NS_LOG_DEBUG ("Got ARP request for IP " << dstIp << 
                    ", resolved to " << dstMac);

      // Get source IP address
      Ipv4Address srcIp; 
      srcIp = ExtractIpv4Address (OXM_OF_ARP_SPA, (ofl_match*)msg->match);

      // Get Source MAC address
      Mac48Address srcMac;
      tlv = oxm_match_lookup (OXM_OF_ARP_SHA, (ofl_match*)msg->match);
      srcMac.CopyFrom (tlv->value);

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
      ofl_action_output *action;
      action = (ofl_action_output*)xmalloc (sizeof (ofl_action_output));
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
  else 
    {
      NS_LOG_WARN ("Not supposed to get ARP reply. Ignoring...");
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

Mac48Address 
OpenFlowEpcController::ArpLookup (Ipv4Address ip)
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

Ptr<Packet> 
OpenFlowEpcController::CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp, 
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

};  // namespace ns3

