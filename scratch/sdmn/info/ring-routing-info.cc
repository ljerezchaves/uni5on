/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include "ring-routing-info.h"
#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingRoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo)
  : m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);

  SetDefaultPaths (RingRoutingInfo::LOCAL, RingRoutingInfo::LOCAL);
}

RingRoutingInfo::~RingRoutingInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RingRoutingInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingRoutingInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetDownPath (void) const
{
  NS_LOG_FUNCTION (this);

  return m_downPath;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetUpPath (void) const
{
  NS_LOG_FUNCTION (this);

  return m_upPath;
}

uint16_t
RingRoutingInfo::GetPgwSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwIdx;
}

uint16_t
RingRoutingInfo::GetSgwSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwIdx;
}

uint64_t
RingRoutingInfo::GetPgwSwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwDpId;
}

uint64_t
RingRoutingInfo::GetSgwSwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwDpId;
}

bool
RingRoutingInfo::IsDefaultPath (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDefaultPath;
}

bool
RingRoutingInfo::IsLocalPath (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isLocalPath;
}

Ptr<RoutingInfo>
RingRoutingInfo::GetRoutingInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rInfo;
}

void
RingRoutingInfo::SetPgwSwIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_pgwIdx = value;
}

void
RingRoutingInfo::SetSgwSwIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwIdx = value;
}

void
RingRoutingInfo::SetPgwSwDpId (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_pgwDpId = value;
}

void
RingRoutingInfo::SetSgwSwDpId (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwDpId = value;
}

void
RingRoutingInfo::SetDefaultPaths (RoutingPath downPath, RoutingPath upPath)
{
  NS_LOG_FUNCTION (this << downPath << upPath);

  m_downPath = downPath;
  m_upPath = upPath;
  m_isDefaultPath = true;
  m_isLocalPath = false;

  // Check for local routing paths
  if (downPath == RingRoutingInfo::LOCAL)
    {
      NS_ASSERT_MSG (upPath == RingRoutingInfo::LOCAL, "For local ring routing"
                     " both downlink and uplink paths must be set to LOCAL.");
      m_isLocalPath = true;
    }
}

void
RingRoutingInfo::InvertBothPaths ()
{
  NS_LOG_FUNCTION (this);

  if (m_isLocalPath == false)
    {
      m_downPath = RingRoutingInfo::InvertPath (m_downPath);
      m_upPath   = RingRoutingInfo::InvertPath (m_upPath);
      m_isDefaultPath = !m_isDefaultPath;
    }
}

void
RingRoutingInfo::ResetToDefaultPaths ()
{
  NS_LOG_FUNCTION (this);

  if (m_isDefaultPath == false)
    {
      InvertBothPaths ();
    }
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::InvertPath (RoutingPath path)
{
  if (path == RingRoutingInfo::LOCAL)
    {
      return RingRoutingInfo::LOCAL;
    }
  else
    {
      return path == RingRoutingInfo::CLOCK ?
             RingRoutingInfo::COUNTER :
             RingRoutingInfo::CLOCK;
    }
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rInfo = 0;
}

};  // namespace ns3
