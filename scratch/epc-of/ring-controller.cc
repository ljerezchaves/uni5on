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
  : m_priority (1000),
    m_gbrBearers (0),
    m_gbrBlocks (0)
{
  NS_LOG_FUNCTION (this);

  // Schedulle continuous stats update
  // Simulator::Schedule (m_timeout, &RingController::QuerySwitchStats, this);
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
  m_routes.clear ();
}

double
RingController::PrintBlockRatioStatistics ()
{
  double ratio = (double)m_gbrBlocks/m_gbrBearers;
  std::cout << "Number of GBR bearers request: " << m_gbrBearers << std::endl
            << "Number of GBR bearers blocked: " << m_gbrBlocks << std::endl
            << "Block ratio: " << ratio << std::endl;
  return ratio;
}

TypeId 
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent (EpcSdnController::GetTypeId ())
    .AddAttribute ("Strategy", 
                   "The ring routing strategy.",
                   EnumValue (RingController::HOPS),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::HOPS, "Hops",
                                    RingController::BAND, "Bandwidth"))
    .AddAttribute ("StatsTimeout",
                   "The interval between query stats from switches.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RingController::m_timeout),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
RingController::NotifyNewSwitchConnection (const Ptr<ConnectionInfo> connInfo)
{
  NS_LOG_FUNCTION (this);
  
  // Call base method which will save connection information
  EpcSdnController::NotifyNewSwitchConnection (connInfo);
  
  // Installing default groups for RingController ring routing. Group
  // RingController::CLOCK is used to send packets from current switch to the
  // next one in clockwise direction.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=" << RingController::CLOCK <<
          " weight=0,port=any,group=any output=" << connInfo->portNum1;
  DpctlCommand (connInfo->switchDev1, cmd1.str ());
                                   
  // Group RingController::COUNTER is used to send packets from the next
  // switch to the current one in counterclockwise direction. 
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=" << RingController::COUNTER <<
          " weight=0,port=any,group=any output=" << connInfo->portNum2;
  DpctlCommand (connInfo->switchDev2, cmd2.str ());
}

void 
RingController::NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
                                         Ipv4Address enbAddr, 
                                         Ipv4Address sgwAddr,
                                         ContextBearers_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr);
  static int defPrio = 100;  // Priority for default bearers
  
  // Call base method which will save context information
  EpcSdnController::NotifyNewContextCreated (imsi, cellId, enbAddr, sgwAddr, 
                                             bearerList);
  
  // Create and save routing information for default bearer
  EpcS11SapMme::BearerContextCreated defaultBearer = bearerList.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
  
  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  NS_ASSERT_MSG (rInfo == 0, "Existing routing for default bearer " << teid);

  rInfo = Create<RoutingInfo> ();
  rInfo->teid = teid;
  rInfo->bearer = defaultBearer;
  rInfo->sgwIdx = GetSwitchIdxFromIp (sgwAddr);
  rInfo->enbIdx = GetSwitchIdxFromIp (enbAddr);
  rInfo->sgwAddr = sgwAddr;
  rInfo->enbAddr = enbAddr;
  rInfo->downPath = FindShortestPath (rInfo->sgwIdx, rInfo->enbIdx);
  rInfo->upPath = InvertRoutingPath (rInfo->downPath);
  rInfo->app = NULL;            // No app for default bearer
  rInfo->priority = defPrio++;  // Priority for default bearer
  rInfo->timeout = 0;           // No timeout for default bearer entries
  rInfo->isInstalled = false;   // Rules not installed yet
  rInfo->isActive = true;       // Default bearer is always active
  rInfo->isDefault = true;      // This is a default bearer

  SaveTeidRoutingInfo (rInfo);
  InstallTeidRouting (rInfo);
}

