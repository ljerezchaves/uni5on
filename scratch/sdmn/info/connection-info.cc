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

#define NS_LOG_APPEND_CONTEXT \
  if (m_switches [0].swDev != 0 && m_switches [1].swDev != 0) { std::clog << "[CInfo " << m_switches [0].swDev->GetDatapathId () << " to " << m_switches [1].swDev->GetDatapathId () << "] "; }

#include <ns3/epc-gtpu-tag.h>
#include "connection-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConnectionInfo");
NS_OBJECT_ENSURE_REGISTERED (ConnectionInfo);

// Initializing ConnectionInfo static members.
ConnectionInfo::ConnInfoMap_t ConnectionInfo::m_connectionsMap;
ConnInfoList_t ConnectionInfo::m_connectionsList;

ConnectionInfo::ConnectionInfo (SwitchData sw1, SwitchData sw2,
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
    "PhyTxEnd", "Forward",
    MakeCallback (&ConnectionInfo::NotifyTxPacket, this));
  m_switches [1].portDev->TraceConnect (
    "PhyTxEnd", "Backward",
    MakeCallback (&ConnectionInfo::NotifyTxPacket, this));

  // Preparing slicing metadata structures.
  memset (m_slices, 0, sizeof (SliceData) * static_cast<uint8_t> (Slice::ALL));
  m_meterBitRate [0] = 0;
  m_meterBitRate [1] = 0;

  RegisterConnectionInfo (Ptr<ConnectionInfo> (this));
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
    .AddAttribute ("AdjustmentStep",
                   "Default meter bit rate adjustment step.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mb/s")),
                   MakeDataRateAccessor (&ConnectionInfo::m_adjustmentStep),
                   MakeDataRateChecker ())
    .AddAttribute ("EwmaAlpha",
                   "The EWMA alpha parameter for averaging link statistics.",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&ConnectionInfo::m_alpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("GbrSliceQuota",
                   "Maximum bandwidth ratio for GBR slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.35),
                   MakeDoubleAccessor (&ConnectionInfo::m_gbrSliceQuota),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("MtcSliceQuota",
                   "Maximum bandwidth ratio for MTC slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.125),
                   MakeDoubleAccessor (&ConnectionInfo::m_mtcSliceQuota),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("UpdateTimeout",
                   "The interval between subsequent link statistics update.",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&ConnectionInfo::m_timeout),
                   MakeTimeChecker ())

    // Trace source used by controller to install/update slicing meters.
    .AddTraceSource ("MeterAdjusted",
                     "Default meter bit rate adjusted.",
                     MakeTraceSourceAccessor (
                       &ConnectionInfo::m_meterAdjustedTrace),
                     "ns3::ConnectionInfo::CInfoTracedCallback")
  ;
  return tid;
}

uint32_t
ConnectionInfo::GetPortNo (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].portNo;
}

uint64_t
ConnectionInfo::GetSwDpId (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return GetSwDev (idx)->GetDatapathId ();
}

Ptr<const OFSwitch13Device>
ConnectionInfo::GetSwDev (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].swDev;
}

Ptr<const CsmaNetDevice>
ConnectionInfo::GetPortDev (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return m_switches [idx].portDev;
}

Mac48Address
ConnectionInfo::GetPortMacAddr (uint8_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");
  return Mac48Address::ConvertFrom (GetPortDev (idx)->GetAddress ());
}

ConnectionInfo::Direction
ConnectionInfo::GetDirection (uint64_t src, uint64_t dst) const
{
  NS_LOG_FUNCTION (this << src << dst);

  NS_ASSERT_MSG ((src == GetSwDpId (0) && dst == GetSwDpId (1))
                 || (src == GetSwDpId (1) && dst == GetSwDpId (0)),
                 "Invalid datapath IDs for this connection.");
  if (IsFullDuplexLink () && src == GetSwDpId (1))
    {
      return ConnectionInfo::BWD;
    }

  // For half-duplex channel always return FWD, as we will
  // only use the forwarding path for resource reservations.
  return ConnectionInfo::FWD;
}

DataRate
ConnectionInfo::GetEwmaThroughput (uint64_t src, uint64_t dst,
                                   Slice slice) const
{
  NS_LOG_FUNCTION (this << src << dst << slice);

  double throughput = 0;
  ConnectionInfo::Direction dir = GetDirection (src, dst);
  if (slice >= Slice::ALL)
    {
      for (int i = 0; i < Slice::ALL; i++)
        {
          throughput += m_slices [i].ewmaThp [dir];
        }
    }
  else
    {
      throughput = m_slices [slice].ewmaThp [dir];
    }
  return DataRate (static_cast<uint64_t> (throughput));
}

double
ConnectionInfo::GetEwmaSliceUsage (uint64_t src, uint64_t dst,
                                   Slice slice) const
{
  NS_LOG_FUNCTION (this << src << dst << slice);

  return static_cast<double> (GetEwmaThroughput (src, dst, slice).GetBitRate ())
         / GetMaxBitRate (slice);
}

