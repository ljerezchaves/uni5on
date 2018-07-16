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
#include "../apps/sdmn-client-app.h"
#include "../epc/epc-controller.h"
#include "../epc/epc-gtpu-tag.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (TrafficStatsCalculator);

TrafficStatsCalculator::TrafficStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SdmnEnbApplication/S1uRx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SdmnEnbApplication/S1uTx",
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
    "/NodeList/*/ApplicationList/*/$ns3::SdmnClientApp/AppStart",
    MakeCallback (&TrafficStatsCalculator::ResetCounters, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SdmnClientApp/AppStop",
    MakeCallback (&TrafficStatsCalculator::DumpStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SdmnClientApp/AppError",
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
                                        Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeid ());

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  Ptr<const QosStatsCalculator> epcStats;

  if (rInfo->HasUplinkTraffic ())
    {
      // Dump uplink statistics.
      epcStats = GetQosStatsFromTeid (teid, false);
      *m_epcWrapper->GetStream ()
      << GetStats (app, rInfo, ueInfo, epcStats, teid, "up")
      << " " << setw (6)  << epcStats->GetLoadDrops ()
      << " " << setw (6)  << epcStats->GetMeterDrops ()
      << " " << setw (6)  << epcStats->GetQueueDrops ()
      << " " << setw (6)  << epcStats->GetSliceDrops ()
      << std::endl;

      *m_appWrapper->GetStream ()
      << GetStats (app, rInfo, ueInfo, app->GetServerQosStats (), teid, "up")
      << std::endl;
    }

  if (rInfo->HasDownlinkTraffic ())
    {
      // Dump downlink statistics.
      epcStats = GetQosStatsFromTeid (teid, true);
      *m_epcWrapper->GetStream ()
      << GetStats (app, rInfo, ueInfo, epcStats, teid, "down")
      << " " << setw (6)  << epcStats->GetLoadDrops ()
      << " " << setw (6)  << epcStats->GetMeterDrops ()
      << " " << setw (6)  << epcStats->GetQueueDrops ()
      << " " << setw (6)  << epcStats->GetSliceDrops ()
      << std::endl;

      *m_appWrapper->GetStream ()
      << GetStats (app, rInfo, ueInfo, app->GetQosStats (), teid, "down")
      << std::endl;
    }
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<SdmnClientApp> app)
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
  TeidQosMap_t::iterator it = m_qosStats.find (teid);
  if (it != m_qosStats.end ())
    {
      QosStatsPair_t value = it->second;
      qosStats = isDown ? value.first : value.second;
    }
  else
    {
      QosStatsPair_t pair (CreateObject<QosStatsCalculator> (),
                           CreateObject<QosStatsCalculator> ());
      std::pair<uint32_t, QosStatsPair_t> entry (teid, pair);
      std::pair<TeidQosMap_t::iterator, bool> ret;
      ret = m_qosStats.insert (entry);
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
  str << left
      << setw (12) << "Time(s)"
      << right
      << setw (8)  << "AppName"
      << setw (8)  << "TEID"
      << setw (5)  << "QCI"
      << setw (7)  << "IsGBR"
      << setw (7)  << "IsMTC"
      << setw (7)  << "IsAgg"
      << setw (6)  << "IMSI"
      << setw (5)  << "CGI"
      << setw (7)  << "Ul/Dl"
      << setw (11) << "Active(s)"
      << setw (11) << "Delay(ms)"
      << setw (12) << "Jitter(ms)"
      << setw (8)  << "TxPkts"
      << setw (8)  << "RxPkts"
      << setw (9)  << "Loss(%)"
      << setw (10) << "RxBytes"
      << setw (12) << "Thp(Kbps)";
  return str.str ();
}

std::string
TrafficStatsCalculator::GetStats (
  Ptr<const SdmnClientApp> app, Ptr<const RoutingInfo> rInfo,
  Ptr<const UeInfo> ueInfo, Ptr<const QosStatsCalculator> stats,
  uint32_t teid, std::string direction)
{
  NS_LOG_FUNCTION (this << stats << direction);

  std::ostringstream str;
  str << fixed << setprecision (3) << boolalpha
      << left
      << setw (11) << Simulator::Now ().GetSeconds ()
      << right
      << " " << setw (8)  << app->GetAppName ()
      << " " << setw (7)  << teid
      << " " << setw (4)  << rInfo->GetQciInfo ()
      << " " << setw (6)  << rInfo->IsGbr ()
      << " " << setw (6)  << rInfo->IsMtc ()
      << " " << setw (6)  << rInfo->IsAggregated ()
      << " " << setw (5)  << ueInfo->GetImsi ()
      << " " << setw (4)  << ueInfo->GetCellId ()
      << " " << setw (6)  << direction
      << " " << setw (10) << stats->GetActiveTime ().GetSeconds ()
      << " " << setw (10) << stats->GetRxDelay ().GetSeconds () * 1000
      << " " << setw (11) << stats->GetRxJitter ().GetSeconds () * 1000
      << " " << setw (7)  << stats->GetTxPackets ()
      << " " << setw (7)  << stats->GetRxPackets ()
      << " " << setw (8)  << stats->GetLossRatio () * 100
      << " " << setw (9)  << stats->GetRxBytes ()
      << " " << setw (11)
      << (double)(stats->GetRxThroughput ().GetBitRate ()) / 1000;
  return str.str ();
}

} // Namespace ns3
