/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include <iomanip>
#include <iostream>
#include "traffic-stats-calculator.h"
#include "../applications/svelte-client.h"
#include "../metadata/ue-info.h"
#include "../metadata/routing-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (TrafficStatsCalculator);

TrafficStatsCalculator::TrafficStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteEnbApplication/S1uRx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteEnbApplication/S1uTx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwTunnelApp/S5Rx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwTunnelApp/S5Tx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/OverloadDrop",
    MakeCallback (&TrafficStatsCalculator::OverloadDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/MeterDrop",
    MakeCallback (&TrafficStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/PortList/*/PortQueue/Drop",
    MakeCallback (&TrafficStatsCalculator::QueueDropPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClient/AppStart",
    MakeCallback (&TrafficStatsCalculator::ResetCounters, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClient/AppStop",
    MakeCallback (&TrafficStatsCalculator::DumpStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClient/AppError",
    MakeCallback (&TrafficStatsCalculator::DumpStatistics, this));
}

TrafficStatsCalculator::~TrafficStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TrafficStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<TrafficStatsCalculator> ()
    .AddAttribute ("AppStatsFilename",
                   "Filename for application L7 traffic statistics.",
                   StringValue ("traffic-application-l7"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_appFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC L2 traffic statistics.",
                   StringValue ("traffic-backhaul-l2"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_epcFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
TrafficStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  for (auto &statsIt : m_qosByTeid)
    {
      for (int d = 0; d <= Direction::ULINK; d++)
        {
          statsIt.second.tffStats [d] = 0;
        }
    }
  m_appWrapper = 0;
  m_epcWrapper = 0;
  Object::DoDispose ();
}

void
TrafficStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("AppStatsFilename", StringValue (prefix + m_appFilename));
  SetAttribute ("EpcStatsFilename", StringValue (prefix + m_epcFilename));

  // Create the output file for application stats.
  m_appWrapper = Create<OutputStreamWrapper> (
      m_appFilename + ".log", std::ios::out);

  // Print the header in output file.
  *m_appWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "TimeSec"
    << " " << setw (9)  << "AppName"
    << " " << setw (11) << "Teid"
    << " " << setw (6)  << "Slice"
    << " " << setw (11) << "GdpDlKbps"
    << " " << setw (11) << "GdpUlKbps"
    << std::endl;

  // Create the output file for EPC stats.
  m_epcWrapper = Create<OutputStreamWrapper> (
      m_epcFilename + ".log", std::ios::out);

  // Print the header in output file.
  *m_epcWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8) << "TimeSec"
    << " " << setw (9) << "AppName"
    << " " << setw (7) << "TrafDir";
  RoutingInfo::PrintHeader (*m_epcWrapper->GetStream ());
  FlowStatsCalculator::PrintHeader (*m_epcWrapper->GetStream ());
  *m_epcWrapper->GetStream () << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
TrafficStatsCalculator::DumpStatistics (std::string context,
                                        Ptr<SvelteClient> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeidHex ());

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  Ptr<const FlowStatsCalculator> stats;

  *m_appWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (9)  << app->GetAppName ()
    << " " << setw (11) << rInfo->GetTeidHex ()
    << " " << setw (6)  << rInfo->GetSliceIdStr ()
    << " " << setw (11) << Bps2Kbps (app->GetDlGoodput ().GetBitRate ())
    << " " << setw (11) << Bps2Kbps (app->GetUlGoodput ().GetBitRate ())
    << std::endl;

  if (rInfo->HasUlTraffic ())
    {
      // Dump uplink statistics.
      stats = GetFlowStats (teid, Direction::ULINK);
      *m_epcWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (9) << app->GetAppName ()
        << " " << setw (7) << DirectionStr (Direction::ULINK)
        << *rInfo
        << *stats
        << std::endl;
    }

  if (rInfo->HasDlTraffic ())
    {
      // Dump downlink statistics.
      stats = GetFlowStats (teid, Direction::DLINK);
      *m_epcWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (9) << app->GetAppName ()
        << " " << setw (7) << DirectionStr (Direction::DLINK)
        << *rInfo
        << *stats
        << std::endl;
    }
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<SvelteClient> app)
{
  NS_LOG_FUNCTION (this << context << app);

  GetFlowStats (app->GetTeid (), Direction::DLINK)->ResetCounters ();
  GetFlowStats (app->GetTeid (), Direction::ULINK)->ResetCounters ();
}

void
TrafficStatsCalculator::OverloadDropPacket (std::string context,
                                            Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
    }
  else
    {
      //
      // This only happens when a packet is dropped at the P-GW, before
      // entering the logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC. To keep
      // consistent log results, we are doing this manually here.
      //
      EthernetHeader ethHeader;
      Ipv4Header ipv4Header;

      Ptr<Packet> packetCopy = packet->Copy ();
      packetCopy->RemoveHeader (ethHeader);
      packetCopy->PeekHeader (ipv4Header);

      Ptr<UeInfo> ueInfo = UeInfo::GetPointer (ipv4Header.GetDestination ());
      uint32_t teid = ueInfo->Classify (packetCopy);

      stats = GetFlowStats (teid, Direction::DLINK);
      stats->NotifyTx (packetCopy->GetSize ());
      stats->NotifyDrop (packetCopy->GetSize (), FlowStatsCalculator::PLOAD);
    }
}

void
TrafficStatsCalculator::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  uint32_t teid;
  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      teid = gtpuTag.GetTeid ();
      stats = GetFlowStats (teid, gtpuTag.GetDirection ());

      // Notify the droped packet, based on meter type (traffic or slicing).
      if (teid == meterId)
        {
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::METER);
        }
      else
        {
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::SLICE);
        }
    }
  else
    {
      //
      // This only happens when a packet is dropped at the P-GW, before
      // entering the logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      //
      teid = meterId;
      stats = GetFlowStats (teid, Direction::DLINK);
      stats->NotifyTx (packet->GetSize ());

      // Notify the droped packet (it must be a traffic meter because we only
      // have slicing meters on ring switches, not on the P-GW).
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::METER);
    }
}

void
TrafficStatsCalculator::QueueDropPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
    }
}

void
TrafficStatsCalculator::EpcInputPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyTx (packet->GetSize ());
    }
}

void
TrafficStatsCalculator::EpcOutputPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

Ptr<FlowStatsCalculator>
TrafficStatsCalculator::GetFlowStats (uint32_t teid, Direction dir)
{
  NS_LOG_FUNCTION (this << teid << dir);

  Ptr<FlowStatsCalculator> stats = 0;
  auto it = m_qosByTeid.find (teid);
  if (it != m_qosByTeid.end ())
    {
      stats = it->second.tffStats [dir];
    }
  else
    {
      FlowStatsPair pair;
      pair.tffStats [Direction::DLINK] = CreateObject<FlowStatsCalculator> ();
      pair.tffStats [Direction::ULINK] = CreateObject<FlowStatsCalculator> ();
      std::pair<uint32_t, FlowStatsPair> entry (teid, pair);
      auto ret = m_qosByTeid.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Error when saving QoS entry.");

      stats = pair.tffStats [dir];
    }
  return stats;
}

} // Namespace ns3
