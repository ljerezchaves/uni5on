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
#include "enb-info.h"
#include "gbr-info.h"
#include "meter-info.h"
#include "pgw-info.h"
#include "sgw-info.h"
#include "ue-info.h"
#include "../infrastructure/backhaul-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);

// Initializing RoutingInfo static members.
RoutingInfo::TeidRoutingMap_t RoutingInfo::m_routingInfoByTeid;

RoutingInfo::RoutingInfo (uint32_t teid, BearerContext_t bearer,
                          Ptr<UeInfo> ueInfo, bool isDefault)
  : m_teid (teid),
  m_bearer (bearer),
  m_blockReason (RoutingInfo::NOTBLOCKED),
  m_isActive (false),
  m_isAggregated (false),
  m_isBlocked (false),
  m_isDefault (isDefault),
  m_isInstalled (false),
  m_pgwTftIdx (0),
  m_priority (0),
  m_timeout (0),
  m_gbrInfo (0),
  m_meterInfo (0),
  m_ueInfo (ueInfo)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_ueInfo, "Invalid UeInfo pointer.");

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

std::string
RoutingInfo::GetTeidHex (void) const
{
  NS_LOG_FUNCTION (this);

  return GetUint32Hex (m_teid);
}

std::string
RoutingInfo::GetBlockReasonStr (void) const
{
  NS_LOG_FUNCTION (this);

  return BlockReasonStr (m_blockReason);
}

bool
RoutingInfo::HasGbrInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_gbrInfo;
}

bool
RoutingInfo::HasMeterInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_meterInfo;
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

  return m_isAggregated;
}

bool
RoutingInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isBlocked;
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

Ptr<GbrInfo>
RoutingInfo::GetGbrInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_gbrInfo;
}

Ptr<MeterInfo>
RoutingInfo::GetMeterInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_meterInfo;
}

Ptr<UeInfo>
RoutingInfo::GetUeInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo;
}

Ipv4Header::DscpType
RoutingInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return Qci2Dscp (GetQciInfo ());
}

uint16_t
RoutingInfo::GetDscpValue (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (GetDscp ());
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

uint64_t
RoutingInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetImsi ();
}

uint16_t
RoutingInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetCellId ();
}

SliceId
RoutingInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSliceId ();
}

std::string
RoutingInfo::GetSliceIdStr (void) const
{
  NS_LOG_FUNCTION (this);

  return SliceIdStr (m_ueInfo->GetSliceId ());
}

Ipv4Address
RoutingInfo::GetEnbS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetS1uAddr ();
}

Ipv4Address
RoutingInfo::GetPgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetMainS5Addr ();
}

Ipv4Address
RoutingInfo::GetSgwS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uAddr ();
}

Ipv4Address
RoutingInfo::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5Addr ();
}

Ipv4Address
RoutingInfo::GetUeAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetUeAddr ();
}

uint64_t
RoutingInfo::GetSgwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetDpId ();
}

uint32_t
RoutingInfo::GetSgwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uPortNo ();
}

uint32_t
RoutingInfo::GetSgwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5PortNo ();
}

uint16_t
RoutingInfo::GetEnbInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetInfraSwIdx ();
}

uint16_t
RoutingInfo::GetPgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetInfraSwIdx ();
}

uint16_t
RoutingInfo::GetSgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetInfraSwIdx ();
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
  auto ret = RoutingInfo::m_routingInfoByTeid.find (teid);
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

  m_gbrInfo = 0;
  m_meterInfo = 0;
  m_ueInfo = 0;
  Object::DoDispose ();
}

void
RoutingInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the GBR and meter metadata, when necessary.
  GbrQosInformation gbrQoS = GetQosInfo ();
  if (gbrQoS.gbrDl || gbrQoS.gbrUl)
    {
      m_gbrInfo = CreateObject<GbrInfo> (Ptr<RoutingInfo> (this));
    }
  if (gbrQoS.mbrDl || gbrQoS.mbrUl)
    {
      m_meterInfo = CreateObject<MeterInfo> (Ptr<RoutingInfo> (this));
    }

  Object::NotifyConstructionCompleted ();
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

RoutingInfoList_t
RoutingInfo::GetInstalledList (SliceId slice, uint16_t pgwTftIdx)
{
  NS_LOG_FUNCTION_NOARGS ();

  RoutingInfoList_t list;
  for (auto const &it : m_routingInfoByTeid)
    {
      Ptr<RoutingInfo> rInfo = it.second;

      if (!rInfo->IsInstalled ())
        {
          continue;
        }
      if (pgwTftIdx != 0 && rInfo->GetPgwTftIdx () != pgwTftIdx)
        {
          continue;
        }
      if (slice != SliceId::ALL && rInfo->GetSliceId () != slice)
        {
          continue;
        }
      list.push_back (rInfo);
    }
  return list;
}

void
RoutingInfo::IncreasePriority ()
{
  NS_LOG_FUNCTION (this);

  m_priority++;
}

void
RoutingInfo::RegisterRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t teid = rInfo->GetTeid ();
  std::pair<uint32_t, Ptr<RoutingInfo> > entry (teid, rInfo);
  auto ret = RoutingInfo::m_routingInfoByTeid.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing routing info for this TEID");
}

} // namespace ns3
