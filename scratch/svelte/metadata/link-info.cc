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

#include <iomanip>
#include <iostream>
#include "link-info.h"
#include "../logical/epc-gtpu-tag.h"

using namespace std;

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                 \
  if (GetSwPort (0) && GetSwPort (1))                                         \
    {                                                                         \
      std::clog << "[LInfo " << GetSwDpId (0)                                 \
                << " to " << GetSwDpId (1) << "] ";                           \
    }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkInfo");
NS_OBJECT_ENSURE_REGISTERED (LinkInfo);

// Initializing LinkInfo static members.
LinkInfo::LinkInfoMap_t LinkInfo::m_linkInfoByDpIds;
LinkInfoList_t LinkInfo::m_linkInfoList;

LinkInfo::LinkInfo (Ptr<OFSwitch13Port> port1, Ptr<OFSwitch13Port> port2,
                    Ptr<CsmaChannel> channel)
  : m_channel (channel)
{
  NS_LOG_FUNCTION (this << port1 << port2 << channel);

  m_ports [0] = port1;
  m_ports [1] = port2;

  // Asserting internal device order to ensure FWD and BWD indices order.
  NS_ASSERT_MSG (channel->GetCsmaDevice (0) == GetPortDev (0)
                 && channel->GetCsmaDevice (1) == GetPortDev (1),
                 "Invalid device order in csma channel.");

  // Asserting full-duplex csma channel.
  NS_ASSERT_MSG (IsFullDuplexLink (), "Invalid half-duplex csma channel.");

  // Validate maximum bit rate for this implementation.
  uint64_t link = m_channel->GetDataRate ().GetBitRate ();
  uint64_t maxr = static_cast<uint64_t> (std::numeric_limits<int64_t>::max ());
  NS_ASSERT_MSG (link < maxr, "Invalid bit rate for this implementation.");

  // Connecting trace source to CsmaNetDevice PhyTxEnd trace source, used to
  // monitor data transmitted over this connection.
  GetPortDev (0)->TraceConnect (
    "PhyTxEnd", "Forward", MakeCallback (&LinkInfo::NotifyTxPacket, this));
  GetPortDev (1)->TraceConnect (
    "PhyTxEnd", "Backward", MakeCallback (&LinkInfo::NotifyTxPacket, this));

  // Clear slice metadata.
  memset (m_slices, 0, sizeof (SliceMetadata) * N_LINK_DIRS * N_SLICE_IDS);

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
    //
    // For sufficiently large N, the first N data points represent about 86%
    // of the total weight in the calculation when EWMA alpha = 2 / (N + 1).
    //
    .AddAttribute ("EwmaLongAlpha",
                   "The EWMA alpha parameter for long-term link throughput.",
                   DoubleValue (0.04),  // Last 5 seconds (N ~= 50)
                   MakeDoubleAccessor (&LinkInfo::m_ewmaLtAlpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("EwmaShortAlpha",
                   "The EWMA alpha parameter for short-term link throughput.",
                   DoubleValue (0.2),   // Last 1 second (N ~= 10)
                   MakeDoubleAccessor (&LinkInfo::m_ewmaStAlpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("EwmaTimeout",
                   "The interval between subsequent EWMA statistics update.",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&LinkInfo::m_ewmaTimeout),
                   MakeTimeChecker ())
  ;
  return tid;
}

Mac48Address
LinkInfo::GetPortAddr (uint8_t idx) const
{
  return Mac48Address::ConvertFrom (GetPortDev (idx)->GetAddress ());
}

Ptr<CsmaNetDevice>
LinkInfo::GetPortDev (uint8_t idx) const
{
  return DynamicCast<CsmaNetDevice> (GetSwPort (idx)->GetPortDevice ());
}

uint32_t
LinkInfo::GetPortNo (uint8_t idx) const
{
  return GetSwPort (idx)->GetPortNo ();
}

Ptr<OFSwitch13Queue>
LinkInfo::GetPortQueue (uint8_t idx) const
{
  return GetSwPort (idx)->GetPortQueue ();
}

Ptr<OFSwitch13Device>
LinkInfo::GetSwDev (uint8_t idx) const
{
  return GetSwPort (idx)->GetSwitchDevice ();
}

uint64_t
LinkInfo::GetSwDpId (uint8_t idx) const
{
  return GetSwDev (idx)->GetDatapathId ();
}

Ptr<OFSwitch13Port>
LinkInfo::GetSwPort (uint8_t idx) const
{
  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_ports [idx];
}

LinkInfo::LinkDir
LinkInfo::GetLinkDir (uint64_t src, uint64_t dst) const
{
  NS_LOG_FUNCTION (this << src << dst);

  NS_ASSERT_MSG ((src == GetSwDpId (0) && dst == GetSwDpId (1))
                 || (src == GetSwDpId (1) && dst == GetSwDpId (0)),
                 "Invalid datapath IDs for this connection.");
  return (src == GetSwDpId (0)) ? LinkInfo::FWD : LinkInfo::BWD;
}

bool
LinkInfo::IsFullDuplexLink (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->IsFullDuplex ();
}

int64_t
LinkInfo::GetLinkBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (m_channel->GetDataRate ().GetBitRate ());
}

int
LinkInfo::GetQuota (LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [dir][slice].quota;
}

int64_t
LinkInfo::GetQuoBitRate (LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return GetLinkBitRate () * GetQuota (dir, slice) / 100;
}

int64_t
LinkInfo::GetMaxBitRate (LinkDir dir, SliceId slice, QosType type) const
{
  NS_LOG_FUNCTION (this << dir << slice << type);

  int64_t bitRate = GetQuoBitRate (dir, slice);
  if (type != QosType::GBR)
    {
      bitRate += GetExtBitRate (dir, slice);
    }
  return bitRate;
}

int64_t
LinkInfo::GetResBitRate (LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [dir][slice].reserved;
}

int64_t
LinkInfo::GetFreBitRate (LinkDir dir, SliceId slice, QosType type) const
{
  NS_LOG_FUNCTION (this << dir << slice << type);

  return GetMaxBitRate (dir, slice, type) - GetResBitRate (dir, slice);
}

int64_t
LinkInfo::GetUseBitRate (EwmaTerm term, LinkDir dir, SliceId slice,
                         QosType type) const
{
  NS_LOG_FUNCTION (this << term << dir << slice << type);

  return m_slices [dir][slice].ewmaThp [type][term];
}

int64_t
LinkInfo::GetIdlBitRate (EwmaTerm term, LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << term << dir << slice);

  return GetMaxBitRate (dir, slice, QosType::BOTH) -
         GetUseBitRate (term, dir, slice, QosType::BOTH);
}

int64_t
LinkInfo::GetExtBitRate (LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [dir][slice].extra;
}

int64_t
LinkInfo::GetMetBitRate (LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return m_slices [dir][slice].meter;
}

bool
LinkInfo::HasBitRate (LinkDir dir, SliceId slice, int64_t bitRate) const
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  return (GetFreBitRate (dir, slice, QosType::GBR) >= bitRate);
}

