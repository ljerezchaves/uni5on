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

LinkInfo::LinkInfo (SwitchData sw1, SwitchData sw2,
                    Ptr<CsmaChannel> channel, bool slicing)
  : m_channel (channel),
  m_slicing (slicing)
{
  NS_LOG_FUNCTION (this << sw1.swDev << sw2.swDev << channel << slicing);

  m_switches [0] = sw1;
  m_switches [1] = sw2;

  // Asserting internal device order to ensure thar forward and backward
  // indexes are correct.
  NS_ASSERT_MSG (channel->GetCsmaDevice (0) == GetPortDev (0)
                 && channel->GetCsmaDevice (1) == GetPortDev (1),
                 "Invalid device order in csma channel.");

  // Connecting trace source to CsmaNetDevice PhyTxEnd trace source, used to
  // monitor data transmitted over this connection.
  m_switches [0].portDev->TraceConnect (
    "PhyTxEnd", "Forward", MakeCallback (&LinkInfo::NotifyTxPacket, this));
  m_switches [1].portDev->TraceConnect (
    "PhyTxEnd", "Backward", MakeCallback (&LinkInfo::NotifyTxPacket, this));

  // Preparing slicing metadata structures.
  uint8_t numSlices = static_cast<uint8_t> (LinkSlice::ALL);
  memset (m_slices, 0, sizeof (SliceData) * numSlices);

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
    .AddAttribute ("GbrSliceQuota",
                   "Maximum bandwidth ratio for GBR slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.28),
                   MakeDoubleAccessor (&LinkInfo::m_gbrSliceQuota),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("M2mSliceQuota",
                   "Maximum bandwidth ratio for M2M slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.18),
                   MakeDoubleAccessor (&LinkInfo::m_m2mSliceQuota),
                   MakeDoubleChecker<double> (0.0, 0.5))
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
  return GetSwDev (idx)->GetDatapathId ();
}

Ptr<const OFSwitch13Device>
LinkInfo::GetSwDev (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].swDev;
}

Ptr<const CsmaNetDevice>
LinkInfo::GetPortDev (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].portDev;
}

Mac48Address
LinkInfo::GetPortMacAddr (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return Mac48Address::ConvertFrom (GetPortDev (idx)->GetAddress ());
}

LinkInfo::Direction
LinkInfo::GetDirection (uint64_t src, uint64_t dst) const
{
  NS_LOG_FUNCTION (this << src << dst);

  NS_ASSERT_MSG ((src == GetSwDpId (0) && dst == GetSwDpId (1))
                 || (src == GetSwDpId (1) && dst == GetSwDpId (0)),
                 "Invalid datapath IDs for this connection.");
  if (IsFullDuplexLink () && src == GetSwDpId (1))
    {
      return LinkInfo::BWD;
    }

  // For half-duplex channel always return FWD, as we will
  // only use the forwarding path for resource reservations.
  return LinkInfo::FWD;
}

uint64_t
LinkInfo::GetThpBitRate (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  double throughput = 0;
  if (slice >= LinkSlice::ALL)
    {
      for (int i = 0; i < LinkSlice::ALL; i++)
        {
          throughput += m_slices [i].ewmaThp [dir];
        }
    }
  else
    {
      throughput = m_slices [slice].ewmaThp [dir];
    }
  return static_cast<uint64_t> (throughput);
}

double
LinkInfo::GetThpSliceRatio (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (slice) == 0)
    {
      NS_ASSERT_MSG (GetThpBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetThpBitRate (dir, slice))
             / GetMaxBitRate (slice);
    }
}

uint64_t
LinkInfo::GetFreeBitRate (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return GetMaxBitRate (slice) - GetResBitRate (dir, slice);
}

double
LinkInfo::GetFreeSliceRatio (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (slice) == 0)
    {
      NS_ASSERT_MSG (GetFreeBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetFreeBitRate (dir, slice))
             / GetMaxBitRate (slice);
    }
}

uint64_t
LinkInfo::GetResBitRate (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  uint64_t bitrate = 0;
  if (slice >= LinkSlice::ALL)
    {
      for (int i = 0; i < LinkSlice::ALL; i++)
        {
          bitrate += m_slices [i].resRate [dir];
        }
    }
  else
    {
      bitrate = m_slices [slice].resRate [dir];
    }
  return bitrate;
}

double
LinkInfo::GetResSliceRatio (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  if (GetMaxBitRate (slice) == 0)
    {
      NS_ASSERT_MSG (GetResBitRate (dir, slice) == 0, "Invalid slice usage.");
      return 0.0;
    }
  else
    {
      return static_cast<double> (GetResBitRate (dir, slice))
             / GetMaxBitRate (slice);
    }
}

uint64_t
LinkInfo::GetMaxBitRate (LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << slice);

  if (slice >= LinkSlice::ALL)
    {
      return GetLinkBitRate ();
    }
  else
    {
      return m_slices [slice].maxRate;
    }
}

DpIdPair_t
LinkInfo::GetSwitchDpIdPair (void) const
{
  NS_LOG_FUNCTION (this);

  return DpIdPair_t (GetSwDpId (0), GetSwDpId (1));
}

uint64_t
LinkInfo::GetTxBytes (Direction dir, LinkSlice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  uint64_t bytes = 0;
  if (slice >= LinkSlice::ALL)
    {
      for (int i = 0; i < LinkSlice::ALL; i++)
        {
          bytes += m_slices [i].txBytes [dir];
        }
    }
  else
    {
      bytes = m_slices [slice].txBytes [dir];
    }
  return bytes;
}

