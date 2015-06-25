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
#include "connection-info.h"
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
    .AddAttribute ("Strategy",
                   "The ring path routing strategy.",
                   EnumValue (RingController::HOPS),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::HOPS, "hops",
                                    RingController::BAND, "bandwidth",
                                    RingController::SMART, "smart"))
    .AddAttribute ("BwReserve",
                   "Bandwitdth saving factor to reserve.",
                   DoubleValue (0.2),
                   MakeDoubleAccessor (&RingController::m_bwFactor),
                   MakeDoubleChecker<double> (0.0, 1.0))
  ;
  return tid;
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_connections.clear ();
  OpenFlowEpcController::DoDispose ();
}

void
RingController::NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this);

  // Save this connection info for further usage
  SaveConnectionInfo (cInfo);

  // Installing default groups for RingController ring routing. Group
  // RingRoutingInfo::CLOCK is used to send packets from current switch to the
  // next one in clockwise direction. Note that this method works as
  // connections are created in clockwise direction, and switchs inside cInfo
  // are saved in the same clockwise direction.
  std::ostringstream cmd1;
  cmd1 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::CLOCK 
       << " weight=0,port=any,group=any output=" << cInfo->GetPortNoFirst ();
  DpctlCommand (cInfo->GetSwDevFirst (), cmd1.str ());

  // Group RingRoutingInfo::COUNTER is used to send packets from the next
  // switch to the current one in counterclockwise direction.
  std::ostringstream cmd2;
  cmd2 << "group-mod cmd=add,type=ind,group=" << RingRoutingInfo::COUNTER 
       << " weight=0,port=any,group=any output=" << cInfo->GetPortNoSecond ();
  DpctlCommand (cInfo->GetSwDevSecond (), cmd2.str ());
}

void
RingController::NotifyTopologyBuilt (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  // Save the number of switches in network topology.
  m_noSwitches = devices.GetN ();

  // Call base method which will save devices and create the spanning tree
  OpenFlowEpcController::NotifyTopologyBuilt (devices);

  // Configure routes to keep forwarding packets already in the ring until they
  // reach the destination switch.
  for (uint16_t sw1 = 0; sw1 < GetNSwitches (); sw1++)
    {
      uint16_t sw2 = (sw1 + 1) % GetNSwitches ();  // Next clockwise node
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (sw1, sw2);

      std::ostringstream cmd1;
      cmd1 << "flow-mod cmd=add,table=1,flags=0x0002,prio=" 
           << m_t1RingPrio << " in_port=" << cInfo->GetPortNoFirst () 
           << " write:group=" << RingRoutingInfo::COUNTER;
      DpctlCommand (cInfo->GetSwDevFirst (), cmd1.str ());

      std::ostringstream cmd2;
      cmd2 << "flow-mod cmd=add,table=1,flags=0x0002,prio=" 
           << m_t1RingPrio << " in_port=" << cInfo->GetPortNoSecond () 
           << " write:group=" << RingRoutingInfo::CLOCK;
      DpctlCommand (cInfo->GetSwDevSecond (), cmd2.str ());
    }
}