std::ostream &
LinkInfo::PrintValues (std::ostream &os, LinkDir dir, SliceId slice) const
{
  NS_LOG_FUNCTION (this);

  #define ST EwmaTerm::STERM
  #define LT EwmaTerm::LTERM
  #define NT QosType::NON
  #define GT QosType::GBR

  std::string linkDescStr;
  linkDescStr += std::to_string (GetSwDpId (0));
  linkDescStr += "->";
  linkDescStr += std::to_string (GetSwDpId (1));

  os << " " << setw (9)  << linkDescStr
     << " " << setw (11) << Bps2Kbps (GetLinkBitRate ())
     << " " << setw (8)  << GetQuota (dir, slice)
     << " " << setw (11) << Bps2Kbps (GetQuoBitRate (dir, slice))
     << " " << setw (11) << Bps2Kbps (GetResBitRate (dir, slice))
     << " " << setw (11) << Bps2Kbps (GetExtBitRate (dir, slice))
     << " " << setw (11) << Bps2Kbps (GetMetBitRate (dir, slice))    
     << " " << setw (11) << Bps2Kbps (GetMaxBitRate (dir, slice, GT))
     << " " << setw (11) << Bps2Kbps (GetMaxBitRate (dir, slice, NT))
     << " " << setw (11) << Bps2Kbps (GetFreBitRate (dir, slice, GT))
     << " " << setw (11) << Bps2Kbps (GetFreBitRate (dir, slice, NT))
     << " " << setw (11) << Bps2Kbps (GetUseBitRate (ST, dir, slice, GT))
     << " " << setw (11) << Bps2Kbps (GetUseBitRate (ST, dir, slice, NT))
     << " " << setw (11) << Bps2Kbps (GetIdlBitRate (ST, dir, slice))
     << " " << setw (11) << Bps2Kbps (GetUseBitRate (LT, dir, slice, GT))
     << " " << setw (11) << Bps2Kbps (GetUseBitRate (LT, dir, slice, NT))
     << " " << setw (11) << Bps2Kbps (GetIdlBitRate (LT, dir, slice));
  return os;
}

