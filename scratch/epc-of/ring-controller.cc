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

#include "ring-controller.h"

NS_LOG_COMPONENT_DEFINE ("RingController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RingController);

RingController::RingController ()
{
  NS_LOG_FUNCTION (this);
}

RingController::~RingController ()
{
  NS_LOG_FUNCTION (this);
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  EpcSdnController::DoDispose ();
}

TypeId 
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent (EpcSdnController::GetTypeId ())
  ;
  return tid;
}

void
RingController::NotifyNewSwitchConnection (ConnectionInfo connInfo)
{
  NS_LOG_FUNCTION (this);
  
  // Call base method which will save this information
  EpcSdnController::NotifyNewSwitchConnection (connInfo);
  
  // Installing default groups for RingController ring routing. Group #1 is
  // used to send packets from current switch to the next one in clockwise
  // direction.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=" << RingController::CLOCK <<
          " weight=0,port=any,group=any output=" << connInfo.portNum1;
  ScheduleCommand (connInfo.switchDev1, cmd1.str ());
                                   
  // Group #2 is used to send packets from the next switch to the current one
  // in counterclockwise direction. 
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=" << 
          RingController::COUNTERCLOCK <<
          " weight=0,port=any,group=any output=" << connInfo.portNum2;
  ScheduleCommand (connInfo.switchDev2, cmd2.str ());
}

void 
RingController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);
 
  Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
  ContextInfo contextInfo = GetContextFromTft (tft);

  EpcS11SapMme::BearerContextCreated bearerContext;
  bearerContext = GetBearerFromTft (tft);
  uint32_t teid = bearerContext.sgwFteid.teid;
  EpsBearer bearer = bearerContext.bearerLevelQos;

  if (HasTeidRoutingInfo (teid))
    {
      NS_LOG_DEBUG ("Routing path for " << teid << " already defined.");
      return;
    }
 
  NS_LOG_DEBUG ("Bearer TEID " << teid);
 
  // Create and save routing information
  RoutingInfo routingInfo;
  routingInfo.gatewayIdx = contextInfo.sgwIdx;
  routingInfo.enbIdx = contextInfo.enbIdx;
  routingInfo.path = FindShortestPath (routingInfo.gatewayIdx, 
                                       routingInfo.enbIdx); 
  routingInfo.app = app;
  routingInfo.gatewayAddr = contextInfo.sgwAddr;
  routingInfo.enbAddr = contextInfo.enbAddr;
  routingInfo.teid = teid;
 
  // Check for GBR bearer resources
  if (bearer.IsGbr ())
    {
      DataRate downlinkGbr (bearer.gbrQosInfo.gbrDl);
      DataRate uplinkGbr (bearer.gbrQosInfo.gbrUl);
      DataRate reserve = std::max (downlinkGbr, uplinkGbr);
      NS_LOG_DEBUG ("Bearer " << teid << " requesting " << reserve);
      
      DataRate bandwidth = GetAvailableBandwidth (routingInfo.gatewayIdx, 
                                                  routingInfo.enbIdx, 
                                                  routingInfo.path);
      NS_LOG_DEBUG ("Bandwidth from " << routingInfo.gatewayIdx << 
                    " to " << routingInfo.enbIdx << ": " << bandwidth);
      
      if (bandwidth >= reserve)
        {
          ReserveBandwidth (routingInfo.gatewayIdx, routingInfo.enbIdx, 
                            routingInfo.path, reserve);
          routingInfo.reserved = reserve;
        }
      else 
        {
          NS_LOG_WARN ("Not enougth resources for bearer: " << reserve <<
                       "in direction " << routingInfo.path);
          
          // Inverting routing direction and looking for available resources
          Routing newDir = InvertRoutingDirection (routingInfo.path);
          bandwidth = GetAvailableBandwidth (routingInfo.gatewayIdx, 
                                             routingInfo.enbIdx, 
                                             newDir);
          if (bandwidth >= reserve)
            {
              routingInfo.path = newDir;
              ReserveBandwidth (routingInfo.gatewayIdx, routingInfo.enbIdx, 
                               routingInfo.path, reserve);
              routingInfo.reserved = reserve;
            }
          else
            {
              NS_FATAL_ERROR ("Not enought resources in any direction.");
            }
        }
    }
  
  SaveTeidRouting (routingInfo);
  ConfigureRoutingPath (teid);
}

