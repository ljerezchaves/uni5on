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

#include "connection-info.h"
#include <ns3/epc-gtpu-tag.h>
#include "../epc/epc-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConnectionInfo");
NS_OBJECT_ENSURE_REGISTERED (ConnectionInfo);

// Initializing ConnectionInfo static members.
ConnectionInfo::ConnInfoMap_t ConnectionInfo::m_connectionsMap;
ConnInfoList_t ConnectionInfo::m_connectionsList;

ConnectionInfo::ConnectionInfo (SwitchData sw1, SwitchData sw2,
                                Ptr<CsmaChannel> channel)
  : m_channel (channel)
{
  NS_LOG_FUNCTION (this << sw1.swDev << sw2.swDev << channel);

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

  m_gbrBitRate [0] = 0;
  m_gbrBitRate [1] = 0;
  m_nonBitRate [0] = 0;
  m_nonBitRate [1] = 0;
  m_gbrTxBytes [0] = 0;
  m_gbrTxBytes [1] = 0;
  m_nonTxBytes [0] = 0;
  m_nonTxBytes [1] = 0;
  m_gbrAvgThpt [0] = 0;
  m_gbrAvgThpt [1] = 0;
  m_nonAvgThpt [0] = 0;
  m_nonAvgThpt [1] = 0;
  m_gbrAvgLast [0] = 0;
  m_gbrAvgLast [1] = 0;
  m_nonAvgLast [0] = 0;
  m_nonAvgLast [1] = 0;

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
    .AddAttribute ("EwmaAlpha",
                   "The EWMA alpha parameter for averaging link statistics.",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&ConnectionInfo::m_alpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("GbrLinkQuota",
                   "Maximum bandwitdth ratio that can be reserved to GBR "
                   "traffic in this connection.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.4),
                   MakeDoubleAccessor (&ConnectionInfo::SetGbrLinkQuota),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("GbrSafeguard",
                   "Safeguard bandwidth to protect GBR from Non-GBR traffic.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mb/s")),
                   MakeDataRateAccessor (&ConnectionInfo::SetGbrSafeguard),
                   MakeDataRateChecker ())
    .AddAttribute ("NonGbrAdjustmentStep",
                   "Step value used to adjust the bandwidth that "
                   "Non-GBR traffic is allowed to use.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mb/s")),
                   MakeDataRateAccessor (&ConnectionInfo::SetNonGbrAdjustStep),
                   MakeDataRateChecker ())
    .AddAttribute ("UpdateTimeout",
                   "The interval to update link statistics.",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&ConnectionInfo::m_timeout),
                   MakeTimeChecker ())

    // Trace source used by controller to install/update Non-GBR meters
    .AddTraceSource ("NonGbrAdjusted",
                     "Non-GBR allowed bit rate adjusted.",
                     MakeTraceSourceAccessor (
                       &ConnectionInfo::m_nonAdjustedTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
  ;
  return tid;
}

DpIdPair_t
ConnectionInfo::GetSwitchDpIdPair (void) const
{
  NS_LOG_FUNCTION (this);

  return DpIdPair_t (GetSwDpId (0), GetSwDpId (1));
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

uint64_t
ConnectionInfo::GetGbrTxBytes (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_gbrTxBytes [dir];
}

uint64_t
ConnectionInfo::GetNonGbrTxBytes (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_nonTxBytes [dir];
}

DataRate
ConnectionInfo::GetGbrEwmaThp (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_gbrAvgThpt [dir];
}

DataRate
ConnectionInfo::GetNonGbrEwmaThp (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_nonAvgThpt [dir];
}

uint64_t
ConnectionInfo::GetResGbrBitRate (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_gbrBitRate [dir];
}

uint64_t
ConnectionInfo::GetResNonGbrBitRate (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return m_nonBitRate [dir];
}

double
ConnectionInfo::GetResGbrLinkRatio (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return static_cast<double> (GetResGbrBitRate (dir)) / GetLinkBitRate ();
}

double
ConnectionInfo::GetResNonGbrLinkRatio (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  return static_cast<double> (GetResNonGbrBitRate (dir)) / GetLinkBitRate ();
}

bool
ConnectionInfo::IsFullDuplexLink (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->IsFullDuplex ();
}

uint64_t
ConnectionInfo::GetLinkBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel->GetDataRate ().GetBitRate ();
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

  // For half-duplex channel always return true, as we will
  // only use the forwarding path for resource reservations.
  return ConnectionInfo::FWD;
}

uint64_t
ConnectionInfo::GetAvailableGbrBitRate (uint64_t src, uint64_t dst,
                                        double debarFactor) const
{
  NS_LOG_FUNCTION (this << src << dst << debarFactor);

  NS_ASSERT_MSG (debarFactor >= 0.0, "Invalid DeBaR factor.");
  ConnectionInfo::Direction dir = GetDirection (src, dst);
  uint64_t maxBitRate = static_cast<uint64_t> (debarFactor * m_gbrMaxBitRate);

  if (maxBitRate >= GetResGbrBitRate (dir))
    {
      return maxBitRate - GetResGbrBitRate (dir);
    }
  else
    {
      NS_LOG_WARN ("No bandwidth available.");
      return 0;
    }
}

bool
ConnectionInfo::ReserveGbrBitRate (uint64_t src, uint64_t dst,
                                   uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << bitRate);

  ConnectionInfo::Direction dir = GetDirection (src, dst);
  bool reserved = IncResGbrBitRate (dir, bitRate);
  if (reserved)
    {
      // When the guard distance between GRB reserved bit rate and Non-GBR
      // maximum allowed bit rate gets lower than the safeguard value, we need
      // to decrease the Non-GBR allowed bit rate.
      int adjusted = false;
      while (GetGuardBitRate (dir) < m_gbrSafeguard)
        {
          adjusted = true;
          if (!DecResNonGbrBitRate (dir, m_nonAdjustStep))
            {
              NS_ABORT_MSG ("Abort... infinite loop.");
            }
        }
      if (adjusted)
        {
          // Fire adjusted trace source to update meters.
          m_nonAdjustedTrace (this);
        }
    }
  return reserved;
}

