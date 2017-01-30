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

#include <ns3/simulator.h>
#include <ns3/log.h>
#include <iomanip>
#include <iostream>
#include "stats-calculator.h"
#include "routing-info.h"
#include "ring-routing-info.h"
#include "gbr-info.h"
#include "connection-info.h"
#include "epc-controller.h"
#include "apps/real-time-video-client.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (AdmissionStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (BackhaulStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (TrafficStatsCalculator);

AdmissionStatsCalculator::AdmissionStatsCalculator ()
  : m_nonRequests (0),
    m_nonAccepted (0),
    m_nonBlocked  (0),
    m_gbrRequests (0),
    m_gbrAccepted (0),
    m_gbrBlocked  (0),
    m_activeBearers (0)
{
  NS_LOG_FUNCTION (this);
}

AdmissionStatsCalculator::~AdmissionStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
AdmissionStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AdmissionStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<AdmissionStatsCalculator> ()
    .AddAttribute ("AdmStatsFilename",
                   "Filename for bearer admission and counter statistics.",
                   StringValue ("bearer-counters.log"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_admFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("bearer-requests.log"),
                   MakeStringAccessor (
                     &AdmissionStatsCalculator::m_brqFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
AdmissionStatsCalculator::NotifyBearerRequest (bool accepted,
                                               Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << accepted << rInfo);

  // Update internal counters
  if (rInfo->IsGbr ())
    {
      m_gbrRequests++;
      accepted ? m_gbrAccepted++ : m_gbrBlocked++;
    }
  else
    {
      m_nonRequests++;
      accepted ? m_nonAccepted++ : m_nonBlocked++;
    }

  if (accepted)
    {
      m_activeBearers++;
    }

  // Preparing bearer request stats for trace source
  uint64_t downBitRate = 0, upBitRate = 0;
  Ptr<const GbrInfo> gbrInfo = rInfo->GetObject<GbrInfo> ();
  if (gbrInfo)
    {
      downBitRate = gbrInfo->GetDownBitRate ();
      upBitRate = gbrInfo->GetUpBitRate ();
    }

  std::string path = "None";
  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo && accepted)
    {
      path = ringInfo->GetPathDesc ();
      if (rInfo->IsDefault ())
        {
          path += " (default)";
        }
    }

  // Save request stats into output file
  *m_brqWrapper->GetStream ()
  << left
  << setw (9)  << Simulator::Now ().GetSeconds ()           << " "
  << right
  << setw (4)  << rInfo->GetQciInfo ()                      << " "
  << setw (6)  << rInfo->IsGbr ()                           << " "
  << setw (7)  << rInfo->GetImsi ()                         << " "
  << setw (7)  << rInfo->GetCellId ()                       << " "
  << setw (6)  << rInfo->GetEnbSwIdx ()                     << " "
  << setw (6)  << rInfo->GetTeid ()                         << " "
  << setw (9)  << accepted                                  << " "
  << setw (11) << static_cast<double> (downBitRate) / 1000  << " "
  << setw (11) << static_cast<double> (upBitRate) / 1000    << "  "
  << left
  << setw (15) << path
  << std::endl;
}

void
AdmissionStatsCalculator::NotifyBearerRelease (bool success,
                                               Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << success << rInfo);
  NS_ASSERT_MSG (m_activeBearers, "No active bearer here.");

  m_activeBearers--;
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_admWrapper = 0;
  m_brqWrapper = 0;
}

void
AdmissionStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("AdmStatsFilename", StringValue (prefix + m_admFilename));
  SetAttribute ("BrqStatsFilename", StringValue (prefix + m_brqFilename));

  m_admWrapper = Create<OutputStreamWrapper> (m_admFilename, std::ios::out);
  *m_admWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (11) << "Time(s)"
  << right
  << setw (14) << "GbrReqs"
  << setw (14) << "GbrBlocks"
  << setw (14) << "NonGbrReqs"
  << setw (14) << "NonGbrBlocks"
  << setw (14) << "ActiveBearers"
  << std::endl;

  m_brqWrapper = Create<OutputStreamWrapper> (m_brqFilename, std::ios::out);
  *m_brqWrapper->GetStream ()
  << boolalpha << fixed << setprecision (4)
  << left
  << setw (10) << "Time(s)"
  << right
  << setw (4)  << "QCI"
  << setw (7)  << "IsGBR"
  << setw (8)  << "UeImsi"
  << setw (8)  << "CellId"
  << setw (7)  << "SwIdx"
  << setw (7)  << "TEID"
  << setw (10) << "Accepted"
  << setw (12) << "Down(kbps)"
  << setw (12) << "Up(kbps)"
  << left      << "  "
  << setw (12) << "RoutingPath"
  << std::endl;

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time firstDump = timeValue.Get ();
  Simulator::Schedule (firstDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, firstDump);

  Object::NotifyConstructionCompleted ();
}

