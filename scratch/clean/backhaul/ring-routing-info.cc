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
// #include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RingRoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);

// RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this);
// 
//   AggregateObject (rInfo);
//   SetDefaultPath (RingRoutingInfo::LOCAL);
// }

RingRoutingInfo::RingRoutingInfo ()
{
  NS_LOG_FUNCTION (this);

//  SetDefaultPath (RingRoutingInfo::LOCAL);
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

// RingRoutingInfo::RoutingPath
// RingRoutingInfo::GetDownPath (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_downPath;
// }
// 
// RingRoutingInfo::RoutingPath
// RingRoutingInfo::GetUpPath (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return RingRoutingInfo::Invert (m_downPath);
// }
// 
// uint16_t
// RingRoutingInfo::GetPgwSwIdx (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_pgwIdx;
// }
// 
// uint16_t
// RingRoutingInfo::GetSgwSwIdx (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_sgwIdx;
// }
// 
// uint64_t
// RingRoutingInfo::GetPgwSwDpId (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_pgwDpId;
// }
// 
// uint64_t
// RingRoutingInfo::GetSgwSwDpId (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_sgwDpId;
// }
// 
// bool
// RingRoutingInfo::IsDefaultPath (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_isDefaultPath;
// }
// 
// bool
// RingRoutingInfo::IsLocalPath (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   return m_isLocalPath;
// }
// 
// std::string
// RingRoutingInfo::GetPathStr (void) const
// {
//   NS_LOG_FUNCTION (this);
// 
//   bool blocked = GetObject<RoutingInfo> ()->IsBlocked ();
//   if (blocked)
//     {
//       return "-";
//     }
//   else if (IsDefaultPath ())
//     {
//       return "Shortest";
//     }
//   else
//     {
//       return "Inverted";
//     }
// }
// 
// void
// RingRoutingInfo::SetPgwSwIdx (uint16_t value)
// {
//   NS_LOG_FUNCTION (this << value);
// 
//   m_pgwIdx = value;
// }
// 
// void
// RingRoutingInfo::SetSgwSwIdx (uint16_t value)
// {
//   NS_LOG_FUNCTION (this << value);
// 
//   m_sgwIdx = value;
// }
// 
// void
// RingRoutingInfo::SetPgwSwDpId (uint64_t value)
// {
//   NS_LOG_FUNCTION (this << value);
// 
//   m_pgwDpId = value;
// }
// 
// void
// RingRoutingInfo::SetSgwSwDpId (uint64_t value)
// {
//   NS_LOG_FUNCTION (this << value);
// 
//   m_sgwDpId = value;
// }
// 
// void
// RingRoutingInfo::SetDefaultPath (RoutingPath downPath)
// {
//   NS_LOG_FUNCTION (this << downPath);
// 
//   m_downPath = downPath;
//   m_isDefaultPath = true;
//   m_isLocalPath = false;
// 
//   // Check for local routing paths
//   if (downPath == RingRoutingInfo::LOCAL)
//     {
//       m_isLocalPath = true;
//     }
// }
// 
// void
// RingRoutingInfo::InvertPath ()
// {
//   NS_LOG_FUNCTION (this);
// 
//   if (m_isLocalPath == false)
//     {
//       m_downPath = RingRoutingInfo::Invert (m_downPath);
//       m_isDefaultPath = !m_isDefaultPath;
//     }
// }
// 
// void
// RingRoutingInfo::ResetPath ()
// {
//   NS_LOG_FUNCTION (this);
// 
//   if (m_isDefaultPath == false)
//     {
//       InvertPath ();
//     }
// }

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

};  // namespace ns3
