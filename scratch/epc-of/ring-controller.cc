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
#include <string>

NS_LOG_COMPONENT_DEFINE ("RingController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);
NS_OBJECT_ENSURE_REGISTERED (RingController);

// ------------------------------------------------------------------------ //
RingRoutingInfo::RingRoutingInfo ()
{
  NS_LOG_FUNCTION (this);
}

RingRoutingInfo::~RingRoutingInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
RingRoutingInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingRoutingInfo")
    .SetParent (RoutingInfo::GetTypeId ())
    .AddConstructor<RingRoutingInfo> ()
  ;
  return tid;
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void 
RingRoutingInfo::InvertRoutingPath ()
{
  RoutingPath aux = downPath;
  downPath = upPath;
  upPath = aux;
}

void
RingRoutingInfo::SetDownAndUpPath (RoutingPath down)
{
  downPath  = down;
  upPath    = downPath == RingRoutingInfo::CLOCK ? 
                RingRoutingInfo::COUNTER :
                RingRoutingInfo::CLOCK;
}


// ------------------------------------------------------------------------ //
RingController::RingController ()
{
  NS_LOG_FUNCTION (this);
}

RingController::~RingController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent (OpenFlowEpcController::GetTypeId ())
    .AddAttribute ("Strategy", "The ring routing strategy.",
                   EnumValue (RingController::HOPS),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::HOPS, "Hops",
                                    RingController::BAND, "Bandwidth"))
    .AddAttribute ("BwReserve", "Bandwitdth saving factor.",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&RingController::m_bwFactor),
                   MakeDoubleChecker<double> (0.0, 1.0))
//    .AddAttribute ("StatsTimeout",
//                   "The interval between query stats from switches.",
//                   TimeValue (Seconds (5)),
//                   MakeTimeAccessor (&RingController::m_timeout),
//                   MakeTimeChecker ())
  ;
  return tid;
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  OpenFlowEpcController::DoDispose ();
}

void
RingController::NotifyNewSwitchConnection (const Ptr<ConnectionInfo> connInfo)
{
  NS_LOG_FUNCTION (this);
  
  // Call base method which will save connection information
  OpenFlowEpcController::NotifyNewSwitchConnection (connInfo);
  
  // Installing default groups for RingController ring routing. Group
  // RingRoutingInfo::CLOCK is used to send packets from current switch to the
  // next one in clockwise direction.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::CLOCK <<
          " weight=0,port=any,group=any output=" << connInfo->portNum1;
  DpctlCommand (connInfo->switchDev1, cmd1.str ());
                                   
  // Group RingRoutingInfo::COUNTER is used to send packets from the next
  // switch to the current one in counterclockwise direction. 
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::COUNTER <<
          " weight=0,port=any,group=any output=" << connInfo->portNum2;
  DpctlCommand (connInfo->switchDev2, cmd2.str ());
}

void 
RingController::NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
                                         Ipv4Address enbAddr, 
                                         Ipv4Address sgwAddr,
                                         BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr);
  
  // Call base method which will save context information
  OpenFlowEpcController::NotifyNewContextCreated (imsi, cellId, enbAddr, 
                                                  sgwAddr, bearerList);
  
  // Create and save routing information for default bearer
  ContextBearer_t defaultBearer = bearerList.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
  
  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RingRoutingInfo> rrInfo = GetTeidRingRoutingInfo (teid);
  NS_ASSERT_MSG (rrInfo == 0, "Existing routing for default bearer " << teid);

  rrInfo = CreateObject<RingRoutingInfo> ();
  rrInfo->teid = teid;
  rrInfo->sgwIdx = GetSwitchIdxFromIp (sgwAddr);
  rrInfo->enbIdx = GetSwitchIdxFromIp (enbAddr);
  rrInfo->sgwAddr = sgwAddr;
  rrInfo->enbAddr = enbAddr;
  rrInfo->app = 0;                       // No app for default bearer
  rrInfo->priority = m_defaultPriority;  // Priority for default bearer
  rrInfo->timeout = m_defaultTimeout;    // No timeout for default bearer
  rrInfo->isInstalled = false;           // Bearer rules not installed yet
  rrInfo->isActive = true;               // Default bearer is always active
  rrInfo->isDefault = true;              // This is a default bearer
  rrInfo->bearer = defaultBearer;
  rrInfo->SetDownAndUpPath (FindShortestPath (rrInfo->sgwIdx, rrInfo->enbIdx));

  SaveTeidRoutingInfo (rrInfo);
  InstallTeidRouting (rrInfo);
}

