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
#include "bearer-info.h"
#include "enb-info.h"
#include "pgw-info.h"
#include "sgw-info.h"
#include "ue-info.h"
#include "../infrastructure/transport-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BearerInfo");
NS_OBJECT_ENSURE_REGISTERED (BearerInfo);

// Initializing BearerInfo static members.
BearerInfo::TeidBearerMap_t BearerInfo::m_BearerInfoByTeid;

BearerInfo::BearerInfo (uint32_t teid, BearerCreated_t bearer,
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
  NS_ASSERT_MSG ((EpsIface::S1 == 0 && EpsIface::S5 == 1)
                 || (EpsIface::S5 == 0 && EpsIface::S1 == 1),
                 "Incompatible EpsIface enum values.");

  m_isGbrRes [EpsIface::S1] = false;
  m_isGbrRes [EpsIface::S5] = false;
  m_isInstIf [EpsIface::S1] = false;
  m_isInstIf [EpsIface::S5] = false;
  m_isMbrDlInst [EpsIface::S1] = false;
  m_isMbrDlInst [EpsIface::S5] = false;
  m_isMbrUlInst [EpsIface::S1] = false;
  m_isMbrUlInst [EpsIface::S5] = false;

  // Validate the default bearer.
  if (IsDefault ())
    {
      NS_ABORT_MSG_IF (GetBearerId () != 1, "Invalid default BID.");
      NS_ABORT_MSG_IF (GetQciInfo () != EpsBearer::NGBR_VIDEO_TCP_DEFAULT,
                       "Invalid default QCI");
    }

  // Register this bearer information object.
  RegisterBearerInfo (Ptr<BearerInfo> (this));

  // Save this bearer information object into UeInfo.
  ueInfo->AddBearerInfo (Ptr<BearerInfo> (this));
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

uint16_t
BearerInfo::GetBlockReason (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockReason;
}

std::string
BearerInfo::GetBlockReasonHex (void) const
{
  NS_LOG_FUNCTION (this);

  char valueStr [7];
  sprintf (valueStr, "0x%04x", m_blockReason);
  return std::string (valueStr);
}

uint16_t
BearerInfo::GetPgwTftIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwTftIdx;
}

uint16_t
BearerInfo::GetPriority (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priority;
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

uint32_t
BearerInfo::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_teid;
}

std::string
BearerInfo::GetTeidHex (void) const
{
  NS_LOG_FUNCTION (this);

  return GetUint32Hex (m_teid);
}

uint16_t
BearerInfo::GetTimeout (void) const
{
  NS_LOG_FUNCTION (this);

  return m_timeout;
}

bool
BearerInfo::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isActive;
}

bool
BearerInfo::IsAggregated (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isAggregated;
}

bool
BearerInfo::IsBlocked (void) const
{
  NS_LOG_FUNCTION (this);

  return m_blockReason;
}

bool
BearerInfo::IsDefault (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isDefault;
}

bool
BearerInfo::IsGwInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isInstGw;
}

bool
BearerInfo::IsIfInstalled (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  return m_isInstIf [iface];
}

Ipv4Header::DscpType
BearerInfo::GetDscp (void) const
{
  NS_LOG_FUNCTION (this);

  return Qci2Dscp (GetQciInfo ());
}

std::string
BearerInfo::GetDscpStr (void) const
{
  NS_LOG_FUNCTION (this);

  return DscpTypeStr (GetDscp ());
}

uint16_t
BearerInfo::GetDscpValue (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (GetDscp ());
}

bool
BearerInfo::HasDlTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTft ()->HasDownlinkFilter ();
}

bool
BearerInfo::HasUlTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTft ()->HasUplinkFilter ();
}

bool
BearerInfo::HasTraffic (void) const
{
  NS_LOG_FUNCTION (this);

  return (HasDlTraffic () || HasUlTraffic ());
}

uint8_t
BearerInfo::GetBearerId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.epsBearerId;
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

  return GetEpsBearer ().qci;
}

GbrQosInformation
BearerInfo::GetQosInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return GetEpsBearer ().gbrQosInfo;
}

QosType
BearerInfo::GetQosType (void) const
{
  NS_LOG_FUNCTION (this);

  return IsGbr () ? QosType::GBR : QosType::NON;
}

std::string
BearerInfo::GetQosTypeStr (void) const
{
  NS_LOG_FUNCTION (this);

  return QosTypeStr (GetQosType ());
}

Ptr<EpcTft>
BearerInfo::GetTft (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer.tft;
}

int64_t
BearerInfo::GetGbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().gbrDl);
}

int64_t
BearerInfo::GetGbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().gbrUl);
}

bool
BearerInfo::HasGbrBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (HasGbrDlBitRate () || HasGbrUlBitRate ());
}

bool
BearerInfo::HasGbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (GetGbrDlBitRate () != 0);
}

bool
BearerInfo::HasGbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return (GetGbrUlBitRate () != 0);
}

bool
BearerInfo::IsGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return (!IsDefault () && GetEpsBearer ().IsGbr ());
}

bool
BearerInfo::IsGbrReserved (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  return m_isGbrRes [iface];
}

