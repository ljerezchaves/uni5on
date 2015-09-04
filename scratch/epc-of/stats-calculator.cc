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

#include "ns3/qos-stats-calculator.h"
#include "stats-calculator.h"
#include "seq-num-tag.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <iomanip>
#include <iostream>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (AdmissionStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (GatewayStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (BandwidthStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (SwitchRulesStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (WebQueueStatsCalculator);
NS_OBJECT_ENSURE_REGISTERED (EpcS1uStatsCalculator);

AdmissionStatsCalculator::AdmissionStatsCalculator ()
  : m_nonRequests (0),
    m_nonAccepted (0),
    m_nonBlocked (0),
    m_gbrRequests (0),
    m_gbrAccepted (0),
    m_gbrBlocked (0)
{
  NS_LOG_FUNCTION (this);

  // Connecting to OpenFlowEpcController BearerRequest trace source
  Config::ConnectWithoutContext (
    "/Names/MainController/BearerRequest",
    MakeCallback (&AdmissionStatsCalculator::NotifyRequest, this));
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
                   "Filename for bearer admission control statistics.",
                   StringValue ("adm_stats.txt"),
                   MakeStringAccessor (&AdmissionStatsCalculator::m_admStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("brq_stats.txt"),
                   MakeStringAccessor (&AdmissionStatsCalculator::m_brqStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
AdmissionStatsCalculator::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  *admWrapper->GetStream ()
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
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  admWrapper = 0;
  brqWrapper = 0;
}

void
AdmissionStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  admWrapper = Create<OutputStreamWrapper> (m_admStatsFilename, std::ios::out);
  *admWrapper->GetStream () << fixed << setprecision (4)
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

  brqWrapper = Create<OutputStreamWrapper> (m_brqStatsFilename, std::ios::out);
  *brqWrapper->GetStream () << boolalpha << fixed << setprecision (4)
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
}

void
AdmissionStatsCalculator::NotifyRequest (bool accepted, Ptr<const RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << accepted << rInfo);

  // Update internal counters
  if (rInfo->IsGbr ())
    {
      m_gbrRequests++;
      if (accepted)
        {
          m_gbrAccepted++;
        }
      else
        {
          m_gbrBlocked++;
        }
    }
  else
    {
      m_nonRequests++;
      if (accepted)
        {
          m_nonAccepted++;
        }
      else
        {
          m_nonBlocked++;
        }
    }

  // Preparing bearer request stats for trace source
  uint64_t downBitRate = 0, upBitRate = 0;
  Ptr<const ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (reserveInfo)
    {
      downBitRate = reserveInfo->GetDownBitRate ();
      upBitRate = reserveInfo->GetUpBitRate ();
    }

  std::string path = "Shortest (default)";
  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo)
    {
      path = accepted ? ringInfo->GetPathDesc () : "None";
    }

  // Save request stats into output file
  *brqWrapper->GetStream ()
    << left
    << setw (9) << Simulator::Now ().GetSeconds ()          << " "
    << right
    << setw (4)  << rInfo->GetQciInfo ()                    << " " 
    << setw (6)  << rInfo->IsGbr ()                         << " " 
    << setw (7)  << rInfo->GetImsi ()                       << " " 
    << setw (7)  << rInfo->GetCellId ()                     << " " 
    << setw (6)  << rInfo->GetEnbSwIdx ()                   << " " 
    << setw (6)  << rInfo->GetTeid ()                       << " "
    << setw (9)  << accepted                                << " "
    << setw (11) << (double)(downBitRate) / 1000            << " "
    << setw (11) << (double)(upBitRate) / 1000              << "  "
    << left
    << setw (15) << path
    << std::endl;
}

void
AdmissionStatsCalculator::ResetCounters ()
{
  m_nonRequests = 0;
  m_nonAccepted = 0;
  m_nonBlocked = 0;
  m_gbrRequests = 0;
  m_gbrAccepted = 0;
  m_gbrBlocked = 0;
}

double
AdmissionStatsCalculator::GetNonGbrBlockRatio (void) const
{
  return m_nonRequests ? (double)m_nonBlocked / m_nonRequests : 0;
}

double
AdmissionStatsCalculator::GetGbrBlockRatio (void) const
{
  return m_gbrRequests ? (double)m_gbrBlocked / m_gbrRequests : 0;
}


