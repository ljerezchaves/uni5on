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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <iomanip>
#include <iostream>
#include "backhaul-stats-calculator.h"
#include "../metadata/ue-info.h"
#include "../metadata/routing-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (BackhaulStatsCalculator);

BackhaulStatsCalculator::BackhaulStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Clear slice metadata.
  memset (m_slices, 0, sizeof (SliceMetadata) * N_SLICE_IDS_ALL);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::Uni5onEnbApplication/S1uRx",
    MakeCallback (&BackhaulStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::Uni5onEnbApplication/S1uTx",
    MakeCallback (&BackhaulStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwTunnelApp/S5Rx",
    MakeCallback (&BackhaulStatsCalculator::EpcOutputPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::PgwTunnelApp/S5Tx",
    MakeCallback (&BackhaulStatsCalculator::EpcInputPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/OverloadDrop",
    MakeCallback (&BackhaulStatsCalculator::OverloadDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/MeterDrop",
    MakeCallback (&BackhaulStatsCalculator::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/TableDrop",
    MakeCallback (&BackhaulStatsCalculator::TableDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/PortList/*/PortQueue/Drop",
    MakeCallback (&BackhaulStatsCalculator::QueueDropPacket, this));
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
    .AddAttribute ("BwdStatsFilename",
                   "Filename for backhaul bandwidth statistics.",
                   StringValue ("backhaul-bandwidth"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_bwdFilename),
                   MakeStringChecker ())
    .AddAttribute ("TffStatsFilename",
                   "Filename for backhaul traffic statistics.",
                   StringValue ("backhaul-traffic"),
                   MakeStringAccessor (
                     &BackhaulStatsCalculator::m_tffFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
BackhaulStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  for (int s = 0; s < N_SLICE_IDS_ALL; s++)
    {
      for (int d = 0; d < N_DIRECTIONS; d++)
        {
          for (int t = 0; t < N_QOS_TYPES; t++)
            {
              m_slices [s].flowStats [d][t] = 0;
            }
        }
      m_slices [s].bwdWrapper = 0;
      m_slices [s].tffWrapper = 0;
    }

  Object::DoDispose ();
}

void
BackhaulStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("BwdStatsFilename", StringValue (prefix + m_bwdFilename));
  SetAttribute ("TffStatsFilename", StringValue (prefix + m_tffFilename));

  for (int s = 0; s < N_SLICE_IDS_ALL; s++)
    {
      std::string sliceStr = SliceIdStr (static_cast<SliceId> (s));
      SliceMetadata &slData = m_slices [s];
      for (int d = 0; d < N_DIRECTIONS; d++)
        {
          for (int t = 0; t < N_QOS_TYPES; t++)
            {
              slData.flowStats [d][t] =
                CreateObjectWithAttributes<FlowStatsCalculator> (
                  "Continuous", BooleanValue (true));
            }
        }

      // Create the output files for this slice.
      slData.bwdWrapper = Create<OutputStreamWrapper> (
          m_bwdFilename + "-" + sliceStr + ".log", std::ios::out);
      slData.tffWrapper = Create<OutputStreamWrapper> (
          m_tffFilename + "-" + sliceStr + ".log", std::ios::out);

      // Print the headers in output files.
      *slData.bwdWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec"
        << " " << setw (7) << "LinkDir";
      LinkInfo::PrintHeader (*slData.bwdWrapper->GetStream ());
      *slData.bwdWrapper->GetStream () << std::endl;

      *slData.tffWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec"
        << " " << setw (7) << "TrafDir"
        << " " << setw (8) << "QosType";
      FlowStatsCalculator::PrintHeader (*slData.tffWrapper->GetStream ());
      *slData.tffWrapper->GetStream () << std::endl;
    }

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

  // Dump statistics for each network slice.
  for (int s = 0; s < N_SLICE_IDS_ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      SliceMetadata &slData = m_slices [s];

      // Dump slice bandwidth usage for each link.
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          for (int d = 0; d < N_LINK_DIRS; d++)
            {
              LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

              *slData.bwdWrapper->GetStream ()
                << " " << setw (8) << Simulator::Now ().GetSeconds ()
                << " " << setw (7) << LinkInfo::LinkDirStr (dir);
              lInfo->PrintValues (*slData.bwdWrapper->GetStream (), dir, slice);
              *slData.bwdWrapper->GetStream () << std::endl;
            }
        }
      *slData.bwdWrapper->GetStream () << std::endl;

      // Dump slice traffic stats for each direction.
      for (int t = 0; t < N_QOS_TYPES; t++)
        {
          QosType type = static_cast<QosType> (t);
          for (int d = 0; d < N_DIRECTIONS; d++)
            {
              Direction dir = static_cast<Direction> (d);
              Ptr<FlowStatsCalculator> flowStats = slData.flowStats [d][t];

              *slData.tffWrapper->GetStream ()
                << " " << setw (8) << Simulator::Now ().GetSeconds ()
                << " " << setw (7) << DirectionStr (dir)
                << " " << setw (8) << QosTypeStr (type)
                << *flowStats
                << std::endl;
              flowStats->ResetCounters ();
            }
        }
      *slData.tffWrapper->GetStream () << std::endl;
    }

  Simulator::Schedule (nextDump, &BackhaulStatsCalculator::DumpStatistics,
                       this, nextDump);
}

void
BackhaulStatsCalculator::OverloadDropPacket (std::string context,
                                             Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
          SliceId slice = rInfo->GetSliceId ();
          Direction dir = Direction::DLINK;
          QosType type = rInfo->GetQosType ();

          sliStats = m_slices [slice].flowStats [dir][type];
          sliStats->NotifyTx (packet->GetSize ());
          sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);

          aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
          aggStats->NotifyTx (packet->GetSize ());
          aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
        }
    }
}

void
BackhaulStatsCalculator::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      // Identify the meter type (MBR or slicing).
      FlowStatsCalculator::DropReason dropReason = FlowStatsCalculator::METER;
      if ((meterId & METER_SLC_TYPE) == METER_SLC_TYPE)
        {
          dropReason = FlowStatsCalculator::SLICE;
        }

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyDrop (packet->GetSize (), dropReason);

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyDrop (packet->GetSize (), dropReason);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      // It must be a packed dropped by a traffic meter because this is the
      // only type of meters that we can have in the P-GW TFT switches.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
          SliceId slice = rInfo->GetSliceId ();
          Direction dir = Direction::DLINK;
          QosType type = rInfo->GetQosType ();

          sliStats = m_slices [slice].flowStats [dir][type];
          sliStats->NotifyTx (packet->GetSize ());
          sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::METER);

          aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
          aggStats->NotifyTx (packet->GetSize ());
          aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::METER);
        }
    }
}

void
BackhaulStatsCalculator::QueueDropPacket (std::string context,
                                          Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
          SliceId slice = rInfo->GetSliceId ();
          Direction dir = Direction::DLINK;
          QosType type = rInfo->GetQosType ();

          sliStats = m_slices [slice].flowStats [dir][type];
          sliStats->NotifyTx (packet->GetSize ());
          sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);

          aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
          aggStats->NotifyTx (packet->GetSize ());
          aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
        }
    }
}

void
BackhaulStatsCalculator::TableDropPacket (
  std::string context, Ptr<const Packet> packet, uint8_t tableId)
{
  NS_LOG_FUNCTION (this << context << packet <<
                   static_cast<uint16_t> (tableId));

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);
    }
  else
    {
      // This only happens when a packet is dropped at the P-GW, before
      // entering the TFT logical port that is responsible for attaching the
      // EpcGtpuTag and notifying that the packet is entering the EPC.
      // To keep consistent log results, we are doing this manually here.
      uint32_t teid = PgwTftClassify (packet);
      if (teid)
        {
          Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
          SliceId slice = rInfo->GetSliceId ();
          Direction dir = Direction::DLINK;
          QosType type = rInfo->GetQosType ();

          sliStats = m_slices [slice].flowStats [dir][type];
          sliStats->NotifyTx (packet->GetSize ());
          sliStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);

          aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
          aggStats->NotifyTx (packet->GetSize ());
          aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);
        }
    }
}

void
BackhaulStatsCalculator::EpcInputPacket (std::string context,
                                         Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyTx (packet->GetSize ());

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyTx (packet->GetSize ());
    }
}

void
BackhaulStatsCalculator::EpcOutputPacket (std::string context,
                                          Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  EpcGtpuTag gtpuTag;
  Ptr<FlowStatsCalculator> sliStats;
  Ptr<FlowStatsCalculator> aggStats;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      Direction dir = gtpuTag.GetDirection ();
      QosType type = gtpuTag.GetQosType ();

      sliStats = m_slices [slice].flowStats [dir][type];
      sliStats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());

      aggStats = m_slices [SliceId::ALL].flowStats [dir][type];
      aggStats->NotifyRx (packet->GetSize (), gtpuTag.GetTimestamp ());
    }
}

uint32_t
BackhaulStatsCalculator::PgwTftClassify (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  Ptr<Packet> packetCopy = packet->Copy ();

  EthernetHeader ethHeader;
  packetCopy->RemoveHeader (ethHeader);

  Ipv4Header ipv4Header;
  packetCopy->PeekHeader (ipv4Header);

  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (ipv4Header.GetDestination ());
  return ueInfo ? ueInfo->Classify (packetCopy) : 0;
}

} // Namespace ns3
