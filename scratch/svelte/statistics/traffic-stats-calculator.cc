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
#include "../logical/epc-gtpu-tag.h"
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
                   StringValue ("traffic-qos-l7-app.log"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_appFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for L3 traffic EPC QoS statistics.",
                   StringValue ("traffic-qos-l3-epc.log"),
                   MakeStringAccessor (&TrafficStatsCalculator::m_epcFilename),
                   MakeStringChecker ())
  ;
  return tid;
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

  m_appWrapper = Create<OutputStreamWrapper> (m_appFilename, std::ios::out);
  *m_appWrapper->GetStream ()
    << fixed << setprecision (3) << boolalpha << GetHeader () << std::endl;

  m_epcWrapper = Create<OutputStreamWrapper> (m_epcFilename, std::ios::out);
  *m_epcWrapper->GetStream ()
    << fixed << setprecision (3) << boolalpha << GetHeader ()
    << setw (7)  << "Load"
    << setw (7)  << "Meter"
    << setw (7)  << "Queue"
    << setw (7)  << "Slice"
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
  Ptr<const QosStatsCalculator> epcStats;

  if (rInfo->HasUlTraffic ())
    {
      // Dump uplink statistics.
      epcStats = GetQosStatsFromTeid (teid, false);
      *m_epcWrapper->GetStream ()
        << GetStats (app, epcStats, rInfo, "up")
        << " " << setw (6)  << epcStats->GetLoadDrops ()
        << " " << setw (6)  << epcStats->GetMeterDrops ()
        << " " << setw (6)  << epcStats->GetQueueDrops ()
        << " " << setw (6)  << epcStats->GetSliceDrops ()
        << std::endl;

      *m_appWrapper->GetStream ()
        << GetStats (app, app->GetServerQosStats (), rInfo, "up")
        << std::endl;
    }

  if (rInfo->HasDlTraffic ())
    {
      // Dump downlink statistics.
      epcStats = GetQosStatsFromTeid (teid, true);
      *m_epcWrapper->GetStream ()
        << GetStats (app, epcStats, rInfo, "down")
        << " " << setw (6)  << epcStats->GetLoadDrops ()
        << " " << setw (6)  << epcStats->GetMeterDrops ()
        << " " << setw (6)  << epcStats->GetQueueDrops ()
        << " " << setw (6)  << epcStats->GetSliceDrops ()
        << std::endl;

      *m_appWrapper->GetStream ()
        << GetStats (app, app->GetQosStats (), rInfo, "down")
        << std::endl;
    }
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<SvelteClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app);

  GetQosStatsFromTeid (app->GetTeid (),  true)->ResetCounters ();
  GetQosStatsFromTeid (app->GetTeid (), false)->ResetCounters ();
}

void
TrafficStatsCalculator::LoadDropPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      qosStats->NotifyLoadDrop ();
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

      Ptr<QosStatsCalculator> qosStats = GetQosStatsFromTeid (teid, true);
      qosStats->NotifyTx (packetCopy->GetSize ());
      qosStats->NotifyLoadDrop ();
    }
}

void
TrafficStatsCalculator::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  uint32_t teid;
  EpcGtpuTag gtpuTag;
  Ptr<QosStatsCalculator> qosStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      teid = gtpuTag.GetTeid ();
      qosStats = GetQosStatsFromTeid (teid, gtpuTag.IsDownlink ());

      // Notify the droped packet, based on meter type (traffic or slicing).
      if (teid == meterId)
        {
          qosStats->NotifyMeterDrop ();
        }
      else
        {
          qosStats->NotifySliceDrop ();
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
      qosStats = GetQosStatsFromTeid (teid, true);
      qosStats->NotifyTx (packet->GetSize ());

      // Notify the droped packet (it must be a traffic meter because we only
      // have slicing meters on ring switches, not on the P-GW).
      qosStats->NotifyMeterDrop ();
    }
}

void
TrafficStatsCalculator::QueueDropPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      qosStats->NotifyQueueDrop ();
    }
}

void
TrafficStatsCalculator::EpcInputPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      qosStats->NotifyTx (packet->GetSize ());
    }
}

void
TrafficStatsCalculator::EpcOutputPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      qosStats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

Ptr<QosStatsCalculator>
TrafficStatsCalculator::GetQosStatsFromTeid (uint32_t teid, bool isDown)
{
  NS_LOG_FUNCTION (this << teid << isDown);

  Ptr<QosStatsCalculator> qosStats = 0;
  auto it = m_qosByTeid.find (teid);
  if (it != m_qosByTeid.end ())
    {
      QosStatsPair_t value = it->second;
      qosStats = isDown ? value.first : value.second;
    }
  else
    {
      QosStatsPair_t pair (CreateObject<QosStatsCalculator> (),
                           CreateObject<QosStatsCalculator> ());
      std::pair<uint32_t, QosStatsPair_t> entry (teid, pair);
      auto ret = m_qosByTeid.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Existing QoS entry for teid " << teid);
        }
      qosStats = isDown ? pair.first : pair.second;
    }
  return qosStats;
}

std::string
TrafficStatsCalculator::GetHeader (void)
{
  NS_LOG_FUNCTION (this);

  std::ostringstream str;
  str << right
      << setw (8)  << "Time(s)"
      << setw (9)  << "AppName"
      << RoutingInfo::PrintHeader ()
      << setw (7)  << "Ul/Dl"
      << QosStatsCalculator::PrintHeader ();
  return str.str ();
}

std::string
TrafficStatsCalculator::GetStats (
  Ptr<const SvelteClientApp> app, Ptr<const QosStatsCalculator> stats,
  Ptr<const RoutingInfo> rInfo, std::string direction)
{
  NS_LOG_FUNCTION (this << app << stats << rInfo << direction);

  std::ostringstream str;
  str << fixed << setprecision (3) << boolalpha << right
      << setw (8)  << Simulator::Now ().GetSeconds ()
      << setw (9)  << app->GetAppName ()
      << *rInfo
      << setw (7)  << direction
      << *stats;
  return str.str ();
}

} // Namespace ns3
