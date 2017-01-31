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

RoutingInfo::RoutingInfo ()
  : m_teid (0),
    m_imsi (0),
    m_cellId (0),
    m_sgwIdx (0),
    m_enbIdx (0),
    m_priority (0),
    m_timeout (0),
    m_isDefault (0),
    m_isInstalled (0),
    m_isActive (0)
{
  NS_LOG_FUNCTION (this);

  m_enbAddr = Ipv4Address ();
  m_sgwAddr = Ipv4Address ();
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
    .AddConstructor<RoutingInfo> ()
  ;
  return tid;
}

void
RoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
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

uint16_t
RoutingInfo::GetEnbSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbIdx;
}

uint16_t
RoutingInfo::GetSgwSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwIdx;
}

Ipv4Address
RoutingInfo::GetEnbAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbAddr;
}

Ipv4Address
RoutingInfo::GetSgwAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwAddr;
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
RoutingInfo::IncreasePriority (void)
{
  NS_LOG_FUNCTION (this);

  m_priority++;
}

};  // namespace ns3
