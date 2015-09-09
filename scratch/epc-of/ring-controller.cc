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
                   "The ring routing strategy.",
                   EnumValue (RingController::SPO),
                   MakeEnumAccessor (&RingController::m_strategy),
                   MakeEnumChecker (RingController::SPO, "spo",
                                    RingController::SPF, "spf"))
    .AddAttribute ("DebarIncStep",
                   "DeBaR increase adjustment step.",
                   DoubleValue (0.025), // 2.5% of GBR quota
                   MakeDoubleAccessor (&RingController::m_debarStep),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("EnableShortDebar",
                   "Enable GBR Distance-Based Reservation algorithm (DeBaR) "
                   "in shortest path.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RingController::m_debarShortPath),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableLongDebar",
                   "Enable GBR Distance-Based Reservation algorithm (DeBaR) "
                   "in longest (inverted) paths.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RingController::m_debarLongPath),
                   MakeBooleanChecker ())
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
RingController::TopologyInstallRouting (Ptr<RoutingInfo> rInfo,
                                        uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << rInfo->GetPriority ()
                        << buffer);
  NS_ASSERT_MSG (rInfo->IsActive (), "Rule not active.");

  // Getting rInfo associated metadata
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  bool meterInstalled = false;

  // Increasing the priority every time we (re)install TEID rules.
  rInfo->IncreasePriority ();

  // flow-mod flags OFPFF_SEND_FLOW_REM and OFPFF_CHECK_OVERLAP, used to notify
  // the controller when a flow entry expires and to avoid overlapping rules.
  std::string flagsStr ("0x0003");

  // Printing the cookie and buffer values in dpctl string format
  char cookieStr [9], bufferStr [12];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
  sprintf (bufferStr, "%u",   buffer);

  // Building the dpctl command + arguments string
  std::ostringstream args;
  args << "flow-mod cmd=add,table=1"
       << ",buffer=" << bufferStr
       << ",flags=" << flagsStr
       << ",cookie=" << cookieStr
       << ",prio=" << rInfo->GetPriority ()
       << ",idle=" << rInfo->GetTimeout ();

  // Configuring downlink routing
  if (rInfo->HasDownlinkTraffic ())
    {
      std::ostringstream match, inst;

      // In downlink the input switch is the gateway
      uint16_t swIdx = rInfo->GetSgwSwIdx ();

      // Building the match string
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_src=" << rInfo->GetSgwAddr ()
            << ",ip_dst=" << rInfo->GetEnbAddr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // Check for meter entry
      if (meterInfo && meterInfo->HasDown ())
        {
          if (!meterInfo->IsInstalled ())
            {
              // Install the meter entry
              DpctlCommand (GetSwitchDevice (swIdx),
                            meterInfo->GetDownAddCmd ());
              meterInstalled = true;
            }

          // Building the meter instruction string
          inst << " meter:" << rInfo->GetTeid ();
        }

      // Building the output instruction string
      inst << " write:group=" << ringInfo->GetDownPath ();

      // Installing the rule into input switch
      std::string commandStr = args.str () + match.str () + inst.str ();
      DpctlCommand (GetSwitchDevice (swIdx), commandStr);
    }

  // Configuring uplink routing
  if (rInfo->HasUplinkTraffic ())
    {
      std::ostringstream match, inst;

      // In uplink the input switch is the eNB
      uint16_t swIdx = rInfo->GetEnbSwIdx ();

      // Building the match string
      match << " eth_type=0x800,ip_proto=17"
            << ",ip_src=" << rInfo->GetEnbAddr ()
            << ",ip_dst=" << rInfo->GetSgwAddr ()
            << ",gtp_teid=" << rInfo->GetTeid ();

      // Check for meter entry
      if (meterInfo && meterInfo->HasUp ())
        {
          if (!meterInfo->IsInstalled ())
            {
              // Install the meter entry
              DpctlCommand (GetSwitchDevice (swIdx),
                            meterInfo->GetUpAddCmd ());
              meterInstalled = true;
            }

          // Building the meter instruction string
          inst << " meter:" << rInfo->GetTeid ();
        }

      // Building the output instruction string
      inst << " write:group=" << ringInfo->GetUpPath ();

      // Installing the rule into input switch
      std::string commandStr = args.str () + match.str () + inst.str ();
      DpctlCommand (GetSwitchDevice (swIdx), commandStr);
    }

  // Updating meter installation flag
  if (meterInstalled)
    {
      meterInfo->SetInstalled (true);
    }

  rInfo->SetInstalled (true);
  return true;
}

