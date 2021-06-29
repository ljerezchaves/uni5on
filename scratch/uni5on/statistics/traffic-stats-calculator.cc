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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <iomanip>
#include <iostream>
#include "traffic-stats-calculator.h"
#include "../applications/base-client.h"
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
    "/NodeList/*/ApplicationList/*/$ns3::EnbApplication/S1uRx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EnbApplication/S1uTx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwuTunnelApp/S5Rx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwuTunnelApp/S5Tx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/OverloadDrop",
    MakeCallback (&TrafficStatsCalculator::OverloadDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/MeterDrop",
    MakeCallback (&TrafficStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/TableDrop",
    MakeCallback (&TrafficStatsCalculator::TableDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/PortList/*/PortQueue/Drop",
    MakeCallback (&TrafficStatsCalculator::QueueDropPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::Uni5onClient/AppStart",
    MakeCallback (&TrafficStatsCalculator::ResetCounters, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::Uni5onClient/AppStop",
    MakeCallback (&TrafficStatsCalculator::DumpStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::Uni5onClient/AppError",
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
      for (int d = 0; d < N_DIRECTIONS; d++)
        {
          statsIt.second.flowStats [d] = 0;
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

  // Create the output files.
  m_appWrapper = Create<OutputStreamWrapper> (
      m_appFilename + ".log", std::ios::out);
  m_epcWrapper = Create<OutputStreamWrapper> (
      m_epcFilename + ".log", std::ios::out);

  // Print the headers in output files.
  *m_appWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8)  << "TimeSec"
    << " " << setw (9)  << "AppName"
    << " " << setw (11) << "Teid"
    << " " << setw (6)  << "Slice"
    << " " << setw (11) << "GdpDlKbps"
    << " " << setw (11) << "GdpUlKbps"
    << std::endl;

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
                                        Ptr<BaseClient> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeidHex ());

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  Ptr<const FlowStatsCalculator> stats;

  // Dump application statistics.
  *m_appWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (9)  << app->GetAppName ()
    << " " << setw (11) << rInfo->GetTeidHex ()
    << " " << setw (6)  << rInfo->GetSliceIdStr ()
    << " " << setw (11) << Bps2Kbps (app->GetDlGoodput ())
    << " " << setw (11) << Bps2Kbps (app->GetUlGoodput ())
    << std::endl;

  // Dump backhaul statistics.
  if (rInfo->HasUlTraffic ())
    {
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
                                       Ptr<BaseClient> app)
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

  GtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // GtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          stats = GetFlowStats (teid, Direction::DLINK);
          stats->NotifyTx (packet->GetSize ());
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
        }
    }
}

void
TrafficStatsCalculator::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  GtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      // Identify the meter type (MBR or slicing).
      FlowStatsCalculator::DropReason dropReason = FlowStatsCalculator::METER;
      if ((meterId & METER_SLC_TYPE) == METER_SLC_TYPE)
        {
          dropReason = FlowStatsCalculator::SLICE;
        }

      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), dropReason);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // GtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      // It must be a packed dropped by a traffic meter because this is the
      // only type of meters that we can have in the P-GW TFT switches.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          stats = GetFlowStats (teid, Direction::DLINK);
          stats->NotifyTx (packet->GetSize ());
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::METER);
        }
    }
}

void
TrafficStatsCalculator::QueueDropPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  GtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // GtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          stats = GetFlowStats (teid, Direction::DLINK);
          stats->NotifyTx (packet->GetSize ());
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
        }
    }
}

void
TrafficStatsCalculator::TableDropPacket (
  std::string context, Ptr<const Packet> packet, uint8_t tableId)
{
  NS_LOG_FUNCTION (this << context << packet <<
                   static_cast<uint16_t> (tableId));

  GtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // GtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          stats = GetFlowStats (teid, Direction::DLINK);
          stats->NotifyTx (packet->GetSize ());
          stats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);
        }
    }
}

void
TrafficStatsCalculator::EpcInputPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  GtpuTag gtpuTag;
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

  GtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> stats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      stats = GetFlowStats (gtpuTag.GetTeid (), gtpuTag.GetDirection ());
      stats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

uint32_t
TrafficStatsCalculator::PgwTftClassify (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  Ptr<Packet> packetCopy = packet->Copy ();

  EthernetHeader ethHeader;
  packetCopy->RemoveHeader (ethHeader);

  Ipv4Header ipv4Header;
  packetCopy->PeekHeader (ipv4Header);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (ipv4Header.GetDestination ());
  return ueInfo ? ueInfo->Classify (packetCopy) : 0;
}

Ptr<FlowStatsCalculator>
TrafficStatsCalculator::GetFlowStats (uint32_t teid, Direction dir)
{
  NS_LOG_FUNCTION (this << teid << dir);

  Ptr<FlowStatsCalculator> stats = 0;
  auto it = m_qosByTeid.find (teid);
  if (it != m_qosByTeid.end ())
    {
      stats = it->second.flowStats [dir];
    }
  else
    {
      FlowStatsPair pair;
      pair.flowStats [Direction::DLINK] = CreateObject<FlowStatsCalculator> ();
      pair.flowStats [Direction::ULINK] = CreateObject<FlowStatsCalculator> ();
      std::pair<uint32_t, FlowStatsPair> entry (teid, pair);
      auto ret = m_qosByTeid.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Error when saving QoS entry.");

      stats = pair.flowStats [dir];
    }
  return stats;
}

} // Namespace ns3
