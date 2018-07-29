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

#include "ring-info.h"
#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingInfo);

RingInfo::RingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG ((LteIface::S1U == 0 && LteIface::S5 == 1)
                 || (LteIface::S5 == 0 && LteIface::S1U == 1),
                 "Incompatible LteIface enum values.");

  AggregateObject (rInfo);
  SetDefaultPath (RingInfo::LOCAL, LteIface::S1U);
  SetDefaultPath (RingInfo::LOCAL, LteIface::S5);
}

RingInfo::~RingInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RingInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

RingInfo::RingPath
RingInfo::GetDownPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      return m_downPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

RingInfo::RingPath
RingInfo::GetUpPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      return RingInfo::Invert (m_downPath [iface]);
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

bool
RingInfo::IsDefaultPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      return m_isDefaultPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

bool
RingInfo::IsLocalPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      return m_isLocalPath [iface];
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

bool
RingInfo::IsLocalPath (void) const
{
  NS_LOG_FUNCTION (this);

  return (IsLocalPath (LteIface::S1U) && IsLocalPath (LteIface::S5));
}

std::string
RingInfo::GetPathStr (LteIface iface) const
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
RingInfo::GetEnbInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetEnbInfraSwIdx ();
}

uint16_t
RingInfo::GetPgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetPgwInfraSwIdx ();
}

uint16_t
RingInfo::GetSgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<RoutingInfo> ()->GetSgwInfraSwIdx ();
}

RingInfo::RingPath
RingInfo::Invert (RingPath path)
{
  if (path == RingInfo::LOCAL)
    {
      return RingInfo::LOCAL;
    }
  else
    {
      return path == RingInfo::CLOCK ?
             RingInfo::COUNTER :
             RingInfo::CLOCK;
    }
}

std::string
RingInfo::RingPathStr (RingPath path)
{
  switch (path)
    {
    case RingInfo::LOCAL:
      return "local";
    case RingInfo::CLOCK:
      return "clockwise";
    case RingInfo::COUNTER:
      return "counterclockwise";
    default:
      return "-";
    }
}

void
RingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
RingInfo::SetDefaultPath (RingPath downPath, LteIface iface)
{
  NS_LOG_FUNCTION (this << downPath << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      m_downPath [iface] = downPath;
      m_isDefaultPath [iface] = true;
      m_isLocalPath [iface] = false;
      if (downPath == RingInfo::LOCAL)
        {
          m_isLocalPath [iface] = true;
        }
      break;
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

void
RingInfo::InvertPath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  switch (iface)
    {
    case LteIface::S1U:
    case LteIface::S5:
      if (m_isLocalPath [iface] == false)
        {
          m_downPath [iface] = RingInfo::Invert (m_downPath [iface]);
          m_isDefaultPath [iface] = !m_isDefaultPath [iface];
        }
      break;
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }
}

void
RingInfo::ResetToDefaults ()
{
  NS_LOG_FUNCTION (this);

  if (m_isDefaultPath [LteIface::S1U] == false)
    {
      InvertPath (LteIface::S1U);
    }

  if (m_isDefaultPath [LteIface::S5] == false)
    {
      InvertPath (LteIface::S5);
    }
}

} // namespace ns3