bool
ConnectionInfo::ReleaseGbrBitRate (uint64_t src, uint64_t dst,
                                   uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << src << dst << bitRate);

  ConnectionInfo::Direction dir = GetDirection (src, dst);
  bool released = DecResGbrBitRate (dir, bitRate);
  if (released)
    {
      // When the guard distance between GRB reserved bit rate and Non-GBR
      // maximum allowed bit rate gets higher than the safeguard value + one
      // adjustment step, we need to increase the Non-GBR allowed bit rate.
      int adjusted = 0;
      while (GetGuardBitRate (dir) > m_gbrSafeguard + m_nonAdjustStep)
        {
          adjusted = true;
          if (!IncResNonGbrBitRate (dir, m_nonAdjustStep))
            {
              NS_ABORT_MSG ("Abort... infinite loop.");
            }
        }
      if (adjusted)
        {
          // Fire adjusted trace source to update meters.
          m_nonAdjustedTrace (this);
        }
    }
  return released;
}

Ptr<ConnectionInfo>
ConnectionInfo::GetPointer (uint64_t dpId1, uint64_t dpId2)
{
  NS_LOG_FUNCTION_NOARGS ();

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

ConnInfoList_t
ConnectionInfo::GetList (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  return ConnectionInfo::m_connectionsList;
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

  m_gbrMinBitRate = 0;
  m_gbrMaxBitRate = static_cast<uint64_t> (m_gbrLinkQuota * GetLinkBitRate ());
  NS_LOG_DEBUG ("GBR maximum bit rate: " << m_gbrMaxBitRate << " " <<
                "GBR minimum bit rate: " << m_gbrMinBitRate);

  m_nonMinBitRate = GetLinkBitRate () - m_gbrMaxBitRate - m_gbrSafeguard;
  m_nonMaxBitRate = GetLinkBitRate () - m_gbrSafeguard;
  NS_LOG_DEBUG ("Non-GBR maximum bit rate: " << m_nonMaxBitRate << " " <<
                "Non-GBR minimum bit rate: " << m_nonMinBitRate);

  m_nonBitRate [0] = m_nonMaxBitRate - m_nonAdjustStep;
  m_nonBitRate [1] = m_nonMaxBitRate - m_nonAdjustStep;

  // Fire adjusted trace source to update meters.
  m_nonAdjustedTrace (this);

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
      if (rInfo->IsGbr () && !rInfo->IsAggregated ())
        {
          m_gbrTxBytes [dir] += packet->GetSize ();
        }
      else
        {
          m_nonTxBytes [dir] += packet->GetSize ();
        }
    }
  else
    {
      // For the case of non-tagged packets, save bytes in Non-GBR variable.
      NS_LOG_WARN ("No GTPU packet tag found.");
      m_nonTxBytes [dir] += packet->GetSize ();
    }
}

uint64_t
ConnectionInfo::GetGuardBitRate (Direction dir) const
{
  NS_LOG_FUNCTION (this << dir);

  if (GetResGbrBitRate (dir) + GetResNonGbrBitRate (dir) > GetLinkBitRate ())
    {
      // In this temporary case, there guard interval would be 'negative', but
      // we return 0, as we are working with unsigned numbers.
      return 0;
    }
  else
    {
      return GetLinkBitRate () - GetResGbrBitRate (dir) -
             GetResNonGbrBitRate (dir);
    }
}

