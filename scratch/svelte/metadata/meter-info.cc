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

#include <ns3/ofswitch13-module.h>
#include "meter-info.h"
#include "routing-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MeterInfo");
NS_OBJECT_ENSURE_REGISTERED (MeterInfo);

MeterInfo::MeterInfo (Ptr<RoutingInfo> rInfo)
  : m_isDownInstalled (false),
  m_isUpInstalled (false),
  m_hasDown (false),
  m_hasUp (false),
  m_downBitRate (0),
  m_upBitRate (0)
{
  NS_LOG_FUNCTION (this);

  AggregateObject (rInfo);
  m_teid = rInfo->GetTeid ();
  NS_ASSERT_MSG (m_teid <= OFPM_MAX, "Invalid meter ID.");

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
  ;
  return tid;
}

bool
MeterInfo::IsDownInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDownInstalled;
}

bool
MeterInfo::IsUpInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isUpInstalled;
}

bool
MeterInfo::HasDown (void) const
{
  NS_LOG_FUNCTION (this);

  return m_hasDown;
}

bool
MeterInfo::HasUp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_hasUp;
}

std::string
MeterInfo::GetDownAddCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << m_downBitRate / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetUpAddCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << m_upBitRate / 1000;
  return meter.str ();
}

std::string
MeterInfo::GetDelCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=del,meter=" << m_teid;
  return meter.str ();
}

void
MeterInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
MeterInfo::SetDownInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isDownInstalled = value;
}

void
MeterInfo::SetUpInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isUpInstalled = value;
}

} // namespace ns3
