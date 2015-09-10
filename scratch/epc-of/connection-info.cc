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
  : m_sw1 (sw1),
    m_sw2 (sw2),
    m_channel (channel)
{
  NS_LOG_FUNCTION (this);

  // Asserting internal device order to ensure thar forward and backward
  // indexes are correct.
  NS_ASSERT_MSG ((channel->GetCsmaDevice (0) == GetPortDevFirst ()
                  && channel->GetCsmaDevice (1) == GetPortDevSecond ()),
                 "Invalid device order in csma channel.");

  // Connecting trace source to CsmaNetDevice PhyTxEnd trace source, used to
  // monitor data transmitted over this connection.
  m_sw1.portDev->TraceConnect ("PhyTxEnd", "Forward",
      MakeCallback (&ConnectionInfo::NotifyTxPacket, this));
  m_sw2.portDev->TraceConnect ("PhyTxEnd", "Backward",
      MakeCallback (&ConnectionInfo::NotifyTxPacket, this));

  ResetStatistics ();
  m_gbrReserved [0] = 0;
  m_gbrReserved [1] = 0;
}

TypeId
ConnectionInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionInfo")
    .SetParent<Object> ()
    .AddConstructor<ConnectionInfo> ()
    .AddAttribute ("GbrReserveQuota",
                   "Maximum bandwitdth ratio that can be reserved to GBR "
                   "traffic in this connection.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.4),   // 40% of link capacity
                   MakeDoubleAccessor (&ConnectionInfo::SetGbrReserveQuota),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("GbrSafeguard",
                   "Safeguard bandwidth to protect GBR from Non-GBR traffic.", 
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Mb/s")),
                   MakeDataRateAccessor (&ConnectionInfo::SetGbrSafeguard),
                   MakeDataRateChecker ())
    .AddAttribute ("NonGbrAdjustmentStep",
                   "Step value used to adjust the bandwidth that " 
                   "Non-GBR traffic is allowed to use.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("5Mb/s")),
                   MakeDataRateAccessor (&ConnectionInfo::SetNonGbrAdjStep),
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
  return SwitchPair_t (m_sw1.swIdx, m_sw2.swIdx);
}

uint16_t
ConnectionInfo::GetSwIdxFirst (void) const
{
  return m_sw1.swIdx;
}

uint16_t
ConnectionInfo::GetSwIdxSecond (void) const
{
  return m_sw2.swIdx;
}

uint32_t
ConnectionInfo::GetPortNoFirst (void) const
{
  return m_sw1.portNum;
}

uint32_t
ConnectionInfo::GetPortNoSecond (void) const
{
  return m_sw2.portNum;
}

Ptr<const OFSwitch13NetDevice>
ConnectionInfo::GetSwDevFirst (void) const
{
  return m_sw1.swDev;
}

Ptr<const OFSwitch13NetDevice>
ConnectionInfo::GetSwDevSecond (void) const
{
  return m_sw2.swDev;
}

Ptr<const CsmaNetDevice>
ConnectionInfo::GetPortDevFirst (void) const
{
  return m_sw1.portDev;
}

Ptr<const CsmaNetDevice>
ConnectionInfo::GetPortDevSecond (void) const
{
  return m_sw2.portDev;
}

double
ConnectionInfo::GetForwardGbrReservedRatio (void) const
{
  return static_cast<double>
         (m_gbrReserved [ConnectionInfo::FORWARD]) / GetLinkBitRate ();
}

double
ConnectionInfo::GetBackwardGbrReservedRatio (void) const
{
  return static_cast<double>
         (m_gbrReserved [ConnectionInfo::BACKWARD]) / GetLinkBitRate ();
}

double
ConnectionInfo::GetForwardNonGbrAllowedRatio (void) const
{
  return static_cast<double>
         (m_nonAllowed [ConnectionInfo::FORWARD]) / GetLinkBitRate ();
}

double
ConnectionInfo::GetBackwardNonGbrAllowedRatio (void) const
{
  return static_cast<double>
         (m_nonAllowed [ConnectionInfo::BACKWARD]) / GetLinkBitRate ();
}

uint32_t
ConnectionInfo::GetForwardBytes (void) const
{
  return GetForwardGbrBytes () + GetForwardNonGbrBytes ();
}

uint32_t
ConnectionInfo::GetBackwardBytes (void) const
{
  return GetBackwardGbrBytes () + GetBackwardNonGbrBytes ();
}

uint32_t
ConnectionInfo::GetForwardGbrBytes (void) const
{
  return m_gbrTxBytes [ConnectionInfo::FORWARD];
}

uint32_t
ConnectionInfo::GetBackwardGbrBytes (void) const
{
  return m_gbrTxBytes [ConnectionInfo::BACKWARD];
}

uint32_t
ConnectionInfo::GetForwardNonGbrBytes (void) const
{
  return m_nonTxBytes [ConnectionInfo::FORWARD];
}

uint32_t
ConnectionInfo::GetBackwardNonGbrBytes (void) const
{
  return m_nonTxBytes [ConnectionInfo::BACKWARD];
}

void
ConnectionInfo::ResetStatistics (void)
{
  NS_LOG_FUNCTION (this);

  m_gbrTxBytes [0] = 0;
  m_gbrTxBytes [1] = 0;
  m_nonTxBytes [0] = 0;
  m_nonTxBytes [1] = 0;
}

bool
ConnectionInfo::IsFullDuplex (void) const
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
  NS_ASSERT_MSG (((src == GetSwIdxFirst ()  && dst == GetSwIdxSecond ())
                  || (src == GetSwIdxSecond () && dst == GetSwIdxFirst ())),
                 "Invalid switch indexes for this connection.");

  if (IsFullDuplex () && src == GetSwIdxSecond ())
    {
      return ConnectionInfo::BACKWARD;
    }

  // For half-duplex channel, always return true, as we will
  // only use the forwarding path for resource reservations.
  return ConnectionInfo::FORWARD;
}