void
AdmissionStatsCalculator::DumpStatistics (Time nextDump)
{
  NS_LOG_FUNCTION (this);

  *m_admWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds () << " "
  << right
  << setw (13) << m_gbrRequests                   << " "
  << setw (13) << m_gbrBlocked                    << " "
  << setw (13) << m_nonRequests                   << " "
  << setw (13) << m_nonBlocked                    << " "
  << setw (13) << m_activeBearers
  << std::endl;

  ResetCounters ();
  Simulator::Schedule (nextDump, &AdmissionStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
AdmissionStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);
  m_nonRequests = 0;
  m_nonAccepted = 0;
  m_nonBlocked  = 0;
  m_gbrRequests = 0;
  m_gbrAccepted = 0;
  m_gbrBlocked  = 0;
}


// ------------------------------------------------------------------------ //
BackhaulStatsCalculator::BackhaulStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
}

BackhaulStatsCalculator::~BackhaulStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BackhaulStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BackhaulStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<BackhaulStatsCalculator> ()
    .AddAttribute ("RegStatsFilename",
                   "Filename for GBR reservation statistics.",
                   StringValue ("ofnetwork-reserve-gbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_regFilename),
                   MakeStringChecker ())
    .AddAttribute ("RenStatsFilename",
                   "Filename for Non-GBR allowed bandwidth statistics.",
                   StringValue ("ofnetwork-reserve-nongbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_renFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwbStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("ofnetwork-throughput-all.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwbFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwgStatsFilename",
                   "Filename for GBR bandwidth statistics.",
                   StringValue ("ofnetwork-throughput-gbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwgFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwnStatsFilename",
                   "Filename for Non-GBR bandwidth statistics.",
                   StringValue ("ofnetwork-throughput-nongbr.log"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwnFilename),
                   MakeStringChecker ());
  return tid;
}

void
BackhaulStatsCalculator::NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  m_connections.push_back (cInfo);
  SwitchPair_t key = cInfo->GetSwitchIndexPair ();

  *m_bwbWrapper->GetStream ()
  << right << setw (10) << key.first  << "-"
  << left  << setw (10) << key.second << "   ";
  
  *m_bwgWrapper->GetStream ()
  << right << setw (10) << key.first  << "-"
  << left  << setw (10) << key.second << "   ";

  *m_bwnWrapper->GetStream ()
  << right << setw (10) << key.first  << "-"
  << left  << setw (10) << key.second << "   ";

  *m_regWrapper->GetStream ()
  << left
  << right << setw (6) << key.first  << "-"
  << left  << setw (6) << key.second << "   ";

  *m_renWrapper->GetStream ()
  << left
  << right << setw (6) << key.first  << "-"
  << left  << setw (6) << key.second << "   ";
}

void
BackhaulStatsCalculator::NotifyTopologyBuilt (
    OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  *m_bwbWrapper->GetStream () << left << std::endl;
  *m_bwgWrapper->GetStream () << left << std::endl;
  *m_bwnWrapper->GetStream () << left << std::endl;
  *m_regWrapper->GetStream () << left << std::endl;
  *m_renWrapper->GetStream () << left << std::endl;
}

void
BackhaulStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_bwbWrapper = 0;
  m_bwgWrapper = 0;
  m_bwnWrapper = 0;
  m_regWrapper = 0;
  m_renWrapper = 0;
  m_connections.clear ();
}

