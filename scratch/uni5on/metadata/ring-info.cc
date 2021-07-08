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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <iomanip>
#include <iostream>
#include "ring-info.h"
#include "bearer-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingInfo);

RingInfo::RingInfo (Ptr<BearerInfo> bInfo)
  : m_bInfo (bInfo)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG ((EpsIface::S1 == 0 && EpsIface::S5 == 1)
                 || (EpsIface::S5 == 0 && EpsIface::S1 == 1),
                 "Incompatible EpsIface enum values.");

  AggregateObject (bInfo);
  m_downPath [EpsIface::S1] = RingInfo::UNDEF;
  m_downPath [EpsIface::S5] = RingInfo::UNDEF;
  m_shortPath [EpsIface::S1] = true;
  m_shortPath [EpsIface::S5] = true;
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
RingInfo::GetDlPath (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  return m_downPath [iface];
}

RingInfo::RingPath
RingInfo::GetUlPath (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return RingInfo::InvertPath (GetDlPath (iface));
}

bool
RingInfo::IsLocalPath (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return (GetDlPath (iface) == RingInfo::LOCAL);
}

bool
RingInfo::IsShortPath (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  return m_shortPath [iface];
}

bool
RingInfo::IsUndefPath (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return (GetDlPath (iface) == RingInfo::UNDEF);
}

Ptr<BearerInfo>
RingInfo::GetBearerInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bInfo;
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

  m_bInfo = 0;
  Object::DoDispose ();
}

void
RingInfo::SetShortDlPath (EpsIface iface, RingPath path)
{
  NS_LOG_FUNCTION (this << iface << path);

  NS_ASSERT_MSG (path != RingInfo::UNDEF, "Invalid ring routing path.");
  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  m_downPath [iface] = path;
  m_shortPath [iface] = true;
}

void
RingInfo::InvertPath (EpsIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  if (!IsLocalPath (iface) && !IsUndefPath (iface))
    {
      m_downPath [iface] = RingInfo::InvertPath (m_downPath [iface]);
      m_shortPath [iface] = !m_shortPath [iface];
    }
}

void
RingInfo::ResetPath (EpsIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  if (!IsShortPath (iface))
    {
      InvertPath (iface);
    }
}

std::ostream & operator << (std::ostream &os, const RingInfo &ringInfo)
{
  if (ringInfo.GetBearerInfo ()->IsBlocked ())
    {
      os << " " << setw (7) << "-"
         << " " << setw (7) << "-"
         << " " << setw (7) << "-"
         << " " << setw (7) << "-";
    }
  else
    {
      RingInfo::RingPath s1Path = ringInfo.GetDlPath (EpsIface::S1);
      RingInfo::RingPath s5Path = ringInfo.GetDlPath (EpsIface::S5);
      os << " " << setw (7) << ringInfo.IsShortPath (EpsIface::S1)
         << " " << setw (7) << RingInfo::RingPathStr (s1Path)
         << " " << setw (7) << ringInfo.IsShortPath (EpsIface::S5)
         << " " << setw (7) << RingInfo::RingPathStr (s5Path);
    }
  return os;
}

} // namespace ns3