bool
RingController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  // Get GTP TEID for this application
  Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
  EpcS11SapMme::BearerContextCreated dedicatedBearer = GetBearerFromTft (tft);
  uint32_t teid = dedicatedBearer.sgwFteid.teid;

  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_LOG_DEBUG ("First use of bearer TEID " << teid);
      Ptr<ContextInfo> cInfo = GetContextFromTft (tft);

      // Create and save routing information for dedicated bearer
      rInfo = Create<RoutingInfo> ();
      rInfo->teid = teid;
      rInfo->bearer = dedicatedBearer;
      rInfo->sgwIdx = cInfo->sgwIdx;
      rInfo->enbIdx = cInfo->enbIdx;
      rInfo->sgwAddr = cInfo->sgwAddr;
      rInfo->enbAddr = cInfo->enbAddr;
      rInfo->downPath = FindShortestPath (rInfo->sgwIdx, rInfo->enbIdx);
      rInfo->upPath = InvertRoutingPath (rInfo->downPath);
      rInfo->app = app;               // App for this dedicated bearer
      rInfo->priority = ++m_priority; // Priority for dedicated bearer
      rInfo->timeout = 10;            // Default idle timeout (10 sec)
      rInfo->isInstalled = false;     // Not installed yet
      rInfo->isActive = false;        // Bearer not active yet
      rInfo->isDefault = false;       // This is a dedicated bearer

      SaveTeidRoutingInfo (rInfo);
    }

  // Check for dedicated GBR bearer
  if (rInfo->isActive == false && rInfo->bearer.bearerLevelQos.IsGbr ())
    {
      ProcessGbrRequest (rInfo);
    }

  rInfo->isActive = true;     // Activated by AppStartSending callback
  if (rInfo->isInstalled == false)
    {
      InstallTeidRouting (rInfo);
    }
  else
    {
      NS_LOG_DEBUG ("Routing path for " << teid << " already installed.");
    }
  return true;
}

bool
RingController::NotifyAppStop (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
  uint32_t teid = GetBearerFromTft (tft).sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("No routing information for teid " << teid);
    }
  
  // Check for active application
  if (rInfo->isActive == true)
    {
      rInfo->isActive = false;
      if (rInfo->bearer.bearerLevelQos.IsGbr ())
        {
          ReleaseBandwidth (rInfo); 
        }
      // No need to remove the rules... wait for idle timeout
    }
  return true;
}

void
RingController::CreateSpanningTree ()
{
  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD). Here we are disabling the farthest gateway link,
  // configuring its ports to OFPPC_NO_FWD flag (0x20).
  
  uint16_t half = (GetNSwitches () / 2);
  Ptr<ConnectionInfo> connInfo = GetConnectionInfo (half, half+1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " << 
                 half+1 << " for broadcast messages.");
  
  Mac48Address macAddr1;
  macAddr1 = Mac48Address::ConvertFrom (connInfo->portDev1->GetAddress ());
  std::ostringstream cmd1;
  cmd1 << "port-mod port=" << connInfo->portNum1 << ",addr=" << 
           macAddr1 << ",conf=0x00000020,mask=0x00000020";
  DpctlCommand (connInfo->switchDev1, cmd1.str ());

  Mac48Address macAddr2;
  macAddr2 = Mac48Address::ConvertFrom (connInfo->portDev2->GetAddress ());
  std::ostringstream cmd2;
  cmd2 << "port-mod port=" << connInfo->portNum2 << ",addr=" << 
           macAddr2 << ",conf=0x00000020,mask=0x00000020";
  DpctlCommand (connInfo->switchDev2, cmd2.str ());
}

ofl_err
RingController::HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
                                        SwitchInfo swtch, uint32_t xid, 
                                        uint32_t teid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << teid);

  // Let's check for existing routing path
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo && rInfo->isActive)
    {
      NS_LOG_WARN ("Not supposed to happen, but we can handle this.");

      // Reinstalling the rules, setting the buffer in flow-mod message;
      InstallTeidRouting (rInfo, msg->buffer_id);
    }
  else
    {
      NS_LOG_WARN ("Ignoring TEID packet sent to controller.");
    }
  
  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*dp->exp*/);
  return 0;
}