void
ConnectionInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
}

void
ConnectionInfo::NotifyTxPacket (std::string context, Ptr<const Packet> packet)
{
  ConnectionInfo::Direction dir;
  dir = (context == "Forward") ? ConnectionInfo::FORWARD :
                                 ConnectionInfo::BACKWARD;

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
ConnectionInfo::GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx) const
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);

  if (m_gbrMaxBitRate >= m_gbrReserved [dir])
    {
      return m_gbrMaxBitRate - m_gbrReserved [dir];
    }
  else
    {
      return 0;
    }
}

uint64_t
ConnectionInfo::GetAvailableGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                        double debarFactor) const
{
  NS_ASSERT_MSG (debarFactor >= 0.0, "Invalid DeBaR factor.");

  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  uint64_t maxBitRate = static_cast<uint64_t> (debarFactor * m_gbrMaxBitRate);

  if (maxBitRate >= m_gbrReserved [dir])
    {
      return maxBitRate - m_gbrReserved [dir];
    }
  else
    {
      return 0;
    }
}

bool
ConnectionInfo::ReserveGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   uint64_t bitRate)
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);

  if (m_gbrReserved [dir] + bitRate <= m_gbrMaxBitRate)
    {
      m_gbrReserved [dir] += bitRate;
      
      // When the distance between the GRB reserved bit rate and the Non-GBR
      // maximum allowed bit rate gets lower than the safeguard value, we need
      // to reduce the Non-GBR allowed bit rate by one adjustment step value.
      if (GetLinkBitRate () - m_nonAllowed [dir] < 
          m_gbrReserved [dir] + m_gbrSafeguard)
        {
          m_nonAllowed [dir] -= m_nonAdjustStep;
          
          // Fire trace source to update meters
          m_nonAdjustedTrace (this);
        }
          
      return true;
    }
  NS_FATAL_ERROR ("No bandwidth available to reserve.");
}

bool
ConnectionInfo::ReleaseGbrBitRate (uint16_t srcIdx, uint16_t dstIdx,
                                   uint64_t bitRate)
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);

  if (m_gbrReserved [dir] - bitRate >= 0)
    {
      m_gbrReserved [dir] -= bitRate;
      
      // When the distance between the GRB reserved bit rate and the Non-GBR
      // maximum allowed bit rate gets higher than the safeguard value + one
      // adjustment step, we can increase the Non-GBR allowed bit rate by one
      // adjustment step value, still respecting the safeguard value. 
      if (GetLinkBitRate () - m_nonAllowed [dir] - m_gbrSafeguard > 
          m_gbrReserved [dir] + m_nonAdjustStep)
        {
          m_nonAllowed [dir] += m_nonAdjustStep;
        
          // Fire trace source to update meters
          m_nonAdjustedTrace (this);
        }

      return true;
    }
  NS_FATAL_ERROR ("No bandwidth available to release.");
}

void
ConnectionInfo::SetGbrReserveQuota (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_gbrReserveQuota = value;
  m_gbrMaxBitRate =
    static_cast<uint64_t> (m_gbrReserveQuota * GetLinkBitRate ());
}

void
ConnectionInfo::SetGbrSafeguard (DataRate value)
{
  NS_LOG_FUNCTION (this << value);

  m_gbrSafeguard = value.GetBitRate ();
}

void
ConnectionInfo::SetNonGbrAdjStep (DataRate value)
{
  NS_LOG_FUNCTION (this << value);

  m_nonAdjustStep = value.GetBitRate ();
  m_nonAllowed [0] = GetLinkBitRate () - (m_gbrSafeguard + m_nonAdjustStep);
  m_nonAllowed [1] = GetLinkBitRate () - (m_gbrSafeguard + m_nonAdjustStep);
  
  // Fire trace source to install meters
  m_nonAdjustedTrace (this);
}

};  // namespace ns3
