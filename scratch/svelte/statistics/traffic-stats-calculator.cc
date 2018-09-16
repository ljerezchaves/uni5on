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
#include "../applications/svelte-client-app.h"
#include "../metadata/ue-info.h"
#include "../metadata/routing-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (EpcStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (TrafficStatsCalculator);

EpcStatsCalculator::EpcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  for (int r = 0; r <= DropReason::ALL; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
}

EpcStatsCalculator::~EpcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcStatsCalculator")
    .SetParent<AppStatsCalculator> ()
    .AddConstructor<EpcStatsCalculator> ()
  ;
  return tid;
}

void
EpcStatsCalculator::ResetCounters (void)
{
  NS_LOG_FUNCTION (this);

  for (int r = 0; r <= DropReason::ALL; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
  AppStatsCalculator::ResetCounters ();
}

uint32_t
EpcStatsCalculator::GetDpBytes (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpBytes [reason];
}

uint32_t
EpcStatsCalculator::GetDpPackets (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpPackets [reason];
}

void
EpcStatsCalculator::NotifyDrop (uint32_t dpBytes, DropReason reason)
{
  NS_LOG_FUNCTION (this << dpBytes << reason);

  m_dpPackets [reason]++;
  m_dpBytes [reason] += dpBytes;

  m_dpPackets [DropReason::ALL]++;
  m_dpBytes [DropReason::ALL] += dpBytes;
}

std::string
EpcStatsCalculator::PrintHeader (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::ostringstream str;
  str << AppStatsCalculator::PrintHeader ()
      << " " << setw (6) << "DpLoa"
      << " " << setw (6) << "DpMbr"
      << " " << setw (6) << "DpSli"
      << " " << setw (6) << "DpQue"
      << " " << setw (6) << "DpAll";
  return str.str ();
}

void
EpcStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  AppStatsCalculator::DoDispose ();
}

