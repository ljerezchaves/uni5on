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
  SetIfacePath (LteIface::S1, RingInfo::LOCAL);
  SetIfacePath (LteIface::S5, RingInfo::LOCAL);
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

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return RingInfo::InvertPath (m_downPath [iface]);
}

bool
RingInfo::IsShortPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isShortPath [iface];
}

bool
RingInfo::IsLocalPath (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isLocalPath [iface];
}

bool
RingInfo::AreLocalPaths (void) const
{
  NS_LOG_FUNCTION (this);

  return (IsLocalPath (LteIface::S1) && IsLocalPath (LteIface::S5));
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
             RingInfo::COUNT :
             RingInfo::CLOCK;
    }
}

RingInfo::RingPath
RingInfo::LinkDirToRingPath (LinkInfo::LinkDir dir)
{
  return dir == LinkInfo::FWD ? RingInfo::CLOCK : RingInfo::COUNT;
}

LinkInfo::LinkDir
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
      return "clock";
    case RingInfo::COUNT:
      return "count";
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

bool
RingInfo::HasS5Link (Ptr<LinkInfo> lInfo) const
{
  NS_LOG_FUNCTION (this << lInfo);

  auto it = m_s5Links.find (lInfo);
  return (it != m_s5Links.end ());
}

void
RingInfo::ResetS5Links (void)
{
  NS_LOG_FUNCTION (this);

  m_s5Links.clear ();
}

void
RingInfo::SaveS5Link (Ptr<LinkInfo> lInfo)
{
  NS_LOG_FUNCTION (this << lInfo);

  auto ret = m_s5Links.insert (lInfo);
  NS_ABORT_MSG_IF (ret.second == false, "Error saving link info.");
}

void
RingInfo::SetIfacePath (LteIface iface, RingPath path)
{
  NS_LOG_FUNCTION (this << iface << path);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_downPath [iface] = path;
  m_isShortPath [iface] = true;
  m_isLocalPath [iface] = false;
  if (path == RingInfo::LOCAL)
    {
      m_isLocalPath [iface] = true;
    }
}

void
RingInfo::InvertIfacePath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  if (m_isLocalPath [iface] == false)
    {
      m_downPath [iface] = RingInfo::InvertPath (m_downPath [iface]);
      m_isShortPath [iface] = !m_isShortPath [iface];
    }
}

void
RingInfo::ResetIfacePath (LteIface iface)
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  if (m_isShortPath [iface] == false)
    {
      InvertIfacePath (iface);
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