bool
RingController::TopologyInstallRouting (Ptr<RoutingInfo> rInfo, uint32_t buffer)
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
  // the controller when a flow entry expires and to avoid overlapping rules.
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
  {
    std::ostringstream match, inst;

    // In downlink the input switch is the gateway
    uint16_t swIdx = rInfo->m_sgwIdx;

    // Building the match string
    match << " eth_type=0x800,ip_proto=17" <<
    ",ip_src=" << rInfo->m_sgwAddr <<
    ",ip_dst=" << rInfo->m_enbAddr <<
    ",gtp_teid=" << rInfo->m_teid;

    // Check for meter entry
    if (meterInfo && meterInfo->m_hasDown)
      {
        if (!meterInfo->m_isInstalled)
          {
            // Install the meter entry
            DpctlCommand (GetSwitchDevice (swIdx), meterInfo->GetDownAddCmd ());
            meterInstalled = true;
          }

        // Building the meter instruction string
        inst << " meter:" << rInfo->m_teid;
      }

    // Building the output instruction string
    inst << " write:group=" << ringInfo->m_downPath;

    // Installing the rule into input switch
    std::string commandStr = args.str () + match.str () + inst.str ();
    DpctlCommand (GetSwitchDevice (swIdx), commandStr);
  }

  // Configuring uplink routing
  {
    std::ostringstream match, inst;

    // In uplink the input switch is the eNB
    uint16_t swIdx = rInfo->m_enbIdx;

    // Building the match string
    match << " eth_type=0x800,ip_proto=17" <<
    ",ip_src=" << rInfo->m_enbAddr <<
    ",ip_dst=" << rInfo->m_sgwAddr <<
    ",gtp_teid=" << rInfo->m_teid;

    // Check for meter entry
    if (meterInfo && meterInfo->m_hasUp)
      {
        if (!meterInfo->m_isInstalled)
          {
            // Install the meter entry
            DpctlCommand (GetSwitchDevice (swIdx), meterInfo->GetUpAddCmd ());
            meterInstalled = true;
          }

        // Building the meter instruction string
        inst << " meter:" << rInfo->m_teid;
      }

    // Building the output instruction string
    inst << " write:group=" << ringInfo->m_upPath;

    // Installing the rule into input switch
    std::string commandStr = args.str () + match.str () + inst.str ();
    DpctlCommand (GetSwitchDevice (swIdx), commandStr);
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
RingController::TopologyRemoveRouting (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // We will only remove meter entries from switch. This will automatically
  // will remove referring flow rules. The other rules will expired due idle
  // timeout. Doing this we avoid race conditions and allow 'in transit'
  // packets reach its destination. So, let's wait 3 seconds before removing
  // these rules.
  Simulator::Schedule (Seconds (3),
                       &RingController::RemoveMeterRules, this, rInfo);

  return true;
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  ringInfo->ResetPaths ();    // Reseting to short paths
  uint32_t teid = rInfo->m_teid;

  if (rInfo->m_isDefault)
    {
      // We always accept default bearers.
      return true;
    }

  Ptr<ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (!reserveInfo)
    {
      // For bearers without resource reservation requests (probably a
      // Non-GBR one), let's accept it, without guarantees.
      return true;
    }

  // Getting available bandwidth in both paths
  DataRate shortPathBw = GetAvailableBandwidth (rInfo->m_sgwIdx, 
      rInfo->m_enbIdx, ringInfo->m_downPath);
  DataRate longPathBw = GetAvailableBandwidth (rInfo->m_sgwIdx, 
      rInfo->m_enbIdx, RingRoutingInfo::InvertPath (ringInfo->m_downPath));

  // Reserving downlink resources
  if (reserveInfo->m_hasDown)
    {
      DataRate request = reserveInfo->m_downDataRate;
      NS_LOG_DEBUG (teid << ": downlink request: " << request);

      switch (m_strategy)
        {
        case RingController::HOPS:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            if (shortPathBw >= request)
              {
                shortPathBw = shortPathBw - request;
                ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                  ringInfo->m_downPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                return false;
              }
            break;
          }
        case RingController::BAND:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            NS_LOG_DEBUG (teid << ": available in long path: " << longPathBw);
            if (shortPathBw >= longPathBw && shortPathBw >= request)
              {
                shortPathBw = shortPathBw - request;
                ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                  ringInfo->m_downPath, request);
              }
            else if (shortPathBw < longPathBw && longPathBw >= request)
              {
                // Let's invert the path and reserve the bandwidth
                NS_LOG_DEBUG (teid << ": inverting from short to long path.");
                ringInfo->InvertDownPath ();
                longPathBw = longPathBw - request;
                ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                  ringInfo->m_downPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                return false;
              }
            break;
          }
        case RingController::SMART:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            NS_LOG_DEBUG (teid << ": available in long path: " << longPathBw);
            if (shortPathBw >= request)
              {
                shortPathBw = shortPathBw - request;
                ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                  ringInfo->m_downPath, request);
              }
            // No available bandwidth in short path. Let's check the long path.
            else if (longPathBw >= request)
              {
                // Let's invert the path and reserve the bandwidth
                NS_LOG_DEBUG (teid << ": inverting from short to long path.");
                ringInfo->InvertDownPath ();
                longPathBw = longPathBw - request;
                ReserveBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                  ringInfo->m_downPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                return false;
              }
            break;
          }
        default:
          {
            NS_ABORT_MSG ("Invalid Routing strategy.");
          }
        }
    }

  // Reserving uplink resources
  if (reserveInfo->m_hasUp)
    {
      DataRate request = reserveInfo->m_upDataRate;
      NS_LOG_DEBUG (teid << ": uplink request: " << request);

      switch (m_strategy)
        {
        case RingController::HOPS:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            if (shortPathBw >= request)
              {
                ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                                  ringInfo->m_upPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                if (reserveInfo->m_hasDown)
                  {
                    ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, 
                        ringInfo->m_downPath, reserveInfo->m_downDataRate);
                  }
                return false;
              }
            break;
          }
        case RingController::BAND:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            NS_LOG_DEBUG (teid << ": available in long path: " << longPathBw);
            if (shortPathBw >= longPathBw && shortPathBw >= request)
              {
                ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                                  ringInfo->m_upPath, request);
              }
            else if (shortPathBw < longPathBw && longPathBw >= request)
              {
                // Let's invert the path and reserve it
                NS_LOG_DEBUG (teid << ": inverting from short to long path.");
                ringInfo->InvertUpPath ();
                ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                                  ringInfo->m_upPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                if (reserveInfo->m_hasDown)
                  {
                    ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx,
                                      ringInfo->m_downPath, reserveInfo->m_downDataRate);
                  }
                return false;
              }
            break;
          }
        case RingController::SMART:
          {
            NS_LOG_DEBUG (teid << ": available in short path: " << shortPathBw);
            NS_LOG_DEBUG (teid << ": available in long path: " << longPathBw);
            if (shortPathBw >= request)
              {
                ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                                  ringInfo->m_upPath, request);
              }
            // No available bandwidth in short path. Let's check the long path.
            else if (longPathBw >= request)
              {
                // Let's invert the path and reserve it
                NS_LOG_DEBUG (teid << ": inverting from short to long path.");
                ringInfo->InvertUpPath ();
                ReserveBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx,
                                  ringInfo->m_upPath, request);
              }
            else
              {
                NS_LOG_WARN (teid << ": no resources. Block!");
                if (reserveInfo->m_hasDown)
                  {
                    ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, 
                        ringInfo->m_downPath, reserveInfo->m_downDataRate);
                  }
                return false;
              }
            break;
          }
        default:
          {
            NS_ABORT_MSG ("Invalid Routing strategy.");
          }
        }
    }

  reserveInfo->m_isReserved = true;
  return true;
}