uint64_t
ConnectionInfo::GetLinkBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->GetDataRate ().GetBitRate ();
}

uint64_t
ConnectionInfo::GetMaxBitRate (Slice slice) const
{
  NS_LOG_FUNCTION (this << slice);

  uint64_t bitrate = 0;
  if (slice >= Slice::ALL)
    {
      bitrate = GetLinkBitRate ();
    }
  else
    {
      bitrate = m_slices [slice].maxRate;
    }
  return bitrate;
}

uint64_t
ConnectionInfo::GetMeterBitRate (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_meterBitRate [dir];
}

uint64_t
ConnectionInfo::GetResBitRate (Direction dir, Slice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  uint64_t bitrate = 0;
  if (slice >= Slice::ALL)
    {
      for (int i = 0; i < Slice::ALL; i++)
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
ConnectionInfo::GetResSliceRatio (Direction dir, Slice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  return static_cast<double> (GetResBitRate (dir, slice))
         / GetMaxBitRate (slice);
}

DpIdPair_t
ConnectionInfo::GetSwitchDpIdPair (void) const
{
  NS_LOG_FUNCTION (this);

  return DpIdPair_t (GetSwDpId (0), GetSwDpId (1));
}

uint64_t
ConnectionInfo::GetTxBytes (Direction dir, Slice slice) const
{
  NS_LOG_FUNCTION (this << dir << slice);

  uint64_t bytes = 0;
  if (slice >= Slice::ALL)
    {
      for (int i = 0; i < Slice::ALL; i++)
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
ConnectionInfo::HasBitRate (uint64_t src, uint64_t dst, Slice slice,
                            uint64_t bitRate) const
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < Slice::ALL, "Invalid slice for this operation.");
  ConnectionInfo::Direction dir = GetDirection (src, dst);

  return (GetResBitRate (dir, slice) + bitRate <= GetMaxBitRate (slice));
}

bool
ConnectionInfo::IsFullDuplexLink (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->IsFullDuplex ();
}

bool
ConnectionInfo::ReleaseBitRate (uint64_t src, uint64_t dst, Slice slice,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < Slice::ALL, "Invalid slice for this operation.");
  ConnectionInfo::Direction dir = GetDirection (src, dst);

  // Check for reserved bit rate.
  if (GetResBitRate (dir, slice) < bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  // Releasing the bit rate.
  NS_LOG_DEBUG ("Releasing bit rate on slice " << SliceStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  m_slices [slice].resRate [dir] -= bitRate;
  NS_LOG_DEBUG ("Current reserved bit rate: " << GetResBitRate (dir, slice));

  // Updating the meter bit rate.
  NS_ASSERT_MSG (GetMeterBitRate (dir) + bitRate <= GetLinkBitRate (),
                 "Invalid meter bit rate.");
  m_meterBitRate [dir] += bitRate;
  m_meterDiff [dir] += bitRate;
  NS_LOG_DEBUG ("Current meter bit rate: " << GetMeterBitRate (dir));
  NS_LOG_DEBUG ("Current meter diff: " << m_meterDiff [dir]);
  NS_LOG_DEBUG ("Current meter threshold: " << m_meterThresh);

  if (std::abs (m_meterDiff [dir]) >= std::abs (m_meterThresh))
    {
      // Fire adjusted trace source to update meters.
      NS_LOG_DEBUG ("Fire meter adjustment and clear meter diff.");
      m_meterAdjustedTrace (Ptr<ConnectionInfo> (this));
      m_meterDiff [dir] = 0;
    }
  return true;
}

bool
ConnectionInfo::ReserveBitRate (uint64_t src, uint64_t dst, Slice slice,
                                uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << slice << bitRate);

  NS_ASSERT_MSG (slice < Slice::ALL, "Invalid slice for this operation.");
  ConnectionInfo::Direction dir = GetDirection (src, dst);

  // Check for available bit rate.
  if (!HasBitRate (src, dst, slice, bitRate))
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  // Reserving the bit rate.
  NS_LOG_DEBUG ("Reserving bit rate on slice " << SliceStr (slice) <<
                " in " << DirectionStr (dir) << " direction.");
  m_slices [slice].resRate [dir] += bitRate;
  NS_LOG_DEBUG ("Current reserved bit rate: " << GetResBitRate (dir, slice));

  // Updating the meter bit rate.
  NS_ASSERT_MSG (GetMeterBitRate (dir) >= bitRate, "Invalid meter bit rate.");
  m_meterBitRate [dir] -= bitRate;
  m_meterDiff [dir] -= bitRate;
  NS_LOG_DEBUG ("Current meter bit rate: " << GetMeterBitRate (dir));
  NS_LOG_DEBUG ("Current meter diff: " << m_meterDiff [dir]);
  NS_LOG_DEBUG ("Current meter threshold: " << m_meterThresh);

  if (std::abs (m_meterDiff [dir]) >= std::abs (m_meterThresh))
    {
      // Fire adjusted trace source to update meters.
      NS_LOG_DEBUG ("Fire meter adjustment and clear meter diff.");
      m_meterAdjustedTrace (Ptr<ConnectionInfo> (this));
      m_meterDiff [dir] = 0;
    }
  return true;
}

std::string
ConnectionInfo::DirectionStr (Direction dir)
{
  switch (dir)
    {
    case ConnectionInfo::FWD:
      return "forward";
    case ConnectionInfo::BWD:
      return "backward";
    default:
      return "-";
    }
}

ConnInfoList_t
ConnectionInfo::GetList (void)
{
  return ConnectionInfo::m_connectionsList;
}

Ptr<ConnectionInfo>
ConnectionInfo::GetPointer (uint64_t dpId1, uint64_t dpId2)
{
  DpIdPair_t key;
  key.first  = std::min (dpId1, dpId2);
  key.second = std::max (dpId1, dpId2);

  Ptr<ConnectionInfo> cInfo = 0;
  ConnInfoMap_t::iterator ret;
  ret = ConnectionInfo::m_connectionsMap.find (key);
  if (ret != ConnectionInfo::m_connectionsMap.end ())
    {
      cInfo = ret->second;
    }
  return cInfo;
}

void
ConnectionInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_channel = 0;
}

void
ConnectionInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  if (m_slicing)
    {
      uint64_t mtcRate, gbrRate, dftRate;
      mtcRate = static_cast<uint64_t> (GetLinkBitRate () * m_mtcSliceQuota);
      gbrRate = static_cast<uint64_t> (GetLinkBitRate () * m_gbrSliceQuota);
      dftRate = GetLinkBitRate () - gbrRate - mtcRate;

      m_slices [Slice::MTC].maxRate = mtcRate;
      m_slices [Slice::GBR].maxRate = gbrRate;
      m_slices [Slice::DFT].maxRate = dftRate;
    }
  else
    {
      m_slices [Slice::DFT].maxRate = GetLinkBitRate ();
    }

  NS_LOG_DEBUG ("DFT maximum bit rate: " <<  m_slices [Slice::DFT].maxRate);
  NS_LOG_DEBUG ("GBR maximum bit rate: " <<  m_slices [Slice::GBR].maxRate);
  NS_LOG_DEBUG ("MTC maximum bit rate: " <<  m_slices [Slice::MTC].maxRate);

  // Set initial meter bit rate to maximum, as we don't have any reserved bit
  // rate at this moment.
  m_meterBitRate [0] = GetLinkBitRate ();
  m_meterBitRate [1] = GetLinkBitRate ();
  m_meterDiff [0] = 0;
  m_meterDiff [1] = 0;
  m_meterThresh = m_adjustmentStep.GetBitRate ();

  // Fire the adjusted trace source to create the meters.
  m_meterAdjustedTrace (Ptr<ConnectionInfo> (this));

  // Scheduling the first update statistics.
  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (m_timeout, &ConnectionInfo::UpdateStatistics, this);

  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
ConnectionInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  ConnectionInfo::Direction dir;
  dir = (context == "Forward") ? ConnectionInfo::FWD : ConnectionInfo::BWD;

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (gtpuTag.GetTeid ());
      m_slices [rInfo->GetSlice ()].txBytes [dir] += packet->GetSize ();
    }
  else
    {
      // For the case of non-tagged packets, save bytes in default slice.
      NS_LOG_WARN ("No GTPU packet tag found.");
      m_slices [Slice::DFT].txBytes [dir] += packet->GetSize ();
    }
}