ofl_err
RingController::HandleFlowRemoved (ofl_msg_flow_removed *msg, 
                                   SwitchInfo swtch, uint32_t xid)
{
  uint8_t table = msg->stats->table_id;
  uint32_t teid = msg->stats->cookie;
  
  NS_LOG_FUNCTION (swtch.ipv4 << teid);

  // Since handlers must free the message when everything is ok, let's remove
  // it now as we can handle it anyway.
  ofl_msg_free_flow_removed (msg, true, NULL);

  // Ignoring flows removed from tables other than teid table #1
  if (table != 1)
    {
      NS_LOG_WARN ("Ignoring flow removed from table " << table);
      return 0;
    }

  // Check for existing routing information
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // Check for active application
  if (rInfo->isActive == true)
    {
      NS_LOG_DEBUG ("Routing info for TEID " << teid << " is active.");
      // In this case, the switch removed the flow entry of an active route.
      // Let's reinstall the entry.
      InstallTeidRouting (rInfo);
      return 0;
    }

  // Set this rule as uninstalled
  rInfo->isInstalled = false;
  return 0;
}

ofl_err
RingController::HandleMultipartReply (ofl_msg_multipart_reply_header *msg, 
                                      SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (swtch.ipv4 << xid);

  char *msg_str = ofl_msg_to_string ((ofl_msg_header*)msg, NULL);
  NS_LOG_DEBUG ("Multipart reply: " << msg_str);
  free (msg_str);

  // Check for multipart reply type
  uint16_t switchIdx = GetSwitchIdxForDevice (swtch.netdev);
  switch (msg->type) 
    {
      case (OFPMP_FLOW): 
        {
          // Handle multipart reply flow messages, requested by this contrller
          // and used here to update average traffic usage for each GTP tunnel
          uint32_t teid;
          Ptr<RoutingInfo> rInfo;
          ofl_flow_stats *flowStats;
          ofl_msg_multipart_reply_flow *replyFlow;
          
          replyFlow = (ofl_msg_multipart_reply_flow*)msg;
          for (size_t f = 0; f < replyFlow->stats_num; f++)
            {
              flowStats = replyFlow->stats[f];
              teid = flowStats->cookie;
              if (teid == 0) continue; // Skipping table miss entry.
             
              rInfo = GetTeidRoutingInfo (teid);
              if (IsInputSwitch (rInfo, switchIdx))
                {
                  UpdateAverageTraffic (rInfo, switchIdx, flowStats);
                }
            }
          break;
        }
      default:
        {
          NS_LOG_WARN ("Unexpected multipart message.");
        }
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, NULL /*exp*/);
  return 0;
}


void
RingController::ProcessGbrRequest (Ptr<RoutingInfo> rInfo)
{
  m_gbrBearers++;

  EpsBearer bearer = rInfo->bearer.bearerLevelQos;
  DataRate downlinkGbr (bearer.gbrQosInfo.gbrDl);
  DataRate uplinkGbr (bearer.gbrQosInfo.gbrUl);
  //DataRate reserve = std::max (downlinkGbr, uplinkGbr);
  DataRate reserve = downlinkGbr + uplinkGbr;
  NS_LOG_DEBUG ("Bearer " << rInfo->teid << " requesting " << reserve);

  DataRate bandwidth = GetAvailableBandwidth (rInfo->sgwIdx, rInfo->enbIdx, 
                                              rInfo->downPath);
  NS_LOG_DEBUG ("Bandwidth from " << rInfo->sgwIdx <<  " to " << 
                rInfo->enbIdx << " in shortest path: " << bandwidth);

  if (m_strategy == RingController::HOPS)
    {
      if (bandwidth < reserve)
        {
          NS_LOG_WARN ("No resources for bearer " << rInfo->teid <<
                       " in shortest path. Proceed without reservation.");
          m_gbrBlocks++;
          return;
        }
    }
  else if (m_strategy == RingController::BAND)
    {
      if (bandwidth < reserve)
        {
          NS_LOG_DEBUG ("No resources for bearer " << rInfo->teid <<
                       " in shortest path. Checking the other path.");
          bandwidth = GetAvailableBandwidth (rInfo->sgwIdx, rInfo->enbIdx, 
                                             rInfo->upPath);
          if (bandwidth < reserve)
            {
              NS_LOG_WARN ("No resources for bearer " << rInfo->teid <<
                           " in both paths. Proceed without reservation.");
              m_gbrBlocks++;
              return;
            }
          else
            {
              NS_LOG_DEBUG ("Found resources in other path. Inverting paths.");
              rInfo->upPath = InvertRoutingPath (rInfo->upPath);
              rInfo->downPath = InvertRoutingPath (rInfo->downPath);
            }
        }
    }
  rInfo->reserved = reserve;
  ReserveBandwidth (rInfo);
}