bool
BearerInfo::IsNonGbr (void) const
{
  NS_LOG_FUNCTION (this);

  return !IsGbr ();
}

int64_t
BearerInfo::GetMbrDlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().mbrDl);
}

int64_t
BearerInfo::GetMbrUlBitRate (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<int64_t> (GetQosInfo ().mbrUl);
}

bool
BearerInfo::HasMbrDl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrDlBitRate () != 0;
}

bool
BearerInfo::HasMbrUl (void) const
{
  NS_LOG_FUNCTION (this);

  return GetMbrUlBitRate () != 0;
}

bool
BearerInfo::HasMbr (void) const
{
  NS_LOG_FUNCTION (this);

  return HasMbrDl () || HasMbrUl ();
}

bool
BearerInfo::IsMbrDlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrDlInst [EpsIface::S1] || m_isMbrDlInst [EpsIface::S5];
}

bool
BearerInfo::IsMbrUlInstalled (void) const
{
  NS_LOG_FUNCTION (this);

  return m_isMbrUlInst [EpsIface::S1] || m_isMbrUlInst [EpsIface::S5];
}

bool
BearerInfo::IsMbrDlInstalled (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return m_isMbrDlInst [iface];
}

bool
BearerInfo::IsMbrUlInstalled (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  return m_isMbrUlInst [iface];
}

Ipv4Address
BearerInfo::GetUeAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetAddr ();
}

uint64_t
BearerInfo::GetUeImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetImsi ();
}

Ptr<UeInfo>
BearerInfo::GetUeInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo;
}

uint16_t
BearerInfo::GetEnbCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetCellId ();
}

uint16_t
BearerInfo::GetEnbInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetEnbS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetEnbInfo ()->GetS1uAddr ();
}

uint32_t
BearerInfo::GetPgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetPgwId ();
}

uint16_t
BearerInfo::GetPgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetPgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetMainS5Addr ();
}

uint64_t
BearerInfo::GetPgwTftDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetTftDpId (GetPgwTftIdx ());
}

uint32_t
BearerInfo::GetPgwTftS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetPgwInfo ()->GetTftS5PortNo (GetPgwTftIdx ());
}

uint64_t
BearerInfo::GetSgwDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetDpId ();
}

uint32_t
BearerInfo::GetSgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetSgwId ();
}

uint16_t
BearerInfo::GetSgwInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetSgwS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uAddr ();
}

uint32_t
BearerInfo::GetSgwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS1uPortNo ();
}

Ipv4Address
BearerInfo::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5Addr ();
}

uint32_t
BearerInfo::GetSgwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueInfo->GetSgwInfo ()->GetS5PortNo ();
}

uint16_t
BearerInfo::GetDstDlInfraSwIdx (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S5) ? GetSgwInfraSwIdx () : GetEnbInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetDstDlAddr (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S5) ? GetSgwS5Addr () : GetEnbS1uAddr ();
}

uint16_t
BearerInfo::GetDstUlInfraSwIdx (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S1) ? GetSgwInfraSwIdx () : GetPgwInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetDstUlAddr (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S1) ? GetSgwS1uAddr () : GetPgwS5Addr ();
}

uint16_t
BearerInfo::GetSrcDlInfraSwIdx (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S5) ? GetPgwInfraSwIdx () : GetSgwInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetSrcDlAddr (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S5) ? GetPgwS5Addr () : GetSgwS1uAddr ();
}

uint16_t
BearerInfo::GetSrcUlInfraSwIdx (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S1) ? GetEnbInfraSwIdx () : GetSgwInfraSwIdx ();
}

Ipv4Address
BearerInfo::GetSrcUlAddr (EpsIface iface) const
{
  NS_LOG_FUNCTION (this << iface);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");
  return (iface == EpsIface::S1) ? GetEnbS1uAddr () : GetSgwS5Addr ();
}

std::string
BearerInfo::BlockReasonStr (BlockReason reason)
{
  switch (reason)
    {
    case BearerInfo::BRPGWTAB:
      return "PgwTable";
    case BearerInfo::BRPGWCPU:
      return "PgwLoad";
    case BearerInfo::BRSGWTAB:
      return "SgwTable";
    case BearerInfo::BRSGWCPU:
      return "SgwLoad";
    case BearerInfo::BRTPNTAB:
      return "BackTable";
    case BearerInfo::BRTPNCPU:
      return "BackLoad";
    case BearerInfo::BRTPNBWD:
      return "BackBand";
    default:
      return "-";
    }
}

EpsBearer
BearerInfo::GetEpsBearer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  return BearerInfo::GetPointer (teid)->GetEpsBearer ();
}

Ptr<BearerInfo>
BearerInfo::GetPointer (uint32_t teid)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<BearerInfo> bInfo = 0;
  auto ret = BearerInfo::m_BearerInfoByTeid.find (teid);
  if (ret != BearerInfo::m_BearerInfoByTeid.end ())
    {
      bInfo = ret->second;
    }
  return bInfo;
}

std::ostream &
BearerInfo::PrintHeader (std::ostream &os)
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
BearerInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ueInfo = 0;
  Object::DoDispose ();
}