void
RingController::CreateSpanningTree ()
{
  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD). Here we are disabling the farthest gateway link,
  // configuring its ports to OFPPC_NO_FWD flag (0x20).
  
  uint16_t half = (GetNSwitches () / 2);
  ConnectionInfo* info = GetConnectionInfo (half, half+1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " << 
                 half+1 << " for broadcast messages.");
  
  std::ostringstream cmd1;
  Mac48Address macAddr1 = 
      Mac48Address::ConvertFrom (info->portDev1->GetAddress ());
  cmd1 << "port-mod port=" << info->portNum1 << ",addr=" << 
           macAddr1 << ",conf=0x00000020,mask=0x00000020";
  ScheduleCommand (info->switchDev1, cmd1.str ());

  std::ostringstream cmd2;
  Mac48Address macAddr2 = 
      Mac48Address::ConvertFrom (info->portDev2->GetAddress ());
  cmd2 << "port-mod port=" << info->portNum2 << ",addr=" << 
           macAddr2 << ",conf=0x00000020,mask=0x00000020";
  ScheduleCommand (info->switchDev2, cmd2.str ());
}

ofl_err
RingController::HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
                                        SwitchInfo swtch, uint32_t xid, 
                                        uint32_t teid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << teid);

  // Get input port
  uint32_t inPort;
  ofl_match_tlv *inPortTlv = 
      oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
  memcpy (&inPort, inPortTlv->value, OXM_LENGTH (OXM_OF_IN_PORT));

  // Creating group action.
  ofl_action_group *action = 
      (ofl_action_group*)xmalloc (sizeof (ofl_action_group));
  action->header.type = OFPAT_GROUP;
  action->group_id = 1;

  // Let's check for existing routing path 
  if (HasTeidRoutingInfo (teid))
    {
      RoutingInfo rInfo = GetTeidRoutingInfo (teid);
      ContextInfo cInfo = GetContextFromTeid (teid);
      Ipv4Address dest = 
          ExtractIpv4Address (OXM_OF_IPV4_DST, (ofl_match*)msg->match);
      if (dest.IsEqual (cInfo.enbAddr))
        {
          // Downlink packet. Send in path direction
          action->group_id = (int)rInfo.path;
        }
      else if (dest.IsEqual (cInfo.sgwAddr))
        {
          Routing invert = InvertRoutingDirection (rInfo.path);
          action->group_id = (int) invert;
        }
    }
  else
    {
      // FIXME Flood??? Discard???
    }

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

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*dp->exp*/);
  return 0;
}

RingController::Routing
RingController::FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) < GetNSwitches ());
  
  // General rule for ring with 3 or more switches
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstSwitchIdx - srcSwitchIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  
  return (clockwiseDistance <= maxHops) ? 
      RingController::CLOCK : 
      RingController::COUNTERCLOCK;
}

RingController::Routing 
RingController::InvertRoutingDirection (Routing original)
{
  return original == RingController::CLOCK ? 
                     RingController::COUNTERCLOCK : 
                     RingController::CLOCK; 
}

DataRate 
RingController::GetAvailableBandwidth (uint16_t srcSwitchIdx, 
                                       uint16_t dstSwitchIdx,
                                       Routing routingPath)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  
  // Get bandwitdh for first hop
  uint16_t current = srcSwitchIdx;
  uint16_t next = NextSwitchIndex (current, routingPath);
  ConnectionInfo* conn = GetConnectionInfo (current, next);
  DataRate bandwidth = conn->availableDataRate;

  // Repeat the proccess for next hops
  while (next != dstSwitchIdx)
    {
      current = next;
      next = NextSwitchIndex (current, routingPath);
      conn = GetConnectionInfo (current, next);
      if (conn->availableDataRate < bandwidth)
        {
          bandwidth = conn->availableDataRate;
        }
    }
  return bandwidth;
}

bool 
RingController::ReserveBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                  Routing routingPath, DataRate bandwidth)
{
  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      ConnectionInfo* conn = GetConnectionInfo (current, next);
      conn->availableDataRate = conn->availableDataRate - bandwidth;
      NS_ABORT_IF (conn->availableDataRate < 0);
      current = next;
    }
  return true;
}