RingController::RoutingPath
RingController::FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) < GetNSwitches ());
  
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstSwitchIdx - srcSwitchIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  
  return (clockwiseDistance <= maxHops) ? 
      RingController::CLOCK : 
      RingController::COUNTER;
}

RingController::RoutingPath 
RingController::InvertRoutingPath (RoutingPath original)
{
  return original == RingController::CLOCK ? 
         RingController::COUNTER : 
         RingController::CLOCK; 
}

DataRate 
RingController::GetAvailableBandwidth (uint16_t srcSwitchIdx, 
                                       uint16_t dstSwitchIdx,
                                       RoutingPath routingPath)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  
  // Get bandwitdh for first hop
  uint16_t current = srcSwitchIdx;
  uint16_t next = NextSwitchIndex (current, routingPath);
  Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
  DataRate bandwidth = conn->GetAvailableDataRate ();

  // Repeat the proccess for next hops
  while (next != dstSwitchIdx)
    {
      current = next;
      next = NextSwitchIndex (current, routingPath);
      conn = GetConnectionInfo (current, next);
      if (conn->GetAvailableDataRate () < bandwidth)
        {
          bandwidth = conn->GetAvailableDataRate ();
        }
    }
  return bandwidth;
}

bool 
RingController::ReserveBandwidth (const Ptr<RoutingInfo> rInfo)
{
  // Iterating over connections in downlink direction
  uint16_t current = rInfo->sgwIdx;
  while (current != rInfo->enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, rInfo->downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReserveDataRate (rInfo->reserved);
      NS_ABORT_IF (conn->GetAvailableDataRate () < 0);
      current = next;
    }
  return true;
}

bool
RingController::ReleaseBandwidth (const Ptr<RoutingInfo> rInfo)
{
  // Iterating over connections in downlink direction
  uint16_t current = rInfo->sgwIdx;
  while (current != rInfo->enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, rInfo->downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReleaseDataRate (rInfo->reserved);
      current = next;
    }
  return true;
}

uint16_t 
RingController::NextSwitchIndex (uint16_t current, RoutingPath path)
{
  return path == RingController::CLOCK ?
      (current + 1) % GetNSwitches () : 
      (current == 0 ? GetNSwitches () - 1 : (current - 1));
}

DataRate 
RingController::GetTunnelAverageTraffic (uint32_t teid)
{
//  std::ostringstream cmd;
//  cmd << "stats-flow table=1";
//
//  RoutingInfo rInfo = GetTeidRoutingInfo (teid);
//  char teidHexStr [9];
//  sprintf (teidHexStr, "0x%x", teid);
//
//  uint16_t current = rInfo.sgwIdx;
//  Ptr<OFSwitch13NetDevice> currentDevice = GetSwitchDevice (current);       
//  DpctlCommand (currentDevice, cmd.str ());
//
//  
  return DataRate ();
}

void 
RingController::SaveTeidRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  std::pair <uint32_t, Ptr<RoutingInfo> > entry (rInfo->teid, rInfo);
  std::pair <TeidRoutingMap_t::iterator, bool> ret;
  ret = m_routes.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing routing information for teid " << rInfo->teid);
    }
}

