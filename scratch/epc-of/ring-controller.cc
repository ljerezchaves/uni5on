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
  : m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo)
  : m_rInfo (rInfo)
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
    .SetParent<Object> ()
    .AddConstructor<RingRoutingInfo> ()
  ;
  return tid;
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
RingRoutingInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

void 
RingRoutingInfo::InvertRoutingPath ()
{
  RoutingPath aux = m_downPath;
  m_downPath = m_upPath;
  m_upPath = aux;
}

void
RingRoutingInfo::SetDownAndUpPath (RoutingPath down)
{
  m_downPath  = down;
  m_upPath    = m_downPath == RingRoutingInfo::CLOCK ? 
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
    Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr);
  
  // Call base method which will save context information and create routing
  // info for default bearer.
  OpenFlowEpcController::NotifyNewContextCreated (imsi, cellId, enbAddr, 
      sgwAddr, bearerList);
  
  // Create ringInfo for default bearer and aggregate it to rInfo.
  uint32_t teid = bearerList.front ().sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  Ptr<RingRoutingInfo> ringInfo = CreateObject<RingRoutingInfo> (rInfo);
  ringInfo->SetDownAndUpPath (FindShortestPath (rInfo->m_sgwIdx, rInfo->m_enbIdx));
  rInfo->AggregateObject (ringInfo);
  
  // Install rules for default bearer
  InstallTeidRouting (ringInfo);
}

bool
RingController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  // Call base method which will create routing info for the bearer associated
  // with this app, if necessary.
  OpenFlowEpcController::NotifyAppStart (app);

  // At first usage, create ringInfo for dedicated bearer and aggregate it to rInfo.
  uint32_t teid = GetTeidFromApplication (app);
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo == 0)
    {
      // This is the first time in simulation we are using this dedicated
      // bearer in the ring. Let's create and aggregate it's ring routing metadata.
      ringInfo = CreateObject<RingRoutingInfo> (rInfo);
      ringInfo->SetDownAndUpPath (FindShortestPath (rInfo->m_sgwIdx, rInfo->m_enbIdx));
      rInfo->AggregateObject (ringInfo);
    }

  // Is it a default bearer?
  if (rInfo->m_isDefault)
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource reservation nor reinstall the switch rules, as
      // default rules were supposed to remain installed during entire
      // simulation.
      NS_ASSERT_MSG (rInfo->m_isActive && rInfo->m_isInstalled, 
                     "Default bearer should be intalled and activated.");
      return true;
    }

  // Is it an active bearer?
  if (rInfo->m_isActive)
    {
      // This happens with VoIP application, which are installed in pairs and,
      // when the second application starts, the first one has already
      // configured the routing for this bearer and set the active flag.
      NS_ASSERT_MSG (rInfo->m_isInstalled, "Bearer should be installed.");
      NS_LOG_DEBUG ("Routing path for " << teid << " is already installed.");
      return true;
    }

  NS_ASSERT_MSG (!rInfo->m_isActive, "Bearer should be inactive.");
  // So, this bearer must be inactive and we are goind to reuse it's metadata.
  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will inscrease the bearer
  // priority.  Doing this, we avoid problems with old 'expiring' rules, and we
  // can even use new routing paths when necessary.

  // For dedicated GBR bearers, let's check for available resources. 
  if (rInfo->IsGbr ())
    {
      if (!ProcessGbrRequest (ringInfo))
        {
          return false;
        }
    }

  // Everything is ok! Let's activate and install this bearer.
  rInfo->m_isActive = true;
  InstallTeidRouting (ringInfo);  
  return true;
}

bool
RingController::NotifyAppStop (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (GetTeidFromApplication (app));
  NS_ASSERT_MSG (rInfo, "No routing information for AppStop.");
  
  // Release resources for active application
  if (rInfo->m_isActive && rInfo->IsGbr ())
    {
      ReleaseBandwidth (rInfo->GetObject<RingRoutingInfo> ());   
    }

  // Call base method to print app stats and update routing info.
  OpenFlowEpcController::NotifyAppStop (app);
  return true;
}

