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

uint16_t RingController::m_flowPrio = 2048;

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
}

TypeId 
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent (EpcSdnController::GetTypeId ())
    .AddAttribute ("LinkDataRate", 
                   "The data rate to be used for the CSMA OpenFlow links to be created",
                   DataRateValue (DataRate ("10Mb/s")),
                   MakeDataRateAccessor (&RingController::m_LinkDataRate),
                   MakeDataRateChecker ())
  ;
  return tid;
}

uint8_t
RingController::NotifyNewBearer (uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this);
  return 0;
}

void
RingController::NotifyNewSwitchConnection (ConnectionInfo connInfo)
{
  NS_LOG_FUNCTION (this);
  
  // Call base method which will save this information
  EpcSdnController::NotifyNewSwitchConnection (connInfo);
  
  // Installing default groups for RingController ring routing.  Group #1 is
  // used to send packets from current switch to the next one in clockwise
  // direction.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=1 weight=0,port=any,group=any output=" << connInfo.portNum1;
  ScheduleCommand (connInfo.switchDev1, cmd1.str ());
                                   
  // Group #2 is used to send packets from the next switch to the current one
  // in counterclockwise direction. 
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=2 weight=0,port=any,group=any output=" << connInfo.portNum2;
  ScheduleCommand (connInfo.switchDev2, cmd2.str ());
}

void
RingController::CreateSpanningTree ()
{
  uint16_t half = (GetNSwitches () / 2);
  ConnectionInfo* info = GetConnectionInfo (half, half+1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " << half+1 << " for broadcast messages.");
  
  std::ostringstream cmd1;
  Mac48Address macAddr1 = Mac48Address::ConvertFrom (info->portDev1->GetAddress ());
  cmd1 << "port-mod port=" << info->portNum1 << ",addr=" << macAddr1 << ",conf=0x00000020,mask=0x00000020";
  ScheduleCommand (info->switchDev1, cmd1.str ());

  std::ostringstream cmd2;
  Mac48Address macAddr2 = Mac48Address::ConvertFrom (info->portDev2->GetAddress ());
  cmd2 << "port-mod port=" << info->portNum2 << ",addr=" << macAddr2 << ",conf=0x00000020,mask=0x00000020";
  ScheduleCommand (info->switchDev2, cmd2.str ());
}

bool
RingController::FindShortestPath (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx)
{
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) <= GetNSwitches ());
  
  uint16_t maxHops = GetNSwitches () / 2;
  int clockwiseDistance = dstSwitchIdx - srcSwitchIdx;
  if (clockwiseDistance < 0)
    {
      clockwiseDistance += GetNSwitches ();
    }
  
  return (clockwiseDistance <= maxHops) ? true : false;
}

};  // namespace ns3