bool
RingController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  // Get TEID, bearer and tft from application, and reset statistics.
  Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
  uint32_t teid = GetTeidFromApplication (app);
  ContextBearer_t dedicatedBearer = GetBearerFromTft (tft);
  ResetAppStatistics (app);
  
  Ptr<RingRoutingInfo> rrInfo = GetTeidRingRoutingInfo (teid);
  if (rrInfo == 0)
    {
      // This is the first time in simulation we are using this dedicated
      // bearer. Let's create and save it's routing metadata.
      NS_LOG_DEBUG ("First use of bearer TEID " << teid);
      Ptr<ContextInfo> cInfo = GetContextFromTft (tft);
      rrInfo = CreateObject<RingRoutingInfo> ();
      rrInfo->teid = teid;
      rrInfo->sgwIdx = cInfo->sgwIdx;
      rrInfo->enbIdx = cInfo->enbIdx;
      rrInfo->sgwAddr = cInfo->sgwAddr;
      rrInfo->enbAddr = cInfo->enbAddr;
      rrInfo->app = app;                      // App for this dedicated bearer
      rrInfo->priority = m_dedicatedPriority; // Priority for dedicated bearer
      rrInfo->timeout = m_dedicatedTimeout;   // Timeout for dedicated bearer
      rrInfo->isInstalled = false;            // Switch rules not installed yet
      rrInfo->isActive = false;               // Dedicated bearer not active yet
      rrInfo->isDefault = false;              // This is a dedicated bearer
      rrInfo->bearer = dedicatedBearer;
      rrInfo->SetDownAndUpPath (FindShortestPath (rrInfo->sgwIdx, rrInfo->enbIdx));

      SaveTeidRoutingInfo (rrInfo);
    }

  // Is it a default bearer?
  if (rrInfo->isDefault)
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource reservation nor reinstall the switch rules, as
      // default rules were supposed to remain installed during entire
      // simulation.
      NS_ASSERT_MSG (rrInfo->isActive && rrInfo->isInstalled, 
                     "Default bearer should be intalled and activated.");
      return true;
    }

  // Is it an active bearer?
  if (rrInfo->isActive)
    {
      // This happens with VoIP application, which are installed in pairs and,
      // when the second application starts, the first one has already
      // configured the routing for this bearer and set the active flag.
      NS_ASSERT_MSG (rrInfo->isInstalled, "Bearer should be installed.");
      NS_LOG_DEBUG ("Routing path for " << teid << " is already installed.");
      return true;
    }

  NS_ASSERT_MSG (!rrInfo->isActive, "Bearer should be inactive.");
  // So, this bearer must be inactive and we are goind to reuse it's metadata.
  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will inscrease the bearer
  // priority.  Doing this, we avoid problems with old 'expiring' rules, and we
  // can even use new routing paths when necessary.

  // For dedicated GBR bearers, let's check for available resources. 
  if (rrInfo->IsGbr ())
    {
      if (!ProcessGbrRequest (rrInfo))
        {
          return false;
        }
    }

  // Everything is ok! Let's activate and install this bearer.
  rrInfo->isActive = true;
  InstallTeidRouting (rrInfo);  
  return true;
}