std::ostream & operator << (std::ostream &os, const EpcStatsCalculator &stats)
{
  os << static_cast<AppStatsCalculator> (stats);
  for (int r = 0; r <= EpcStatsCalculator::ALL; r++)
    {
      os << " " << setw (6) << stats.GetDpPackets (
        static_cast<EpcStatsCalculator::DropReason> (r));
    }
  return os;
}


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
    "/NodeList/*/$ns3::OFSwitch13Device/LoadDrop",
    MakeCallback (&TrafficStatsCalculator::LoadDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/MeterDrop",
    MakeCallback (&TrafficStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/PortList/*/PortQueue/Drop",
    MakeCallback (&TrafficStatsCalculator::QueueDropPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClientApp/AppStart",
    MakeCallback (&TrafficStatsCalculator::ResetCounters, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClientApp/AppStop",
    MakeCallback (&TrafficStatsCalculator::DumpStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SvelteClientApp/AppError",
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
                   "Filename for L7 traffic application QoS statistics.",
                   StringValue ("traffic-qos-l7-app"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_appFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for L3 traffic EPC QoS statistics.",
                   StringValue ("traffic-qos-l3-epc"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_epcFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

std::string
TrafficStatsCalculator::DirectionStr (Direction dir)
{
  switch (dir)
    {
    case TrafficStatsCalculator::DLINK:
      return "Dlink";
    case TrafficStatsCalculator::ULINK:
      return "Ulink";
    default:
      return "-";
    }
}

TrafficStatsCalculator::Direction
TrafficStatsCalculator::GetDirection (EpcGtpuTag &gtpuTag)
{
  return gtpuTag.IsDownlink () ? Direction::DLINK : Direction::ULINK;
}

void
TrafficStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

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

  m_appWrapper = Create<OutputStreamWrapper> (
      m_appFilename + ".log", std::ios::out);
  *m_appWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8) << "Time:s"
    << " " << setw (8) << "AppName"
    << " " << setw (6) << "Ul/Dl"
    << RoutingInfo::PrintHeader ()
    << AppStatsCalculator::PrintHeader ()
    << std::endl;

  m_epcWrapper = Create<OutputStreamWrapper> (
      m_epcFilename + ".log", std::ios::out);
  *m_epcWrapper->GetStream ()
    << boolalpha << right << fixed << setprecision (3)
    << " " << setw (8) << "Time:s"
    << " " << setw (8) << "AppName"
    << " " << setw (6) << "Ul/Dl"
    << RoutingInfo::PrintHeader ()
    << EpcStatsCalculator::PrintHeader ()
    << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
TrafficStatsCalculator::DumpStatistics (std::string context,
                                        Ptr<SvelteClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeidHex ());

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  Ptr<const EpcStatsCalculator> epcStats;

  if (rInfo->HasUlTraffic ())
    {
      // Dump uplink statistics.
      epcStats = GetEpcStats (teid, Direction::ULINK);
      *m_epcWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (8) << app->GetAppName ()
        << " " << setw (6) << DirectionStr (Direction::ULINK)
        << *rInfo
        << *epcStats
        << std::endl;

      *m_appWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (8) << app->GetAppName ()
        << " " << setw (6) << DirectionStr (Direction::ULINK)
        << *rInfo
        << *app->GetServerAppStats ()
        << std::endl;
    }

  if (rInfo->HasDlTraffic ())
    {
      // Dump downlink statistics.
      epcStats = GetEpcStats (teid, Direction::DLINK);
      *m_epcWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (8) << app->GetAppName ()
        << " " << setw (6) << DirectionStr (Direction::DLINK)
        << *rInfo
        << *epcStats
        << std::endl;

      *m_appWrapper->GetStream ()
        << " " << setw (8) << Simulator::Now ().GetSeconds ()
        << " " << setw (8) << app->GetAppName ()
        << " " << setw (6) << DirectionStr (Direction::DLINK)
        << *rInfo
        << *app->GetAppStats ()
        << std::endl;
    }
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<SvelteClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app);

  GetEpcStats (app->GetTeid (), Direction::DLINK)->ResetCounters ();
  GetEpcStats (app->GetTeid (), Direction::ULINK)->ResetCounters ();
}

void
TrafficStatsCalculator::LoadDropPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<EpcStatsCalculator> epcStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      epcStats = GetEpcStats (gtpuTag.GetTeid (), GetDirection (gtpuTag));
      epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::PLOAD);
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

      epcStats = GetEpcStats (teid, Direction::DLINK);
      epcStats->NotifyTx (packetCopy->GetSize ());
      epcStats->NotifyDrop (packetCopy->GetSize (), EpcStatsCalculator::PLOAD);
    }
}

void
TrafficStatsCalculator::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  uint32_t teid;
  EpcGtpuTag gtpuTag;
  Ptr<EpcStatsCalculator> epcStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      teid = gtpuTag.GetTeid ();
      epcStats = GetEpcStats (teid, GetDirection (gtpuTag));

      // Notify the droped packet, based on meter type (traffic or slicing).
      if (teid == meterId)
        {
          epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::METER);
        }
      else
        {
          epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::SLICE);
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
      epcStats = GetEpcStats (teid, Direction::DLINK);
      epcStats->NotifyTx (packet->GetSize ());

      // Notify the droped packet (it must be a traffic meter because we only
      // have slicing meters on ring switches, not on the P-GW).
      epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::METER);
    }
}

void
TrafficStatsCalculator::QueueDropPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<EpcStatsCalculator> epcStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      epcStats = GetEpcStats (gtpuTag.GetTeid (), GetDirection (gtpuTag));
      epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::QUEUE);
    }
}

void
TrafficStatsCalculator::EpcInputPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<EpcStatsCalculator> epcStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      epcStats = GetEpcStats (gtpuTag.GetTeid (), GetDirection (gtpuTag));
      epcStats->NotifyTx (packet->GetSize ());
    }
}

void
TrafficStatsCalculator::EpcOutputPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<EpcStatsCalculator> epcStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      epcStats = GetEpcStats (gtpuTag.GetTeid (), GetDirection (gtpuTag));
      epcStats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

Ptr<EpcStatsCalculator>
TrafficStatsCalculator::GetEpcStats (uint32_t teid,
                                     TrafficStatsCalculator::Direction dir)
{
  NS_LOG_FUNCTION (this << teid << dir);

  Ptr<EpcStatsCalculator> epcStats = 0;
  auto it = m_qosByTeid.find (teid);
  if (it != m_qosByTeid.end ())
    {
      epcStats = it->second.stats [dir];
    }
  else
    {
      EpcStatsPair pair;
      pair.stats [Direction::DLINK] = CreateObject<EpcStatsCalculator> ();
      pair.stats [Direction::ULINK] = CreateObject<EpcStatsCalculator> ();
      std::pair<uint32_t, EpcStatsPair> entry (teid, pair);
      auto ret = m_qosByTeid.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Existing QoS entry for teid " << teid);
        }
      epcStats = pair.stats [dir];
    }
  return epcStats;
}

} // Namespace ns3
