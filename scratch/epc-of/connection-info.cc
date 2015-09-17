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
#include "openflow-epc-controller.h"
#include "ns3/epc-gtpu-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConnectionInfo");
NS_OBJECT_ENSURE_REGISTERED (ConnectionInfo);

ConnectionInfo::ConnectionInfo ()
{
  NS_LOG_FUNCTION (this);
}

ConnectionInfo::~ConnectionInfo ()
{
  NS_LOG_FUNCTION (this);
}

ConnectionInfo::ConnectionInfo (SwitchData sw1, SwitchData sw2,
                                Ptr<CsmaChannel> channel)
  : m_channel (channel)
{
  NS_LOG_FUNCTION (this << sw1.swIdx << sw2.swIdx);

  m_switches [0] = sw1;
  m_switches [1] = sw2;

  // Asserting internal device order to ensure thar forward and backward
  // indexes are correct.
  NS_ASSERT_MSG (channel->GetCsmaDevice (0) == GetPortDev (0)
                 && channel->GetCsmaDevice (1) == GetPortDev (1),
                 "Invalid device order in csma channel.");

  // Connecting trace source to CsmaNetDevice PhyTxEnd trace source, used to
  // monitor data transmitted over this connection.
  m_switches [0].portDev->TraceConnect ("PhyTxEnd", "Forward",
      MakeCallback (&ConnectionInfo::NotifyTxPacket, this));
  m_switches [1].portDev->TraceConnect ("PhyTxEnd", "Backward",
      MakeCallback (&ConnectionInfo::NotifyTxPacket, this));

  m_gbrBitRate [0] = 0;
  m_gbrBitRate [1] = 0;
  m_nonBitRate [0] = 0;
  m_nonBitRate [1] = 0;
  ResetTxBytes ();
}

TypeId
ConnectionInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionInfo")
    .SetParent<Object> ()
    .AddConstructor<ConnectionInfo> ()
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

    // Trace source used by controller to install/update Non-GBR meters
    .AddTraceSource ("NonGbrAdjusted",
                     "Non-GBR allowed bit rate adjusted.",
                     MakeTraceSourceAccessor (&ConnectionInfo::m_nonAdjustedTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
  ;
  return tid;
}

SwitchPair_t
ConnectionInfo::GetSwitchIndexPair (void) const
{
  return SwitchPair_t (GetSwIdx (0), GetSwIdx (1));
}

uint16_t
ConnectionInfo::GetSwIdx (uint8_t idx) const
{
  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");

  return m_switches [idx].swIdx;
}

uint32_t
ConnectionInfo::GetPortNo (uint8_t idx) const
{
  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");

  return m_switches [idx].portNum;
}

Ptr<const OFSwitch13NetDevice>
ConnectionInfo::GetSwDev (uint8_t idx) const
{
  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");

  return m_switches [idx].swDev;
}

Ptr<const CsmaNetDevice>
ConnectionInfo::GetPortDev (uint8_t idx) const
{
  NS_ASSERT_MSG (idx == 0 || idx == 1, "Invalid switch index.");

  return m_switches [idx].portDev;
}

uint32_t
ConnectionInfo::GetGbrBytes (Direction dir) const
{
  return m_gbrTxBytes [dir];
}

uint64_t
ConnectionInfo::GetGbrBitRate (Direction dir) const
{
  return m_gbrBitRate [dir];
}

double
ConnectionInfo::GetGbrLinkRatio (Direction dir) const
{
  return static_cast<double> (GetGbrBitRate (dir)) / GetLinkBitRate ();
}

uint32_t
ConnectionInfo::GetNonGbrBytes (Direction dir) const
{
  return m_nonTxBytes [dir];
}

uint64_t
ConnectionInfo::GetNonGbrBitRate (Direction dir) const
{
  return m_nonBitRate [dir];
}

double
ConnectionInfo::GetNonGbrLinkRatio (Direction dir) const
{
  return static_cast<double> (GetNonGbrBitRate (dir)) / GetLinkBitRate ();
}

void
ConnectionInfo::ResetTxBytes (void)
{
  NS_LOG_FUNCTION (this);

  m_gbrTxBytes [0] = 0;
  m_gbrTxBytes [1] = 0;
  m_nonTxBytes [0] = 0;
  m_nonTxBytes [1] = 0;
}

bool
ConnectionInfo::IsFullDuplexLink (void) const
{
  return m_channel->IsFullDuplex ();
}

uint64_t
ConnectionInfo::GetLinkBitRate (void) const
{
  return m_channel->GetDataRate ().GetBitRate ();
}

ConnectionInfo::Direction
ConnectionInfo::GetDirection (uint16_t src, uint16_t dst) const
{
  NS_ASSERT_MSG ((src == GetSwIdx (0) && dst == GetSwIdx (1))
                 || (src == GetSwIdx (1) && dst == GetSwIdx (0)),
                 "Invalid switch indexes for this connection.");

  if (IsFullDuplexLink () && src == GetSwIdx (1))
    {
      return ConnectionInfo::BWD;
    }

  // For half-duplex channel always return true, as we will
  // only use the forwarding path for resource reservations.
  return ConnectionInfo::FWD;
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
  Object::NotifyConstructionCompleted ();

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
}

void
ConnectionInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  ConnectionInfo::Direction dir;
  dir = (context == "Forward") ? ConnectionInfo::FWD : ConnectionInfo::BWD;

  EpcGtpuTag gtpuTag;
  if (packet->PeekPacketTag (gtpuTag))
    {
      EpsBearer bearer =
        OpenFlowEpcController::GetEpsBearer (gtpuTag.GetTeid ());
      if (bearer.IsGbr ())
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
ConnectionInfo::GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                        double debarFactor) const
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << debarFactor);

  NS_ASSERT_MSG (debarFactor >= 0.0, "Invalid DeBaR factor.");

  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  uint64_t maxBitRate = static_cast<uint64_t> (debarFactor * m_gbrMaxBitRate);

  if (maxBitRate >= GetGbrBitRate (dir))
    {
      return maxBitRate - GetGbrBitRate (dir);
    }
  else
    {
      NS_LOG_DEBUG ("No bandwidth available.");
      return 0;
    }
}

