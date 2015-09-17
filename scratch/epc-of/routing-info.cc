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
NS_OBJECT_ENSURE_REGISTERED (MeterInfo);
NS_OBJECT_ENSURE_REGISTERED (GbrInfo);
NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);

// ------------------------------------------------------------------------ //
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
  return m_bearer.bearerLevelQos.gbrQosInfo;
}

EpsBearer::Qci
RoutingInfo::GetQciInfo (void) const
{
  return m_bearer.bearerLevelQos.qci;
}

EpsBearer
RoutingInfo::GetEpsBearer (void) const
{
  return m_bearer.bearerLevelQos;
}

uint32_t
RoutingInfo::GetTeid (void) const
{
  return m_teid;
}

uint64_t
RoutingInfo::GetImsi (void) const
{
  return m_imsi;
}

uint16_t
RoutingInfo::GetCellId (void) const
{
  return m_cellId;
}

uint16_t
RoutingInfo::GetEnbSwIdx (void) const
{
  return m_enbIdx;
}

uint16_t
RoutingInfo::GetSgwSwIdx (void) const
{
  return m_sgwIdx;
}

Ipv4Address
RoutingInfo::GetEnbAddr (void) const
{
  return m_enbAddr;
}

Ipv4Address
RoutingInfo::GetSgwAddr (void) const
{
  return m_sgwAddr;
}

uint16_t
RoutingInfo::GetPriority (void) const
{
  return m_priority;
}

uint16_t
RoutingInfo::GetTimeout (void) const
{
  return m_timeout;
}

bool
RoutingInfo::HasDownlinkTraffic (void) const
{
  return m_bearer.tft->HasDownlinkFilter ();
}

bool
RoutingInfo::HasUplinkTraffic (void) const
{
  return m_bearer.tft->HasUplinkFilter ();
}

bool
RoutingInfo::IsGbr (void) const
{
  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
}

bool
RoutingInfo::IsDefault (void) const
{
  return m_isDefault;
}

bool
RoutingInfo::IsInstalled (void) const
{
  return m_isInstalled;
}

bool
RoutingInfo::IsActive (void) const
{
  return m_isActive;
}

void
RoutingInfo::SetInstalled (bool installed)
{
  m_isInstalled = installed;
}

void
RoutingInfo::SetActive (bool active)
{
  m_isActive = active;
}

void
RoutingInfo::IncreasePriority (void)
{
  m_priority++;
}

// ------------------------------------------------------------------------ //
MeterInfo::MeterInfo ()
  : m_isInstalled (false),
    m_hasDown (false),
    m_hasUp (false),
    m_downBitRate (0),
    m_upBitRate (0),
    m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

MeterInfo::MeterInfo (Ptr<RoutingInfo> rInfo)
  : m_isInstalled (false),
    m_hasDown (false),
    m_hasUp (false),
    m_downBitRate (0),
    m_upBitRate (0),
    m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);

  m_teid = rInfo->GetTeid ();
  GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
  if (gbrQoS.mbrDl)
    {
      m_hasDown = true;
      m_downBitRate = gbrQoS.mbrDl;
    }
  if (gbrQoS.mbrUl)
    {
      m_hasUp = true;
      m_upBitRate = gbrQoS.mbrUl;
    }
}

MeterInfo::~MeterInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MeterInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MeterInfo")
    .SetParent<Object> ()
    .AddConstructor<MeterInfo> ()
  ;
  return tid;
}

void
MeterInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
MeterInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

bool
MeterInfo::IsInstalled (void) const
{
  return m_isInstalled;
}

bool
MeterInfo::HasDown (void) const
{
  return m_hasDown;
}

bool
MeterInfo::HasUp (void) const
{
  return m_hasUp;
}

std::string
MeterInfo::GetDownAddCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << m_downBitRate / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetUpAddCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << m_upBitRate / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetDelCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=del,meter=" << m_teid;
  return meter.str ();
}

void
MeterInfo::SetInstalled (bool installed)
{
  m_isInstalled = installed;
}

// ------------------------------------------------------------------------ //
GbrInfo::GbrInfo ()
  : m_isReserved (false),
    m_hasDown (false),
    m_hasUp (false),
    m_downBitRate (0),
    m_upBitRate (0),
    m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

GbrInfo::GbrInfo (Ptr<RoutingInfo> rInfo)
  : m_isReserved (false),
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
    .AddConstructor<GbrInfo> ()
  ;
  return tid;
}

void
GbrInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
GbrInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

void
GbrInfo::SetReserved (bool reserved)
{
  m_isReserved = reserved;
}

uint64_t
GbrInfo::GetDownBitRate (void) const
{
  return m_hasDown ? m_downBitRate : 0;
}

uint64_t
GbrInfo::GetUpBitRate (void) const
{
  return m_hasUp ? m_upBitRate : 0;
}

bool
GbrInfo::IsReserved (void) const
{
  return m_isReserved;
}


// ------------------------------------------------------------------------ //
RingRoutingInfo::RingRoutingInfo ()
  : m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo,
                                  RoutingPath shortDownPath)
  : m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);
  m_downPath   = shortDownPath;
  m_upPath     = RingRoutingInfo::InvertPath (shortDownPath);
  m_isInverted = false;
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
    .AddConstructor<RingRoutingInfo> ()
  ;
  return tid;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::InvertPath (RoutingPath path)
{
  return path == RingRoutingInfo::CLOCK ?
         RingRoutingInfo::COUNTER :
         RingRoutingInfo::CLOCK;
}

void
RingRoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
RingRoutingInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

bool
RingRoutingInfo::IsInverted (void) const
{
  return m_isInverted;
}

uint16_t
RingRoutingInfo::GetSgwSwIdx (void) const
{
  return m_rInfo->GetSgwSwIdx ();
}

uint16_t
RingRoutingInfo::GetEnbSwIdx (void) const
{
  return m_rInfo->GetEnbSwIdx ();
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetDownPath (void) const
{
  return m_downPath;
}

RingRoutingInfo::RoutingPath
RingRoutingInfo::GetUpPath (void) const
{
  return m_upPath;
}

std::string
RingRoutingInfo::GetPathDesc (void) const
{
  if (IsInverted ())
    {
      return "Inverted";
    }
  else
    {
      return "Shortest";
    }
}

void
RingRoutingInfo::InvertPaths ()
{
  m_downPath = RingRoutingInfo::InvertPath (m_downPath);
  m_upPath   = RingRoutingInfo::InvertPath (m_upPath);
  m_isInverted = !m_isInverted;
}

void
RingRoutingInfo::ResetToShortestPaths ()
{
  NS_LOG_FUNCTION (this);

  if (IsInverted ())
    {
      InvertPaths ();
    }
}

};  // namespace ns3
