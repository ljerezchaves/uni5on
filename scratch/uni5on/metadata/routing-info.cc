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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
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

RoutingInfo::RoutingInfo (uint32_t teid, BearerCreated_t bearer,
                          Ptr<UeInfo> ueInfo, bool isDefault)
  : m_bearer (bearer),
  m_blockReason (0),
  m_isActive (false),
  m_isAggregated (false),
  m_isDefault (isDefault),
  m_isInstGw (false),
  m_pgwTftIdx (0),
  m_priority (1),
  m_sliceId (ueInfo->GetSliceId ()),
  m_teid (teid),
  m_timeout (0),
  m_ueInfo (ueInfo)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_ueInfo, "Invalid UeInfo pointer.");
  NS_ASSERT_MSG ((LteIface::S1 == 0 && LteIface::S5 == 1)
                 || (LteIface::S5 == 0 && LteIface::S1 == 1),
                 "Incompatible LteIface enum values.");

  m_isGbrRes [LteIface::S1] = false;
  m_isGbrRes [LteIface::S5] = false;
  m_isInstIf [LteIface::S1] = false;
  m_isInstIf [LteIface::S5] = false;
  m_isMbrDlInst [LteIface::S1] = false;
  m_isMbrDlInst [LteIface::S5] = false;
  m_isMbrUlInst [LteIface::S1] = false;
  m_isMbrUlInst [LteIface::S5] = false;

  // Validate the default bearer.
  if (IsDefault ())
    {
      NS_ABORT_MSG_IF (GetBearerId () != 1, "Invalid default BID.");
      NS_ABORT_MSG_IF (GetQciInfo () != EpsBearer::NGBR_VIDEO_TCP_DEFAULT,
                       "Invalid default QCI");
    }

  // Register this routing information object.
  RegisterRoutingInfo (Ptr<RoutingInfo> (this));

  // Save this routing information object into UeInfo.
  ueInfo->AddRoutingInfo (Ptr<RoutingInfo> (this));
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

uint16_t
RoutingInfo::GetBlockReason (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockReason;
}

std::string
RoutingInfo::GetBlockReasonHex (void) const
{
  NS_LOG_FUNCTION (this);

  char valueStr [7];
  sprintf (valueStr, "0x%04x", m_blockReason);
  return std::string (valueStr);
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
RoutingInfo::IsAggregated (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isAggregated;
}

bool
RoutingInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockReason;
}

bool
RoutingInfo::IsDefault (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDefault;
}

bool
RoutingInfo::IsGwInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isInstGw;
}

bool
RoutingInfo::IsIfInstalled (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isInstIf [iface];
}

Ipv4Header::DscpType
RoutingInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return Qci2Dscp (GetQciInfo ());
}

std::string
RoutingInfo::GetDscpStr (void) const
{
  NS_LOG_FUNCTION (this);

  return DscpTypeStr (GetDscp ());
}

uint16_t
RoutingInfo::GetDscpValue (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (GetDscp ());
}

bool
RoutingInfo::HasDlTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTft ()->HasDownlinkFilter ();
}

bool
RoutingInfo::HasUlTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTft ()->HasUplinkFilter ();
}

bool
RoutingInfo::HasTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return (HasDlTraffic () || HasUlTraffic ());
}

uint8_t
RoutingInfo::GetBearerId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.epsBearerId;
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

  return GetEpsBearer ().qci;
}

GbrQosInformation
RoutingInfo::GetQosInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return GetEpsBearer ().gbrQosInfo;
}

QosType
RoutingInfo::GetQosType (void) const
{
  NS_LOG_FUNCTION (this);

  return IsGbr () ? QosType::GBR : QosType::NON;
}

std::string
RoutingInfo::GetQosTypeStr (void) const
{
  NS_LOG_FUNCTION (this);

  return QosTypeStr (GetQosType ());
}

Ptr<EpcTft>
RoutingInfo::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft;
}

int64_t
RoutingInfo::GetGbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().gbrDl);
}

int64_t
RoutingInfo::GetGbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().gbrUl);
}

bool
RoutingInfo::HasGbrBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (HasGbrDlBitRate () || HasGbrUlBitRate ());
}

bool
RoutingInfo::HasGbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (GetGbrDlBitRate () != 0);
}

bool
RoutingInfo::HasGbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (GetGbrUlBitRate () != 0);
}

bool
RoutingInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!IsDefault () && GetEpsBearer ().IsGbr ());
}

bool
RoutingInfo::IsGbrReserved (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  return m_isGbrRes [iface];
}

bool
RoutingInfo::IsNonGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return !IsGbr ();
}

int64_t
RoutingInfo::GetMbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().mbrDl);
}

int64_t
RoutingInfo::GetMbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().mbrUl);
}