bool
RingController::TopologyRemoveRouting (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // We will only remove meter entries from switch. This will automatically
  // will remove referring flow rules. The other rules will expired due idle
  // timeout. Doing this we avoid race conditions and allow 'in transit'
  // packets reach its destination. So, let's wait 1 second before removing
  // these rules.
  Simulator::Schedule (Seconds (1), &RingController::RemoveMeterRules,
                       this, rInfo);

  return true;
}

bool
RingController::TopologyBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  // Reseting ring routing info to the shortest path
  Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
  ringInfo->ResetToShortestPaths ();

  if (rInfo->IsDefault ())
    {
      // We always accept default bearers over the shortest path.
      return true;
    }

  Ptr<ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (!reserveInfo)
    {
      // For bearers without resource reservation requests (probably a
      // Non-GBR one), let's accept it, without guarantees.
      return true;

      // TODO: In current implementation, Non-GBR bearers are always routed
      // over the shortest path. However, nothing prevents the use of a more
      // sofisticated routing approach.
    }

  NS_ASSERT_MSG (rInfo->IsGbr (), "Invalid configuration for bearer request.");
  uint32_t teid = rInfo->GetTeid ();

  // Getting available downlink and uplink bit rates in both paths
  std::pair<uint64_t, uint64_t> shortPathBand, longPathBand;
  shortPathBand = GetAvailableGbrBitRate (ringInfo, true);
  longPathBand  = GetAvailableGbrBitRate (ringInfo, false);

  uint64_t dlShortBw = shortPathBand.first;
  uint64_t ulShortBw = shortPathBand.second;
  uint64_t dlLongBw  = longPathBand.first;
  uint64_t ulLongBw  = longPathBand.second;

  // Getting bit rate requests
  uint64_t dlRequest = reserveInfo->GetDownBitRate ();
  uint64_t ulRequest = reserveInfo->GetUpBitRate ();

  NS_LOG_DEBUG (teid << ":    request: downlink " << dlRequest << " - uplink " << ulRequest);
  NS_LOG_DEBUG (teid << ": short path: downlink " << dlShortBw << " - uplink " << ulShortBw);
  NS_LOG_DEBUG (teid << ":  long path: downlink " << dlLongBw  << " - uplink " << ulLongBw);

  switch (m_strategy)
    {
    case RingController::SPO:
      {
        if (dlShortBw >= dlRequest && ulShortBw >= ulRequest)
          {
            return ReserveGbrBitRate (ringInfo, reserveInfo);
          }
        else
          {
            NS_LOG_WARN ("No resources. Block!");
            return false;
          }
        break;
      }
    case RingController::SPF:
      {
        if (dlShortBw >= dlRequest && ulShortBw >= ulRequest)
          {
            return ReserveGbrBitRate (ringInfo, reserveInfo);
          }
        else if (dlLongBw >= dlRequest && ulLongBw >= ulRequest)
          {
            // Let's invert the path and reserve the bit rate
            NS_LOG_DEBUG ("Inverting from short to long path.");
            ringInfo->InvertPaths ();
            return ReserveGbrBitRate (ringInfo, reserveInfo);
          }
        else
          {
            NS_LOG_WARN ("No resources. Block!");
            return false;
          }
        break;
      }
    }
  NS_ABORT_MSG ("Invalid Routing strategy.");
}

bool
RingController::TopologyBearerRelease (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  Ptr<ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (reserveInfo && reserveInfo->IsReserved ())
    {
      Ptr<RingRoutingInfo> ringInfo = GetRingRoutingInfo (rInfo);
      NS_ASSERT_MSG (ringInfo, "No ringInfo for bearer release.");
      ReleaseGbrBitRate (ringInfo, reserveInfo);
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
        FindShortestPath (rInfo->GetSgwSwIdx (), rInfo->GetEnbSwIdx ());
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

uint16_t
RingController::HopCounter (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                            RingRoutingInfo::RoutingPath routingPath)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx);
  NS_ASSERT (srcSwitchIdx != dstSwitchIdx);
  NS_ASSERT (std::max (srcSwitchIdx, dstSwitchIdx) < GetNSwitches ());

  int distance = dstSwitchIdx - srcSwitchIdx;
  if (routingPath == RingRoutingInfo::COUNTER)
    {
      distance = srcSwitchIdx - dstSwitchIdx;
    }
  if (distance < 0)
    {
      distance += GetNSwitches ();
    }
  return distance;
}