void
RingController::CreateSpanningTree ()
{
  NS_LOG_FUNCTION (this);

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

// ofl_err
// RingController::HandleMultipartReply (ofl_msg_multipart_reply_header *msg, 
//     SwitchInfo swtch, uint32_t xid)
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

bool 
RingController::InstallTeidRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo << buffer);

  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  return InstallTeidRouting (ringInfo, buffer);
}

bool
RingController::ProcessGbrRequest (Ptr<RingRoutingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);
  
  IncreaseGbrRequest ();
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
  DataRate request, available;
  uint32_t teid = rInfo->m_teid;

  request = DataRate (gbrQoS.gbrDl + gbrQoS.gbrUl);
  NS_LOG_DEBUG ("Bearer " << teid << " requesting " << request);

  available = GetAvailableBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
      ringInfo->m_downPath);
  NS_LOG_DEBUG ("Available bandwidth in current path: " << available);

  if (available >= request)
    {
      // Let's reserve it and return true to the application.
      rInfo->m_reserved = request;
      return (ReserveBandwidth (ringInfo));
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
          
          available = GetAvailableBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
              ringInfo->m_upPath);
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
          ringInfo->InvertRoutingPath ();
          rInfo->m_reserved = request;
          return (ReserveBandwidth (ringInfo));
        }
        
      default:
        {
          NS_ABORT_MSG ("Invalid Routing strategy.");
        }
    }
}

bool 
RingController::InstallTeidRouting (Ptr<RingRoutingInfo> ringInfo,
    uint32_t buffer)
{
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  NS_LOG_FUNCTION (this << rInfo->m_teid << rInfo->m_priority << buffer);
  NS_ASSERT_MSG (rInfo->m_isActive, "Rule not active.");

  // Increasing the priority every time we (re)install TEID rules.
  rInfo->m_priority++;

  // flow-mod flags OFPFF_SEND_FLOW_REM and OFPFF_CHECK_OVERLAP, used to notify
  // the controller when a flow entry expires and to avoid overlaping rules.
  std::string flagsStr ("0x0003");

  // Printing the cookie and buffer values in dpctl string format
  char cookieStr [9], bufferStr [12];
  sprintf (cookieStr, "0x%x", rInfo->m_teid);
  sprintf (bufferStr, "%u",   buffer);

  // Building the dpctl command + arguments string
  std::ostringstream args;
  args << "flow-mod cmd=add,table=1"
          ",buffer=" << bufferStr <<
          ",flags=" << flagsStr <<
          ",cookie=" << cookieStr <<
          ",prio=" << rInfo->m_priority <<
          ",idle=" << rInfo->m_timeout;
          
  // Configuring downlink routing
  if (!rInfo->m_app || rInfo->m_app->GetDirection () != Application::UPLINK)
    {
      // Building the match string
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17" << 
               ",ip_src=" << rInfo->m_sgwAddr <<
               ",ip_dst=" << rInfo->m_enbAddr <<
               ",gtp_teid=" << rInfo->m_teid;

      // Building the output instruction string
      std::ostringstream inst;
      inst << " apply:group=" << ringInfo->m_downPath;

      // In downlink we start at gateway switch
      uint16_t current = rInfo->m_sgwIdx;
  
      // When necessary, install the meter rule just in gateway switch
      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
      if (gbrQoS.mbrDl)
        {
          // Install the meter entry
          std::ostringstream meter;
          meter << "meter-mod cmd=add,flags=1" << 
                   ",meter=" << rInfo->m_teid <<
                   " drop:rate=" << gbrQoS.mbrDl / 1024;
          DpctlCommand (GetSwitchDevice (current), meter.str ());
            
          // Building the meter apply instruction string
          std::ostringstream meterInst;
          meterInst << " meter:" << rInfo->m_teid;

          std::string commandStr = args.str () + match.str () + 
                                   meterInst.str () + inst.str ();

          // Installing the rules for gateway
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, ringInfo->m_downPath);
        }

      // Keep installing the rule at every switch in path
      std::string commandStr = args.str () + match.str () + inst.str ();
      while (current != rInfo->m_enbIdx)
        {
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, ringInfo->m_downPath);
        }
    }
    
  // Configuring uplink routing
  if (!rInfo->m_app || rInfo->m_app->GetDirection () != Application::DOWNLINK)
    {
      // Building the match string
      std::ostringstream match;
      match << " eth_type=0x800,ip_proto=17" << 
               ",ip_src=" << rInfo->m_enbAddr <<
               ",ip_dst=" << rInfo->m_sgwAddr <<
               ",gtp_teid=" << rInfo->m_teid;

      // Building the output instruction string
      std::ostringstream inst;
      inst << " apply:group=" << ringInfo->m_upPath;

      // In uplink we start at eNB switch
      uint16_t current = rInfo->m_enbIdx;

      // When necessary, install the meter rule just in eNB switch
      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
      if (gbrQoS.mbrUl)
        {
          // Install the meter entry
          std::ostringstream meter;
          meter << "meter-mod cmd=add,flags=1" << 
                   ",meter=" << rInfo->m_teid <<
                   " drop:rate=" << gbrQoS.mbrDl / 1024;
          DpctlCommand (GetSwitchDevice (current), meter.str ());
            
          // Building the meter apply instruction string
          std::ostringstream meterInst;
          meterInst << " meter:" << rInfo->m_teid;

          std::string commandStr = args.str () + match.str () + 
                                   meterInst.str () + inst.str ();

          // Installing the rules for gateway
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, ringInfo->m_upPath);
        }

      // Keep installing the rule at every switch in path
      std::string commandStr = args.str () + match.str () + inst.str ();
      while (current != rInfo->m_sgwIdx)
        {
          DpctlCommand (GetSwitchDevice (current), commandStr);
          current = NextSwitchIndex (current, ringInfo->m_upPath);
        }
    }
  
  rInfo->m_isInstalled = true;
  return true;
}