bool
RoutingInfo::HasMbrDl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrDlBitRate () != 0;
}

bool
RoutingInfo::HasMbrUl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrUlBitRate () != 0;
}

bool
RoutingInfo::HasMbr (void) const
{
  NS_LOG_FUNCTION (this);

  return HasMbrDl () || HasMbrUl ();
}

bool
RoutingInfo::IsMbrDlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrDlInst [LteIface::S1] || m_isMbrDlInst [LteIface::S5];
}

bool
RoutingInfo::IsMbrUlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrUlInst [LteIface::S1] || m_isMbrUlInst [LteIface::S5];
}

bool
RoutingInfo::IsMbrDlInstalled (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return m_isMbrDlInst [iface];
}

bool
RoutingInfo::IsMbrUlInstalled (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return m_isMbrUlInst [iface];
}

Ipv4Address
RoutingInfo::GetUeAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetAddr ();
}

uint64_t
RoutingInfo::GetUeImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetImsi ();
}

Ptr<UeInfo>
RoutingInfo::GetUeInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo;
}

uint16_t
RoutingInfo::GetEnbCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetCellId ();
}

uint16_t
RoutingInfo::GetEnbInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetEnbS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetS1uAddr ();
}

uint32_t
RoutingInfo::GetPgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetPgwId ();
}

uint16_t
RoutingInfo::GetPgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetPgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetMainS5Addr ();
}

uint64_t
RoutingInfo::GetPgwTftDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetTftDpId (GetPgwTftIdx ());
}

uint32_t
RoutingInfo::GetPgwTftS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetTftS5PortNo (GetPgwTftIdx ());
}

uint64_t
RoutingInfo::GetSgwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetDpId ();
}

uint32_t
RoutingInfo::GetSgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetSgwId ();
}

uint16_t
RoutingInfo::GetSgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetSgwS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uAddr ();
}

uint32_t
RoutingInfo::GetSgwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uPortNo ();
}

Ipv4Address
RoutingInfo::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5Addr ();
}

uint32_t
RoutingInfo::GetSgwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5PortNo ();
}

uint16_t
RoutingInfo::GetDstDlInfraSwIdx (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S5) ? GetSgwInfraSwIdx () : GetEnbInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetDstDlAddr (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S5) ? GetSgwS5Addr () : GetEnbS1uAddr ();
}

uint16_t
RoutingInfo::GetDstUlInfraSwIdx (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S1) ? GetSgwInfraSwIdx () : GetPgwInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetDstUlAddr (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S1) ? GetSgwS1uAddr () : GetPgwS5Addr ();
}

uint16_t
RoutingInfo::GetSrcDlInfraSwIdx (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S5) ? GetPgwInfraSwIdx () : GetSgwInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetSrcDlAddr (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S5) ? GetPgwS5Addr () : GetSgwS1uAddr ();
}

uint16_t
RoutingInfo::GetSrcUlInfraSwIdx (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S1) ? GetEnbInfraSwIdx () : GetSgwInfraSwIdx ();
}

Ipv4Address
RoutingInfo::GetSrcUlAddr (LteIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");
  return (iface == LteIface::S1) ? GetEnbS1uAddr () : GetSgwS5Addr ();
}

std::string
RoutingInfo::BlockReasonStr (BlockReason reason)
{
  switch (reason)
    {
    case RoutingInfo::PGWTABLE:
      return "PgwTable";
    case RoutingInfo::PGWLOAD:
      return "PgwLoad";
    case RoutingInfo::SGWTABLE:
      return "SgwTable";
    case RoutingInfo::SGWLOAD:
      return "SgwLoad";
    case RoutingInfo::BACKTABLE:
      return "BackTable";
    case RoutingInfo::BACKLOAD:
      return "BackLoad";
    case RoutingInfo::BACKBAND:
      return "BackBand";
    default:
      return "-";
    }
}

EpsBearer
RoutingInfo::GetEpsBearer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return RoutingInfo::GetPointer (teid)->GetEpsBearer ();
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

std::ostream &
RoutingInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (11) << "Teid"
     << " " << setw (6)  << "Slice"
     << " " << setw (6)  << "IsDft"
     << " " << setw (6)  << "IsAct"
     << " " << setw (6)  << "IsAgg"
     << " " << setw (6)  << "IsBlk"
     << " " << setw (8)  << "BlkReas"
     << " " << setw (4)  << "Qci"
     << " " << setw (8)  << "QosType"
     << " " << setw (5)  << "Dscp"
     << " " << setw (6)  << "Dlink"
     << " " << setw (10) << "DlGbrKbps"
     << " " << setw (10) << "DlMbrKbps"
     << " " << setw (6)  << "DMbIns"
     << " " << setw (6)  << "Ulink"
     << " " << setw (10) << "UlGbrKbps"
     << " " << setw (10) << "UlMbrKbps"
     << " " << setw (6)  << "UMbIns"
     << " " << setw (6)  << "S1Res"
     << " " << setw (6)  << "S5Res"
     << " " << setw (6)  << "S1Ins"
     << " " << setw (6)  << "S5Ins"
     << " " << setw (6)  << "GwIns"
     << " " << setw (3)  << "Ttf"
     << " " << setw (7)  << "Prio"
     << " " << setw (3)  << "Tmo";
  return os;
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
RoutingInfo::SetGbrReserved (LteIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_isGbrRes [iface] = value;
}

