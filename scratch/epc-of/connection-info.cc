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
                                DataRate linkSpeed, bool fullDuplex)
  : m_sw1 (sw1),
    m_sw2 (sw2),
    m_fwDataRate (linkSpeed),
    m_bwDataRate (linkSpeed),
    m_fullDuplex (fullDuplex) 
{
  NS_LOG_FUNCTION (this);
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

double
ConnectionInfo::GetUsageRatio (void) const
{
  return (double)m_fwReserved.GetBitRate () / m_fwDataRate.GetBitRate ();
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

void
ConnectionInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

DataRate
ConnectionInfo::GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx) const
{
  // TODO implementar lógica full duplex
  return m_fwDataRate - m_fwReserved;
}

DataRate
ConnectionInfo::GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx, 
                                      double bwFactor) const
{
  // TODO implementar lógica full duplex
  return (m_fwDataRate * (1. - bwFactor)) - m_fwReserved;
}

bool
ConnectionInfo::ReserveDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr)
{
  // TODO implementar lógica full duplex
  if (m_fwReserved + dr <= m_fwDataRate)
    {
      m_fwReserved = m_fwReserved + dr;
      return true;
    }
  return false;
}

bool
ConnectionInfo::ReleaseDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr)
{
  // TODO implementar lógica full duplex
  if (m_fwReserved - dr >= DataRate (0))
    {
      m_fwReserved = m_fwReserved - dr;
      return true;
    }
  return false;
}

};  // namespace ns3
