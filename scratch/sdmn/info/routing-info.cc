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
#include "s5-aggregation-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);

// Initializing RoutingInfo static members.
RoutingInfo::TeidRoutingMap_t RoutingInfo::m_globalInfoMap;

RoutingInfo::RoutingInfo (uint32_t teid)
  : m_teid (teid),
    m_imsi (0),
    m_dscp (0),
    m_pgwTftIdx (0),
    m_priority (0),
    m_timeout (0),
    m_isDefault (0),
    m_isInstalled (0),
    m_isActive (0),
    m_isBlocked (0)
{
  NS_LOG_FUNCTION (this);

  m_sgwS5Addr = Ipv4Address ();
  m_pgwS5Addr = Ipv4Address ();

  RegisterRoutingInfo (Ptr<RoutingInfo> (this));
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
RoutingInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_dscp;
}

Ipv4Address
RoutingInfo::GetPgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwS5Addr;
}

Ipv4Address
RoutingInfo::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwS5Addr;
}

uint16_t
RoutingInfo::GetPgwTftIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwTftIdx;
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

std::string
RoutingInfo::GetBlockReasonStr (void) const
{
  NS_LOG_FUNCTION (this);

  switch (m_blockReason)
    {
    case RoutingInfo::TFTTABLEFULL:
      return "TabFull";
    case RoutingInfo::TFTMAXLOAD:
      return "MaxLoad";
    case RoutingInfo::BANDWIDTH:
      return "LinkFull";
    case RoutingInfo::NOREASON:
    default:
      return "-";
    }
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

bool
RoutingInfo::IsAggregated (void) const
{
  NS_LOG_FUNCTION (this);

  return GetObject<S5AggregationInfo> ()->IsAggregated ();
}

bool
RoutingInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isBlocked;
}

void
RoutingInfo::SetImsi (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_imsi = value;
}

void
RoutingInfo::SetDscp (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_dscp = value;
}

void
RoutingInfo::SetPgwS5Addr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_pgwS5Addr = value;
}

void
RoutingInfo::SetSgwS5Addr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwS5Addr = value;
}

void
RoutingInfo::SetPgwTftIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  NS_ASSERT_MSG (value > 0, "The index 0 cannot be used.");
  m_pgwTftIdx = value;
}

void
RoutingInfo::SetPriority (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_priority = value;
}

void
RoutingInfo::SetTimeout (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_timeout = value;
}

void
RoutingInfo::SetDefault (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isDefault = value;
}

void
RoutingInfo::SetInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isInstalled = value;
}

void
RoutingInfo::SetActive (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isActive = value;
}

void
RoutingInfo::SetBlocked (bool value, BlockReason reason)
{
  NS_LOG_FUNCTION (this << value << reason);

  NS_ASSERT_MSG (IsDefault () == false || value == false,
                 "Can't block the default bearer traffic.");
  NS_ASSERT_MSG (value == false || reason != RoutingInfo::NOREASON,
                 "Specify the reason why this bearer was blocked.");

  m_isBlocked = value;
  m_blockReason = reason;
}

void
RoutingInfo::SetBearerContext (BearerContext_t value)
{
  NS_LOG_FUNCTION (this);

  m_bearer = value;
}

bool
RoutingInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
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

void
RoutingInfo::IncreasePriority ()
{
  NS_LOG_FUNCTION (this);

  m_priority++;
}

EpsBearer
RoutingInfo::GetEpsBearer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return GetPointer (teid)->GetEpsBearer ();
}

Ptr<RoutingInfo>
RoutingInfo::GetPointer (uint32_t teid)
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

RoutingInfoList_t
RoutingInfo::GetInstalledList (uint16_t pgwTftIdx)
{
  NS_LOG_FUNCTION_NOARGS ();

  RoutingInfoList_t list;
  Ptr<RoutingInfo> rInfo;
  TeidRoutingMap_t::iterator it;
  for (it = RoutingInfo::m_globalInfoMap.begin ();
       it != RoutingInfo::m_globalInfoMap.end (); ++it)
    {
      rInfo = it->second;
      if (pgwTftIdx > 0 && rInfo->GetPgwTftIdx () != pgwTftIdx)
        {
          continue;
        }
      if (rInfo->IsInstalled ())
        {
          list.push_back (rInfo);
        }
    }
  return list;
}

void
RoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
RoutingInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the S5 traffic aggregation metadata,
  // (will be aggregated to this routing info object).
  CreateObject<S5AggregationInfo> (Ptr<RoutingInfo> (this));
}

void
RoutingInfo::RegisterRoutingInfo (Ptr<RoutingInfo> rInfo)
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
