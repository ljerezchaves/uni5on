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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingController");
NS_OBJECT_ENSURE_REGISTERED (RingController);

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
RingController::NotifyNewAttachToSwitch (Ptr<NetDevice> nodeDev, 
    Ipv4Address nodeIp, Ptr<OFSwitch13NetDevice> swtchDev, uint16_t swtchIdx, 
    uint32_t swtchPort)
{
  NS_LOG_FUNCTION (this << nodeIp << swtchIdx << swtchPort);

  // Call base method which will save IP and configure local delivery
  OpenFlowEpcController::NotifyNewAttachToSwitch (nodeDev, nodeIp, swtchDev, 
          swtchIdx, swtchPort);
}

void
RingController::NotifyNewConnBtwnSwitches (const Ptr<ConnectionInfo> connInfo)
{
  NS_LOG_FUNCTION (this);
  
  // Call base method which will save connection information
  OpenFlowEpcController::NotifyNewConnBtwnSwitches (connInfo);
  
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

bool 
RingController::InstallTeidRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo->m_teid << rInfo->m_priority << buffer);
  NS_ASSERT_MSG (rInfo->m_isActive, "Rule not active.");

  // Getting rInfo associated metadata
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  bool meterInstalled = false;
  
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
      if (meterInfo && meterInfo->m_hasDown)
        {
          if (!meterInfo->m_isInstalled)
            {
              // Install the meter entry
              DpctlCommand (GetSwitchDevice (current), meterInfo->GetDownAddCmd ());
              meterInstalled = true;
            }

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
      if (meterInfo && meterInfo->m_hasUp)
        {
           if (!meterInfo->m_isInstalled)
            {
              // Install the meter entry
              DpctlCommand (GetSwitchDevice (current), meterInfo->GetUpAddCmd ());
              meterInstalled = true;
            }

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

  // Updating meter installation flag
  if (meterInstalled)
    {
      meterInfo->m_isInstalled = true;
    }

  rInfo->m_isInstalled = true;
  return true;
}

bool
RingController::GbrBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);
  
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  uint32_t teid = rInfo->m_teid;
  DataRate available, request;
  bool downOk = true;
  bool upOk = true;
  
  IncreaseGbrRequest ();
  ringInfo->ResetPaths ();
  
  // Reserving downlink resources
  if (gbrInfo->m_hasDown)
    {
      downOk = false;
      
      request = gbrInfo->m_downDataRate;
      NS_LOG_DEBUG (teid << ": downlink request: " << request);
      available = GetAvailableBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, 
          ringInfo->m_downPath);
      NS_LOG_DEBUG (teid << ": available in current path: " << available);

      if (available >= request)
        {
          // Let's reserve it
          downOk = ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, 
              ringInfo->m_downPath, request);
        }
      else
        {
          // We don't have the available bandwitdh for this bearer in current
          // path.  Let's check the routing strategy and see if we can change
          // the route.
          switch (m_strategy)
            {
              case RingController::HOPS:
                {
                  NS_LOG_WARN (teid << ": no resources. Block!");
                  IncreaseGbrBlocks ();
                  break;
                }
        
              case RingController::BAND:
                {
                  NS_LOG_DEBUG (teid << ": checking the other path.");
                  available = GetAvailableBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                      RingRoutingInfo::InvertPath (ringInfo->m_downPath));
                  NS_LOG_DEBUG (teid << ": available in other path: " << available);
        
                  if (available >= request)
                    {
                      // Let's invert the path and reserve it 
                      NS_LOG_DEBUG (teid << ": inverting down path.");
                      ringInfo->InvertDownPath ();
                      downOk = ReserveBandwidth (rInfo->m_sgwIdx, 
                          rInfo->m_enbIdx, ringInfo->m_downPath, request);
                    }
                  else
                    {
                      NS_LOG_WARN (teid << ": no resources. Block!");
                      IncreaseGbrBlocks ();
                    }
                  break;
                }

              default:
                {
                  NS_ABORT_MSG ("Invalid Routing strategy.");
                }
            }
        }
  
      // Check for success in downlink reserve
      if (!downOk)
        {
          return false;
        }
    }

  // Reserving uplink resources
  if (gbrInfo->m_hasUp)
    {
      upOk = false;
      
      request = gbrInfo->m_upDataRate;
      NS_LOG_DEBUG (teid << ": uplink request: " << request);
      available = GetAvailableBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx, 
          ringInfo->m_upPath);
      NS_LOG_DEBUG (teid << ": available in current path: " << available);

      if (available >= request)
        {
          // Let's reserve it
          upOk = ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx, 
              ringInfo->m_upPath, request);
        }
      else
        {
          // We don't have the available bandwitdh for this bearer in current
          // path.  Let's check the routing strategy and see if we can change
          // the route.
          switch (m_strategy)
            {
              case RingController::HOPS:
                {
                  NS_LOG_WARN (teid << ": no resources. Block!");
                  IncreaseGbrBlocks ();
                  break;
                }
        
              case RingController::BAND:
                {
                  NS_LOG_DEBUG (teid << ": checking the other path.");
                  available = GetAvailableBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                      RingRoutingInfo::InvertPath (ringInfo->m_upPath));
                  NS_LOG_DEBUG (teid << ": available in other path: " << available);
        
                  if (available >= request)
                    {
                      // Let's invert the path and reserve it 
                      NS_LOG_DEBUG (teid << ": inverting up path.");
                      ringInfo->InvertUpPath ();
                      upOk = ReserveBandwidth (rInfo->m_enbIdx, 
                          rInfo->m_sgwIdx, ringInfo->m_upPath, request);
                    }
                  else
                    {
                      NS_LOG_WARN (teid << ": no resources. Block!");
                      IncreaseGbrBlocks ();
                    }
                  break;
                }

              default:
                {
                  NS_ABORT_MSG ("Invalid Routing strategy.");
                }
            }
        }

      // Check for success in uplink reserve.
      if (!upOk)
        {
          if (gbrInfo->m_hasDown && downOk)
            {
              ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, 
                  ringInfo->m_downPath, request);
            }
          return false;
        }
    }
  NS_ASSERT_MSG (downOk && upOk, "Error during GBR reserve.");
  gbrInfo->m_isReserved = true;
  return true;
}

