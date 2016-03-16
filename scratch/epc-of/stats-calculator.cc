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
#include "seq-num-tag.h"
#include "routing-info.h"
#include "connection-info.h"
#include "openflow-epc-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (ControllerStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (LinkQueuesStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (NetworkStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (EpcS1uStatsCalculator);

ControllerStatsCalculator::ControllerStatsCalculator ()
  : m_nonRequests (0),
    m_nonAccepted (0),
    m_nonBlocked (0),
    m_gbrRequests (0),
    m_gbrAccepted (0),
    m_gbrBlocked (0)
{
  NS_LOG_FUNCTION (this);
}

ControllerStatsCalculator::~ControllerStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ControllerStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ControllerStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<ControllerStatsCalculator> ()
    .AddAttribute ("AdmStatsFilename",
                   "Filename for bearer admission control statistics.",
                   StringValue ("adm_stats.txt"),
                   MakeStringAccessor (
                     &ControllerStatsCalculator::m_admStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("brq_stats.txt"),
                   MakeStringAccessor (
                     &ControllerStatsCalculator::m_brqStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
ControllerStatsCalculator::DumpStatistics (Time next)
{
  NS_LOG_FUNCTION (this);

  *m_admWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds () << " "
  << right
  << setw (9)  << m_gbrRequests                   << " "
  << setw (9)  << m_gbrBlocked                    << " "
  << setw (9)  << GetGbrBlockRatio ()             << " "
  << setw (9)  << m_nonRequests                   << " "
  << setw (9)  << m_nonBlocked                    << " "
  << setw (9)  << GetNonGbrBlockRatio ()
  << std::endl;

  ResetCounters ();
  Simulator::Schedule (next,
    &ControllerStatsCalculator::DumpStatistics, this, next);
}

void
ControllerStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_admWrapper = 0;
  m_brqWrapper = 0;
}

void
ControllerStatsCalculator::NotifyBearerRequest (bool accepted,
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
  << setw (9) << Simulator::Now ().GetSeconds ()            << " "
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
ControllerStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("AdmStatsFilename", StringValue (prefix + m_admStatsFilename));
  SetAttribute ("BrqStatsFilename", StringValue (prefix + m_brqStatsFilename));

  // Opening output files and printing header lines
  m_admWrapper = Create<OutputStreamWrapper> (m_admStatsFilename,
                                              std::ios::out);
  *m_admWrapper->GetStream ()
  << fixed << setprecision (4)
  << left
  << setw (11) << "Time(s)"
  << right
  << setw (10)  << "GBRReqs"
  << setw (10)  << "GBRBlocks"
  << setw (10)  << "GBRRatio"
  << setw (10)  << "NonReqs"
  << setw (10)  << "NonBlocks"
  << setw (10)  << "NonRatio"
  << std::endl;

  m_brqWrapper = Create<OutputStreamWrapper> (m_brqStatsFilename,
                                              std::ios::out);
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
  << left << "  "
  << setw (12) << "RoutingPath"
  << std::endl;

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time next = timeValue.Get ();
  Simulator::Schedule (next,
    &ControllerStatsCalculator::DumpStatistics, this, next);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
ControllerStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);
  m_nonRequests = 0;
  m_nonAccepted = 0;
  m_nonBlocked = 0;
  m_gbrRequests = 0;
  m_gbrAccepted = 0;
  m_gbrBlocked = 0;
}

double
ControllerStatsCalculator::GetNonGbrBlockRatio (void) const
{
  return m_nonRequests ?
         static_cast<double> (m_nonBlocked) / m_nonRequests
         : 0;
}

double
ControllerStatsCalculator::GetGbrBlockRatio (void) const
{
  return m_gbrRequests ?
         static_cast<double> (m_gbrBlocked) / m_gbrRequests
         : 0;
}


// ------------------------------------------------------------------------ //
NetworkStatsCalculator::NetworkStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
}

