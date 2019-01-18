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

#include <iomanip>
#include <iostream>
#include "ring-info.h"
#include "routing-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingInfo);

RingInfo::RingInfo (Ptr<RoutingInfo> rInfo)
  : m_rInfo (rInfo)
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
RingInfo::GetDlPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_downPath [iface];
}

RingInfo::RingPath
RingInfo::GetUlPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return RingInfo::InvertPath (m_downPath [iface]);
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

std::string
RingInfo::GetPathStr (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_rInfo->IsBlocked ())
    {
      return "-";
    }

  std::ostringstream str;
  str << (IsDefaultPath (LteIface::S5) ? "s5:dft+" : "s5:inv+")
      << RingPathShortStr (GetDlPath (LteIface::S5))
      << "|"
      << (IsDefaultPath (LteIface::S1U) ? "s1:dft+" : "s1:inv+")
      << RingPathShortStr (GetDlPath (LteIface::S1U));

  return str.str ();
}

Ptr<RoutingInfo>
RingInfo::GetRoutingInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rInfo;
}

RingInfo::RingPath
RingInfo::InvertPath (RingPath path)
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
RingInfo::LinkDirToRingPath (LinkInfo::LinkDir dir)
{
  return dir == LinkInfo::FWD ? RingInfo::CLOCK : RingInfo::COUNTER;
}

LinkInfo::LinkDir
RingInfo::RingPathToLinkDir (RingPath path)
{
  return path == RingInfo::CLOCK ? LinkInfo::FWD : LinkInfo::BWD;
}

std::string
RingInfo::RingPathShortStr (RingPath path)
{
  switch (path)
    {
    case RingInfo::LOCAL:
      return "loc";
    case RingInfo::CLOCK:
      return "clk";
    case RingInfo::COUNTER:
      return "cnt";
    default:
      return "-";
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

std::ostream &
RingInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (22) << "RingDownPathDesc";
  return os;
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
RingInfo::InvertIfacePath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1U || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  if (m_isLocalPath [iface] == false)
    {
      m_downPath [iface] = RingInfo::InvertPath (m_downPath [iface]);
      m_isDefaultPath [iface] = !m_isDefaultPath [iface];
    }
}

void
RingInfo::ResetToDefaults ()
{
  NS_LOG_FUNCTION (this);

  if (m_isDefaultPath [LteIface::S1U] == false)
    {
      InvertIfacePath (LteIface::S1U);
    }

  if (m_isDefaultPath [LteIface::S5] == false)
    {
      InvertIfacePath (LteIface::S5);
    }
}

std::ostream & operator << (std::ostream &os, const RingInfo &ringInfo)
{
  os << " " << setw (22) << ringInfo.GetPathStr ();
  return os;
}

} // namespace ns3