bool
RingController::NotifyAppStop (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  uint32_t teid = GetTeidFromApplication (app);
  Ptr<RingRoutingInfo> rrInfo = GetTeidRingRoutingInfo (teid);
  if (rrInfo == 0)
    {
      NS_FATAL_ERROR ("No routing information for teid " << teid);
    }
  
  // Check for active application
  if (rrInfo->isActive == true)
    {
      rrInfo->isActive = false;
      rrInfo->isInstalled = false;
      if (rrInfo->IsGbr ())
        {
          ReleaseBandwidth (rrInfo); 
        }
      // There is no need to remove mannyaly remove the 
      // rules from switch. Just wait for idle timeout.
    }

  PrintAppStatistics (app);
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
  Ptr<RingRoutingInfo> rrInfo = GetTeidRingRoutingInfo (teid);
  if (rrInfo && rrInfo->isActive)
    {
      NS_LOG_WARN ("Not supposed to happen, but we can handle this.");

      // Reinstalling the rules, setting the buffer in flow-mod message;
      InstallTeidRouting (rrInfo, msg->buffer_id);
    }
  else
    {
      NS_LOG_WARN ("Ignoring TEID packet sent to controller.");
    }
  
  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

ofl_err
RingController::HandleFlowRemoved (ofl_msg_flow_removed *msg, 
                                   SwitchInfo swtch, uint32_t xid)
{
  uint8_t table = msg->stats->table_id;
  uint32_t teid = msg->stats->cookie;
  uint16_t prio = msg->stats->priority;
  
  NS_LOG_FUNCTION (swtch.ipv4 << teid);
      
  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  // NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // Since handlers must free the message when everything is ok, 
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // Ignoring flows removed from tables other than TEID table #1
  if (table != 1)
    {
      NS_LOG_WARN ("Ignoring flow removed from table " << table);
      return 0;
    }

  // Check for existing routing information for this bearer
  Ptr<RingRoutingInfo> rrInfo = GetTeidRingRoutingInfo (teid);
  if (rrInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // When a rule expires due to idle timeout, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rrInfo->isActive)
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for stopped application.");
      return 0;
    }
  
  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rrInfo->priority > prio)
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for old rule.");
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we need to reinstall the rules to avoid problems. Let's
  // increase the priority to avoid conflicts (so, any other flow expired
  // message from other switches for this same path will be handle by case #2
  // (lower priority).
  NS_ASSERT_MSG (rrInfo->priority == prio, "Invalid flow priority.");
  if (rrInfo->isActive)
    {
      NS_LOG_DEBUG ("Flow " << teid << " is still active. Reinstall rules...");
      InstallTeidRouting (rrInfo);
      return 0;
    }

  NS_ABORT_MSG ("Should not get here :/");
}

// ofl_err
// RingController::HandleMultipartReply (ofl_msg_multipart_reply_header *msg, 
//                                       SwitchInfo swtch, uint32_t xid)
// {
//   NS_LOG_FUNCTION (swtch.ipv4 << xid);
// 
//   char *msg_str = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
//   NS_LOG_DEBUG ("Multipart reply: " << msg_str);
//   free (msg_str);
// 
//   // Check for multipart reply type
//   switch (msg->type) 
//     {
//       case (OFPMP_FLOW): 
//         {
//           // Handle multipart reply flow messages, requested by this contrller
//           // and used here to update average traffic usage for each GTP tunnel
//           uint32_t teid;
//           Ptr<RoutingInfo> rInfo;
//           ofl_flow_stats *flowStats;
//           ofl_msg_multipart_reply_flow *replyFlow;
//           
//           replyFlow = (ofl_msg_multipart_reply_flow*)msg;
//           for (size_t f = 0; f < replyFlow->stats_num; f++)
//             {
//               flowStats = replyFlow->stats[f];
//               teid = flowStats->cookie;
//               if (teid == 0) continue; // Skipping table miss entry.
//              
//               rInfo = GetTeidRoutingInfo (teid);
// //              uint16_t switchIdx = GetSwitchIdxFromDevice (swtch.netdev);
// //              if (IsInputSwitch (rInfo, switchIdx))
// //                {
// //                  UpdateAverageTraffic (rInfo, switchIdx, flowStats);
// //                }
//             }
//           break;
//         }
//       default:
//         {
//           NS_LOG_WARN ("Unexpected multipart message.");
//         }
//     }
// 
//   // All handlers must free the message when everything is ok
//   ofl_msg_free ((ofl_msg_header*)msg, 0 /*exp*/);
//   return 0;
// }

