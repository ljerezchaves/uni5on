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

#include "meter-info.h"
#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MeterInfo");
NS_OBJECT_ENSURE_REGISTERED (MeterInfo);

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

};  // namespace ns3