NetworkStatsCalculator::~NetworkStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
NetworkStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<NetworkStatsCalculator> ()
    .AddAttribute ("RegStatsFilename",
                   "Filename for GBR reservation statistics.",
                   StringValue ("reg_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_regStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("RenStatsFilename",
                   "Filename for Non-GBR allowed bandwidth statistics.",
                   StringValue ("ren_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_renStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwbStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("bwb_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_bwbStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwgStatsFilename",
                   "Filename for GBR bandwidth statistics.",
                   StringValue ("bwg_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_bwgStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwnStatsFilename",
                   "Filename for Non-GBR bandwidth statistics.",
                   StringValue ("bwn_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_bwnStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("SwtStatsFilename",
                   "FilName for flow table entries statistics.",
                   StringValue ("swt_stats.txt"),
                   MakeStringAccessor (
                     &NetworkStatsCalculator::m_swtStatsFilename),
                   MakeStringChecker ())  ;
  return tid;
}

void
NetworkStatsCalculator::DumpStatistics (Time next)
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

  *m_swtWrapper->GetStream ()
  << left << setw (11)
  << Simulator::Now ().GetSeconds ()
  << " " << right;

  double interval = GetActiveTime ().GetSeconds ();
  std::vector<Ptr<ConnectionInfo> >::iterator it;
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

      cInfo->ResetTxBytes ();
    }

  Ptr<OFSwitch13NetDevice> dev;
  NetDeviceContainer::Iterator itDev;
  for (itDev = m_devices.Begin (); itDev < m_devices.End (); itDev++)
    {
      dev = DynamicCast<OFSwitch13NetDevice> (*itDev);
      *m_swtWrapper->GetStream ()
      << setw (6)
      << dev->GetNumberFlowEntries (1) << " ";
    }

  *m_bwbWrapper->GetStream () << std::endl;
  *m_bwgWrapper->GetStream () << std::endl;
  *m_bwnWrapper->GetStream () << std::endl;
  *m_regWrapper->GetStream () << std::endl;
  *m_renWrapper->GetStream () << std::endl;
  *m_swtWrapper->GetStream () << std::endl;

  ResetCounters ();
  Simulator::Schedule (next,
    &NetworkStatsCalculator::DumpStatistics, this, next);
}

void
NetworkStatsCalculator::NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  // Save this connection info for further usage
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
NetworkStatsCalculator::NotifyTopologyBuilt (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  m_devices = devices;
  for (uint16_t i = 0; i < m_devices.GetN (); i++)
    {
      *m_swtWrapper->GetStream () << right << setw (7) << i;
    }

  *m_bwbWrapper->GetStream () << left << std::endl;
  *m_bwgWrapper->GetStream () << left << std::endl;
  *m_bwnWrapper->GetStream () << left << std::endl;
  *m_regWrapper->GetStream () << left << std::endl;
  *m_renWrapper->GetStream () << left << std::endl;
  *m_swtWrapper->GetStream () << right << std::endl;
}

void
NetworkStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_bwbWrapper = 0;
  m_bwgWrapper = 0;
  m_bwnWrapper = 0;
  m_regWrapper = 0;
  m_renWrapper = 0;
  m_swtWrapper = 0;
  m_connections.clear ();
}

void
NetworkStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("RegStatsFilename", StringValue (prefix + m_regStatsFilename));
  SetAttribute ("RenStatsFilename", StringValue (prefix + m_renStatsFilename));
  SetAttribute ("BwbStatsFilename", StringValue (prefix + m_bwbStatsFilename));
  SetAttribute ("BwgStatsFilename", StringValue (prefix + m_bwgStatsFilename));
  SetAttribute ("BwnStatsFilename", StringValue (prefix + m_bwnStatsFilename));
  SetAttribute ("SwtStatsFilename", StringValue (prefix + m_swtStatsFilename));

  // Opening output files and printing header lines
  m_bwbWrapper = Create<OutputStreamWrapper> (m_bwbStatsFilename,
                                              std::ios::out);
  *m_bwbWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwgWrapper = Create<OutputStreamWrapper> (m_bwgStatsFilename,
                                              std::ios::out);
  *m_bwgWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_bwnWrapper = Create<OutputStreamWrapper> (m_bwnStatsFilename,
                                              std::ios::out);
  *m_bwnWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_regWrapper = Create<OutputStreamWrapper> (m_regStatsFilename,
                                              std::ios::out);
  *m_regWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_renWrapper = Create<OutputStreamWrapper> (m_renStatsFilename,
                                              std::ios::out);
  *m_renWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  m_swtWrapper = Create<OutputStreamWrapper> (m_swtStatsFilename,
                                              std::ios::out);
  *m_swtWrapper->GetStream ()
  << left << fixed << setprecision (4)
  << setw (12) << "Time(s)";

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time next = timeValue.Get ();
  Simulator::Schedule (next,
    &NetworkStatsCalculator::DumpStatistics, this, next);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
NetworkStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);
  m_lastResetTime = Simulator::Now ();
}

Time
NetworkStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}


// ------------------------------------------------------------------------ //
LinkQueuesStatsCalculator::LinkQueuesStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
}

