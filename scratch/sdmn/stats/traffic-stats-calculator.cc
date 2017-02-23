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
#include "../epc-controller.h"
#include "../apps/sdmn-client-app.h"
#include "../apps/real-time-video-client.h"
#include "../info/routing-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (TrafficStatsCalculator);

TrafficStatsCalculator::TrafficStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TrafficStatsCalculator::~TrafficStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TrafficStatsCalculator::TrafficStatsCalculator (Ptr<EpcController> controller)
  : m_controller (controller)
{
  NS_LOG_FUNCTION (this);

  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uRx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uTx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwUserApp/S5Rx",
    MakeCallback (&TrafficStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwUserApp/S5Tx",
    MakeCallback (&TrafficStatsCalculator::EpcInputPacket, this));
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

  m_controller = 0;
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
  << fixed << setprecision (4) << boolalpha
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (8)  << "AppName"
  << setw (5)  << "QCI"
  << setw (7)  << "IsGBR"
  << setw (8)  << "UeImsi"
  << setw (8)  << "CellId"
  << setw (7)  << "SgwSw"
  << setw (7)  << "PgwSw"
  << setw (11) << "Direction"
  << setw (6)  << "TEID"
  << setw (11) << "Active(s)"
  << setw (12) << "Delay(ms)"
  << setw (12) << "Jitter(ms)"
  << setw (9)  << "RxPkts"
  << setw (12) << "LossRatio"
  << setw (6)  << "Losts"
  << setw (10) << "RxBytes"
  << setw (17) << "Throughput(kbps)"
  << std::endl;

  m_epcWrapper = Create<OutputStreamWrapper> (m_epcFilename, std::ios::out);
  *m_epcWrapper->GetStream ()
  << fixed << setprecision (4) << boolalpha
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (8)  << "AppName"
  << setw (5)  << "QCI"
  << setw (7)  << "IsGBR"
  << setw (8)  << "UeImsi"
  << setw (8)  << "CellId"
  << setw (7)  << "SgwSw"
  << setw (7)  << "PgwSw"
  << setw (11) << "Direction"
  << setw (6)  << "TEID"
  << setw (11) << "Active(s)"
  << setw (12) << "Delay(ms)"
  << setw (12) << "Jitter(ms)"
  << setw (9)  << "RxPkts"
  << setw (12) << "LossRatio"
  << setw (7)  << "Losts"
  << setw (7)  << "Meter"
  << setw (7)  << "Queue"
  << setw (10) << "RxBytes"
  << setw (17) << "Throughput(kbps)"
  << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
TrafficStatsCalculator::DumpStatistics (std::string context,
                                        Ptr<const SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeid ());

  NS_ASSERT_MSG (m_controller, "Invalid controller application.");
  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = m_controller->GetConstRoutingInfo (teid);
  Ptr<const QosStatsCalculator> epcStats;
  Ptr<const QosStatsCalculator> appStats;

  // The real time video streaming is the only app with no uplink traffic.
  if (app->GetInstanceTypeId () != RealTimeVideoClient::GetTypeId ())
    {
      // Dump uplink statistics.
      epcStats = GetQosStatsFromTeid (teid, false);
      DataRate epcThp = epcStats->GetRxThroughput ();
      *m_epcWrapper->GetStream ()
      << left
      << setw (11) << Simulator::Now ().GetSeconds ()                 << " "
      << right
      << setw (8)  << app->GetAppName ()                              << " "
      << setw (4)  << rInfo->GetQciInfo ()                            << " "
      << setw (6)  << rInfo->IsGbr ()                                 << " "
      << setw (7)  << rInfo->GetImsi ()                               << " "
      << setw (7)  << rInfo->GetCellId ()                             << " "
      << setw (6)  << rInfo->GetEnbSwDpId ()                          << " "
      << setw (6)  << rInfo->GetPgwSwDpId ()                          << " "
      << setw (10) << "up"                                            << " "
      << setw (5)  << teid                                            << " "
      << setw (10) << epcStats->GetActiveTime ().GetSeconds ()        << " "
      << setw (11) << epcStats->GetRxDelay ().GetSeconds () * 1000    << " "
      << setw (11) << epcStats->GetRxJitter ().GetSeconds () * 1000   << " "
      << setw (8)  << epcStats->GetRxPackets ()                       << " "
      << setw (11) << epcStats->GetLossRatio ()                       << " "
      << setw (6)  << epcStats->GetLostPackets ()                     << " "
      << setw (6)  << epcStats->GetMeterDrops ()                      << " "
      << setw (6)  << epcStats->GetQueueDrops ()                      << " "
      << setw (9)  << epcStats->GetRxBytes ()                         << " "
      << setw (16) << static_cast<double> (epcThp.GetBitRate ()) / 1000
      << std::endl;

      appStats = app->GetServerQosStats ();
      DataRate appThp = appStats->GetRxThroughput ();
      *m_appWrapper->GetStream ()
      << left
      << setw (11) << Simulator::Now ().GetSeconds ()                 << " "
      << right
      << setw (8)  << app->GetAppName ()                              << " "
      << setw (4)  << rInfo->GetQciInfo ()                            << " "
      << setw (6)  << rInfo->IsGbr ()                                 << " "
      << setw (7)  << rInfo->GetImsi ()                               << " "
      << setw (7)  << rInfo->GetCellId ()                             << " "
      << setw (6)  << rInfo->GetEnbSwDpId ()                          << " "
      << setw (6)  << rInfo->GetPgwSwDpId ()                          << " "
      << setw (10) << "up"                                            << " "
      << setw (5)  << teid                                            << " "
      << setw (10) << appStats->GetActiveTime ().GetSeconds ()        << " "
      << setw (11) << appStats->GetRxDelay ().GetSeconds () * 1000    << " "
      << setw (11) << appStats->GetRxJitter ().GetSeconds () * 1000   << " "
      << setw (8)  << appStats->GetRxPackets ()                       << " "
      << setw (11) << appStats->GetLossRatio ()                       << " "
      << setw (5)  << appStats->GetLostPackets ()                     << " "
      << setw (9)  << appStats->GetRxBytes ()                         << " "
      << setw (16) << static_cast<double> (appThp.GetBitRate ()) / 1000
      << std::endl;
    }

  // Dump downlink statistics.
  epcStats = GetQosStatsFromTeid (teid, true);
  DataRate epcThp = epcStats->GetRxThroughput ();
  *m_epcWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()                     << " "
  << right
  << setw (8)  << app->GetAppName ()                                  << " "
  << setw (4)  << rInfo->GetQciInfo ()                                << " "
  << setw (6)  << rInfo->IsGbr ()                                     << " "
  << setw (7)  << rInfo->GetImsi ()                                   << " "
  << setw (7)  << rInfo->GetCellId ()                                 << " "
  << setw (6)  << rInfo->GetEnbSwDpId ()                              << " "
  << setw (6)  << rInfo->GetPgwSwDpId ()                              << " "
  << setw (10) << "down"                                              << " "
  << setw (5)  << teid                                                << " "
  << setw (10) << epcStats->GetActiveTime ().GetSeconds ()            << " "
  << setw (11) << epcStats->GetRxDelay ().GetSeconds () * 1000        << " "
  << setw (11) << epcStats->GetRxJitter ().GetSeconds () * 1000       << " "
  << setw (8)  << epcStats->GetRxPackets ()                           << " "
  << setw (11) << epcStats->GetLossRatio ()                           << " "
  << setw (6)  << epcStats->GetLostPackets ()                         << " "
  << setw (6)  << epcStats->GetMeterDrops ()                          << " "
  << setw (6)  << epcStats->GetQueueDrops ()                          << " "
  << setw (9)  << epcStats->GetRxBytes ()                             << " "
  << setw (16) << static_cast<double> (epcThp.GetBitRate ()) / 1000
  << std::endl;

  appStats = app->GetQosStats ();
  DataRate appThp = appStats->GetRxThroughput ();
  *m_appWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()                     << " "
  << right
  << setw (8)  << app->GetAppName ()                                  << " "
  << setw (4)  << rInfo->GetQciInfo ()                                << " "
  << setw (6)  << rInfo->IsGbr ()                                     << " "
  << setw (7)  << rInfo->GetImsi ()                                   << " "
  << setw (7)  << rInfo->GetCellId ()                                 << " "
  << setw (6)  << rInfo->GetEnbSwDpId ()                              << " "
  << setw (6)  << rInfo->GetPgwSwDpId ()                              << " "
  << setw (10) << "down"                                              << " "
  << setw (5)  << teid                                                << " "
  << setw (10) << appStats->GetActiveTime ().GetSeconds ()            << " "
  << setw (11) << appStats->GetRxDelay ().GetSeconds () * 1000        << " "
  << setw (11) << appStats->GetRxJitter ().GetSeconds () * 1000       << " "
  << setw (8)  << appStats->GetRxPackets ()                           << " "
  << setw (11) << appStats->GetLossRatio ()                           << " "
  << setw (5)  << appStats->GetLostPackets ()                         << " "
  << setw (9)  << appStats->GetRxBytes ()                             << " "
  << setw (16) << static_cast<double> (appThp.GetBitRate ()) / 1000
  << std::endl;
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<const SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app);

  GetQosStatsFromTeid (app->GetTeid (),  true)->ResetCounters ();
  GetQosStatsFromTeid (app->GetTeid (), false)->ResetCounters ();
}

void
TrafficStatsCalculator::MeterDropPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
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
      std::pair <uint32_t, QosStatsPair_t> entry (teid, pair);
      std::pair <TeidQosMap_t::iterator, bool> ret;
      ret = m_qosStats.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Existing QoS entry for teid " << teid);
        }
      qosStats = isDown ? pair.first : pair.second;
    }
  return qosStats;
}

} // Namespace ns3
