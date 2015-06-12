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

TypeId 
ConnectionInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectionInfo")
    .SetParent<Object> ()
    .AddConstructor<ConnectionInfo> ()
    .AddTraceSource ("UsageRatio",
                     "Bandwidth usage ratio trace source.",
                     MakeTraceSourceAccessor (&ConnectionInfo::m_usageTrace),
                     "ns3::ConnectionInfo::UsageTracedCallback")
  ;
  return tid;
}

void
ConnectionInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

DataRate
ConnectionInfo::GetAvailableDataRate (void) const
{
  return m_maxDataRate - m_reservedDataRate;
}

DataRate
ConnectionInfo::GetAvailableDataRate (double bwFactor) const
{
  return (m_maxDataRate * (1. - bwFactor)) - m_reservedDataRate;
}

double
ConnectionInfo::GetUsageRatio (void) const
{
  return (double)m_reservedDataRate.GetBitRate () / m_maxDataRate.GetBitRate ();
}

bool
ConnectionInfo::ReserveDataRate (DataRate dr)
{
  if (m_reservedDataRate + dr <= m_maxDataRate)
    {
      m_reservedDataRate = m_reservedDataRate + dr;
      m_usageTrace (m_switchIdx1, m_switchIdx2, GetUsageRatio ());
      return true;
    }
  return false;
}

bool
ConnectionInfo::ReleaseDataRate (DataRate dr)
{
  if (m_reservedDataRate - dr >= DataRate (0))
    {
      m_reservedDataRate = m_reservedDataRate - dr;
      m_usageTrace (m_switchIdx1, m_switchIdx2, GetUsageRatio ());
      return true;
    }
  return false;
}

};  // namespace ns3
