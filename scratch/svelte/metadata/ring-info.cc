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

  NS_ASSERT_MSG ((LteIface::S1 == 0 && LteIface::S5 == 1)
                 || (LteIface::S5 == 0 && LteIface::S1 == 1),
                 "Incompatible LteIface enum values.");

  AggregateObject (rInfo);
  m_downPath [LteIface::S1] = RingInfo::UNDEF;
  m_downPath [LteIface::S5] = RingInfo::UNDEF;
  m_shortPath [LteIface::S1] = true;
  m_shortPath [LteIface::S5] = true;
  m_instRules [LteIface::S1] = false;
  m_instRules [LteIface::S5] = false;
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

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_downPath [iface];
}

RingInfo::RingPath
RingInfo::GetUlPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return RingInfo::InvertPath (GetDlPath (iface));
}

bool
RingInfo::IsInstalled (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_instRules [iface];
}

bool
RingInfo::IsLocalPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return (GetDlPath (iface) == RingInfo::LOCAL);
}

bool
RingInfo::IsShortPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_shortPath [iface];
}

bool
RingInfo::IsUndefPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return (GetDlPath (iface) == RingInfo::UNDEF);
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
  switch (path)
    {
    case RingInfo::CLOCK:
      return RingInfo::COUNT;
    case RingInfo::COUNT:
      return RingInfo::CLOCK;
    case RingInfo::LOCAL:
      return RingInfo::LOCAL;
    default:
      return RingInfo::UNDEF;
    }
}

RingInfo::RingPath
RingInfo::LinkDirToRingPath (LinkInfo::LinkDir dir)
{
  switch (dir)
    {
    case LinkInfo::FWD:
      return RingInfo::CLOCK;
    case LinkInfo::BWD:
      return RingInfo::COUNT;
    default:
      return RingInfo::UNDEF;
    }
}

std::string
RingInfo::RingPathStr (RingPath path)
{
  switch (path)
    {
    case RingInfo::UNDEF:
      return "undef";
    case RingInfo::CLOCK:
      return "clock";
    case RingInfo::COUNT:
      return "count";
    case RingInfo::LOCAL:
      return "local";
    default:
      return "-";
    }
}

std::ostream &
RingInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (7) << "S1Shor"
     << " " << setw (7) << "S1Path"
     << " " << setw (7) << "S5Shor"
     << " " << setw (7) << "S5Path";
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
RingInfo::SetInstalled (LteIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_instRules [iface] = value;
}

void
RingInfo::SetShortDlPath (LteIface iface, RingPath path)
{
  NS_LOG_FUNCTION (this << iface << path);

  NS_ASSERT_MSG (path != RingInfo::UNDEF, "Invalid ring routing path.");
  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_downPath [iface] = path;
  m_shortPath [iface] = true;
}

void
RingInfo::InvertPath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  if (!IsLocalPath (iface) && !IsUndefPath (iface))
    {
      m_downPath [iface] = RingInfo::InvertPath (m_downPath [iface]);
      m_shortPath [iface] = !m_shortPath [iface];
    }
}

void
RingInfo::ResetPath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  if (!IsShortPath (iface))
    {
      InvertPath (iface);
    }
}

std::ostream & operator << (std::ostream &os, const RingInfo &ringInfo)
{
  if (ringInfo.GetRoutingInfo ()->IsBlocked ())
    {
      os << " " << setw (7) << "-"
         << " " << setw (7) << "-"
         << " " << setw (7) << "-"
         << " " << setw (7) << "-";
    }
  else
    {
      RingInfo::RingPath s1Path = ringInfo.GetDlPath (LteIface::S1);
      RingInfo::RingPath s5Path = ringInfo.GetDlPath (LteIface::S5);
      os << " " << setw (7) << ringInfo.IsShortPath (LteIface::S1)
         << " " << setw (7) << RingInfo::RingPathStr (s1Path)
         << " " << setw (7) << ringInfo.IsShortPath (LteIface::S5)
         << " " << setw (7) << RingInfo::RingPathStr (s5Path);
    }
  return os;
}

} // namespace ns3
