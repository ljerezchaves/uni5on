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

#include "gbr-info.h"
#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GbrInfo");
NS_OBJECT_ENSURE_REGISTERED (GbrInfo);

GbrInfo::GbrInfo (Ptr<RoutingInfo> rInfo)
  : m_dscp (0),
    m_isReserved (false),
    m_hasDown (false),
    m_hasUp (false),
    m_downBitRate (0),
    m_upBitRate (0),
    m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);

  m_teid = rInfo->GetTeid ();
  GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
  if (gbrQoS.gbrDl)
    {
      m_hasDown = true;
      m_downBitRate = gbrQoS.gbrDl;
    }
  if (gbrQoS.gbrUl)
    {
      m_hasUp = true;
      m_upBitRate = gbrQoS.gbrUl;
    }
}

GbrInfo::~GbrInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
GbrInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GbrInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint32_t
GbrInfo::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rInfo ? m_rInfo->GetTeid () : 0;
}

uint16_t
GbrInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_dscp;
}

bool
GbrInfo::IsReserved (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isReserved;
}

uint64_t
GbrInfo::GetDownBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_hasDown ? m_downBitRate : 0;
}

uint64_t
GbrInfo::GetUpBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return m_hasUp ? m_upBitRate : 0;
}

Ptr<RoutingInfo>
GbrInfo::GetRoutingInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rInfo;
}

void
GbrInfo::SetDscp (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_dscp = value;
}

void
GbrInfo::SetReserved (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isReserved = value;
}

void
GbrInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rInfo = 0;
}

};  // namespace ns3
