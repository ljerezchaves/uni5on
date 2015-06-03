/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#include "openflow-epc-network.h"
#include "openflow-epc-controller.h"
#include "seq-num-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (ConnectionInfo);
NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcNetwork);

ConnectionInfo::ConnectionInfo ()
{
  NS_LOG_FUNCTION (this);
}

ConnectionInfo::~ConnectionInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
ConnectionInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionInfo")
    .SetParent<Object> ()
    .AddConstructor<ConnectionInfo> ()
  ;
  return tid;
}

void
ConnectionInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

DataRate
ConnectionInfo::GetAvailableDataRate ()
{
  return maxDataRate - reservedDataRate;
}

DataRate
ConnectionInfo::GetAvailableDataRate (double bwFactor)
{
  return (maxDataRate * (1. - bwFactor)) - reservedDataRate;
}

double
ConnectionInfo::GetUsageRatio (void) const
{
  return (double)reservedDataRate.GetBitRate () / maxDataRate.GetBitRate ();
}

bool
ConnectionInfo::ReserveDataRate (DataRate dr)
{
  reservedDataRate = reservedDataRate + dr;
  return (reservedDataRate <= maxDataRate);
}

bool
ConnectionInfo::ReleaseDataRate (DataRate dr)
{
  reservedDataRate = reservedDataRate - dr;
  return (reservedDataRate >= 0);
}


// ------------------------------------------------------------------------ //
OpenFlowEpcNetwork::OpenFlowEpcNetwork ()
  : m_ofCtrlApp (0),
    m_ofCtrlNode (0),
    m_pgwDownBytes (0),
    m_pgwUpBytes (0)
{
  NS_LOG_FUNCTION (this);
  m_ofHelper = CreateObjectWithAttributes<OFSwitch13Helper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));
}

OpenFlowEpcNetwork::~OpenFlowEpcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
OpenFlowEpcNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::OpenFlowEpcNetwork") 
    .SetParent<Object> ()
    .AddAttribute ("DumpStatsTimeout",
                   "Periodic statistics dump interval.",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&OpenFlowEpcNetwork::SetDumpTimeout),
                   MakeTimeChecker ())
    .AddTraceSource ("PgwStats",
                     "EPC Pgw traffic trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_pgwTrace),
                     "ns3::OpenFlowEpcNetwork::PgwTracedCallback")
    .AddTraceSource ("EpcStats",
                     "LTE EPC GTPU QoS trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_epcTrace),
                     "ns3::OpenFlowEpcNetwork::QosTracedCallback")
    .AddTraceSource ("SwtStats",
                     "The switch flow table entries trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_swtTrace),
                     "ns3::OpenFlowEpcNetwork::SwtTracedCallback")
    .AddTraceSource ("BwdStats",
                     "The network bandwidth usage trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcNetwork::m_bwdTrace),
                     "ns3::OpenFlowEpcNetwork::BwdTracedCallback")
  ;
  return tid; 
}

void
OpenFlowEpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ofCtrlApp = 0;
  m_ofCtrlNode = 0;
  m_ofHelper = 0;
  m_nodeSwitchMap.clear ();
  Object::DoDispose ();
}

void
OpenFlowEpcNetwork::EnableDataPcap (std::string prefix, bool promiscuous)
{
  m_ofCsmaHelper.EnablePcap (prefix, m_ofSwitches, promiscuous);
}

void 
OpenFlowEpcNetwork::EnableOpenFlowPcap (std::string prefix)
{
  m_ofHelper->EnableOpenFlowPcap (prefix);
}

void 
OpenFlowEpcNetwork::EnableOpenFlowAscii (std::string prefix)
{
  m_ofHelper->EnableOpenFlowAscii (prefix);
}

void 
OpenFlowEpcNetwork::EnableDatapathLogs (std::string level)
{
  m_ofHelper->EnableDatapathLogs (level);
}

CsmaHelper
OpenFlowEpcNetwork::GetCsmaHelper ()
{
  return m_ofCsmaHelper;
}

NodeContainer
OpenFlowEpcNetwork::GetSwitchNodes ()
{
  return m_ofSwitches;
}

NetDeviceContainer
OpenFlowEpcNetwork::GetSwitchDevices ()
{
  return m_ofDevices;
}

Ptr<OFSwitch13Controller>
OpenFlowEpcNetwork::GetControllerApp ()
{
  return m_ofCtrlApp;
}

Ptr<Node>
OpenFlowEpcNetwork::GetControllerNode ()
{
  return m_ofCtrlNode;
}

uint16_t 
OpenFlowEpcNetwork::GetNSwitches ()
{
  return m_ofSwitches.GetN ();
}

void
OpenFlowEpcNetwork::SetDumpTimeout (Time timeout)
{
  m_dumpTimeout = timeout;
  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpPgwStatistics, this);
  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpSwtStatistics, this);
  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpBwdStatistics, this);
}

