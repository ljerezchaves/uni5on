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

  // FIXME
  std::ostringstream str;
  str << AppStatsCalculator::PrintHeader ()
      << setw (7)  << "DpSwt"
      << setw (7)  << "DpMbr"
      << setw (7)  << "DpQue"
      << setw (7)  << "DpSli"
      << setw (7)  << "DpAll";
  return str.str ();
}

void
EpcStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  AppStatsCalculator::DoDispose ();
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
  Ptr<const EpcStatsCalculator> epcStats;

  if (rInfo->HasUlTraffic ())
    {
      // Dump uplink statistics.
      epcStats = GetEpcStatsFromTeid (teid, false);
      *m_epcWrapper->GetStream ()
        << GetStats (app, epcStats, rInfo, "up")
        << *epcStats
        << std::endl;

      *m_appWrapper->GetStream ()
        << GetStats (app, app->GetServerAppStats (), rInfo, "up")
        << std::endl;
    }

  if (rInfo->HasDlTraffic ())
    {
      // Dump downlink statistics.
      epcStats = GetEpcStatsFromTeid (teid, true);
      *m_epcWrapper->GetStream ()
        << GetStats (app, epcStats, rInfo, "down")
        << *epcStats
        << std::endl;

      *m_appWrapper->GetStream ()
        << GetStats (app, app->GetAppStats (), rInfo, "down")
        << std::endl;
    }
}

void
TrafficStatsCalculator::ResetCounters (std::string context,
                                       Ptr<SvelteClientApp> app)
{
  NS_LOG_FUNCTION (this << context << app);

  GetEpcStatsFromTeid (app->GetTeid (),  true)->ResetCounters ();
  GetEpcStatsFromTeid (app->GetTeid (), false)->ResetCounters ();
}

void
TrafficStatsCalculator::LoadDropPacket (std::string context,
                                        Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<EpcStatsCalculator> epcStats =
        GetEpcStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::SWTCH);
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

      Ptr<EpcStatsCalculator> epcStats = GetEpcStatsFromTeid (teid, true);
      epcStats->NotifyTx (packetCopy->GetSize ());
      epcStats->NotifyDrop (packetCopy->GetSize (), EpcStatsCalculator::SWTCH);
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
      epcStats = GetEpcStatsFromTeid (teid, gtpuTag.IsDownlink ());

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
      epcStats = GetEpcStatsFromTeid (teid, true);
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
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<EpcStatsCalculator> epcStats =
        GetEpcStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      epcStats->NotifyDrop (packet->GetSize (), EpcStatsCalculator::QUEUE);
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
      Ptr<EpcStatsCalculator> epcStats =
        GetEpcStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      epcStats->NotifyTx (packet->GetSize ());
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
      Ptr<EpcStatsCalculator> epcStats =
        GetEpcStatsFromTeid (gtpuTag.GetTeid (), gtpuTag.IsDownlink ());
      epcStats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

Ptr<EpcStatsCalculator>
TrafficStatsCalculator::GetEpcStatsFromTeid (uint32_t teid, bool isDown)
{
  NS_LOG_FUNCTION (this << teid << isDown);

  Ptr<EpcStatsCalculator> epcStats = 0;
  auto it = m_qosByTeid.find (teid);
  if (it != m_qosByTeid.end ())
    {
      QosStatsPair_t value = it->second;
      epcStats = isDown ? value.first : value.second;
    }
  else
    {
      QosStatsPair_t pair (CreateObject<EpcStatsCalculator> (),
                           CreateObject<EpcStatsCalculator> ());
      std::pair<uint32_t, QosStatsPair_t> entry (teid, pair);
      auto ret = m_qosByTeid.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Existing QoS entry for teid " << teid);
        }
      epcStats = isDown ? pair.first : pair.second;
    }
  return epcStats;
}

std::string
TrafficStatsCalculator::GetHeader (void)
{
  NS_LOG_FUNCTION (this);

  std::ostringstream str;
  str << right
      << GetTimeHeader ()
      << setw (9)  << "AppName"
      << setw (7)  << "Ul/Dl"
      << RoutingInfo::PrintHeader ()
      << AppStatsCalculator::PrintHeader ();
  return str.str ();
}

std::string
TrafficStatsCalculator::GetStats (
  Ptr<const SvelteClientApp> app, Ptr<const AppStatsCalculator> stats,
  Ptr<const RoutingInfo> rInfo, std::string direction)
{
  NS_LOG_FUNCTION (this << app << stats << rInfo << direction);

  std::ostringstream str;
  str << fixed << setprecision (3) << boolalpha << right
      << GetTimeStr ()
      << setw (9)  << app->GetAppName ()
      << setw (7)  << direction
      << *rInfo
      << *stats;
  return str.str ();
}

std::ostream & operator << (std::ostream &os, const EpcStatsCalculator &stats)
{
  // FIXME
  os << static_cast<AppStatsCalculator> (stats);
  for (int r = 0; r <= EpcStatsCalculator::ALL; r++)
    {
      os << setw (8) << stats.GetDpPackets (
        static_cast<EpcStatsCalculator::DropReason> (r));
    }
  return os;
}

} // Namespace ns3
