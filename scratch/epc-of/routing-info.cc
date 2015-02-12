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

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ContextInfo);
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);
NS_OBJECT_ENSURE_REGISTERED (MeterInfo);
NS_OBJECT_ENSURE_REGISTERED (RingRoutingInfo);

// ------------------------------------------------------------------------ //
ContextInfo::ContextInfo ()
  : m_imsi (0),
    m_cellId (0),
    m_enbIdx (0),
    m_sgwIdx (0)
{
  NS_LOG_FUNCTION (this);
  m_enbAddr = Ipv4Address ();
  m_sgwAddr = Ipv4Address ();
  m_bearerList.clear ();
}

ContextInfo::~ContextInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
ContextInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContextInfo")
    .SetParent<Object> ()
    .AddConstructor<ContextInfo> ()
  ;
  return tid;
}

void
ContextInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_bearerList.clear ();
}

uint16_t    
ContextInfo::GetEnbIdx () const
{
  return m_enbIdx;
}

uint16_t    
ContextInfo::GetSgwIdx () const
{
  return m_sgwIdx;
}

Ipv4Address 
ContextInfo::GetEnbAddr () const
{
  return m_enbAddr;
}

Ipv4Address 
ContextInfo::GetSgwAddr () const
{
  return m_sgwAddr;
}


// ------------------------------------------------------------------------ //
RoutingInfo::RoutingInfo ()
  : m_teid (0),
    m_sgwIdx (0),
    m_enbIdx (0),
    m_app (0),
    m_priority (0),
    m_timeout (0),
    m_isDefault (0),
    m_isInstalled (0),
    m_isActive (0)
{
  NS_LOG_FUNCTION (this);
  m_enbAddr = Ipv4Address ();
  m_sgwAddr = Ipv4Address ();
  m_reserved = DataRate ();
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
  m_app = 0;
}

bool
RoutingInfo::IsGbr ()
{
  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
}

EpsBearer
RoutingInfo::GetEpsBearer ()
{
  return m_bearer.bearerLevelQos;
}

GbrQosInformation
RoutingInfo::GetQosInfo ()
{
  return m_bearer.bearerLevelQos.gbrQosInfo;
}


// ------------------------------------------------------------------------ //
MeterInfo::MeterInfo ()
  : m_isInstalled (false),
    m_hasDown (false),
    m_hasUp (false)
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
MeterInfo::GetDownAddCmd ()
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid <<
           " drop:rate=" << m_downBitRate / 1024;
  return meter.str ();
}

std::string
MeterInfo::GetUpAddCmd ()
{
  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid <<
           " drop:rate=" << m_upBitRate / 1024;
  return meter.str ();
}

std::string
MeterInfo::GetDelCmd ()
{
  std::ostringstream meter;
  meter << "meter-mod cmd=del,meter=" << m_teid;
  return meter.str ();
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
  m_upPath    = m_downPath == RingRoutingInfo::CLOCK ? 
                RingRoutingInfo::COUNTER :
                RingRoutingInfo::CLOCK;
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

void 
RingRoutingInfo::InvertRoutingPath ()
{
  RoutingPath aux = m_downPath;
  m_downPath = m_upPath;
  m_upPath = aux;
}

};  // namespace ns3