void 
OpenFlowEpcNetwork::ConnectTraceSinks ()
{
  // EPC trace sinks for QoS monitoring
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uRx",
    MakeCallback (&OpenFlowEpcNetwork::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcEnbApplication/S1uTx",
    MakeCallback (&OpenFlowEpcNetwork::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uRx",
    MakeCallback (&OpenFlowEpcNetwork::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uTx",
    MakeCallback (&OpenFlowEpcNetwork::EpcInputPacket, this));

  // Pgw traffic trace sinks
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uRx",
    MakeCallback (&OpenFlowEpcNetwork::PgwTraffic, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcSgwPgwApplication/S1uTx",
    MakeCallback (&OpenFlowEpcNetwork::PgwTraffic, this));
}

void 
OpenFlowEpcNetwork::EpcInputPacket (std::string context, 
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
OpenFlowEpcNetwork::EpcOutputPacket (std::string context, 
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
OpenFlowEpcNetwork::PgwTraffic (std::string context, 
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
OpenFlowEpcNetwork::MeterDropPacket (std::string context, 
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
OpenFlowEpcNetwork::QueueDropPacket (std::string context,
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
OpenFlowEpcNetwork::DumpPgwStatistics ()
{
  DataRate downRate (8 * m_pgwDownBytes / 10);
  DataRate upRate (8 * m_pgwUpBytes / 10);
  m_pgwTrace (downRate, upRate);

  m_pgwUpBytes = m_pgwDownBytes = 0;
  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpPgwStatistics, this);
}

void
OpenFlowEpcNetwork::DumpSwtStatistics ()
{
  std::vector<uint32_t> teid;

  Ptr<OFSwitch13NetDevice> swDev;
  for (uint16_t i = 0; i < GetNSwitches (); i++)
    {
      swDev = GetSwitchDevice (i);
      teid.push_back (swDev->GetNumberFlowEntries (1)); // TEID table is 1
    }
  m_swtTrace (teid);

  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpSwtStatistics, this);
}

void
OpenFlowEpcNetwork::DumpBwdStatistics ()
{ 
  m_bwdTrace (m_ofCtrlApp->GetBandwidthStats ());

  Simulator::Schedule (m_dumpTimeout, 
    &OpenFlowEpcNetwork::DumpBwdStatistics, this);
}

void
OpenFlowEpcNetwork::DumpEpcStatistics (uint32_t teid, std::string desc, 
                                       bool uplink)
{
  NS_LOG_FUNCTION (this << teid << desc);

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
OpenFlowEpcNetwork::ResetEpcStatistics (uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);
  
  GetQosStatsFromTeid (teid, true)->ResetCounters ();
  GetQosStatsFromTeid (teid, false)->ResetCounters ();
}

void
OpenFlowEpcNetwork::SetSwitchDeviceAttribute (std::string n1, 
                                              const AttributeValue &v1)
{
  m_ofHelper->SetDeviceAttribute (n1, v1);
} 

Ptr<OFSwitch13NetDevice> 
OpenFlowEpcNetwork::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (index));
}

void
OpenFlowEpcNetwork::RegisterNodeAtSwitch (uint16_t switchIdx, Ptr<Node> node)
{
  std::pair<NodeSwitchMap_t::iterator, bool> ret;
  std::pair<Ptr<Node>, uint16_t> entry (node, switchIdx);
  ret = m_nodeSwitchMap.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("Node " << node << " -- switch " << (int)switchIdx);
      return;
    }
  NS_FATAL_ERROR ("Can't register node at switch.");
}

void
OpenFlowEpcNetwork::RegisterGatewayAtSwitch (uint16_t switchIdx, Ptr<Node> node)
{
  m_gatewaySwitch = switchIdx;
  m_gatewayNode = node;
}

uint16_t
OpenFlowEpcNetwork::GetSwitchIdxForNode (Ptr<Node> node)
{
  NodeSwitchMap_t::iterator ret;
  ret = m_nodeSwitchMap.find (node);
  if (ret != m_nodeSwitchMap.end ())
    {
      NS_LOG_DEBUG ("Found switch " << (int)ret->second << " for " << node);
      return ret->second;
    }
  NS_FATAL_ERROR ("Node not registered.");
}

uint16_t
OpenFlowEpcNetwork::GetSwitchIdxForDevice (Ptr<OFSwitch13NetDevice> dev)
{
  uint16_t i;
  for (i = 0; i < GetNSwitches (); i++)
    {
      if (dev == GetSwitchDevice (i))
        {
          return i;
        }
    }
  NS_FATAL_ERROR ("Device not registered.");
}

uint16_t
OpenFlowEpcNetwork::GetGatewaySwitchIdx ()
{
  return m_gatewaySwitch;
}

Ptr<Node>
OpenFlowEpcNetwork::GetGatewayNode ()
{
  return m_gatewayNode;
}

void
OpenFlowEpcNetwork::SetController (Ptr<OpenFlowEpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  NS_ASSERT_MSG (!m_ofCtrlApp, "Controller application already set.");
  
  // Installing the controller app into a new controller node
  m_ofCtrlApp = controller;
  m_ofCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_ofCtrlNode);

  m_ofHelper->InstallControllerApp (m_ofCtrlNode, m_ofCtrlApp);
  m_ofCtrlApp->SetOfNetwork (this);
}

Ptr<QosStatsCalculator>
OpenFlowEpcNetwork::GetQosStatsFromTeid (uint32_t teid, bool isDown)
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

};  // namespace ns3

