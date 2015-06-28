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
  NS_ASSERT_MSG ((channel->GetCsmaDevice (0) == GetPortDevFirst () &&
                  channel->GetCsmaDevice (1) == GetPortDevSecond ()),
                  "Invalid device order in csma channel.");
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
ConnectionInfo::GetReservedRatio (void) const
{
  if (IsFullDuplex ())
    {
      // For full duplex links, considering usage ratio in both directions.
      return (double)
        ((m_reserved [0] + m_reserved [1]).GetBitRate ()) / 
         (LinkDataRate ().GetBitRate () * 2);
    }
  else
    {
      return GetFowardReservedRatio ();
    }
}

double
ConnectionInfo::GetFowardReservedRatio (void) const
{
  return (double)
    (m_reserved [ConnectionInfo::FORWARD]).GetBitRate () / 
    (LinkDataRate ().GetBitRate ());
}

double
ConnectionInfo::GetBackwardReservedRatio (void) const
{
  return (double)
    (m_reserved [ConnectionInfo::BACKWARD]).GetBitRate () / 
    (LinkDataRate ().GetBitRate ());
}

bool
ConnectionInfo::IsFullDuplex (void) const
{
  return m_channel->IsFullDuplex ();
}

DataRate
ConnectionInfo::LinkDataRate (void) const
{
  return m_channel->GetDataRate ();
}

ConnectionInfo::Direction
ConnectionInfo::GetDirection (uint16_t src, uint16_t dst) const
{
  NS_ASSERT_MSG (((src == GetSwIdxFirst ()  && dst == GetSwIdxSecond ()) ||
                  (src == GetSwIdxSecond () && dst == GetSwIdxFirst ())),
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

DataRate
ConnectionInfo::GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx) const
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  return LinkDataRate () - m_reserved [dir];
}

DataRate
ConnectionInfo::GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx, 
                                      double maxBwFactor) const
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);
  return (LinkDataRate () * maxBwFactor) - m_reserved [dir];
}

bool
ConnectionInfo::ReserveDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate rate)
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);

  if (m_reserved [dir] + rate > LinkDataRate ())
    {
      return false;
    }
  
  m_reserved [dir] = m_reserved [dir] + rate;
  return true;
}

bool
ConnectionInfo::ReleaseDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate rate)
{
  ConnectionInfo::Direction dir = GetDirection (srcIdx, dstIdx);

  if (m_reserved [dir] - rate < DataRate (0))
    {
      return false;
    }
  
  m_reserved [dir] = m_reserved [dir] - rate;
  return true;
}

};  // namespace ns3
