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

#include "link-info.h"
#include "../logical/epc-gtpu-tag.h"
#include "../metadata/routing-info.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                 \
  if (m_switches [0].swDev != 0 && m_switches [1].swDev != 0)                 \
    {                                                                         \
      std::clog << "[LInfo " << m_switches [0].swDev->GetDatapathId ()        \
                << " to " << m_switches [1].swDev->GetDatapathId () << "] ";  \
    }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkInfo");
NS_OBJECT_ENSURE_REGISTERED (LinkInfo);

// Initializing LinkInfo static members.
LinkInfo::LinkInfoMap_t LinkInfo::m_linksMap;
LinkInfoList_t LinkInfo::m_linksList;

LinkInfo::LinkInfo (SwitchData sw1, SwitchData sw2, Ptr<CsmaChannel> channel)
  : m_channel (channel)
{
  NS_LOG_FUNCTION (this << sw1.swDev << sw2.swDev << channel);

  m_switches [0] = sw1;
  m_switches [1] = sw2;

  // Asserting internal device order to ensure FWD and BWD indices order.
  NS_ASSERT_MSG (channel->GetCsmaDevice (0) == m_switches [0].portDev
                 && channel->GetCsmaDevice (1) == m_switches [1].portDev,
                 "Invalid device order in csma channel.");

  // Asserting full-duplex csma channel.
  NS_ASSERT_MSG (IsFullDuplexLink (), "Invalid half-duplex csma channel.");

  // Connecting trace source to CsmaNetDevice PhyTxEnd trace source, used to
  // monitor data transmitted over this connection.
  m_switches [0].portDev->TraceConnect (
    "PhyTxEnd", "Forward", MakeCallback (&LinkInfo::NotifyTxPacket, this));
  m_switches [1].portDev->TraceConnect (
    "PhyTxEnd", "Backward", MakeCallback (&LinkInfo::NotifyTxPacket, this));

  // Preparing slicing metadata structures.
  uint8_t numSlices = static_cast<uint8_t> (SliceId::ALL) + 1;
  memset (m_slices, 0, sizeof (SliceData) * numSlices * 2);

  RegisterLinkInfo (Ptr<LinkInfo> (this));
}

LinkInfo::~LinkInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LinkInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkInfo")
    .SetParent<Object> ()
    .AddAttribute ("AdjustmentStep",
                   "Default meter bit rate adjustment step.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mb/s")),
                   MakeDataRateAccessor (&LinkInfo::m_adjustmentStep),
                   MakeDataRateChecker ())
    .AddAttribute ("EwmaAlpha",
                   "The EWMA alpha parameter for averaging link statistics.",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&LinkInfo::m_alpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("UpdateTimeout",
                   "The interval between subsequent link statistics update.",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&LinkInfo::m_timeout),
                   MakeTimeChecker ())

    // Trace source used by controller to update slicing meters.
    .AddTraceSource ("MeterAdjusted",
                     "Meter bit rate adjusted.",
                     MakeTraceSourceAccessor (&LinkInfo::m_meterAdjustedTrace),
                     "ns3::LinkInfo::MeterAdjustedTracedCallback")
  ;
  return tid;
}

uint32_t
LinkInfo::GetPortNo (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].portNo;
}

uint64_t
LinkInfo::GetSwDpId (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].swDev->GetDatapathId ();
}

Mac48Address
LinkInfo::GetPortMacAddr (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return Mac48Address::ConvertFrom (m_switches [idx].portDev->GetAddress ());
}

uint64_t
LinkInfo::GetThpBitRate (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [slice][dir].ewmaThp;
}

double
LinkInfo::GetThpSliceRatio (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (dir, slice) == 0)
    {
      NS_ASSERT_MSG (GetThpBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetThpBitRate (dir, slice))
             / GetMaxBitRate (dir, slice);
    }
}

uint64_t
LinkInfo::GetFreeBitRate (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return GetMaxBitRate (dir, slice) - GetResBitRate (dir, slice);
}

double
LinkInfo::GetFreeSliceRatio (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (dir, slice) == 0)
    {
      NS_ASSERT_MSG (GetFreeBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetFreeBitRate (dir, slice))
             / GetMaxBitRate (dir, slice);
    }
}

uint64_t
LinkInfo::GetResBitRate (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [slice][dir].resRate;
}

double
LinkInfo::GetResSliceRatio (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (dir, slice) == 0)
    {
      NS_ASSERT_MSG (GetResBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetResBitRate (dir, slice))
             / GetMaxBitRate (dir, slice);
    }
}

