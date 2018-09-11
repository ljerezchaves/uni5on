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

  m_rInfo = rInfo;
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

Ptr<RoutingInfo>
RingInfo::GetRoutingInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rInfo;
}

RingInfo::RingPath
RingInfo::GetDownPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_downPath [iface];
}

RingInfo::RingPath
RingInfo::GetUpPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return RingInfo::Invert (m_downPath [iface]);
}

bool
RingInfo::IsDefaultPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isDefaultPath [iface];
}

bool
RingInfo::IsLocalPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isLocalPath [iface];
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

  bool blocked = m_rInfo->IsBlocked ();
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

RingInfo::RingPath
RingInfo::LinkDirToRingPath (LinkInfo::Direction dir)
{
  return dir == LinkInfo::FWD ? RingInfo::CLOCK : RingInfo::COUNTER;
}

LinkInfo::Direction
RingInfo::RingPathToLinkDir (RingPath path)
{
  return path == RingInfo::CLOCK ? LinkInfo::FWD : LinkInfo::BWD;
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

  m_rInfo = 0;
  Object::DoDispose ();
}

void
RingInfo::SetDefaultPath (RingPath downPath, LteIface iface)
{
  NS_LOG_FUNCTION (this << downPath << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_downPath [iface] = downPath;
  m_isDefaultPath [iface] = true;
  m_isLocalPath [iface] = false;
  if (downPath == RingInfo::LOCAL)
    {
      m_isLocalPath [iface] = true;
    }
}

void
RingInfo::InvertPath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  if (m_isLocalPath [iface] == false)
    {
      m_downPath [iface] = RingInfo::Invert (m_downPath [iface]);
      m_isDefaultPath [iface] = !m_isDefaultPath [iface];
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
