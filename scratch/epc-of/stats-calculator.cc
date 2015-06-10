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

#include "stats-calculator.h"
#include "seq-num-tag.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

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
    m_gbrBlocked (0),
    m_lastResetTime (Simulator::Now ())
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
    .AddTraceSource ("BrqStats",
                     "LTE EPC Bearer request trace source.",
                     MakeTraceSourceAccessor (&AdmissionStatsCalculator::m_brqTrace),
                     "ns3::AdmissionStatsCalculator::BrqTracedCallback")
    ;
  return tid;
}

void
AdmissionStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void 
AdmissionStatsCalculator::NotifyRequest (bool accepted, 
                                         Ptr<const RoutingInfo> rInfo)
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
  DataRate downRate, upRate;
  std::string path = "Shortest paths";
  
  Ptr<const ReserveInfo> reserveInfo = rInfo->GetObject<ReserveInfo> ();
  if (reserveInfo)
    {
      downRate = reserveInfo->GetDownDataRate ();
      upRate = reserveInfo->GetUpDataRate ();
    }

  // FIXME: This path description should be generic, for any topology.
  // FIXME: No traffic description by now.
  Ptr<const RingRoutingInfo> ringInfo = rInfo->GetObject<RingRoutingInfo> ();
  if (ringInfo)
    {
      if (ringInfo->IsDownInv () && ringInfo->IsUpInv ())
        {
          path = "Inverted paths";
        }
      else if (ringInfo->IsDownInv () || ringInfo->IsUpInv ())
        {
          path = ringInfo->IsDownInv () ? "Inverted down path" : "Inverted up path";
        }
    }

  m_brqTrace ("", rInfo->GetTeid (), accepted, downRate, upRate, path);
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
  m_lastResetTime = Simulator::Now ();
}

Time      
AdmissionStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrRequests (void) const
{
  return m_nonRequests;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrAccepted (void) const
{
  return m_nonAccepted;
}

uint32_t
AdmissionStatsCalculator::GetNonGbrBlocked (void) const
{
  return m_nonBlocked;
}

double
AdmissionStatsCalculator::GetNonGbrBlockRatio (void) const
{
  uint32_t req = GetNonGbrRequests ();
  return req ? (double)GetNonGbrBlocked () / req : 0; 
}

uint32_t
AdmissionStatsCalculator::GetGbrRequests (void) const
{
  return m_gbrRequests;
}

uint32_t
AdmissionStatsCalculator::GetGbrAccepted (void) const
{
  return m_gbrAccepted;
}

uint32_t
AdmissionStatsCalculator::GetGbrBlocked (void) const
{
  return m_gbrBlocked;
}

double
AdmissionStatsCalculator::GetGbrBlockRatio (void) const
{
  uint32_t req = GetGbrRequests ();
  return req ? (double)GetGbrBlocked () / req : 0; 
}

uint32_t
AdmissionStatsCalculator::GetTotalRequests (void) const
{
  return GetNonGbrRequests () + GetGbrRequests ();
}

uint32_t
AdmissionStatsCalculator::GetTotalAccepted (void) const
{
  return GetNonGbrAccepted () + GetGbrAccepted ();
}

uint32_t
AdmissionStatsCalculator::GetTotalBlocked (void) const
{
  return GetNonGbrBlocked () + GetGbrBlocked ();
}


// ------------------------------------------------------------------------ //
GatewayStatsCalculator::GatewayStatsCalculator ()
  : m_pgwDownBytes (0),
    m_pgwUpBytes (0),
    m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);

  // Connecting all gateway trace sinks for traffic bandwidth monitoring
  Config::Connect (
    "/Names/SgwPgwApplication/S1uRx",
    MakeCallback (&GatewayStatsCalculator::NotifyTraffic, this));
  Config::Connect (
    "/Names/SgwPgwApplication/S1uTx",
    MakeCallback (&GatewayStatsCalculator::NotifyTraffic, this));
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
  ;
  return tid;
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

void
GatewayStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
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
  m_lastResetTime = Simulator::Now ();
}


// ------------------------------------------------------------------------ //
BandwidthStatsCalculator::BandwidthStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
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
  ;
  return tid;
}

Time      
BandwidthStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

void
BandwidthStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
BandwidthStatsCalculator::ResetCounters ()
{
  m_lastResetTime = Simulator::Now ();
}


// ------------------------------------------------------------------------ //
SwitchRulesStatsCalculator::SwitchRulesStatsCalculator ()
  : m_lastResetTime (Simulator::Now ())
{
  NS_LOG_FUNCTION (this);
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
  ;
  return tid;
}

Time      
SwitchRulesStatsCalculator::GetActiveTime (void) const
{
  return Simulator::Now () - m_lastResetTime;
}

void
SwitchRulesStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
SwitchRulesStatsCalculator::ResetCounters ()
{
  m_lastResetTime = Simulator::Now ();
}


// ------------------------------------------------------------------------ //
WebQueueStatsCalculator::WebQueueStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  m_downQueue = Names::Find<Queue> ("/Names/InternetNetwork/DownQueue");
  m_upQueue   = Names::Find<Queue> ("/Names/InternetNetwork/UpQueue");
  NS_ASSERT_MSG (m_downQueue && m_upQueue, "Web network queues not found.");
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
  ;
  return tid;
}

Ptr<const Queue>
WebQueueStatsCalculator::GetDownlinkQueue (void) const
{
  return m_downQueue;
}

Ptr<const Queue>
WebQueueStatsCalculator::GetUplinkQueue (void) const
{
  return m_upQueue;
}

void
WebQueueStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_downQueue = 0;
  m_upQueue = 0;
}

void
WebQueueStatsCalculator::ResetCounters ()
{
  m_downQueue->ResetStatistics ();
  m_upQueue->ResetStatistics ();
}


// ------------------------------------------------------------------------ //
EpcS1uStatsCalculator::EpcS1uStatsCalculator ()
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
    "/NodeList/*/$ns3::TrafficManager/AppStart",
    MakeCallback (&EpcS1uStatsCalculator::ResetEpcStatistics, this));
  Config::Connect (
    "/NodeList/*/$ns3::TrafficManager/AppStop",
    MakeCallback (&EpcS1uStatsCalculator::DumpEpcStatistics, this));
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
    .AddTraceSource ("EpcStats",
                     "OpenFlow EPC QoS trace source.",
                     MakeTraceSourceAccessor (&EpcS1uStatsCalculator::m_epcTrace),
                     "ns3::EpcS1uStatsCalculator::EpcTracedCallback")
  ;
  return tid;
}

void
EpcS1uStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
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
EpcS1uStatsCalculator::DumpEpcStatistics (std::string context, 
                                          Ptr<const EpcApplication> app)
{
  NS_LOG_FUNCTION (this << context << app);

  uint32_t teid = app->GetTeid ();
  bool uplink = (app->GetInstanceTypeId () == VoipClient::GetTypeId ());
  std::string desc = app->GetDescription ();

  Ptr<const QosStatsCalculator> epcStats;
  if (uplink)
    {
      // Dump uplink statistics
      epcStats = GetQosStatsFromTeid (teid, false);
      m_epcTrace (desc + "ul", teid, epcStats);
    }
  // Dump downlink statistics
  epcStats = GetQosStatsFromTeid (teid, true);
  m_epcTrace (desc + "dl", teid, epcStats);
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
