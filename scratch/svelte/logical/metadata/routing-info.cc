/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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
#include "gbr-info.h"
#include "meter-info.h"
#include "../../infrastructure/backhaul-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);

// Initializing RoutingInfo static members.
RoutingInfo::TeidRoutingMap_t RoutingInfo::m_routingInfoByTeid;

RoutingInfo::RoutingInfo (uint32_t teid, BearerContext_t bearer, uint64_t imsi,
                          SliceId sliceId, bool isDefault)
  : m_teid (teid),
  m_bearer (bearer),
  m_imsi (imsi),
  m_sliceId (sliceId),
  m_priority (0),
  m_timeout (0),
  m_isActive (false),
  m_isBlocked (false),
  m_isDefault (isDefault),
  m_isInstalled (false),
  m_blockReason (RoutingInfo::NOTBLOCKED)
{
  NS_LOG_FUNCTION (this);

  // Register this routing information object.
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
RoutingInfo::GetPriority (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priority;
}

SliceId
RoutingInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

std::string
RoutingInfo::GetSliceIdStr (void) const
{
  NS_LOG_FUNCTION (this);

  return SliceIdStr (m_sliceId);
}

uint16_t
RoutingInfo::GetTimeout (void) const
{
  NS_LOG_FUNCTION (this);

  return m_timeout;
}

bool
RoutingInfo::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isActive;
}

bool
RoutingInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isBlocked;
}

std::string
RoutingInfo::GetBlockReasonStr (void) const
{
  NS_LOG_FUNCTION (this);

  return BlockReasonStr (m_blockReason);
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

EpsBearer
RoutingInfo::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos;
}

EpsBearer::Qci
RoutingInfo::GetQciInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.qci;
}

GbrQosInformation
RoutingInfo::GetQosInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.gbrQosInfo;
}

Ptr<EpcTft>
RoutingInfo::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft;
}

Ipv4Header::DscpType
RoutingInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return BackhaulController::Qci2Dscp (GetQciInfo ());
}

uint16_t
RoutingInfo::GetDscpValue (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (GetDscp ());
}

bool
RoutingInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
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

std::string
RoutingInfo::BlockReasonStr (BlockReason reason)
{
  switch (reason)
    {
    case RoutingInfo::TFTTABLEFULL:
      return "TabFull";
    case RoutingInfo::TFTMAXLOAD:
      return "MaxLoad";
    case RoutingInfo::NOBANDWIDTH:
      return "SliceFull";
    case RoutingInfo::NOTBLOCKED:
    default:
      return "-";
    }
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
  ret = RoutingInfo::m_routingInfoByTeid.find (teid);
  if (ret != RoutingInfo::m_routingInfoByTeid.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
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

  // Create the GBR and meter metadata, when necessary.
  GbrQosInformation gbrQoS = GetQosInfo ();
  if (gbrQoS.gbrDl || gbrQoS.gbrUl)
    {
      CreateObject<GbrInfo> (Ptr<RoutingInfo> (this));
    }
  if (gbrQoS.mbrDl || gbrQoS.mbrUl)
    {
      CreateObject<MeterInfo> (Ptr<RoutingInfo> (this));
    }
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
  NS_ASSERT_MSG (value == false || reason != RoutingInfo::NOTBLOCKED,
                 "Specify the reason why this bearer was blocked.");

  m_isBlocked = value;
  m_blockReason = reason;
}

void
RoutingInfo::SetInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isInstalled = value;
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
RoutingInfo::RegisterRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t teid = rInfo->GetTeid ();
  std::pair<uint32_t, Ptr<RoutingInfo> > entry (teid, rInfo);
  std::pair<TeidRoutingMap_t::iterator, bool> ret;
  ret = RoutingInfo::m_routingInfoByTeid.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing routing info for this TEID");
}

} // namespace ns3