uint64_t
LinkInfo::GetMaxBitRate (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [slice][dir].maxRate;
}

DpIdPair_t
LinkInfo::GetSwitchDpIdPair (void) const
{
  NS_LOG_FUNCTION (this);

  return DpIdPair_t (GetSwDpId (0), GetSwDpId (1));
}

uint64_t
LinkInfo::GetTxBytes (Direction dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [slice][dir].txBytes;
}

bool
LinkInfo::HasBitRate (
  uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate) const
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  return (GetFreeBitRate (dir, slice) >= bitRate);
}

bool
LinkInfo::ReleaseBitRate (
  uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  // Check for reserved bit rate.
  if (GetResBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  // Releasing the bit rate.
  m_slices [slice][dir].resRate -= bitRate;
  NS_LOG_DEBUG ("Releasing " << bitRate <<
                " bit rate on slice " << SliceIdStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  NS_LOG_DEBUG ("Current " << SliceIdStr (slice) <<
                " reserved bit rate: " << GetResBitRate (dir, slice));
  NS_LOG_DEBUG ("Current " << SliceIdStr (slice) <<
                " free bit rate: " << GetFreeBitRate (dir, slice));

  // Updating the meter bit rate.
  UpdateMeterDiff (dir, slice, bitRate, false);

  // Updating statistics for the fake shared slice.
  m_slices [SliceId::ALL][dir].resRate -= bitRate;
  UpdateMeterDiff (dir, SliceId::ALL, bitRate, false);
  return true;
}

bool
LinkInfo::ReserveBitRate (
  uint64_t src, uint64_t dst, SliceId slice, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  // Check for available bit rate.
  if (GetFreeBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  // Reserving the bit rate.
  m_slices [slice][dir].resRate += bitRate;
  NS_LOG_DEBUG ("Reserving " << bitRate <<
                " bit rate on slice " << SliceIdStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  NS_LOG_DEBUG ("Current " << SliceIdStr (slice) <<
                " reserved bit rate: " << GetResBitRate (dir, slice));
  NS_LOG_DEBUG ("Current " << SliceIdStr (slice) <<
                " free bit rate: " << GetFreeBitRate (dir, slice));

  // Updating the meter bit rate.
  UpdateMeterDiff (dir, slice, bitRate, true);

  // Updating statistics for the fake shared slice.
  m_slices [SliceId::ALL][dir].resRate += bitRate;
  UpdateMeterDiff (dir, SliceId::ALL, bitRate, true);
  return true;
}

bool
LinkInfo::SetSliceQuotas (
  Direction dir, SliceQuotaMap_t &quotas)
{
  NS_LOG_FUNCTION (this << dir);

  // First, check for consistent slice quotas.
  uint16_t sumQuotas = 0;
  for (int s = 0; s < SliceId::ALL; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      SliceQuotaMap_t::iterator it = quotas.find (slice);
      NS_ASSERT_MSG (it != quotas.end (), "Missing slice quota.");
      NS_ASSERT_MSG (it->second >= 0 && it->second <= 100, "Invalid quota.");

      sumQuotas += it->second;
      uint64_t newRate = (GetLinkBitRate () * it->second) / 100;
      if (GetResBitRate (dir, slice) > newRate)
        {
          NS_LOG_WARN ("Can't change the slice quota. The new bit rate is "
                       "lower than the already reserved bit rate.");
          return false;
        }
    }
  NS_ABORT_MSG_IF (sumQuotas != 100, "Inconsistent slice quotas.");

  // Then, update slice maximum bit rates.
  for (auto const &it : quotas)
    {
      m_slices [it.first][dir].maxRate = (GetLinkBitRate () * it.second) / 100;
      NS_LOG_DEBUG (SliceIdStr (it.first) << " slice quota: " << it.second);
    }
  return true;
}

std::string
LinkInfo::DirectionStr (Direction dir)
{
  switch (dir)
    {
    case LinkInfo::FWD:
      return "forward";
    case LinkInfo::BWD:
      return "backward";
    default:
      return "-";
    }
}

LinkInfoList_t
LinkInfo::GetList (void)
{
  return LinkInfo::m_linksList;
}

Ptr<LinkInfo>
LinkInfo::GetPointer (uint64_t dpId1, uint64_t dpId2)
{
  DpIdPair_t key;
  key.first  = std::min (dpId1, dpId2);
  key.second = std::max (dpId1, dpId2);

  Ptr<LinkInfo> lInfo = 0;
  LinkInfoMap_t::iterator ret;
  ret = LinkInfo::m_linksMap.find (key);
  if (ret != LinkInfo::m_linksMap.end ())
    {
      lInfo = ret->second;
    }
  return lInfo;
}

void
LinkInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_switches [0].swDev = 0;
  m_switches [1].swDev = 0;
  m_switches [0].portDev = 0;
  m_switches [1].portDev = 0;
  m_channel = 0;
  Object::DoDispose ();
}

void
LinkInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Scheduling the first update statistics.
  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (m_timeout, &LinkInfo::UpdateStatistics, this);

  // Set the maximum bit rate for the fake shared slice.
  m_slices [SliceId::ALL][0].maxRate = GetLinkBitRate ();
  m_slices [SliceId::ALL][1].maxRate = GetLinkBitRate ();

  // Chain up
  Object::NotifyConstructionCompleted ();
}

LinkInfo::Direction
LinkInfo::GetDirection (uint64_t src, uint64_t dst) const
{
  NS_LOG_FUNCTION (this << src << dst);

  NS_ASSERT_MSG ((src == GetSwDpId (0) && dst == GetSwDpId (1))
                 || (src == GetSwDpId (1) && dst == GetSwDpId (0)),
                 "Invalid datapath IDs for this connection.");

  if (src == GetSwDpId (0))
    {
      return LinkInfo::FWD;
    }
  else
    {
      return LinkInfo::BWD;
    }
}

uint64_t
LinkInfo::GetLinkBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->GetDataRate ().GetBitRate ();
}

bool
LinkInfo::IsFullDuplexLink (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->IsFullDuplex ();
}

void
LinkInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  LinkInfo::Direction dir;
  dir = (context == "Forward") ? LinkInfo::FWD : LinkInfo::BWD;

  // Update TX packets for the packet slice.
  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (gtpuTag.GetTeid ());
      m_slices [rInfo->GetSliceId ()][dir].txBytes += packet->GetSize ();
    }
  else
    {
      NS_LOG_WARN ("GTPU packet tag not found for packet " << packet);
    }

  // Update TX packets also for fake shared slice.
  m_slices [SliceId::ALL][dir].txBytes += packet->GetSize ();
}

