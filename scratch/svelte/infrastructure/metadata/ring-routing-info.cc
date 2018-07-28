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
#include "../../logical/metadata/routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingRoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this);

  AggregateObject (rInfo);
  SetDefaultPath (RingRoutingInfo::LOCAL, LteInterface::S1U);
  SetDefaultPath (RingRoutingInfo::LOCAL, LteInterface::S5);
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
RingRoutingInfo::GetDownPath (LteInterface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      return m_downPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetUpPath (LteInterface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      return RingRoutingInfo::Invert (m_downPath [iface]);
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

bool
RingRoutingInfo::IsDefaultPath (LteInterface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      return m_isDefaultPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

bool
RingRoutingInfo::IsLocalPath (LteInterface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      return m_isLocalPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

std::string
RingRoutingInfo::GetPathStr (LteInterface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  bool blocked = GetObject<RoutingInfo> ()->IsBlocked ();
  if (blocked)
    {
      return "-";
    }
  else if (IsDefaultPath (iface))
    {
      return "Shortest";
    }
  else
    {
      return "Inverted";
    }
}

uint16_t
RingRoutingInfo::GetEnbInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetEnbInfraSwIdx ();
}

uint16_t
RingRoutingInfo::GetPgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetPgwInfraSwIdx ();
}

uint16_t
RingRoutingInfo::GetSgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetSgwInfraSwIdx ();
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::Invert (RoutingPath path)
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

std::string
RingRoutingInfo::RoutingPathStr (RoutingPath path)
{
  switch (path)
    {
    case RingRoutingInfo::LOCAL:
      return "local";
    case RingRoutingInfo::CLOCK:
      return "clockwise";
    case RingRoutingInfo::COUNTER:
      return "counterclockwise";
    default:
      return "-";
    }
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
RingRoutingInfo::SetDefaultPath (RoutingPath downPath, LteInterface iface)
{
  NS_LOG_FUNCTION (this << downPath << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      m_downPath [iface] = downPath;
      m_isDefaultPath [iface] = true;
      m_isLocalPath [iface] = false;
      if (downPath == RingRoutingInfo::LOCAL)
        {
          m_isLocalPath [iface] = true;
        }
      break;
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

void
RingRoutingInfo::InvertPath (LteInterface iface)
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteInterface::S1U:
    case LteInterface::S5:
      if (m_isLocalPath [iface] == false)
        {
          m_downPath [iface] = RingRoutingInfo::Invert (m_downPath [iface]);
          m_isDefaultPath [iface] = !m_isDefaultPath [iface];
        }
      break;
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

void
RingRoutingInfo::ResetToDefaults ()
{
  NS_LOG_FUNCTION (this);

  if (m_isDefaultPath [LteInterface::S1U] == false)
    {
      InvertPath (LteInterface::S1U);
    }

  if (m_isDefaultPath [LteInterface::S5] == false)
    {
      InvertPath (LteInterface::S5);
    }
}

} // namespace ns3