void
RoutingInfo::SetMbrDlInstalled (LteIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  m_isMbrDlInst [iface] = value;
}

void
RoutingInfo::SetMbrUlInstalled (LteIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  m_isMbrUlInst [iface] = value;
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
  NS_ASSERT_MSG (m_priority > 0, "Invalid zero priority.");
}

void
RoutingInfo::SetTimeout (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_timeout = value;
}

void
RoutingInfo::SetGwInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isInstGw = value;
}

void
RoutingInfo::SetIfInstalled (LteIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  NS_ASSERT_MSG (iface == LteIface::S1 || iface == LteIface::S5,
                 "Invalid LTE interface. Expected S1-U or S5 interface.");

  m_isInstIf [iface] = value;
}

void
RoutingInfo::IncreasePriority (void)
{
  NS_LOG_FUNCTION (this);

  m_priority++;
  NS_ASSERT_MSG (m_priority > 0, "Invalid zero priority.");
}

bool
RoutingInfo::IsBlocked (BlockReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return (m_blockReason & (static_cast<uint16_t> (reason)));
}

void
RoutingInfo::ResetBlocked (void)
{
  NS_LOG_FUNCTION (this);

  m_blockReason = 0;
}

void
RoutingInfo::SetBlocked (BlockReason reason)
{
  NS_LOG_FUNCTION (this << reason);

  NS_ASSERT_MSG (IsDefault () == false, "Can't block the default bearer.");

  m_blockReason |= static_cast<uint16_t> (reason);
}

void
RoutingInfo::UnsetBlocked (BlockReason reason)
{
  NS_LOG_FUNCTION (this << reason);

  m_blockReason &= ~(static_cast<uint16_t> (reason));
}

void
RoutingInfo::GetList (RoutingInfoList_t &returnList, SliceId slice)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (!returnList.size (), "The return list should be empty.");
  for (auto const &it : m_routingInfoByTeid)
    {
      Ptr<RoutingInfo> rInfo = it.second;
      if (slice == SliceId::ALL || rInfo->GetSliceId () == slice)
        {
          returnList.push_back (rInfo);
        }
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
  char prioStr [10];
  sprintf (prioStr, "0x%x", rInfo.GetPriority ());

  os << " " << setw (11) << rInfo.GetTeidHex ()
     << " " << setw (6)  << rInfo.GetSliceIdStr ()
     << " " << setw (6)  << rInfo.IsDefault ()
     << " " << setw (6)  << rInfo.IsActive ()
     << " " << setw (6)  << rInfo.IsAggregated ()
     << " " << setw (6)  << rInfo.IsBlocked ()
     << " " << setw (8)  << rInfo.GetBlockReasonHex ()
     << " " << setw (4)  << rInfo.GetQciInfo ()
     << " " << setw (8)  << rInfo.GetQosTypeStr ()
     << " " << setw (5)  << rInfo.GetDscpStr ()
     << " " << setw (6)  << rInfo.HasDlTraffic ()
     << " " << setw (10) << Bps2Kbps (rInfo.GetGbrDlBitRate ())
     << " " << setw (10) << Bps2Kbps (rInfo.GetMbrDlBitRate ())
     << " " << setw (6)  << rInfo.IsMbrDlInstalled ()
     << " " << setw (6)  << rInfo.HasUlTraffic ()
     << " " << setw (10) << Bps2Kbps (rInfo.GetGbrUlBitRate ())
     << " " << setw (10) << Bps2Kbps (rInfo.GetMbrUlBitRate ())
     << " " << setw (6)  << rInfo.IsMbrUlInstalled ()
     << " " << setw (6)  << rInfo.IsGbrReserved (LteIface::S1)
     << " " << setw (6)  << rInfo.IsGbrReserved (LteIface::S5)
     << " " << setw (6)  << rInfo.IsIfInstalled (LteIface::S1)
     << " " << setw (6)  << rInfo.IsIfInstalled (LteIface::S5)
     << " " << setw (6)  << rInfo.IsGwInstalled ()
     << " " << setw (3)  << rInfo.GetPgwTftIdx ()
     << " " << setw (7)  << prioStr
     << " " << setw (3)  << rInfo.GetTimeout ();
  return os;
}

} // namespace ns3