bool
RingController::TopologyBearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  Ptr<ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (reserveInfo && reserveInfo->m_isReserved)
    {
      Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");

      reserveInfo->m_isReserved = false;
      ReleaseBandwidth (rInfo->m_sgwIdx, rInfo->m_enbIdx, ringInfo->m_downPath,
                        reserveInfo->m_downDataRate);
      ReleaseBandwidth (rInfo->m_enbIdx, rInfo->m_sgwIdx, ringInfo->m_upPath,
                        reserveInfo->m_upDataRate);
    }
  return true;
}

void
RingController::TopologyCreateSpanningTree ()
{
  NS_LOG_FUNCTION (this);

  // Let's configure one single link to drop packets when flooding over ports
  // (OFPP_FLOOD). Here we are disabling the farthest gateway link,
  // configuring its ports to OFPPC_NO_FWD flag (0x20).

  uint16_t half = (GetNSwitches () / 2);
  Ptr<ConnectionInfo> cInfo = GetConnectionInfo (half, half + 1);
  NS_LOG_DEBUG ("Disabling link from " << half << " to " <<
                half + 1 << " for broadcast messages.");

  Mac48Address macAddr1 = Mac48Address::ConvertFrom (
    cInfo->GetPortDevFirst ()->GetAddress ());
  std::ostringstream cmd1;
  cmd1 << "port-mod port=" << cInfo->GetPortNoFirst () << ",addr=" 
       <<  macAddr1 << ",conf=0x00000020,mask=0x00000020";
  DpctlCommand (cInfo->GetSwDevFirst (), cmd1.str ());

  Mac48Address macAddr2 = Mac48Address::ConvertFrom (
    cInfo->GetPortDevSecond ()->GetAddress ());
  std::ostringstream cmd2;
  cmd2 << "port-mod port=" << cInfo->GetPortNoSecond () << ",addr=" 
       << macAddr2 << ",conf=0x00000020,mask=0x00000020";
  DpctlCommand (cInfo->GetSwDevSecond (), cmd2.str ());
}