void
BearerInfo::SetActive (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isActive = value;
}

void
BearerInfo::SetAggregated (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isAggregated = value;
}

void
BearerInfo::SetGbrReserved (EpsIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  m_isGbrRes [iface] = value;
}

void
BearerInfo::SetMbrDlInstalled (EpsIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  m_isMbrDlInst [iface] = value;
}

void
BearerInfo::SetMbrUlInstalled (EpsIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  m_isMbrUlInst [iface] = value;
}

void
BearerInfo::SetPgwTftIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  NS_ASSERT_MSG (value > 0, "The index 0 cannot be used.");
  m_pgwTftIdx = value;
}

void
BearerInfo::SetPriority (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_priority = value;
  NS_ASSERT_MSG (m_priority > 0, "Invalid zero priority.");
}

void
BearerInfo::SetTimeout (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_timeout = value;
}

void
BearerInfo::SetGwInstalled (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_isInstGw = value;
}

void
BearerInfo::SetIfInstalled (EpsIface iface, bool value)
{
  NS_LOG_FUNCTION (this << iface << value);

  NS_ASSERT_MSG (iface == EpsIface::S1 || iface == EpsIface::S5,
                 "Invalid interface. Expected S1-U or S5 interface.");

  m_isInstIf [iface] = value;
}

void
BearerInfo::IncreasePriority (void)
{
  NS_LOG_FUNCTION (this);

  m_priority++;
  NS_ASSERT_MSG (m_priority > 0, "Invalid zero priority.");
}

bool
BearerInfo::IsBlocked (BlockReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return (m_blockReason & (static_cast<uint16_t> (reason)));
}

void
BearerInfo::ResetBlocked (void)
{
  NS_LOG_FUNCTION (this);

  m_blockReason = 0;
}

void
BearerInfo::SetBlocked (BlockReason reason)
{
  NS_LOG_FUNCTION (this << reason);

  NS_ASSERT_MSG (IsDefault () == false, "Can't block the default bearer.");

  m_blockReason |= static_cast<uint16_t> (reason);
}

void
BearerInfo::UnsetBlocked (BlockReason reason)
{
  NS_LOG_FUNCTION (this << reason);

  m_blockReason &= ~(static_cast<uint16_t> (reason));
}

void
BearerInfo::GetList (BearerInfoList_t &returnList, SliceId slice)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (!returnList.size (), "The return list should be empty.");
  for (auto const &it : m_BearerInfoByTeid)
    {
      Ptr<BearerInfo> bInfo = it.second;
      if (slice == SliceId::ALL || bInfo->GetSliceId () == slice)
        {
          returnList.push_back (bInfo);
        }
    }
}

void
BearerInfo::RegisterBearerInfo (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t teid = bInfo->GetTeid ();
  std::pair<uint32_t, Ptr<BearerInfo>> entry (teid, bInfo);
  auto ret = BearerInfo::m_BearerInfoByTeid.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing bearer info for this TEID");
}

std::ostream & operator << (std::ostream &os, const BearerInfo &bInfo)
{
  char prioStr [10];
  sprintf (prioStr, "0x%x", bInfo.GetPriority ());

  os << " " << setw (11) << bInfo.GetTeidHex ()
     << " " << setw (6)  << bInfo.GetSliceIdStr ()
     << " " << setw (6)  << bInfo.IsDefault ()
     << " " << setw (6)  << bInfo.IsActive ()
     << " " << setw (6)  << bInfo.IsAggregated ()
     << " " << setw (6)  << bInfo.IsBlocked ()
     << " " << setw (8)  << bInfo.GetBlockReasonHex ()
     << " " << setw (4)  << bInfo.GetQciInfo ()
     << " " << setw (8)  << bInfo.GetQosTypeStr ()
     << " " << setw (5)  << bInfo.GetDscpStr ()
     << " " << setw (6)  << bInfo.HasDlTraffic ()
     << " " << setw (10) << Bps2Kbps (bInfo.GetGbrDlBitRate ())
     << " " << setw (10) << Bps2Kbps (bInfo.GetMbrDlBitRate ())
     << " " << setw (6)  << bInfo.IsMbrDlInstalled ()
     << " " << setw (6)  << bInfo.HasUlTraffic ()
     << " " << setw (10) << Bps2Kbps (bInfo.GetGbrUlBitRate ())
     << " " << setw (10) << Bps2Kbps (bInfo.GetMbrUlBitRate ())
     << " " << setw (6)  << bInfo.IsMbrUlInstalled ()
     << " " << setw (6)  << bInfo.IsGbrReserved (EpsIface::S1)
     << " " << setw (6)  << bInfo.IsGbrReserved (EpsIface::S5)
     << " " << setw (6)  << bInfo.IsIfInstalled (EpsIface::S1)
     << " " << setw (6)  << bInfo.IsIfInstalled (EpsIface::S5)
     << " " << setw (6)  << bInfo.IsGwInstalled ()
     << " " << setw (3)  << bInfo.GetPgwTftIdx ()
     << " " << setw (7)  << prioStr
     << " " << setw (3)  << bInfo.GetTimeout ();
  return os;
}

} // namespace ns3