Ptr<RingRoutingInfo> 
RingController::GetTeidRingRoutingInfo (uint32_t teid)
{
  Ptr<RingRoutingInfo> ptr;
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo)
    {
      ptr = rInfo->GetObject<RingRoutingInfo> ();
      NS_ASSERT_MSG (ptr, "Invalid pointer type.");
    }
  return ptr;
}

bool
RingController::ProcessGbrRequest (Ptr<RingRoutingInfo> rrInfo)
{
  IncreaseGbrRequest ();

  uint32_t teid = rrInfo->teid;
  GbrQosInformation gbrQoS = rrInfo->GetQosInfo ();
  DataRate request, available;

  request = DataRate (gbrQoS.gbrDl + gbrQoS.gbrUl);
  NS_LOG_DEBUG ("Bearer " << teid << " requesting " << request);

  available = GetAvailableBandwidth (rrInfo->sgwIdx, rrInfo->enbIdx, 
                                     rrInfo->downPath);
  NS_LOG_DEBUG ("Available bandwidth in current path: " << available);

  if (available >= request)
    {
      // Let's reserve it and return true to the application.
      rrInfo->reserved = request;
      return (ReserveBandwidth (rrInfo));
    }

  // We don't have the available bandwitdh for this bearer in current path. 
  // Let's check the routing strategy and see if we can change the route.
  switch (m_strategy)
    {
      case RingController::HOPS:
        {
          NS_LOG_WARN ("No resources for bearer " << teid << ". Block!");
          IncreaseGbrBlocks ();
          return false;
        }

      case RingController::BAND:
        {
          NS_LOG_DEBUG ("No resources for bearer " << teid << "." 
                        " Checking the other path.");
          
          available = GetAvailableBandwidth (rrInfo->sgwIdx, rrInfo->enbIdx, 
                                             rrInfo->upPath);
          NS_LOG_DEBUG ("Available bandwidth in other path: " << available);

          if (available < request)
            {
              NS_LOG_WARN ("No resources for bearer " << teid << ". Block!");
              IncreaseGbrBlocks ();
              return false;
            }
          
          // Let's invert the path, reserve the bandwidth and return true to
          // the application.
          NS_LOG_DEBUG ("Inverting paths.");
          rrInfo->InvertRoutingPath ();
          rrInfo->reserved = request;
          return (ReserveBandwidth (rrInfo));
        }
        
      default:
        {
          NS_ABORT_MSG ("Invalid Routing strategy.");
        }
    }
}

RingRoutingInfo::RoutingPath
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
      RingRoutingInfo::CLOCK : 
      RingRoutingInfo::COUNTER;
}

DataRate 
RingController::GetAvailableBandwidth (uint16_t srcSwitchIdx, 
                                       uint16_t dstSwitchIdx,
                                       RingRoutingInfo::RoutingPath routingPath)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  
  // Get bandwitdh for first hop
  uint16_t current = srcSwitchIdx;
  uint16_t next = NextSwitchIndex (current, routingPath);
  Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
  DataRate bandwidth = conn->GetAvailableDataRate (m_bwFactor);

  // Repeat the proccess for next hops
  while (next != dstSwitchIdx)
    {
      current = next;
      next = NextSwitchIndex (current, routingPath);
      conn = GetConnectionInfo (current, next);
      if (conn->GetAvailableDataRate (m_bwFactor) < bandwidth)
        {
          bandwidth = conn->GetAvailableDataRate (m_bwFactor);
        }
    }
  return bandwidth;
}