std::pair<uint64_t, uint64_t>
RingController::GetAvailableGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                        bool useShortPath)
{
  NS_LOG_FUNCTION (this << ringInfo << useShortPath);

  uint16_t sgwIdx      = ringInfo->GetSgwSwIdx ();
  uint16_t enbIdx      = ringInfo->GetEnbSwIdx ();
  uint64_t downBitRate = std::numeric_limits<uint64_t>::max ();
  uint64_t upBitRate   = std::numeric_limits<uint64_t>::max ();
  uint64_t bitRate     = 0;
  uint16_t current     = enbIdx;
  double   debarFactor = 1.0;

  RingRoutingInfo::RoutingPath upPath = FindShortestPath (enbIdx, sgwIdx);
  if (!useShortPath)
    {
      upPath = RingRoutingInfo::InvertPath (upPath);
    }

  // From the eNB to the gateway switch index, get the bit rate for each link
  while (current != sgwIdx)
    {
      uint16_t next = NextSwitchIndex (current, upPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);

      // Check for available bit rate in uplink direction
      bitRate = cInfo->GetAvailableGbrBitRate (current, next, debarFactor);
      if (bitRate < upBitRate)
        {
          upBitRate = bitRate;
        }

      // Check for available bit rate in downlink direction
      bitRate = cInfo->GetAvailableGbrBitRate (next, current, debarFactor);
      if (bitRate < downBitRate)
        {
          downBitRate = bitRate;
        }
      current = next;

      // If enable, apply the GBR Distance-Based Reservation algorithm (DeBaR)
      // when looking for the available bit rate in routing path.
      if ((m_debarShortPath && useShortPath)
          || (m_debarLongPath && !useShortPath))
        {
          // Avoiding negative DeBaR factor
          debarFactor = std::max (debarFactor - m_debarStep, 0.0);
        }
    }

  // Return the pair of available GBR bit rates (downlink and uplink)
  return std::pair<uint64_t, uint64_t> (downBitRate, upBitRate);
}

bool
RingController::ReserveGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<ReserveInfo> reserveInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << reserveInfo);

  // Reserving resources in both directions
  PerLinkReserve (ringInfo->GetSgwSwIdx (), ringInfo->GetEnbSwIdx (),
                  ringInfo->GetDownPath (), reserveInfo->GetDownBitRate ());
  PerLinkReserve (ringInfo->GetEnbSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetUpPath (), reserveInfo->GetUpBitRate ());
  reserveInfo->SetReserved (true);
  return true;
}

bool
RingController::ReleaseGbrBitRate (Ptr<const RingRoutingInfo> ringInfo,
                                   Ptr<ReserveInfo> reserveInfo)
{
  NS_LOG_FUNCTION (this << ringInfo << reserveInfo);

  // Releasing resources in both directions
  PerLinkRelease (ringInfo->GetSgwSwIdx (), ringInfo->GetEnbSwIdx (),
                  ringInfo->GetDownPath (), reserveInfo->GetDownBitRate ());
  PerLinkRelease (ringInfo->GetEnbSwIdx (), ringInfo->GetSgwSwIdx (),
                  ringInfo->GetUpPath (), reserveInfo->GetUpBitRate ());
  reserveInfo->SetReserved (false);
  return true;
}

bool
RingController::PerLinkReserve (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                RingRoutingInfo::RoutingPath routingPath,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx
                        << routingPath << bitRate);

  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      cInfo->ReserveGbrBitRate (current, next, bitRate);
      current = next;
    }
  return true;
}

bool
RingController::PerLinkRelease (uint16_t srcSwitchIdx, uint16_t dstSwitchIdx,
                                RingRoutingInfo::RoutingPath routingPath,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcSwitchIdx << dstSwitchIdx
                        << routingPath << bitRate);

  uint16_t current = srcSwitchIdx;
  while (current != dstSwitchIdx)
    {
      uint16_t next = NextSwitchIndex (current, routingPath);
      Ptr<ConnectionInfo> cInfo = GetConnectionInfo (current, next);
      cInfo->ReleaseGbrBitRate (current, next, bitRate);
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

  NS_ASSERT_MSG (!rInfo->IsActive () && !rInfo->IsInstalled (),
                 "Can't delete meter for valid traffic.");

  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->IsInstalled ())
    {
      NS_LOG_DEBUG ("Removing meter entries.");
      if (meterInfo->HasDown ())
        {
          DpctlCommand (GetSwitchDevice (rInfo->GetSgwSwIdx ()),
                        meterInfo->GetDelCmd ());
        }
      if (meterInfo->HasUp ())
        {
          DpctlCommand (GetSwitchDevice (rInfo->GetEnbSwIdx ()),
                        meterInfo->GetDelCmd ());
        }
      meterInfo->SetInstalled (false);
    }
  return true;
}

};  // namespace ns3