// ------------------------------------------------------------------------ //
GatewayStatsCalculator::GatewayStatsCalculator ()
  : m_pgwDownBytes (0),
    m_pgwUpBytes (0),
    m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);

  m_downQueue = Names::Find<Queue> ("/Names/OpenFlowNetwork/PgwDownQueue");
  m_upQueue   = Names::Find<Queue> ("/Names/OpenFlowNetwork/PgwUpQueue");
  NS_ASSERT_MSG (m_downQueue && m_upQueue, "Pgw network queues not found.");

  // Connecting all gateway trace sinks for traffic bandwidth monitoring
  Config::Connect (
    "/Names/SgwPgwApplication/S1uRx",
    MakeCallback (&GatewayStatsCalculator::NotifyTraffic, this));
  Config::Connect (
    "/Names/SgwPgwApplication/S1uTx",
    MakeCallback (&GatewayStatsCalculator::NotifyTraffic, this));

  ResetCounters ();
}

GatewayStatsCalculator::~GatewayStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
GatewayStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GatewayStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<GatewayStatsCalculator> ()
    .AddAttribute ("PgwStatsFilename",
                   "Filename for packet gateway traffic statistics.",
                   StringValue ("pgw_stats.txt"),
                   MakeStringAccessor (&GatewayStatsCalculator::m_pgwStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
GatewayStatsCalculator::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  *pgwWrapper->GetStream () 
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
    << setw (15) << (double)GetDownDataRate ().GetBitRate () / 1000 << " "
    << setw (15) << (double)GetUpDataRate ().GetBitRate () / 1000
    << std::endl;

  ResetCounters ();
}

void
GatewayStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downQueue = 0;
  m_upQueue = 0;
  pgwWrapper = 0;
}

void
GatewayStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  pgwWrapper = Create<OutputStreamWrapper> (m_pgwStatsFilename, std::ios::out);
  *pgwWrapper->GetStream () << fixed << setprecision (4) << boolalpha
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
}

void
GatewayStatsCalculator::NotifyTraffic (std::string context,
                                       Ptr<const Packet> packet)
{
  std::string direction = context.substr (context.rfind ("/") + 1);
  if (direction == "S1uTx")
    {
      m_pgwDownBytes += packet->GetSize ();
    }
  else if (direction == "S1uRx")
    {
      m_pgwUpBytes += packet->GetSize ();
    }
}

void
GatewayStatsCalculator::ResetCounters ()
{
  m_pgwUpBytes = 0;
  m_pgwDownBytes = 0;
  m_downQueue->ResetStatistics ();
  m_upQueue->ResetStatistics ();
  m_lastResetTime = Simulator::Now ();
}

Time
GatewayStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

DataRate
GatewayStatsCalculator::GetDownDataRate (void) const
{
  return DataRate (8 * m_pgwDownBytes / GetActiveTime ().GetSeconds ());
}

DataRate
GatewayStatsCalculator::GetUpDataRate (void) const
{
  return DataRate (8 * m_pgwUpBytes / GetActiveTime ().GetSeconds ());
}


// ------------------------------------------------------------------------ //
BandwidthStatsCalculator::BandwidthStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);

  // Connecting this stats calculator to OpenFlowNetwork trace source, so it
  // can be aware of all connections between switches.
  Ptr<OpenFlowEpcNetwork> network =
    Names::Find<OpenFlowEpcNetwork> ("/Names/OpenFlowNetwork");
  NS_ASSERT_MSG (network, "Network object not found.");
  NS_ASSERT_MSG (!network->IsTopologyCreated (),
                 "Network topology already created.");

  network->TraceConnectWithoutContext ("TopologyBuilt",
    MakeCallback (&BandwidthStatsCalculator::NotifyTopologyBuilt, this));
  network->TraceConnectWithoutContext ("NewSwitchConnection",
    MakeCallback (&BandwidthStatsCalculator::NotifyNewSwitchConnection, this));
}