bool 
RingController::ReserveBandwidth (const Ptr<RingRoutingInfo> rrInfo)
{
  // Iterating over connections in downlink direction
  uint16_t current = rrInfo->sgwIdx;
  while (current != rrInfo->enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, rrInfo->downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReserveDataRate (rrInfo->reserved);
      NS_ABORT_IF (conn->GetAvailableDataRate () < 0);
      current = next;
    }
  return true;
}

bool
RingController::ReleaseBandwidth (const Ptr<RingRoutingInfo> rrInfo)
{
  // Iterating over connections in downlink direction
  uint16_t current = rrInfo->sgwIdx;
  while (current != rrInfo->enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, rrInfo->downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReleaseDataRate (rrInfo->reserved);
      current = next;
    }
  return true;
}

uint16_t 
RingController::NextSwitchIndex (uint16_t current, 
                                 RingRoutingInfo::RoutingPath path)
{
  return path == RingRoutingInfo::CLOCK ?
      (current + 1) % GetNSwitches () : 
      (current == 0 ? GetNSwitches () - 1 : (current - 1));
}

bool 
RingController::InstallTeidRouting (Ptr<RingRoutingInfo> rrInfo, 
                                    uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rrInfo->teid << rrInfo->priority << buffer);
  NS_ASSERT_MSG (rrInfo->isActive, "Rule not active.");
  
  // Increasing the priority every time we (re)install TEID rules.
  rrInfo->priority++;    

  // flow-mod flags OFPFF_SEND_FLOW_REM and OFPFF_CHECK_OVERLAP, used to notify
  // the controller when a flow entry expires and to avoid overlaping rules.
  std::string flagsStr ("0x0003");

  // Printing the cookie and buffer values in dpctl string format
  char cookieStr [9], bufferStr [12];
  sprintf (cookieStr, "0x%x", rrInfo->teid);
  sprintf (bufferStr, "%u",   buffer);

  // Building the dpctl command + arguments string
  std::ostringstream args;
  args << "flow-mod cmd=add,table=1"
          ",buffer=" << bufferStr <<
          ",flags=" << flagsStr <<
          ",cookie=" << cookieStr <<
          ",prio=" << rrInfo->priority <<
          ",idle=" << rrInfo->timeout;
          

  // Configuring downlink routing
  if (!rrInfo->app || rrInfo->app->GetDirection () != Application::UPLINK)
    {
      // Building the match string
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17" << 
               ",ip_src=" << rrInfo->sgwAddr <<
               ",ip_dst=" << rrInfo->enbAddr <<
               ",gtp_teid=" << rrInfo->teid;

      // Building the output instruction string
      std::ostringstream inst;
      inst << " apply:group=" << rrInfo->downPath;

      // In downlink we start at gateway switch
      uint16_t current = rrInfo->sgwIdx;
  
      // When necessary, install the meter rule just in gateway switch
      GbrQosInformation gbrQoS = rrInfo->GetQosInfo ();
      if (gbrQoS.mbrDl)
        {
          // Install the meter entry
          std::ostringstream meter;
          meter << "meter-mod cmd=add,flags=1" << 
                   ",meter=" << rrInfo->teid <<
                   " drop:rate=" << gbrQoS.mbrDl / 1024;
          DpctlCommand (GetSwitchDevice (current), meter.str ());
            
          // Building the meter apply instruction string
          std::ostringstream meterInst;
          meterInst << " meter:" << rrInfo->teid;

          std::string commandStr = args.str () + match.str () + 
                                   meterInst.str () + inst.str ();

          // Installing the rules for gateway
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, rrInfo->downPath);
        }

      // Keep installing the rule at every switch in path
      std::string commandStr = args.str () + match.str () + inst.str ();
      while (current != rrInfo->enbIdx)
        {
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, rrInfo->downPath);
        }
    }
    
  // Configuring uplink routing
  if (!rrInfo->app || rrInfo->app->GetDirection () != Application::DOWNLINK)
    {
      // Building the match string
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17" << 
               ",ip_src=" << rrInfo->enbAddr <<
               ",ip_dst=" << rrInfo->sgwAddr <<
               ",gtp_teid=" << rrInfo->teid;

      // Building the output instruction string
      std::ostringstream inst;
      inst << " apply:group=" << rrInfo->upPath;

      // In uplink we start at eNB switch
      uint16_t current = rrInfo->enbIdx;

      // When necessary, install the meter rule just in eNB switch
      GbrQosInformation gbrQoS = rrInfo->GetQosInfo ();
      if (gbrQoS.mbrUl)
        {
          // Install the meter entry
          std::ostringstream meter;
          meter << "meter-mod cmd=add,flags=1" << 
                   ",meter=" << rrInfo->teid <<
                   " drop:rate=" << gbrQoS.mbrDl / 1024;
          DpctlCommand (GetSwitchDevice (current), meter.str ());
            
          // Building the meter apply instruction string
          std::ostringstream meterInst;
          meterInst << " meter:" << rrInfo->teid;

          std::string commandStr = args.str () + match.str () + 
                                   meterInst.str () + inst.str ();

          // Installing the rules for gateway
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, rrInfo->upPath);
        }

      // Keep installing the rule at every switch in path
      std::string commandStr = args.str () + match.str () + inst.str ();
      while (current != rrInfo->sgwIdx)
        {
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, rrInfo->upPath);
        }
    }
  
  rrInfo->isInstalled = true;
  return true;
}

