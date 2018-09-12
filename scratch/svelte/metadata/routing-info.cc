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

#include <iomanip>
#include <iostream>
#include "routing-info.h"
#include "enb-info.h"
#include "pgw-info.h"
#include "sgw-info.h"
#include "ue-info.h"
#include "../infrastructure/backhaul-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingInfo");
NS_OBJECT_ENSURE_REGISTERED (RoutingInfo);

// Initializing RoutingInfo static members.
RoutingInfo::TeidRoutingMap_t RoutingInfo::m_routingInfoByTeid;

RoutingInfo::RoutingInfo (uint32_t teid, BearerContext_t bearer,
                          Ptr<UeInfo> ueInfo, bool isDefault)
  : m_bearer (bearer),
  m_blockReason (RoutingInfo::NONE),
  m_isActive (false),
  m_isAggregated (false),
  m_isBlocked (false),
  m_isDefault (isDefault),
  m_isGbrRes (false),
  m_isMbrDlInst (false),
  m_isMbrUlInst (false),
  m_isTunnelInst (false),
  m_pgwTftIdx (0),
  m_priority (0),
  m_teid (teid),
  m_timeout (0),
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
RoutingInfo::IsTunnelInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isTunnelInst;
}

bool
RoutingInfo::IsGbrReserved (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isGbrRes;
}

bool
RoutingInfo::IsMbrDlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrDlInst;
}

bool
RoutingInfo::IsMbrUlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrUlInst;
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

std::string
RoutingInfo::GetDscpStr (void) const
{
  NS_LOG_FUNCTION (this);

  return DscpTypeStr (GetDscp ());
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
RoutingInfo::HasDlTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft->HasDownlinkFilter ();
}

bool
RoutingInfo::HasUlTraffic (void) const
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
RoutingInfo::GetGbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return GetQosInfo ().gbrDl;
}

uint64_t
RoutingInfo::GetGbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return GetQosInfo ().gbrUl;
}

uint64_t
RoutingInfo::GetMbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return GetQosInfo ().mbrDl;
}

uint64_t
RoutingInfo::GetMbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return GetQosInfo ().mbrUl;
}

std::string
RoutingInfo::GetMbrDlAddCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << GetMbrDlBitRate () / 1000;
  return meter.str ();
}

std::string
RoutingInfo::GetMbrUlAddCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=add,flags=1,meter=" << m_teid
        << " drop:rate=" << GetMbrUlBitRate () / 1000;
  return meter.str ();
}

std::string
RoutingInfo::GetMbrDelCmd (void) const
{
  NS_LOG_FUNCTION (this);

  std::ostringstream meter;
  meter << "meter-mod cmd=del,meter=" << m_teid;
  return meter.str ();
}

bool
RoutingInfo::HasMbrDl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrDlBitRate ();
}

bool
RoutingInfo::HasMbrUl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrUlBitRate ();
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
    case RoutingInfo::TFTTABLE:
      return "TabFull";
    case RoutingInfo::TFTLOAD:
      return "MaxLoad";
    case RoutingInfo::LINKBAND:
      return "SliceFull";
    case RoutingInfo::NONE:
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

SliceId
RoutingInfo::GetSliceId (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return RoutingInfo::GetPointer (teid)->GetSliceId ();
}

std::string
RoutingInfo::PrintHeader (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::ostringstream str;
  str << setw (12) << "TEID"

      << setw (7)  << "Defau"
      << setw (7)  << "Activ"
      << setw (7)  << "Insta"
      << setw (7)  << "Aggre"
      << setw (7)  << "Block"
      << setw (10) << "Reason"

      << setw (5)  << "QCI"
      << setw (7)  << "IsGbr"
      << setw (6)  << "Dscp"

      << setw (7)  << "DlTff"
      << setw (9)  << "DlReq"
      << setw (7)  << "UlTff"
      << setw (9)  << "UlReq"

      << setw (5)  << "TFT"
      << setw (7)  << "Prio"
      << setw (5)  << "Tmo";
  return str.str ();
}

void
RoutingInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ueInfo = 0;
  Object::DoDispose ();
}

void
RoutingInfo::SetActive (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isActive = value;
}

void
RoutingInfo::SetAggregated (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isAggregated = value;
}

void
RoutingInfo::SetTunnelInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isTunnelInst = value;
}

void
RoutingInfo::SetMbrDlInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isMbrDlInst = value;
}

void
RoutingInfo::SetMbrUlInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isMbrUlInst = value;
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
RoutingInfo::SetGbrReserved (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isGbrRes = value;
}

void
RoutingInfo::SetTimeout (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_timeout = value;
}

void
RoutingInfo::IncreasePriority (void)
{
  NS_LOG_FUNCTION (this);

  m_priority++;
}

void
RoutingInfo::SetBlocked (bool value, BlockReason reason)
{
  NS_LOG_FUNCTION (this << value << reason);

  NS_ASSERT_MSG (IsDefault () == false || value == false,
                 "Can't block the default bearer traffic.");
  NS_ASSERT_MSG (value == false || reason != RoutingInfo::NONE,
                 "Specify the reason why this bearer was blocked.");

  m_isBlocked = value;
  m_blockReason = reason;
}

void
RoutingInfo::GetInstalledList (RoutingInfoList_t &returnList, SliceId slice,
                               uint16_t pgwTftIdx)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (!returnList.size (), "The return list should be empty.");
  for (auto const &it : m_routingInfoByTeid)
    {
      Ptr<RoutingInfo> rInfo = it.second;

      if (!rInfo->IsTunnelInstalled ())
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
      returnList.push_back (rInfo);
    }
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

std::ostream & operator << (std::ostream &os, const RoutingInfo &rInfo)
{
  os << setw (12) << rInfo.GetTeidHex ()

     << setw (7)  << rInfo.IsDefault ()
     << setw (7)  << rInfo.IsActive ()
     << setw (7)  << rInfo.IsTunnelInstalled ()
     << setw (7)  << rInfo.IsAggregated ()
     << setw (7)  << rInfo.IsBlocked ()
     << setw (10) << rInfo.GetBlockReasonStr ()

     << setw (5)  << rInfo.GetQciInfo ()
     << setw (7)  << rInfo.IsGbr ()
     << setw (6)  << rInfo.GetDscpStr ()

     << setw (7)  << rInfo.HasDlTraffic ()
     << setw (9)  << rInfo.GetGbrDlBitRate ()
     << setw (7)  << rInfo.HasUlTraffic ()
     << setw (9)  << rInfo.GetGbrUlBitRate ()

     << setw (5)  << rInfo.GetPgwTftIdx ()
     << setw (7)  << rInfo.GetPriority ()
     << setw (5)  << rInfo.GetTimeout ();
  return os;
}

} // namespace ns3