void
LinkInfo::UpdateMeterDiff (
  Direction dir, SliceId slice, uint64_t bitRate, bool reserve)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate << reserve);

  if (reserve)
    {
      m_slices [slice][dir].meterDiff -= bitRate;
    }
  else // release
    {
      m_slices [slice][dir].meterDiff += bitRate;
    }

  NS_LOG_DEBUG ("Current " << SliceIdStr (slice) <<
                " diff bit rate: " << m_slices [slice][dir].meterDiff);

  uint64_t diffAbs = static_cast<uint64_t> (
      std::abs (m_slices [slice][dir].meterDiff));
  if (diffAbs >= m_adjustmentStep.GetBitRate ())
    {
      // Fire meter adjusted trace source to update meters.
      NS_LOG_DEBUG ("Fire meter adjustment and clear meter diff.");
      m_meterAdjustedTrace (Ptr<LinkInfo> (this), dir, slice);
    }
}

void
LinkInfo::UpdateStatistics (void)
{
  // NS_LOG_FUNCTION (thi);

  double elapSecs = (Simulator::Now () - m_lastUpdate).GetSeconds ();
  uint64_t bytes = 0;
  for (int s = 0; s <= SliceId::ALL; s++)
    {
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          bytes = m_slices [s][d].txBytes - m_slices [s][d].lastTxBytes;
          m_slices [s][d].lastTxBytes = m_slices [s][d].txBytes;

          m_slices [s][d].ewmaThp = (m_alpha * 8 * bytes) / elapSecs +
            (1 - m_alpha) * m_slices [s][d].ewmaThp;
        }
    }

  // Scheduling the next update statistics.
  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (m_timeout, &LinkInfo::UpdateStatistics, this);
}

void
LinkInfo::RegisterLinkInfo (Ptr<LinkInfo> lInfo)
{
  // Respecting the increasing switch index order when saving connection data.
  uint16_t dpId1 = lInfo->GetSwDpId (0);
  uint16_t dpId2 = lInfo->GetSwDpId (1);

  DpIdPair_t key;
  key.first  = std::min (dpId1, dpId2);
  key.second = std::max (dpId1, dpId2);

  std::pair<DpIdPair_t, Ptr<LinkInfo> > entry (key, lInfo);
  std::pair<LinkInfoMap_t::iterator, bool> ret;
  ret = LinkInfo::m_linksMap.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing connection information.");

  LinkInfo::m_linksList.push_back (lInfo);
}

} // namespace ns3