void
ConnectionInfo::UpdateStatistics (void)
{
  NS_LOG_FUNCTION (this);

  double elapSecs = (Simulator::Now () - m_lastUpdate).GetSeconds ();
  for (int s = 0; s < Slice::ALL; s++)
    {
      for (int d = 0; d <= ConnectionInfo::BWD; d++)
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
  Simulator::Schedule (m_timeout, &ConnectionInfo::UpdateStatistics, this);
}

void
ConnectionInfo::RegisterConnectionInfo (Ptr<ConnectionInfo> cInfo)
{
  // Respecting the increasing switch index order when saving connection data.
  uint16_t dpId1 = cInfo->GetSwDpId (0);
  uint16_t dpId2 = cInfo->GetSwDpId (1);

  DpIdPair_t key;
  key.first  = std::min (dpId1, dpId2);
  key.second = std::max (dpId1, dpId2);

  std::pair<DpIdPair_t, Ptr<ConnectionInfo> > entry (key, cInfo);
  std::pair<ConnInfoMap_t::iterator, bool> ret;
  ret = ConnectionInfo::m_connectionsMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing connection information.");
    }

  ConnectionInfo::m_connectionsList.push_back (cInfo);
  // NS_LOG_INFO ("New connection info saved:" <<
  //              " switch " << dpId1 << " port " << cInfo->GetPortNo (0) <<
  //              " switch " << dpId2 << " port " << cInfo->GetPortNo (1));
}

};  // namespace ns3
