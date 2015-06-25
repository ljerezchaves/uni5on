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
                                DataRate linkSpeed)
  : m_sw1 (sw1),
    m_sw2 (sw2),
    m_fwDataRate (linkSpeed),
    m_bwDataRate (linkSpeed)
{
  NS_LOG_FUNCTION (this);
}

TypeId
ConnectionInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionInfo")
    .SetParent<Object> ()
    .AddConstructor<ConnectionInfo> ()
    .AddAttribute ("FullDuplex",
                   "Whether the channel is full duplex.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ConnectionInfo::m_fullDuplex),
                   MakeBooleanChecker ())
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
  if (m_fullDuplex)
    {
      // For full duplex links, considering usage ratio in both directions.
      return (double)(m_fwReserved.GetBitRate () + m_bwReserved.GetBitRate ()) / 
                     (m_fwDataRate.GetBitRate () + m_bwDataRate.GetBitRate ());
    }
  else
    {
      return GetFowardUsageRatio ();
    }
}

double
ConnectionInfo::GetFowardUsageRatio (void) const
{
  return (double)m_fwReserved.GetBitRate () / m_fwDataRate.GetBitRate ();
}

double
ConnectionInfo::GetBackwardUsageRatio (void) const
{
  return (double)m_bwReserved.GetBitRate () / m_bwDataRate.GetBitRate ();
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
  // At first, assuming forwarding direction
  DataRate maximum = m_fwDataRate;
  DataRate reserved = m_fwReserved;
  if (m_fullDuplex)
    {
      if (srcIdx == GetSwIdxSecond () && dstIdx == GetSwIdxFirst ())
        { 
          // Backward direction.
          maximum = m_bwDataRate;
          reserved = m_bwReserved;
        }
    }
  return maximum - reserved;
}

DataRate
ConnectionInfo::GetAvailableDataRate (uint16_t srcIdx, uint16_t dstIdx, 
                                      double bwFactor) const
{
  // At first, assuming forwarding direction
  DataRate maximum = m_fwDataRate;
  DataRate reserved = m_fwReserved;
  if (m_fullDuplex)
    {
      if (srcIdx == GetSwIdxSecond () && dstIdx == GetSwIdxFirst ())
        { 
          // Backward direction.
          maximum = m_bwDataRate;
          reserved = m_bwReserved;
        }
    }
  return (maximum * (1. - bwFactor)) - reserved;
}

bool
ConnectionInfo::ReserveDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr)
{
  if (m_fullDuplex)
    {
      if (srcIdx == GetSwIdxFirst ())
        { 
          // Forward direction
          if (m_fwReserved + dr > m_fwDataRate)
            { 
              // No available bandwidth
              return false;
            }
          m_fwReserved = m_fwReserved + dr;
        }
      else
        { 
          // Backward direction
          if (m_bwReserved + dr > m_bwDataRate)
            { 
              // No available bandwidth
              return false;
            }
          m_bwReserved = m_bwReserved + dr;
        }
    }
  else
    {
      if (m_fwReserved + dr > m_fwDataRate)
        { 
          // No available bandwidth
          return false;
        }
      m_fwReserved = m_fwReserved + dr;
    }
  return true;
}

bool
ConnectionInfo::ReleaseDataRate (uint16_t srcIdx, uint16_t dstIdx, DataRate dr)
{
  if (m_fullDuplex)
    {
      if (srcIdx == GetSwIdxFirst ())
        { 
          // Forward direction
          if (m_fwReserved - dr < DataRate (0))
            { 
              // No available bandwidth
              return false;
            }
          m_fwReserved = m_fwReserved - dr;
        }
      else
        { 
          // Backward direction
          if (m_bwReserved - dr < DataRate (0))
            { 
              // No available bandwidth
              return false;
            }
          m_bwReserved = m_bwReserved - dr;
        }
    }
  else
    {
      if (m_fwReserved - dr < DataRate (0))
        { 
          // No available bandwidth
          return false;
        }
      m_fwReserved = m_fwReserved - dr;
    }
  return true;
}

};  // namespace ns3