RingRoutingInfo::RoutingPath
RingController::FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx);
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
    uint16_t dstSwitchIdx, RingRoutingInfo::RoutingPath routingPath)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx << routingPath);
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
RingController::ReserveBandwidth (const Ptr<RingRoutingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  // Iterating over connections in downlink direction
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  uint16_t current = rInfo->m_sgwIdx;
  while (current != rInfo->m_enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, ringInfo->m_downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReserveDataRate (rInfo->m_reserved);
      NS_ABORT_IF (conn->GetAvailableDataRate () < 0);
      current = next;
    }
  return true;
}

bool
RingController::ReleaseBandwidth (const Ptr<RingRoutingInfo> ringInfo)
{
  NS_LOG_FUNCTION (this << ringInfo);

  // Iterating over connections in downlink direction
  Ptr<RoutingInfo> rInfo = ringInfo->GetRoutingInfo ();
  uint16_t current = rInfo->m_sgwIdx;
  while (current != rInfo->m_enbIdx)
    {
      uint16_t next = NextSwitchIndex (current, ringInfo->m_downPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReleaseDataRate (rInfo->m_reserved);
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
// RingController::IsInputSwitch (const Ptr<RoutingInfo> rInfo, 
//                                uint16_t switchIdx)
// {
//   // For default bearer (no app associated), consider a bidirectional traffic. 
//   Application::Direction direction = Application::BIDIRECTIONAL;
//   if (rInfo->m_app)
//     {
//       direction = rInfo->m_app->GetDirection ();
//     }
//  
//   switch (direction)
//     {
//       case Application::BIDIRECTIONAL:
//         return (switchIdx == rInfo->m_sgwIdx || switchIdx == rInfo->m_enbIdx);
//       
//       case Application::UPLINK:
//         return (switchIdx == rInfo->m_enbIdx);
// 
//       case Application::DOWNLINK:
//         return (switchIdx == rInfo->m_sgwIdx);
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
//       NS_LOG_DEBUG ("Average down traffic for tunnel " << rInfo->m_teid << ": " << dr);
//     }
//   else
//     {
//       NS_LOG_DEBUG ("Average up traffic for tunnel " << rInfo->m_teid << ": " << dr);
//     }
// }

};  // namespace ns3

