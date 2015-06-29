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
NS_OBJECT_ENSURE_REGISTERED (ReserveInfo);
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

std::string
RoutingInfo::GetDescription (void) const
{
  std::ostringstream desc;
  desc  << (IsGbr () ? "GBR" : "Non-GBR")
        << " [" << m_imsi << "@" << m_cellId << "]";
  return desc.str ();
}

bool
RoutingInfo::IsGbr (void) const
{
  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
}

GbrQosInformation
RoutingInfo::GetQosInfo (void) const
{
  return m_bearer.bearerLevelQos.gbrQosInfo;
}

uint32_t
RoutingInfo::GetTeid (void) const
{
  return m_teid;
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


// ------------------------------------------------------------------------ //
MeterInfo::MeterInfo ()
  : m_isInstalled (false),
    m_hasDown (false),
    m_hasUp (false),
    m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

MeterInfo::MeterInfo (Ptr<RoutingInfo> rInfo)
  : m_isInstalled (false),
    m_hasDown (false),
    m_hasUp (false),
    m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);
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

std::string
MeterInfo::GetDownAddCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << m_downDataRate.GetBitRate () / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetUpAddCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid 
        << " drop:rate=" << m_upDataRate.GetBitRate () / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetDelCmd (void) const
{
  std::ostringstream meter;
  meter << "meter-mod cmd=del,meter=" << m_teid;
  return meter.str ();
}


// ------------------------------------------------------------------------ //
ReserveInfo::ReserveInfo ()
  : m_isReserved (false),
    m_hasDown (false),
    m_hasUp (false),
    m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

ReserveInfo::ReserveInfo (Ptr<RoutingInfo> rInfo)
  : m_isReserved (false),
    m_hasDown (false),
    m_hasUp (false),
    m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);
}

ReserveInfo::~ReserveInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
ReserveInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ReserveInfo")
    .SetParent<Object> ()
    .AddConstructor<ReserveInfo> ()
  ;
  return tid;
}

void
ReserveInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_rInfo = 0;
}

Ptr<RoutingInfo>
ReserveInfo::GetRoutingInfo ()
{
  return m_rInfo;
}

DataRate
ReserveInfo::GetDownDataRate (void) const
{
  return m_downDataRate;
}

DataRate
ReserveInfo::GetUpDataRate (void) const
{
  return m_upDataRate;
}


// ------------------------------------------------------------------------ //
RingRoutingInfo::RingRoutingInfo ()
  : m_rInfo (0)
{
  NS_LOG_FUNCTION (this);
}

RingRoutingInfo::RingRoutingInfo (Ptr<RoutingInfo> rInfo, RoutingPath downPath)
  : m_rInfo (rInfo)
{
  NS_LOG_FUNCTION (this);
  m_downPath  = downPath;
  m_upPath    = RingRoutingInfo::InvertPath (m_downPath);
  m_isDownInv = false;
  m_isUpInv   = false;
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
RingRoutingInfo::IsDownInv (void) const
{
  return m_isDownInv;
}

bool
RingRoutingInfo::IsUpInv (void) const
{
  return m_isUpInv;
}

void
RingRoutingInfo::InvertDownPath ()
{
  NS_LOG_FUNCTION (this);

  m_downPath = RingRoutingInfo::InvertPath (m_downPath);
  m_isDownInv = !m_isDownInv;
}

void
RingRoutingInfo::InvertUpPath ()
{
  NS_LOG_FUNCTION (this);

  m_upPath = RingRoutingInfo::InvertPath (m_upPath);
  m_isUpInv = !m_isUpInv;
}

void
RingRoutingInfo::ResetPaths ()
{
  NS_LOG_FUNCTION (this);

  if (m_isDownInv)
    {
      InvertDownPath ();
    }

  if (m_isUpInv)
    {
      InvertUpPath ();
    }
}

};  // namespace ns3