uint64_t
ConnectionInfo::GetGuardBitRate (Direction dir) const
{
  if (GetGbrBitRate (dir) + GetNonGbrBitRate (dir) > GetLinkBitRate ())
    {
      // In this temporary case, there guard interval would be 'negative', but
      // we return 0, as we are working with unsigned numbers.
      return 0;
    }
  else
    {
      return GetLinkBitRate () - GetGbrBitRate (dir) - GetNonGbrBitRate (dir);
    }
}

bool
ConnectionInfo::ReserveGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << bitRate);

  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  bool reserved = IncreaseGbrBitRate (dir, bitRate);
  if (reserved)
    {
      // When the guard distance between GRB reserved bit rate and Non-GBR
      // maximum allowed bit rate gets lower than the safeguard value, we need
      // to decrease the Non-GBR allowed bit rate.
      int adjusted = false;
      while (GetGuardBitRate (dir) <= m_gbrSafeguard)
        {
          adjusted = true;
          if (!DecreaseNonGbrBitRate (dir, m_nonAdjustStep))
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
ConnectionInfo::ReleaseGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   uint64_t bitRate)
{
  NS_LOG_FUNCTION (this << srcIdx << dstIdx << bitRate);

  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  bool released = DecreaseGbrBitRate (dir, bitRate);
  if (released)
    {
      // When the guard distance between GRB reserved bit rate and Non-GBR
      // maximum allowed bit rate gets higher than the safeguard value + one
      // adjustment step, we need to increase the Non-GBR allowed bit rate.
      int adjusted = 0;
      while (GetGuardBitRate (dir) >= m_gbrSafeguard + m_nonAdjustStep)
        {
          adjusted = true;
          if (!IncreaseNonGbrBitRate (dir, m_nonAdjustStep))
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

bool
ConnectionInfo::IncreaseGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this);

  if (GetGbrBitRate (dir) + bitRate >= m_gbrMaxBitRate)
    {
      NS_LOG_WARN ("No bandwidth available to reserve.");
      return false;
    }

  m_gbrBitRate [dir] += bitRate;
  NS_LOG_DEBUG ("GBR bit rate adjusted to " << GetGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::DecreaseGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this);

  if (GetGbrBitRate (dir) < m_gbrMinBitRate + bitRate)
    {
      NS_LOG_WARN ("No bandwidth available to release.");
      return false;
    }

  m_gbrBitRate [dir] -= bitRate;
  NS_LOG_DEBUG ("GBR bit rate adjusted to " << GetGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::IncreaseNonGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this);

  if (GetNonGbrBitRate (dir) == m_nonMaxBitRate)
    {
      NS_LOG_DEBUG ("Can't increase Non-GBR bit rate.");
      return false;
    }

  if (GetNonGbrBitRate (dir) + bitRate > m_nonMaxBitRate)
    {
      m_nonBitRate [dir] = m_nonMaxBitRate;
    }
  else
    {
      m_nonBitRate [dir] += bitRate;
    }

  NS_LOG_DEBUG ("Non-GBR bit rate adjusted to " << GetNonGbrBitRate (dir));
  return true;
}

bool
ConnectionInfo::DecreaseNonGbrBitRate (Direction dir, uint64_t bitRate)
{
  NS_LOG_FUNCTION (this);

  if (GetNonGbrBitRate (dir) == m_nonMinBitRate)
    {
      NS_LOG_DEBUG ("Can't decrease Non-GBR bit rate.");
      return false;
    }

  if (GetNonGbrBitRate (dir) < m_nonMinBitRate + bitRate)
    {
      m_nonBitRate [dir] = m_nonMinBitRate;
    }
  else
    {
      m_nonBitRate [dir] -= bitRate;
    }

  NS_LOG_DEBUG ("Non-GBR bit rate adjusted to " << GetNonGbrBitRate (dir));
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

};  // namespace ns3