bool
ConnectionInfo::IncResGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << bitRate);

  if (GetResGbrBitRate (dir) + bitRate > m_gbrMaxBitRate)
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  m_gbrBitRate [dir] += bitRate;
  NS_LOG_DEBUG ("GBR bit rate adjusted to " << GetResGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::DecResGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << bitRate);

  if (GetResGbrBitRate (dir) < m_gbrMinBitRate + bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  m_gbrBitRate [dir] -= bitRate;
  NS_LOG_DEBUG ("GBR bit rate adjusted to " << GetResGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::IncResNonGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << bitRate);

  if (GetResNonGbrBitRate (dir) >= m_nonMaxBitRate)
    {
      NS_LOG_WARN ("Can't increase Non-GBR bit rate.");
      return false;
    }

  if (GetResNonGbrBitRate (dir) + bitRate > m_nonMaxBitRate)
    {
      m_nonBitRate [dir] = m_nonMaxBitRate;
    }
  else
    {
      m_nonBitRate [dir] += bitRate;
    }

  NS_LOG_DEBUG ("Non-GBR bit rate adjusted to " << GetResNonGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::DecResNonGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << dir << bitRate);

  if (GetResNonGbrBitRate (dir) <= m_nonMinBitRate)
    {
      NS_LOG_WARN ("Can't decrease Non-GBR bit rate.");
      return false;
    }

  if (GetResNonGbrBitRate (dir) < m_nonMinBitRate + bitRate)
    {
      m_nonBitRate [dir] = m_nonMinBitRate;
    }
  else
    {
      m_nonBitRate [dir] -= bitRate;
    }

  NS_LOG_DEBUG ("Non-GBR bit rate adjusted to " << GetResNonGbrBitRate (dir));
  return true;
}

void
ConnectionInfo::SetGbrLinkQuota (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_gbrLinkQuota = value;
}

void
ConnectionInfo::SetGbrSafeguard (DataRate value)
{
  NS_LOG_FUNCTION (this << value);

  m_gbrSafeguard = value.GetBitRate ();
  if (m_gbrSafeguard > GetLinkBitRate () / 10)
    {
      NS_ABORT_MSG ("GBR safeguard cannot exceed 10% of link capacity.");
    }
}

void
ConnectionInfo::SetNonGbrAdjustStep (DataRate value)
{
  NS_LOG_FUNCTION (this << value);

  m_nonAdjustStep = value.GetBitRate ();
  if (m_nonAdjustStep == 0)
    {
      NS_ABORT_MSG ("Non-GBR ajust step can't be null.");
    }
  else if (m_nonAdjustStep > GetLinkBitRate () / 5)
    {
      NS_ABORT_MSG ("Non-GBR ajust step can't exceed 20% of link capacity.");
    }
}

void
ConnectionInfo::UpdateStatistics (void)
{
  NS_LOG_FUNCTION (this);

  uint64_t gbrFwdBytes = GetGbrTxBytes (ConnectionInfo::FWD);
  uint64_t gbrBwdBytes = GetGbrTxBytes (ConnectionInfo::BWD);
  uint64_t nonFwdBytes = GetNonGbrTxBytes (ConnectionInfo::FWD);
  uint64_t nonBwdBytes = GetNonGbrTxBytes (ConnectionInfo::BWD);

  double elapSecs = (Simulator::Now () - m_lastUpdate).GetSeconds ();

  m_gbrAvgThpt [0] = (1 - m_alpha) * m_gbrAvgThpt [0] + m_alpha *
    static_cast<double> (gbrFwdBytes - m_gbrAvgLast [0]) * 8 / elapSecs;
  m_gbrAvgThpt [1] = (1 - m_alpha) * m_gbrAvgThpt [1] + m_alpha *
    static_cast<double> (gbrBwdBytes - m_gbrAvgLast [1]) * 8 / elapSecs;

  m_nonAvgThpt [0] = (1 - m_alpha) * m_nonAvgThpt [0] + m_alpha *
    static_cast<double> (nonFwdBytes - m_nonAvgLast [0]) * 8 / elapSecs;
  m_nonAvgThpt [1] = (1 - m_alpha) * m_nonAvgThpt [1] + m_alpha *
    static_cast<double> (nonBwdBytes - m_nonAvgLast [1]) * 8 / elapSecs;

  m_gbrAvgLast [0] = gbrFwdBytes;
  m_gbrAvgLast [1] = gbrBwdBytes;
  m_nonAvgLast [0] = nonFwdBytes;
  m_nonAvgLast [1] = nonBwdBytes;

  // Scheduling the next update statistics.
  m_lastUpdate = Simulator::Now ();
  Simulator::Schedule (m_timeout, &ConnectionInfo::UpdateStatistics, this);
}

void
ConnectionInfo::RegisterConnectionInfo (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

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
  NS_LOG_INFO ("New connection info saved:" <<
               " switch " << dpId1 << " port " << cInfo->GetPortNo (0) <<
               " switch " << dpId2 << " port " << cInfo->GetPortNo (1));
}

};  // namespace ns3
