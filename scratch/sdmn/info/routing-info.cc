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

#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);

// Initializing RoutingInfo static members.
RoutingInfo::TeidRoutingMap_t RoutingInfo::m_globalInfoMap;

RoutingInfo::RoutingInfo (uint32_t teid)
  : m_teid (teid),
    m_imsi (0),
    m_cellId (0),
    m_pgwDpId (0),
    m_enbDpId (0),
    m_priority (0),
    m_timeout (0),
    m_isDefault (0),
    m_isInstalled (0),
    m_isActive (0)
{
  NS_LOG_FUNCTION (this);

  m_enbAddr = Ipv4Address ();
  m_pgwAddr = Ipv4Address ();

  SaveRoutingInfo (Ptr<RoutingInfo> (this));
}

RoutingInfo::~RoutingInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RoutingInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RoutingInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

GbrQosInformation
RoutingInfo::GetQosInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.gbrQosInfo;
}

EpsBearer::Qci
RoutingInfo::GetQciInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.qci;
}

EpsBearer
RoutingInfo::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos;
}

Ptr<EpcTft>
RoutingInfo::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft;
}

uint32_t
RoutingInfo::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_teid;
}

uint64_t
RoutingInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_imsi;
}

uint16_t
RoutingInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_cellId;
}

uint64_t
RoutingInfo::GetEnbSwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbDpId;
}

uint64_t
RoutingInfo::GetPgwSwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwDpId;
}

Ipv4Address
RoutingInfo::GetEnbAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbAddr;
}

Ipv4Address
RoutingInfo::GetPgwAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwAddr;
}

uint16_t
RoutingInfo::GetPriority (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priority;
}

uint16_t
RoutingInfo::GetTimeout (void) const
{
  NS_LOG_FUNCTION (this);

  return m_timeout;
}

bool
RoutingInfo::HasDownlinkTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft->HasDownlinkFilter ();
}

bool
RoutingInfo::HasUplinkTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft->HasUplinkFilter ();
}

bool
RoutingInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
}

bool
RoutingInfo::IsDefault (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDefault;
}

bool
RoutingInfo::IsInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isInstalled;
}

bool
RoutingInfo::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isActive;
}

Ptr<const RoutingInfo>
RoutingInfo::GetConstInfo (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<const RoutingInfo> rInfo = 0;
  TeidRoutingMap_t::const_iterator ret;
  ret = RoutingInfo::m_globalInfoMap.find (teid);
  if (ret != RoutingInfo::m_globalInfoMap.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
}

EpsBearer
RoutingInfo::GetEpsBearer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return GetConstInfo (teid)->GetEpsBearer ();
}

void
RoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
RoutingInfo::SetInstalled (bool installed)
{
  NS_LOG_FUNCTION (this << installed);

  m_isInstalled = installed;
}

void
RoutingInfo::SetActive (bool active)
{
  NS_LOG_FUNCTION (this << active);

  m_isActive = active;
}

void
RoutingInfo::IncreasePriority ()
{
  NS_LOG_FUNCTION (this);

  m_priority++;
}

Ptr<RoutingInfo>
RoutingInfo::GetInfo (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<RoutingInfo> rInfo = 0;
  TeidRoutingMap_t::iterator ret;
  ret = RoutingInfo::m_globalInfoMap.find (teid);
  if (ret != RoutingInfo::m_globalInfoMap.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
}

void
RoutingInfo::SaveRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t teid = rInfo->GetTeid ();
  std::pair<uint32_t, Ptr<RoutingInfo> > entry (teid, rInfo);
  std::pair<TeidRoutingMap_t::iterator, bool> ret;
  ret = RoutingInfo::m_globalInfoMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing routing information for teid " << teid);
    }
}

};  // namespace ns3