bool
RingController::GbrBearerRelease (Ptr<RoutingInfo> rInfo)
{
  Ptr<GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo && gbrInfo->m_isReserved)
    {
      Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");

      gbrInfo->m_isReserved = false;
      ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, ringInfo->m_downPath,
          gbrInfo->m_downDataRate);
      ReleaseBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx, ringInfo->m_upPath,
          gbrInfo->m_upDataRate);
    }
  return true;
}

Ptr<RingRoutingInfo> 
RingController::GetRingRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo == 0)
    {
      // This is the first time in simulation we are querying ring information
      // for this bearer. Let's create and aggregate it's ring routing
      // metadata.
      RingRoutingInfo::RoutingPath downPath = 
          FindShortestPath (rInfo->m_sgwIdx, rInfo->m_enbIdx);
      ringInfo = CreateObject<RingRoutingInfo> (rInfo, downPath);
      rInfo->AggregateObject (ringInfo);
    }
  return ringInfo;
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
RingController::ReserveBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
    RingRoutingInfo::RoutingPath routingPath, DataRate reserve)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx << routingPath << reserve);

  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReserveDataRate (reserve);
      NS_ABORT_IF (conn->GetAvailableDataRate () < 0);
      current = next;
    }
  return true;
}

bool
RingController::ReleaseBandwidth (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
    RingRoutingInfo::RoutingPath routingPath, DataRate release)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx << routingPath << release);

  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> conn = GetConnectionInfo (current, next);
      conn->ReleaseDataRate (release);
      current = next;
    }
  return true;
}

uint16_t 
RingController::NextSwitchIndex (uint16_t current, 
    RingRoutingInfo::RoutingPath routingPath)
{
  return routingPath == RingRoutingInfo::CLOCK ?
      (current + 1) % GetNSwitches () : 
      (current == 0 ? GetNSwitches () - 1 : (current - 1));
}

};  // namespace ns3