// DataRate 
// RingController::GetTunnelAverageTraffic (uint32_t teid)
// {
// //  std::ostringstream cmd;
// //  cmd << "stats-flow table=1";
// //
// //  RoutingInfo rInfo = GetTeidRoutingInfo (teid);
// //  char cookieStr [9];
// //  sprintf (cookieStr, "0x%x", teid);
// //
// //  uint16_t current = rInfo.sgwIdx;
// //  Ptr<OFSwitch13NetDevice> currentDevice = GetSwitchDevice (current);       
// //  DpctlCommand (currentDevice, cmd.str ());
// //
// //  
//   return DataRate ();
// }
//
// void
// RingController::QuerySwitchStats ()
// {
//   // Getting statistics from all switches
//   for (int i = 0; i < GetNSwitches (); i++)
//     {
//       DpctlCommand (GetSwitchDevice (i), "stats-flow table=1");
//     }
//   Simulator::Schedule (m_timeout, &RingController::QuerySwitchStats, this);
// }
//
// bool
// RingController::IsInputSwitch (const Ptr<RingRoutingInfo> rrInfo, 
//                                uint16_t switchIdx)
// {
//   // For default bearer (no app associated), consider a bidirectional traffic. 
//   Application::Direction direction = Application::BIDIRECTIONAL;
//   if (rrInfo->app)
//     {
//       direction = rrInfo->app->GetDirection ();
//     }
//  
//   switch (direction)
//     {
//       case Application::BIDIRECTIONAL:
//         return (switchIdx == rrInfo->sgwIdx || switchIdx == rrInfo->enbIdx);
//       
//       case Application::UPLINK:
//         return (switchIdx == rrInfo->enbIdx);
// 
//       case Application::DOWNLINK:
//         return (switchIdx == rrInfo->sgwIdx);
// 
//       default:
//         return false;
//     }
// }
//
// void
// RingController::UpdateAverageTraffic (Ptr<RoutingInfo> rInfo, 
//                                       uint16_t switchIdx,
//                                       ofl_flow_stats* flowStats)
// {
//   uint64_t bytes = flowStats->byte_count;
//   double secs = (flowStats->duration_sec + flowStats->duration_nsec / 1000000000);
//   DataRate dr (bytes*8/secs);
// 
// 
//   if (switchIdx == GetSwitchIdxForGateway ())
//     {
//       NS_LOG_DEBUG ("Average down traffic for tunnel " << rInfo->teid << ": " << dr);
//     }
//   else
//     {
//       NS_LOG_DEBUG ("Average up traffic for tunnel " << rInfo->teid << ": " << dr);
//     }
// }

};  // namespace ns3