uint16_t
RingController::GetNSwitches (void) const
{
  return m_noSwitches;
}

Ptr<RingRoutingInfo>
RingController::GetRingRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  Ptr<RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo == 0)
    {
      // This is the first time in simulation we are querying ring information
      // for this bearer. Let's create and aggregate its ring routing
      // metadata. Considering the default down path the one with lower hops.
      RingRoutingInfo::RoutingPath downPath =
        FindShortestPath (rInfo->m_sgwIdx, rInfo->m_enbIdx);
      ringInfo = CreateObject<RingRoutingInfo> (rInfo, downPath);
      rInfo->AggregateObject (ringInfo);
    }
  return ringInfo;
}

void
RingController::SaveConnectionInfo (Ptr<ConnectionInfo> cInfo)
{
  // Respecting the increasing switch index order when saving connection data.
  SwitchPair_t key;
  key.first  = std::min (cInfo->GetSwIdxFirst (), cInfo->GetSwIdxSecond ());
  key.second = std::max (cInfo->GetSwIdxFirst (), cInfo->GetSwIdxSecond ());
  std::pair<SwitchPair_t, Ptr<ConnectionInfo> > entry (key, cInfo);
  std::pair<ConnInfoMap_t::iterator, bool> ret;
  ret = m_connections.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New connection info saved: switch " 
                    << cInfo->GetSwIdxFirst () << " (" 
                    << cInfo->GetPortNoFirst () << ") - switch " 
                    << cInfo->GetSwIdxSecond () << " (" 
                    << cInfo->GetPortNoSecond () << ")");
      return;
    }
  NS_FATAL_ERROR ("Error saving connection info.");
}

Ptr<ConnectionInfo>
RingController::GetConnectionInfo (uint16_t sw1, uint16_t sw2)
{
  // Respecting the increasing switch index order when getting connection data.
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

  // Get bandwidth for first hop
  uint16_t current = srcSwitchIdx;
  uint16_t next = NextSwitchIndex (current, routingPath);
  Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
  DataRate bandwidth = cInfo->GetAvailableDataRate (m_bwFactor);

  // Repeat the process for next hops
  while (next != dstSwitchIdx)
    {
      current = next;
      next = NextSwitchIndex (current, routingPath);
      cInfo = GetConnectionInfo (current, next);
      if (cInfo->GetAvailableDataRate (m_bwFactor) < bandwidth)
        {
          bandwidth = cInfo->GetAvailableDataRate (m_bwFactor);
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
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      cInfo->ReserveDataRate (reserve);
      NS_ABORT_IF (cInfo->GetAvailableDataRate () < 0);
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
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      cInfo->ReleaseDataRate (release);
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

bool
RingController::RemoveMeterRules (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  NS_ASSERT_MSG (!rInfo->m_isActive && !rInfo->m_isInstalled,
                 "Can't delete meter for valid traffic.");

  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->m_isInstalled)
    {
      NS_LOG_DEBUG ("Removing meter entries.");
      if (meterInfo->m_hasDown)
        {
          DpctlCommand (GetSwitchDevice (rInfo->m_sgwIdx),
                        meterInfo->GetDelCmd ());
        }
      if (meterInfo->m_hasUp)
        {
          DpctlCommand (GetSwitchDevice (rInfo->m_enbIdx),
                        meterInfo->GetDelCmd ());
        }
      meterInfo->m_isInstalled = false;
    }
  return true;
}

};  // namespace ns3