std::string
LinkInfo::LinkDirStr (LinkDir dir)
{
  switch (dir)
    {
    case LinkInfo::FWD:
      return "fwd";
    case LinkInfo::BWD:
      return "bwd";
    default:
      return "-";
    }
}

LinkInfo::LinkDir
LinkInfo::InvertDir (LinkDir dir)
{
  return (dir == LinkInfo::FWD) ? LinkInfo::BWD : LinkInfo::FWD;
}

const LinkInfoList_t&
LinkInfo::GetList (void)
{
  return LinkInfo::m_linkInfoList;
}

Ptr<LinkInfo>
LinkInfo::GetPointer (uint64_t dpId1, uint64_t dpId2)
{
  DpIdPair_t key;
  key.first  = std::min (dpId1, dpId2);
  key.second = std::max (dpId1, dpId2);

  Ptr<LinkInfo> lInfo = 0;
  auto ret = LinkInfo::m_linkInfoByDpIds.find (key);
  if (ret != LinkInfo::m_linkInfoByDpIds.end ())
    {
      lInfo = ret->second;
    }
  return lInfo;
}

std::ostream &
LinkInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (9)  << "DpIdDesc"
     << " " << setw (11) << "LinkKbps"
     << " " << setw (8)  << "Quota"
     << " " << setw (11) << "QuoKbps"
     << " " << setw (11) << "ResKpbs"
     << " " << setw (11) << "ExtKbps"
     << " " << setw (11) << "MetKbps"
     << " " << setw (11) << "MaxGbr"
     << " " << setw (11) << "MaxNon"
     << " " << setw (11) << "FreeGbr"
     << " " << setw (11) << "FreeNon"
     << " " << setw (11) << "UseGbrSt"
     << " " << setw (11) << "UseNonSt"
     << " " << setw (11) << "IdleSt"
     << " " << setw (11) << "UseGbrLt"
     << " " << setw (11) << "UseNonLt"
     << " " << setw (11) << "IdleLt";
  return os;
}

void
LinkInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ports [0] = 0;
  m_ports [1] = 0;
  m_channel = 0;
  Object::DoDispose ();
}

void
LinkInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Scheduling the first EWMA update.
  m_ewmaLastTime = Simulator::Now ();
  Simulator::Schedule (m_ewmaTimeout, &LinkInfo::EwmaUpdate, this);

  Object::NotifyConstructionCompleted ();
}

void
LinkInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  LinkInfo::LinkDir dir;
  dir = (context == "Forward") ? LinkInfo::FWD : LinkInfo::BWD;

  // Update TX packets for the packet slice.
  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      SliceId slice = gtpuTag.GetSliceId ();
      QosType type = gtpuTag.GetQosType ();
      uint32_t size = packet->GetSize ();

      // Update TX packets for the traffic slice and for fake shared slice,
      // considering both the traffic type and the fake both type.
      m_slices [dir][slice].txBytes [type] += size;
      m_slices [dir][slice].txBytes [QosType::BOTH] += size;
      m_slices [dir][SliceId::ALL].txBytes [type] += size;
      m_slices [dir][SliceId::ALL].txBytes [QosType::BOTH] += size;
    }
  else
    {
      NS_LOG_WARN ("GTPU packet tag not found for packet " << packet);
    }
}

bool
LinkInfo::ReleaseBitRate (
  LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate);

  // Check for reserved bit rate.
  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  if (GetResBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  // Releasing the bit rate.
  m_slices [dir][slice].reserved -= bitRate;
  m_slices [dir][SliceId::ALL].reserved -= bitRate;
  NS_LOG_DEBUG ("Releasing " << bitRate <<
                " bit rate on slice " << SliceIdStr (slice) <<
                " in " << LinkDirStr (dir) << " direction.");
  return true;
}