BandwidthStatsCalculator::~BandwidthStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BandwidthStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BandwidthStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<BandwidthStatsCalculator> ()
    .AddAttribute ("ResStatsFilename",
                   "Filename for network reservation statistics.",
                   StringValue ("res_stats.txt"),
                   MakeStringAccessor (&BandwidthStatsCalculator::m_resStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwbStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("bwb_stats.txt"),
                   MakeStringAccessor (&BandwidthStatsCalculator::m_bwbStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwgStatsFilename",
                   "Filename for GBR bandwidth statistics.",
                   StringValue ("bwg_stats.txt"),
                   MakeStringAccessor (&BandwidthStatsCalculator::m_bwgStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwnStatsFilename",
                   "Filename for Non-GBR bandwidth statistics.",
                   StringValue ("bwn_stats.txt"),
                   MakeStringAccessor (&BandwidthStatsCalculator::m_bwnStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
BandwidthStatsCalculator::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  *bwbWrapper->GetStream () << left
    << setw (12) << Simulator::Now ().GetSeconds ();
  *bwgWrapper->GetStream () << left
    << setw (12) << Simulator::Now ().GetSeconds ();
  *bwnWrapper->GetStream () << left
    << setw (12) << Simulator::Now ().GetSeconds ();
  *resWrapper->GetStream () << left
    << setw (12) << Simulator::Now ().GetSeconds ();
    
  double interval = GetActiveTime ().GetSeconds ();
  std::vector<Ptr<ConnectionInfo> >::iterator it;
  for (it = m_connections.begin (); it != m_connections.end (); it++)
    {
      double gbrFwKbits = ((double)(*it)->GetForwardGbrBytes ()  * 8) / 1000;
      double gbrBwKbits = ((double)(*it)->GetBackwardGbrBytes () * 8) / 1000;
      double nonFwKbits = ((double)(*it)->GetForwardNonGbrBytes ()  * 8) / 1000;
      double nonBwKbits = ((double)(*it)->GetBackwardNonGbrBytes () * 8) / 1000;
      
      *bwgWrapper->GetStream () << right 
        << setw (10) << gbrFwKbits / interval << " "
        << setw (10) << gbrBwKbits / interval << "   ";

      *bwnWrapper->GetStream () << right 
        << setw (10) << nonFwKbits / interval << " "
        << setw (10) << nonBwKbits / interval << "   ";

      *bwbWrapper->GetStream () << right 
        << setw (10) << (gbrFwKbits + nonFwKbits) / interval << " "
        << setw (10) << (gbrBwKbits + nonBwKbits) / interval << "   ";

      *resWrapper->GetStream () << right
        << setw (6) << (*it)->GetForwardGbrReservedRatio ()  << " "
        << setw (6) << (*it)->GetBackwardGbrReservedRatio () << "   ";

      (*it)->ResetStatistics ();
    }
  *bwbWrapper->GetStream () << std::endl;
  *bwgWrapper->GetStream () << std::endl;
  *bwnWrapper->GetStream () << std::endl;
  *resWrapper->GetStream () << std::endl;

  ResetCounters ();
}

void
BandwidthStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  bwbWrapper = 0;
  bwgWrapper = 0;
  bwnWrapper = 0;
  resWrapper = 0;
  m_connections.clear ();
}

void
BandwidthStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  bwbWrapper = Create<OutputStreamWrapper> (m_bwbStatsFilename, std::ios::out);
  *bwbWrapper->GetStream () << left << fixed << setprecision (4)
    << setw (12) << "Time(s)";

  bwgWrapper = Create<OutputStreamWrapper> (m_bwgStatsFilename, std::ios::out);
  *bwgWrapper->GetStream () << left << fixed << setprecision (4)
    << setw (12) << "Time(s)";

  bwnWrapper = Create<OutputStreamWrapper> (m_bwnStatsFilename, std::ios::out);
  *bwnWrapper->GetStream () << left << fixed << setprecision (4)
    << setw (12) << "Time(s)";

  resWrapper = Create<OutputStreamWrapper> (m_resStatsFilename, std::ios::out);
  *resWrapper->GetStream () << left << fixed << setprecision (4)
    << setw (12) << "Time(s)";
}

void
BandwidthStatsCalculator::NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this);

  // Save this connection info for further usage
  m_connections.push_back (cInfo);
  SwitchPair_t key = cInfo->GetSwitchIndexPair ();

  *bwbWrapper->GetStream ()
    << right << setw (10) << key.first  << "-" 
    << left  << setw (10) << key.second << "   ";

  *bwgWrapper->GetStream ()
    << right << setw (10) << key.first  << "-" 
    << left  << setw (10) << key.second << "   ";

  *bwnWrapper->GetStream ()
    << right << setw (10) << key.first  << "-" 
    << left  << setw (10) << key.second << "   ";

  *resWrapper->GetStream () << left
    << right << setw (6) << key.first  << "-" 
    << left  << setw (6) << key.second << "   ";
}