bool
LinkInfo::HasBitRate (
  uint64_t src, uint64_t dst, LinkSlice slice, uint64_t bitRate) const
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < LinkSlice::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  return (GetFreeBitRate (dir, slice) >= bitRate);
}

bool
LinkInfo::IsFullDuplexLink (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->IsFullDuplex ();
}

bool
LinkInfo::ReleaseBitRate (
  uint64_t src, uint64_t dst, LinkSlice slice, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < LinkSlice::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  // Check for reserved bit rate.
  if (GetResBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  // Releasing the bit rate.
  m_slices [slice].resRate [dir] -= bitRate;
  NS_LOG_DEBUG ("Releasing bit rate on slice " << LinkSliceStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  NS_LOG_DEBUG ("Current reserved bit rate: " << GetResBitRate (dir, slice));
  NS_LOG_DEBUG ("Current free bit rate: " << GetFreeBitRate (dir, slice));

  // Updating the meter bit rate.
  UpdateMeterDiff (dir, slice, bitRate, false);
  return true;
}

bool
LinkInfo::ReserveBitRate (
  uint64_t src, uint64_t dst, LinkSlice slice, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < LinkSlice::ALL, "Invalid slice for this operation.");
  LinkInfo::Direction dir = GetDirection (src, dst);

  // Check for available bit rate.
  if (GetFreeBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  // Reserving the bit rate.
  m_slices [slice].resRate [dir] += bitRate;
  NS_LOG_DEBUG ("Reserving bit rate on slice " << LinkSliceStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  NS_LOG_DEBUG ("Current reserved bit rate: " << GetResBitRate (dir, slice));
  NS_LOG_DEBUG ("Current free bit rate: " << GetFreeBitRate (dir, slice));

  // Updating the meter bit rate.
  UpdateMeterDiff (dir, slice, bitRate, true);
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

  m_channel = 0;
}

void
LinkInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  if (m_slicing)
    {
      uint64_t mtcRate, gbrRate, dftRate;
      mtcRate = static_cast<uint64_t> (GetLinkBitRate () * m_m2mSliceQuota);
      gbrRate = static_cast<uint64_t> (GetLinkBitRate () * m_gbrSliceQuota);
      dftRate = GetLinkBitRate () - gbrRate - mtcRate;

      m_slices [LinkSlice::M2M].maxRate = mtcRate;
      m_slices [LinkSlice::GBR].maxRate = gbrRate;
      m_slices [LinkSlice::DFT].maxRate = dftRate;
    }
  else
    {
      m_slices [LinkSlice::DFT].maxRate = GetLinkBitRate ();
    }

  NS_LOG_DEBUG ("DFT maximum bit rate: " <<  m_slices [LinkSlice::DFT].maxRate);
  NS_LOG_DEBUG ("GBR maximum bit rate: " <<  m_slices [LinkSlice::GBR].maxRate);
  NS_LOG_DEBUG ("M2M maximum bit rate: " <<  m_slices [LinkSlice::M2M].maxRate);

  // Scheduling the first update statistics.
  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (m_timeout, &LinkInfo::UpdateStatistics, this);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

uint64_t
LinkInfo::GetLinkBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->GetDataRate ().GetBitRate ();
}

void
LinkInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  LinkInfo::Direction dir;
  dir = (context == "Forward") ? LinkInfo::FWD : LinkInfo::BWD;

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (gtpuTag.GetTeid ());
      m_slices [rInfo->GetLinkSlice ()].txBytes [dir] += packet->GetSize ();
    }
  else
    {
      // For the case of non-tagged packets, save bytes in default slice.
      NS_LOG_WARN ("No GTPU packet tag found.");
      m_slices [LinkSlice::DFT].txBytes [dir] += packet->GetSize ();
    }
}

void
LinkInfo::UpdateMeterDiff (
  Direction dir, LinkSlice slice, uint64_t bitRate, bool reserve)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate << reserve);

  if (reserve)
    {
      m_slices [slice].meterDiff [dir] -= bitRate;
    }
  else // release
    {
      m_slices [slice].meterDiff [dir] += bitRate;
    }

  NS_LOG_DEBUG ("Current diff: " << m_slices [slice].meterDiff [dir]);
  uint64_t diffAbs = static_cast<uint64_t> (
      std::abs (m_slices [slice].meterDiff [dir]));
  if (diffAbs >= m_adjustmentStep.GetBitRate ())
    {
      // Fire meter adjusted trace source to update meters.
      NS_LOG_DEBUG ("Fire meter adjustment and clear meter diff.");
      m_meterAdjustedTrace (Ptr<LinkInfo> (this), dir, slice);
      m_slices [slice].meterDiff [dir] = 0;
    }
}

void
LinkInfo::UpdateStatistics (void)
{
  NS_LOG_FUNCTION (this);

  double elapSecs = (Simulator::Now () - m_lastUpdate).GetSeconds ();
  for (int s = 0; s < LinkSlice::ALL; s++)
    {
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          double bytes = static_cast<double> (m_slices [s].txBytes [d] -
                                              m_slices [s].lastTxBytes [d]);

          m_slices [s].ewmaThp [d] = (m_alpha * 8 * bytes / elapSecs) +
            (1 - m_alpha) * m_slices [s].ewmaThp [d];
          m_slices [s].lastTxBytes [d] = m_slices [s].txBytes [d];
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