bool
LinkInfo::ReserveBitRate (
  LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate);

  // Check for available bit rate.
  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  if (GetFreBitRate (dir, slice, QosType::GBR) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  // Reserving the bit rate.
  m_slices [dir][slice].reserved += bitRate;
  m_slices [dir][SliceId::ALL].reserved += bitRate;
  NS_LOG_DEBUG ("Reserving " << bitRate <<
                " bit rate on slice " << SliceIdStr (slice) <<
                " in " << LinkDirStr (dir) << " direction.");
  return true;
}

bool
LinkInfo::UpdateQuota (LinkDir dir, SliceId slice, int quota)
{
  NS_LOG_FUNCTION (this << dir << slice << quota);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");

  // Check for valid slice quota.
  int newSliQuota = GetQuota (dir, slice) + quota;
  int newAggQuota = GetQuota (dir, SliceId::ALL) + quota;
  if ((GetResBitRate (dir, slice) > ((GetLinkBitRate () * newSliQuota) / 100))
      || (newSliQuota < 0 || newSliQuota > 100)
      || (newAggQuota < 0 || newAggQuota > 100))
    {
      NS_LOG_WARN ("Can't change the slice quota.");
      return false;
    }

  // Update the slice quota.
  NS_LOG_DEBUG ("Slice " << SliceIdStr (slice) <<
                " with new quota " << newSliQuota <<
                " in " << LinkDirStr (dir) << " direction.");

  m_slices [dir][slice].quota += quota;
  m_slices [dir][SliceId::ALL].quota += quota;
  return true;
}

bool
LinkInfo::UpdateExtraBitRate (LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate);

  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");

  // Check for valid slice extra bit rate.
  int64_t newSliExtra = GetExtBitRate (dir, slice) + bitRate;
  int64_t newAggExtra = GetExtBitRate (dir, SliceId::ALL) + bitRate;
  if ((newSliExtra < 0) || (newAggExtra < 0))
    {
      NS_LOG_WARN ("Can't change the slice extra bit rate.");
      return false;
    }

  // Update the slice extra bit rate.
  NS_LOG_DEBUG ("Slice " << SliceIdStr (slice) <<
                " with new extra bit rate " << newSliExtra <<
                " in " << LinkDirStr (dir) << " direction.");

  m_slices [dir][slice].extra += bitRate;
  m_slices [dir][SliceId::ALL].extra += bitRate;
  return true;
}

bool
LinkInfo::SetMeterBitRate (LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << slice << bitRate);

  if (bitRate < 0 || bitRate > GetMaxBitRate (dir, slice, QosType::NON))
    {
      NS_LOG_WARN ("Invalid meter bit rate.");
      return false;
    }
  m_slices [dir][slice].meter = bitRate;
  return true;
}

void
LinkInfo::EwmaUpdate (void)
{
  double elapSecs = (Simulator::Now () - m_ewmaLastTime).GetSeconds ();
  for (int s = 0; s <= SliceId::ALL; s++)
    {
      for (int d = 0; d <= LinkInfo::BWD; d++)
        {
          SliceMetadata &slData = m_slices [d][s];
          for (int t = 0; t <= QosType::BOTH; t++)
            {
              // Updating both long-term and short-term EWMA throughput.
              slData.ewmaThp [t][EwmaTerm::LTERM] =
                (m_ewmaLtAlpha * 8 * slData.txBytes [t]) / elapSecs +
                (1 - m_ewmaLtAlpha) * slData.ewmaThp [t][EwmaTerm::LTERM];
              slData.ewmaThp [t][EwmaTerm::STERM] =
                (m_ewmaStAlpha * 8 * slData.txBytes [t]) / elapSecs +
                (1 - m_ewmaStAlpha) * slData.ewmaThp [t][EwmaTerm::STERM];
              slData.txBytes [t] = 0;
            }
        }
    }

  // Scheduling the next EWMA update.
  m_ewmaLastTime = Simulator::Now ();
  Simulator::Schedule (m_ewmaTimeout, &LinkInfo::EwmaUpdate, this);
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
  ret = LinkInfo::m_linkInfoByDpIds.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing connection information.");

  LinkInfo::m_linkInfoList.push_back (lInfo);
}

} // namespace ns3