LinkQueuesStatsCalculator::~LinkQueuesStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LinkQueuesStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkQueuesStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<LinkQueuesStatsCalculator> ()
    .AddAttribute ("LnkStatsFilename",
                   "Filename for link queue traffic statistics.",
                   StringValue ("lnk_stats.txt"),
                   MakeStringAccessor (
                     &LinkQueuesStatsCalculator::m_lnkStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
LinkQueuesStatsCalculator::SetQueues (Ptr<Queue> downQueue, Ptr<Queue> upQueue)
{
  NS_LOG_FUNCTION (this << downQueue << upQueue);

  m_downQueue = downQueue;
  m_upQueue = upQueue;
  ResetCounters ();
}

void
LinkQueuesStatsCalculator::DumpStatistics (Time next)
{
  NS_LOG_FUNCTION (this);

  *m_lnkWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()                 << " "
  << right
  << setw (11) << m_downQueue->GetTotalReceivedPackets ()         << " "
  << setw (11) << m_downQueue->GetTotalReceivedBytes ()           << " "
  << setw (11) << m_downQueue->GetTotalDroppedPackets ()          << " "
  << setw (11) << m_downQueue->GetTotalDroppedBytes ()            << " "
  << setw (11) << m_upQueue->GetTotalReceivedPackets ()           << " "
  << setw (11) << m_upQueue->GetTotalReceivedBytes ()             << " "
  << setw (11) << m_upQueue->GetTotalDroppedPackets ()            << " "
  << setw (11) << m_upQueue->GetTotalDroppedBytes ()              << " "
  << setw (15) << static_cast<double> (GetDownBitRate ()) / 1000  << " "
  << setw (15) << static_cast<double> (GetUpBitRate ()) / 1000
  << std::endl;

  ResetCounters ();
  Simulator::Schedule (next,
    &LinkQueuesStatsCalculator::DumpStatistics, this, next);
}

void
LinkQueuesStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downQueue = 0;
  m_upQueue = 0;
  m_lnkWrapper = 0;
}

void
LinkQueuesStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("LnkStatsFilename", StringValue (prefix + m_lnkStatsFilename));

  // Opening output files and printing header lines
  m_lnkWrapper = Create<OutputStreamWrapper> (m_lnkStatsFilename,
                                              std::ios::out);
  *m_lnkWrapper->GetStream ()
  << fixed << setprecision (4) << boolalpha
  << left
  << setw (11) << "Time(s)"
  << right
  << setw (12) << "DlPkts"
  << setw (12) << "DlBytes"
  << setw (12) << "DlPktsDrp"
  << setw (12) << "DlBytesDrp"
  << setw (12) << "UlPkts"
  << setw (12) << "UlBytes"
  << setw (12) << "UlPktsDrp"
  << setw (12) << "UlBytesDrp"
  << setw (16) << "Downlink(kbps)"
  << setw (16) << "Uplink(kbps)"
  << std::endl;

  TimeValue timeValue;
  GlobalValue::GetValueByName ("DumpStatsTimeout", timeValue);
  Time next = timeValue.Get ();
  Simulator::Schedule (next,
    &LinkQueuesStatsCalculator::DumpStatistics, this, next);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
LinkQueuesStatsCalculator::ResetCounters ()
{
  NS_LOG_FUNCTION (this);
  m_downQueue->ResetStatistics ();
  m_upQueue->ResetStatistics ();
  m_lastResetTime = Simulator::Now ();
}

Time
LinkQueuesStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

uint64_t
LinkQueuesStatsCalculator::GetDownBitRate (void) const
{
  return static_cast<uint64_t> (8 * m_downQueue->GetTotalReceivedBytes () /
                                GetActiveTime ().GetSeconds ());
}

uint64_t
LinkQueuesStatsCalculator::GetUpBitRate (void) const
{
  return static_cast<uint64_t> (8 * m_upQueue->GetTotalReceivedBytes () /
                                GetActiveTime ().GetSeconds ());
}


// ------------------------------------------------------------------------ //
EpcS1uStatsCalculator::EpcS1uStatsCalculator (
    Ptr<OpenFlowEpcController> controller)
  : m_controller (controller)
{
  NS_LOG_FUNCTION (this);

  // Connecting all EPC trace sinks for QoS monitoring
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uRx",
    MakeCallback (&EpcS1uStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uTx",
    MakeCallback (&EpcS1uStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uRx",
    MakeCallback (&EpcS1uStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uTx",
    MakeCallback (&EpcS1uStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/$ns3::OFSwitch13NetDevice/MeterDrop",
    MakeCallback (&EpcS1uStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/DeviceList/*/$ns3::OFSwitch13NetDevice/PortList/*/PortQueue/Drop",
    MakeCallback (&EpcS1uStatsCalculator::QueueDropPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcApplication/AppStart",
    MakeCallback (&EpcS1uStatsCalculator::ResetEpcStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcApplication/AppStop",
    MakeCallback (&EpcS1uStatsCalculator::DumpStatistics, this));
}

EpcS1uStatsCalculator::EpcS1uStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

EpcS1uStatsCalculator::~EpcS1uStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcS1uStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcS1uStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<EpcS1uStatsCalculator> ()
    .AddAttribute ("AppStatsFilename",
                   "Filename for application QoS statistics.",
                   StringValue ("app_stats.txt"),
                   MakeStringAccessor (
                     &EpcS1uStatsCalculator::m_appStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC QoS S1U statistics.",
                   StringValue ("epc_stats.txt"),
                   MakeStringAccessor (
                     &EpcS1uStatsCalculator::m_epcStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
EpcS1uStatsCalculator::SetController (Ptr<OpenFlowEpcController> controller)
{
  m_controller = controller;
}

void
EpcS1uStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_controller = 0;
  m_appWrapper = 0;
  m_epcWrapper = 0;
}

void
EpcS1uStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("AppStatsFilename", StringValue (prefix + m_appStatsFilename));
  SetAttribute ("EpcStatsFilename", StringValue (prefix + m_epcStatsFilename));

  // Opening output files and printing header lines
  m_appWrapper = Create<OutputStreamWrapper> (m_appStatsFilename,
                                              std::ios::out);
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
  << setw (17)  << "Throughput(kbps)"
  << std::endl;

  m_epcWrapper = Create<OutputStreamWrapper> (m_epcStatsFilename,
                                              std::ios::out);
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
  << setw (17)  << "Throughput(kbps)"
  << std::endl;

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
EpcS1uStatsCalculator::MeterDropPacket (std::string context,
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
EpcS1uStatsCalculator::QueueDropPacket (std::string context,
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
EpcS1uStatsCalculator::EpcInputPacket (std::string context,
                                       Ptr<const Packet> packet)
{
  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<QosStatsCalculator> qosStats =
        GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      SeqNumTag seqTag (qosStats->GetNextSeqNum ());
      packet->AddPacketTag (seqTag);
    }
}

void
EpcS1uStatsCalculator::EpcOutputPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SeqNumTag seqTag;
      if (packet->PeekPacketTag (seqTag))
        {
          Ptr<QosStatsCalculator> qosStats =
            GetQosStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
          qosStats->NotifyReceived (seqTag.GetSeqNum (),
                                    gtpuTag.GetTimestamp (),
                                    packet->GetSize ());
        }
    }
}

void
EpcS1uStatsCalculator::DumpStatistics (std::string context,
                                       Ptr<const EpcApplication> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeid ());
  NS_ASSERT_MSG (m_controller, "Invalid controller application.");

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = m_controller->GetConstRoutingInfo (teid);
  Ptr<const QosStatsCalculator> epcStats;
  Ptr<const QosStatsCalculator> appStats;

  bool uplink = (app->GetInstanceTypeId () == VoipClient::GetTypeId ());
  if (uplink)
    {
      // Dump uplink statistics
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

      appStats = DynamicCast<const VoipClient> (app)->GetServerQosStats ();
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

  // Dump downlink statistics
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
EpcS1uStatsCalculator::ResetEpcStatistics (std::string context,
                                           Ptr<const EpcApplication> app)
{
  NS_LOG_FUNCTION (this << context << app);

  uint32_t teid = app->GetTeid ();

  GetQosStatsFromTeid (teid, true)->ResetCounters ();
  GetQosStatsFromTeid (teid, false)->ResetCounters ();
}

Ptr<QosStatsCalculator>
EpcS1uStatsCalculator::GetQosStatsFromTeid (uint32_t teid, bool isDown)
{
  Ptr<QosStatsCalculator> qosStats = 0;
  TeidQosMap_t::iterator it;
  it = m_qosStats.find (teid);
  if (it != m_qosStats.end ())
    {
      QosStatsPair_t value = it->second;
      qosStats = isDown ? value.first : value.second;
    }
  else
    {
      // Create and insert the structure
      QosStatsPair_t pair (Create<QosStatsCalculator> (),
                           Create<QosStatsCalculator> ());
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