bool 
RingController::ReleaseBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                  Routing routingPath, DataRate bandwidth)
{
  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      ConnectionInfo* conn = GetConnectionInfo (current, next);
      conn->availableDataRate = conn->availableDataRate + bandwidth;
      current = next;
    }
  return true;
}

uint16_t 
RingController::NextSwitchIndex (uint16_t current, Routing path)
{
  if (path == RingController::CLOCK)
    {
      return (current + 1) % GetNSwitches ();
    }
  else
    {
      return current == 0 ? GetNSwitches () - 1 : (current - 1);
    }
}

DataRate 
RingController::GetTunnelAverageTraffic (uint32_t teid)
{
  return DataRate ();
}

void 
RingController::SaveTeidRouting (RoutingInfo info)
{
  std::pair <uint32_t, RoutingInfo> entry (info.teid, info);
  std::pair <TeidRouting_t::iterator, bool> ret;
  ret = m_routes.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing routing information.");
    }
}

RingController::RoutingInfo 
RingController::GetTeidRoutingInfo (uint32_t teid)
{
  TeidRouting_t::iterator ret;
  ret = m_routes.find (teid);
  if (ret != m_routes.end ())
    {
      NS_LOG_DEBUG ("Found routing information for teid " << teid);
      return ret->second;
    }
  NS_FATAL_ERROR ("No routing information.");
}

bool
RingController::HasTeidRoutingInfo (uint32_t teid)
{
  bool found = false;
  TeidRouting_t::iterator ret;
  ret = m_routes.find (teid);
  if (ret != m_routes.end ())
    {
      found = true;
    }
  return found;
}

bool 
RingController::ConfigureRoutingPath (uint32_t teid)
{
  static int priority = 100;
  
  NS_LOG_FUNCTION (this << teid);

  TeidRouting_t::iterator ret;
  ret = m_routes.find (teid);
  if (ret == m_routes.end ())
    {
      NS_FATAL_ERROR ("No routing information for teid " << teid);
    }
  RoutingInfo info = ret->second;
 
  { // Configuring downlink routing
    std::ostringstream cmdDl;
    cmdDl << "flow-mod cmd=add,table=1,prio=" << priority++ << 
             " eth_type=0x800,ip_proto=17,udp_src=2152," << 
             "udp_dst=2152,ip_src=" << info.gatewayAddr <<
             ",ip_dst=" << info.enbAddr <<
             ",gtp_teid=" << info.teid <<
             " apply:group=" << info.path;
    
    uint16_t current = info.gatewayIdx;
    Ptr<OFSwitch13NetDevice> currentDevice = GetSwitchDevice (current);       
    DpctlCommand (currentDevice, cmdDl.str ());

    uint16_t next = NextSwitchIndex (current, info.path);
    while (next != info.enbIdx)
      {
        current = next;
        currentDevice = GetSwitchDevice (current); 
        DpctlCommand (currentDevice, cmdDl.str ());
        next = NextSwitchIndex (next, info.path);
      }
  }
  
  { // Configuring uplink routing
    
    // Inverting routing direction
    Routing direction = InvertRoutingDirection (info.path); 
    
    std::ostringstream cmdUl;
    cmdUl << "flow-mod cmd=add,table=1,prio=" << priority++ <<
             " eth_type=0x800,ip_proto=17,udp_src=2152," << 
             "udp_dst=2152,ip_src=" << info.enbAddr <<
             ",ip_dst=" << info.gatewayAddr <<
             ",gtp_teid=" << info.teid <<
             " apply:group=" << direction;  

    uint16_t current = info.enbIdx;
    Ptr<OFSwitch13NetDevice> currentDevice = GetSwitchDevice (current);
    DpctlCommand (currentDevice, cmdUl.str ());

    uint16_t next = NextSwitchIndex (current, direction);
    while (next != info.gatewayIdx)
      {
        current = next;
        currentDevice = GetSwitchDevice (current); 
        DpctlCommand (currentDevice, cmdUl.str ());
        next = NextSwitchIndex (next, direction);
      }
  }
  return true;
}

};  // namespace ns3

