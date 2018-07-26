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

#include "bearer-info.h"
#include "gbr-info.h"
#include "meter-info.h"
#include "../../infrastructure/backhaul-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BearerInfo");
NS_OBJECT_ENSURE_REGISTERED (BearerInfo);

// Initializing BearerInfo static members.
BearerInfo::TeidBearerMap_t BearerInfo::m_bearerInfoByTeid;

BearerInfo::BearerInfo (uint32_t teid, BearerContext_t bearer, uint64_t imsi,
                        SliceId sliceId, bool isDefault)
  : m_teid (teid),
  m_bearer (bearer),
  m_imsi (imsi),
  m_sliceId (sliceId),
  m_isDefault (isDefault),
  m_isActive (false),
  m_isBlocked (false),
  m_blockReason (BearerInfo::NOTBLOCKED)
{
  NS_LOG_FUNCTION (this);

  // Register this bearer information object.
  RegisterBearerInfo (Ptr<BearerInfo> (this));
}

BearerInfo::~BearerInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BearerInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BearerInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint32_t
BearerInfo::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_teid;
}

uint64_t
BearerInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_imsi;
}

SliceId
BearerInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

std::string
BearerInfo::GetSliceIdStr (void) const
{
  NS_LOG_FUNCTION (this);

  return SliceIdStr (m_sliceId);
}

bool
BearerInfo::IsDefault (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDefault;
}

bool
BearerInfo::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isActive;
}

bool
BearerInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isBlocked;
}

std::string
BearerInfo::GetBlockReasonStr (void) const
{
  NS_LOG_FUNCTION (this);

  return BlockReasonStr (m_blockReason);
}

EpsBearer
BearerInfo::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos;
}

EpsBearer::Qci
BearerInfo::GetQciInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.qci;
}

GbrQosInformation
BearerInfo::GetQosInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.bearerLevelQos.gbrQosInfo;
}

Ptr<EpcTft>
BearerInfo::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft;
}

Ipv4Header::DscpType
BearerInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return BackhaulController::Qci2Dscp (GetQciInfo ());
}

uint16_t
BearerInfo::GetDscpValue (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (GetDscp ());
}

bool
BearerInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!m_isDefault && m_bearer.bearerLevelQos.IsGbr ());
}

bool
BearerInfo::HasDownlinkTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft->HasDownlinkFilter ();
}

bool
BearerInfo::HasUplinkTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft->HasUplinkFilter ();
}

std::string
BearerInfo::BlockReasonStr (BlockReason reason)
{
  switch (reason)
    {
    case BearerInfo::TFTTABLEFULL:
      return "TabFull";
    case BearerInfo::TFTMAXLOAD:
      return "MaxLoad";
    case BearerInfo::NOBANDWIDTH:
      return "SliceFull";
    case BearerInfo::NOTBLOCKED:
    default:
      return "-";
    }
}

EpsBearer
BearerInfo::GetEpsBearer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return GetPointer (teid)->GetEpsBearer ();
}

Ptr<BearerInfo>
BearerInfo::GetPointer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<BearerInfo> bInfo = 0;
  TeidBearerMap_t::iterator ret;
  ret = BearerInfo::m_bearerInfoByTeid.find (teid);
  if (ret != BearerInfo::m_bearerInfoByTeid.end ())
    {
      bInfo = ret->second;
    }
  return bInfo;
}

void
BearerInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
BearerInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the GBR and meter metadata, when necessary.
  GbrQosInformation gbrQoS = GetQosInfo ();
  if (gbrQoS.gbrDl || gbrQoS.gbrUl)
    {
      CreateObject<GbrInfo> (Ptr<BearerInfo> (this));
    }
  if (gbrQoS.mbrDl || gbrQoS.mbrUl)
    {
      CreateObject<MeterInfo> (Ptr<BearerInfo> (this));
    }
}

void
BearerInfo::SetActive (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isActive = value;
}

void
BearerInfo::SetBlocked (bool value, BlockReason reason)
{
  NS_LOG_FUNCTION (this << value << reason);

  NS_ASSERT_MSG (IsDefault () == false || value == false,
                 "Can't block the default bearer traffic.");
  NS_ASSERT_MSG (value == false || reason != BearerInfo::NOTBLOCKED,
                 "Specify the reason why this bearer was blocked.");

  m_isBlocked = value;
  m_blockReason = reason;
}

void
BearerInfo::RegisterBearerInfo (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t teid = bInfo->GetTeid ();
  std::pair<uint32_t, Ptr<BearerInfo> > entry (teid, bInfo);
  std::pair<TeidBearerMap_t::iterator, bool> ret;
  ret = BearerInfo::m_bearerInfoByTeid.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing bearer info for this TEID.");
}

} // namespace ns3
