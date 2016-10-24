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

RingRoutingInfo::RingRoutingInfo ()
  : m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo,
                                  RoutingPath shortDownPath)
  : m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);
  m_downPath   = shortDownPath;
  m_upPath     = RingRoutingInfo::InvertPath (shortDownPath);
  m_isInverted = false;
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
    .AddConstructor<RingRoutingInfo> ()
  ;
  return tid;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::InvertPath (RoutingPath path)
{
  return path == RingRoutingInfo::CLOCK ?
         RingRoutingInfo::COUNTER :
         RingRoutingInfo::CLOCK;
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
RingRoutingInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

bool
RingRoutingInfo::IsInverted (void) const
{
  return m_isInverted;
}

uint16_t
RingRoutingInfo::GetSgwSwIdx (void) const
{
  return m_rInfo->GetSgwSwIdx ();
}

uint16_t
RingRoutingInfo::GetEnbSwIdx (void) const
{
  return m_rInfo->GetEnbSwIdx ();
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetDownPath (void) const
{
  return m_downPath;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetUpPath (void) const
{
  return m_upPath;
}

std::string
RingRoutingInfo::GetPathDesc (void) const
{
  if (IsInverted ())
    {
      return "Inverted";
    }
  else
    {
      return "Shortest";
    }
}

void
RingRoutingInfo::InvertPaths ()
{
  m_downPath = RingRoutingInfo::InvertPath (m_downPath);
  m_upPath   = RingRoutingInfo::InvertPath (m_upPath);
  m_isInverted = !m_isInverted;
}

void
RingRoutingInfo::ResetToShortestPaths ()
{
  NS_LOG_FUNCTION (this);

  if (IsInverted ())
    {
      InvertPaths ();
    }
}

};  // namespace ns3