Ptr<RingController::RoutingInfo>
RingController::GetTeidRoutingInfo (uint32_t teid)
{
  Ptr<RoutingInfo> rInfo = 0;
  TeidRoutingMap_t::iterator ret;
  ret = m_routes.find (teid);
  if (ret != m_routes.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
}

bool 
RingController::InstallTeidRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo->teid << buffer);
  NS_ASSERT_MSG (rInfo->isActive, "Rule not active.");

  char teidHexStr [9];
  sprintf (teidHexStr, "0x%x", rInfo->teid);

  // flow-mod flags OFPFF_SEND_FLOW_REM
  char flagStr [7];
  sprintf (flagStr, "0x0001");

  char bufferStr [12];
  sprintf (bufferStr, "%u", buffer);
  
  // Configuring downlink routing
  if (!rInfo->app || rInfo->app->GetDirection () != Application::UPLINK)
    {
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,table=1" << 
             ",buffer=" << bufferStr <<
             ",flags=" << flagStr <<
             ",cookie=" << teidHexStr <<
             ",prio=" << rInfo->priority <<
             ",idle=" << rInfo->timeout <<
             " eth_type=0x800,ip_proto=17" << 
             ",ip_src=" << rInfo->sgwAddr <<
             ",ip_dst=" << rInfo->enbAddr <<
             ",gtp_teid=" << rInfo->teid <<
             " apply:group=" << rInfo->downPath;
      
      uint16_t current = rInfo->sgwIdx;
      while (current != rInfo->enbIdx)
        {
          DpctlCommand (GetSwitchDevice (current), cmd.str ());
          current = NextSwitchIndex (current, rInfo->downPath);
        }
    }
    
  // Configuring uplink routing
  if (!rInfo->app || rInfo->app->GetDirection () != Application::DOWNLINK)
    { 
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,table=1" << 
             ",buffer=" << bufferStr <<
             ",flags=" << flagStr <<
             ",cookie=" << teidHexStr <<
             ",prio=" << rInfo->priority <<
             ",idle=" << rInfo->timeout <<
             " eth_type=0x800,ip_proto=17" << 
             ",ip_src=" << rInfo->enbAddr <<
             ",ip_dst=" << rInfo->sgwAddr <<
             ",gtp_teid=" << rInfo->teid <<
             " apply:group=" << rInfo->upPath;

      uint16_t current = rInfo->enbIdx;
      while (current != rInfo->sgwIdx)
        {
          DpctlCommand (GetSwitchDevice (current), cmd.str ());
          current = NextSwitchIndex (current, rInfo->upPath);
        }
    }
  
  rInfo->isInstalled = true;
  return true;
}

void
RingController::QuerySwitchStats ()
{
  // Getting statistics from all switches
  for (int i = 0; i < GetNSwitches (); i++)
    {
      DpctlCommand (GetSwitchDevice (i), "stats-flow table=1");
    }
  Simulator::Schedule (m_timeout, &RingController::QuerySwitchStats, this);
}

bool
RingController::IsInputSwitch (const Ptr<RoutingInfo> rInfo, 
                               uint16_t switchIdx)
{
  // For default bearer (no app associated), consider a bidirectional traffic. 
  Application::Direction direction = Application::BIDIRECTIONAL;
  if (rInfo->app)
    {
      direction = rInfo->app->GetDirection ();
    }
 
  switch (direction)
    {
      case Application::BIDIRECTIONAL:
        return (switchIdx == rInfo->sgwIdx || switchIdx == rInfo->enbIdx);
      
      case Application::UPLINK:
        return (switchIdx == rInfo->enbIdx);

      case Application::DOWNLINK:
        return (switchIdx == rInfo->sgwIdx);

      default:
        return false;
    }
}

void
RingController::UpdateAverageTraffic (Ptr<RoutingInfo> rInfo, 
                                      uint16_t switchIdx,
                                      ofl_flow_stats* flowStats)
{
  uint64_t bytes = flowStats->byte_count;
  double secs = (flowStats->duration_sec + flowStats->duration_nsec / 1000000000);
  DataRate dr (bytes*8/secs);


  if (switchIdx == GetSwitchIdxForGateway ())
    {
      NS_LOG_DEBUG ("Average down traffic for tunnel " << rInfo->teid << ": " << dr);
    }
  else
    {
      NS_LOG_DEBUG ("Average up traffic for tunnel " << rInfo->teid << ": " << dr);
    }
}

};  // namespace ns3