void
BackhaulStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("RegStatsFilename", StringValue (prefix + m_regFilename));
  SetAttribute ("RenStatsFilename", StringValue (prefix + m_renFilename));
  SetAttribute ("BwbStatsFilename", StringValue (prefix + m_bwbFilename));
  SetAttribute ("BwgStatsFilename", StringValue (prefix + m_bwgFilename));
  SetAttribute ("BwnStatsFilename", StringValue (prefix + m_bwnFilename));

  m_bwbWrapper = Create<OutputStreamWrapper> (m_bwbFilename, std::ios::out);
  *m_bwbWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwgWrapper = Create<OutputStreamWrapper> (m_bwgFilename, std::ios::out);
  *m_bwgWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwnWrapper = Create<OutputStreamWrapper> (m_bwnFilename, std::ios::out);
  *m_bwnWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_regWrapper = Create<OutputStreamWrapper> (m_regFilename, std::ios::out);
  *m_regWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_renWrapper = Create<OutputStreamWrapper> (m_renFilename, std::ios::out);
  *m_renWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time firstDump = timeValue.Get ();
  Simulator::Schedule (firstDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, firstDump);

  Object::NotifyConstructionCompleted ();
}
void
BackhaulStatsCalculator::DumpStatistics (Time nextDump)
{
  NS_LOG_FUNCTION (this);

  *m_bwbWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_bwgWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_bwnWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_regWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  *m_renWrapper->GetStream ()
  << left << setw (12)
  << Simulator::Now ().GetSeconds ();

  double interval = (Simulator::Now () - m_lastResetTime).GetSeconds ();
  ConnInfoList_t::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      Ptr<ConnectionInfo> cInfo = *it;
      uint64_t gbrFwdBytes = cInfo->GetGbrBytes (ConnectionInfo::FWD);
      uint64_t gbrBwdBytes = cInfo->GetGbrBytes (ConnectionInfo::BWD);
      uint64_t nonFwdBytes = cInfo->GetNonGbrBytes (ConnectionInfo::FWD);
      uint64_t nonBwdBytes = cInfo->GetNonGbrBytes (ConnectionInfo::BWD);

      double gbrFwdKbits = static_cast<double> (gbrFwdBytes * 8) / 1000;
      double gbrBwdKbits = static_cast<double> (gbrBwdBytes * 8) / 1000;
      double nonFwdKbits = static_cast<double> (nonFwdBytes * 8) / 1000;
      double nonBwdKbits = static_cast<double> (nonBwdBytes * 8) / 1000;

      *m_bwgWrapper->GetStream ()
      << right
      << setw (10) << gbrFwdKbits / interval << " "
      << setw (10) << gbrBwdKbits / interval << "   ";

      *m_bwnWrapper->GetStream ()
      << right
      << setw (10) << nonFwdKbits / interval << " "
      << setw (10) << nonBwdKbits / interval << "   ";

      *m_bwbWrapper->GetStream ()
      << right
      << setw (10) << (gbrFwdKbits + nonFwdKbits) / interval << " "
      << setw (10) << (gbrBwdKbits + nonBwdKbits) / interval << "   ";

      *m_regWrapper->GetStream ()
      << right
      << setw (6) << cInfo->GetGbrLinkRatio (ConnectionInfo::FWD) << " "
      << setw (6) << cInfo->GetGbrLinkRatio (ConnectionInfo::BWD) << "   ";

      *m_renWrapper->GetStream ()
      << right
      << setw (6) << cInfo->GetNonGbrLinkRatio (ConnectionInfo::FWD) << " "
      << setw (6) << cInfo->GetNonGbrLinkRatio (ConnectionInfo::BWD) << "   ";
    }

  *m_bwbWrapper->GetStream () << std::endl;
  *m_bwgWrapper->GetStream () << std::endl;
  *m_bwnWrapper->GetStream () << std::endl;
  *m_regWrapper->GetStream () << std::endl;
  *m_renWrapper->GetStream () << std::endl;

  ResetCounters ();
  Simulator::Schedule (nextDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
BackhaulStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);
  
  m_lastResetTime = Simulator::Now ();
  ConnInfoList_t::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      (*it)->ResetTxBytes ();
    }
}


// ------------------------------------------------------------------------ //
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
  << setw (7)  << "SwIdx"
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
  << setw (7)  << "SwIdx"
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
      << setw (6)  << rInfo->GetEnbSwIdx ()                           << " "
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
      << setw (6)  << rInfo->GetEnbSwIdx ()                           << " "
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
  << setw (6)  << rInfo->GetEnbSwIdx ()                               << " "
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
  << setw (6)  << rInfo->GetEnbSwIdx ()                               << " "
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
  Ptr<QosStatsCalculator> qosStats = 0;
  TeidQosMap_t::iterator it = m_qosStats.find (teid);
  if (it != m_qosStats.end ())
    {
      QosStatsPair_t value = it->second;
      qosStats = isDown ? value.first : value.second;
    }
  else
    {
      QosStatsPair_t pair (
        Create<QosStatsCalculator> (), Create<QosStatsCalculator> ());
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