void
BandwidthStatsCalculator::NotifyTopologyBuilt (NetDeviceContainer devices)
{
  *bwbWrapper->GetStream () << left << std::endl;
  *bwgWrapper->GetStream () << left << std::endl;
  *bwnWrapper->GetStream () << left << std::endl;
  *resWrapper->GetStream () << left << std::endl;
}

void
BandwidthStatsCalculator::ResetCounters ()
{
  m_lastResetTime = Simulator::Now ();
}

Time
BandwidthStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

// ------------------------------------------------------------------------ //
SwitchRulesStatsCalculator::SwitchRulesStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connecting this stats calculator to OpenFlowNetwork trace source, so it
  // can be aware of all switches devices.
  Ptr<OpenFlowEpcNetwork> network =
    Names::Find<OpenFlowEpcNetwork> ("/Names/OpenFlowNetwork");
  NS_ASSERT_MSG (network, "Network object not found.");
  NS_ASSERT_MSG (!network->IsTopologyCreated (),
                 "Network topology already created.");

  network->TraceConnectWithoutContext ("TopologyBuilt",
    MakeCallback (&SwitchRulesStatsCalculator::NotifyTopologyBuilt, this));
}

SwitchRulesStatsCalculator::~SwitchRulesStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SwitchRulesStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchRulesStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<SwitchRulesStatsCalculator> ()
    .AddAttribute ("SwtStatsFilename",
                   "FilName for flow table entries statistics.",
                   StringValue ("swt_stats.txt"),
                   MakeStringAccessor (&SwitchRulesStatsCalculator::m_swtStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
SwitchRulesStatsCalculator::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  *swtWrapper->GetStream () 
    << left
    << setw (11) << Simulator::Now ().GetSeconds () << " " 
    << right;
  
  Ptr<OFSwitch13NetDevice> dev;
  for (uint16_t i = 0; i < m_devices.GetN (); i++)
    {
      dev = DynamicCast<OFSwitch13NetDevice> (m_devices.Get (i));
      *swtWrapper->GetStream () << setw (6) << dev->GetNumberFlowEntries (1) << " ";
    }
  *swtWrapper->GetStream () << std::endl;
}

void
SwitchRulesStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  swtWrapper = 0;
}

void
SwitchRulesStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  swtWrapper = Create<OutputStreamWrapper> (m_swtStatsFilename, std::ios::out);
}

void
SwitchRulesStatsCalculator::NotifyTopologyBuilt (NetDeviceContainer devices)
{
  m_devices = devices;
  *swtWrapper->GetStream () << fixed << setprecision (4)
    << left 
    << setw (11) << "Time(s)" 
    << right;

  for (uint16_t i = 0; i < m_devices.GetN (); i++)
    {
      *swtWrapper->GetStream () << setw (7) << i;
    }
  *swtWrapper->GetStream () << std::endl;
}


// ------------------------------------------------------------------------ //
WebQueueStatsCalculator::WebQueueStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);

  m_downQueue = Names::Find<Queue> ("/Names/InternetNetwork/DownQueue");
  m_upQueue   = Names::Find<Queue> ("/Names/InternetNetwork/UpQueue");
  NS_ASSERT_MSG (m_downQueue && m_upQueue, "Web network queues not found.");

  ResetCounters ();
}

WebQueueStatsCalculator::~WebQueueStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
WebQueueStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WebQueueStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<WebQueueStatsCalculator> ()
    .AddAttribute ("WebStatsFilename",
                   "Filename for internet queue statistics.",
                   StringValue ("web_stats.txt"),
                   MakeStringAccessor (&WebQueueStatsCalculator::m_webStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
WebQueueStatsCalculator::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  *webWrapper->GetStream () 
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
    << setw (15) << (double)GetDownDataRate ().GetBitRate () / 1000 << " "
    << setw (15) << (double)GetUpDataRate ().GetBitRate () / 1000
    << std::endl;

  ResetCounters ();
}

void
WebQueueStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downQueue = 0;
  m_upQueue = 0;
  webWrapper = 0;
}

void
WebQueueStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  webWrapper = Create<OutputStreamWrapper> (m_webStatsFilename, std::ios::out);
  *webWrapper->GetStream () << fixed << setprecision (4) << boolalpha
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
}

void
WebQueueStatsCalculator::ResetCounters ()
{
  m_downQueue->ResetStatistics ();
  m_upQueue->ResetStatistics ();
  m_lastResetTime = Simulator::Now ();
}

Time
WebQueueStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

DataRate
WebQueueStatsCalculator::GetDownDataRate (void) const
{
  return DataRate (8 * m_downQueue->GetTotalReceivedBytes () / 
                   GetActiveTime ().GetSeconds ());
}

DataRate
WebQueueStatsCalculator::GetUpDataRate (void) const
{
  return DataRate (8 * m_upQueue->GetTotalReceivedBytes () / 
                   GetActiveTime ().GetSeconds ());
}


// ------------------------------------------------------------------------ //
EpcS1uStatsCalculator::EpcS1uStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  m_controller = Names::Find<OpenFlowEpcController> ("MainController"); 

  // Connecting all EPC trace sinks for QoS monitoring
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uRx",
    MakeCallback (&EpcS1uStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uTx",
    MakeCallback (&EpcS1uStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/Names/SgwPgwApplication/S1uRx",
    MakeCallback (&EpcS1uStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/Names/SgwPgwApplication/S1uTx",
    MakeCallback (&EpcS1uStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/Names/OpenFlowNetwork/MeterDrop",
    MakeCallback (&EpcS1uStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/Names/OpenFlowNetwork/QueueDrop",
    MakeCallback (&EpcS1uStatsCalculator::QueueDropPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcApplication/AppStart",
    MakeCallback (&EpcS1uStatsCalculator::ResetEpcStatistics, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcApplication/AppStop",
    MakeCallback (&EpcS1uStatsCalculator::DumpStatistics, this));
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
                   MakeStringAccessor (&EpcS1uStatsCalculator::m_appStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC QoS S1U statistics.",
                   StringValue ("epc_stats.txt"),
                   MakeStringAccessor (&EpcS1uStatsCalculator::m_epcStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
EpcS1uStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_controller = 0;
  appWrapper = 0;
  epcWrapper = 0;
}

void
EpcS1uStatsCalculator::NotifyConstructionCompleted (void)
{
  Object::NotifyConstructionCompleted ();

  // Opening output files and printing header lines
  appWrapper = Create<OutputStreamWrapper> (m_appStatsFilename, std::ios::out);
  *appWrapper->GetStream () << fixed << setprecision (4) << boolalpha
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

  epcWrapper = Create<OutputStreamWrapper> (m_epcStatsFilename, std::ios::out);
  *epcWrapper->GetStream () << fixed << setprecision (4) << boolalpha
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
          qosStats->NotifyReceived (seqTag.GetSeqNum (), gtpuTag.GetTimestamp (),
                                    packet->GetSize ());
        }
    }
}

void
EpcS1uStatsCalculator::DumpStatistics (std::string context, Ptr<const EpcApplication> app)
{
  NS_LOG_FUNCTION (this << context << app->GetTeid ());

  uint32_t teid = app->GetTeid ();
  Ptr<const RoutingInfo> rInfo = m_controller->GetConstRoutingInfo (teid);
  Ptr<const QosStatsCalculator> epcStats;
  Ptr<const QosStatsCalculator> appStats;
  
  bool uplink = (app->GetInstanceTypeId () == VoipClient::GetTypeId ());
  if (uplink)
    {
      // Dump uplink statistics
      epcStats = GetQosStatsFromTeid (teid, false);
      *epcWrapper->GetStream () 
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
        << setw (16) << (double)(epcStats->GetRxThroughput ().GetBitRate ()) / 1000
        << std::endl;

      appStats = DynamicCast<const VoipClient> (app)->GetServerQosStats ();
      *appWrapper->GetStream () 
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
        << setw (16) << (double)(appStats->GetRxThroughput ().GetBitRate ()) / 1000
        << std::endl;
    }

  // Dump downlink statistics
  epcStats = GetQosStatsFromTeid (teid, true);
  *epcWrapper->GetStream () 
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
    << setw (16) << (double)(epcStats->GetRxThroughput ().GetBitRate ()) / 1000
    << std::endl;

  appStats = app->GetQosStats ();
  *appWrapper->GetStream () 
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
    << setw (16) << (double)(appStats->GetRxThroughput ().GetBitRate ()) / 1000
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
