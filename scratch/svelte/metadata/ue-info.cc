/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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
#include "ue-info.h"
#include "enb-info.h"
#include "pgw-info.h"
#include "sgw-info.h"
#include "../logical/slice-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UeInfo");
NS_OBJECT_ENSURE_REGISTERED (UeInfo);

// Initializing UeInfo static members.
UeInfo::ImsiUeInfoMap_t UeInfo::m_ueInfoByImsi;
UeInfo::Ipv4UeInfoMap_t UeInfo::m_ueInfoByAddr;

UeInfo::UeInfo (uint64_t imsi, Ipv4Address addr,
                Ptr<SliceController> sliceCtrl)
  : m_addr (addr),
  m_imsi (imsi),
  m_enbInfo (0),
  m_pgwInfo (0),
  m_sgwInfo (0),
  m_sliceCtrl (sliceCtrl),
  m_mmeUeS1Id (imsi),
  m_enbUeS1Id (0),
  m_bearerCounter (0)
{
  NS_LOG_FUNCTION (this);

  RegisterUeInfo (Ptr<UeInfo> (this));
}

UeInfo::~UeInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
UeInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UeInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

Ipv4Address
UeInfo::GetAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_addr;
}

uint16_t
UeInfo::GetEnbCellId (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_enbInfo, "eNB not configured yet.");
  return m_enbInfo->GetCellId ();
}

Ptr<EnbInfo>
UeInfo::GetEnbInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbInfo;
}

uint64_t
UeInfo::GetEnbUeS1Id (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbUeS1Id;
}

uint64_t
UeInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_imsi;
}

uint64_t
UeInfo::GetMmeUeS1Id (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mmeUeS1Id;
}

uint16_t
UeInfo::GetNBearers (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearerCounter;
}

Ptr<PgwInfo>
UeInfo::GetPgwInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwInfo;
}

EpcS11SapSgw*
UeInfo::GetS11SapSgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl->GetS11SapSgw ();
}

EpcS1apSapEnb*
UeInfo::GetS1apSapEnb (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_enbInfo, "eNB not configured yet.");
  return m_enbInfo->GetS1apSapEnb ();
}

Ptr<SgwInfo>
UeInfo::GetSgwInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwInfo;
}

SliceId
UeInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl->GetSliceId ();
}

Ptr<SliceController>
UeInfo::GetSliceCtrl (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl;
}

const std::list<UeInfo::BearerInfo>&
UeInfo::GetBearerList (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearersList;
}

Ptr<UeInfo>
UeInfo::GetPointer (uint64_t imsi)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<UeInfo> ueInfo = 0;
  auto ret = UeInfo::m_ueInfoByImsi.find (imsi);
  if (ret != UeInfo::m_ueInfoByImsi.end ())
    {
      ueInfo = ret->second;
    }
  return ueInfo;
}

Ptr<UeInfo>
UeInfo::GetPointer (Ipv4Address addr)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<UeInfo> ueInfo = 0;
  auto ret = UeInfo::m_ueInfoByAddr.find (addr);
  if (ret != UeInfo::m_ueInfoByAddr.end ())
    {
      ueInfo = ret->second;
    }
  return ueInfo;
}

std::ostream &
UeInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (6)  << "IMSI"
     << " " << setw (6)  << "Slice"
     << " " << setw (11) << "UeAddr";
  return os;
}

std::ostream &
UeInfo::PrintNull (std::ostream &os)
{
  os << " " << setw (6)  << "-"
     << " " << setw (6)  << "-"
     << " " << setw (11) << "-";
  return os;
}

void
UeInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_enbInfo = 0;
  m_sgwInfo = 0;
  m_pgwInfo = 0;
  m_sliceCtrl = 0;
  m_bearersList.clear ();
  Object::DoDispose ();
}

void
UeInfo::SetEnbUeS1Id (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbUeS1Id = value;
}

void
UeInfo::SetEnbInfo (Ptr<EnbInfo> value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbInfo = value;
}

void
UeInfo::SetPgwInfo (Ptr<PgwInfo> value)
{
  NS_LOG_FUNCTION (this << value);

  m_pgwInfo = value;
}

void
UeInfo::SetSgwInfo (Ptr<SgwInfo> value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwInfo = value;
}

uint8_t
UeInfo::AddBearer (BearerInfo bearer)
{
  NS_LOG_FUNCTION (this << bearer.bearerId);

  NS_ASSERT_MSG (m_bearerCounter < 11, "No more bearers allowed!");
  bearer.bearerId = ++m_bearerCounter;
  m_bearersList.push_back (bearer);
  return bearer.bearerId;
}

void
UeInfo::AddTft (Ptr<EpcTft> tft, uint32_t teid)
{
  NS_LOG_FUNCTION (this << tft << teid);

  m_tftClassifier.Add (tft, teid);
}

uint32_t
UeInfo::Classify (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // We hardcoded DOWNLINK direction since this function will only be used by
  // the PgwTunnelApp to classify downlink packets when attaching the
  // EpcGtpuTag. The effective GTP encapsulation is performed by OpenFlow rules
  // installed into P-GW TFT switches and can use a different teid value.
  return m_tftClassifier.Classify (packet, EpcTft::DOWNLINK);
}

void
UeInfo::RegisterUeInfo (Ptr<UeInfo> ueInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t imsi = ueInfo->GetImsi ();
  std::pair<uint64_t, Ptr<UeInfo> > entryImsi (imsi, ueInfo);
  auto retImsi = UeInfo::m_ueInfoByImsi.insert (entryImsi);
  NS_ABORT_MSG_IF (retImsi.second == false, "Existing UE info for this ISMI.");

  Ipv4Address ipv4 = ueInfo->GetAddr ();
  std::pair<Ipv4Address, Ptr<UeInfo> > entryIpv4 (ipv4, ueInfo);
  auto retIpv4 = UeInfo::m_ueInfoByAddr.insert (entryIpv4);
  NS_ABORT_MSG_IF (retIpv4.second == false, "Existing UE info for this IP.");
}

std::ostream & operator << (std::ostream &os, const UeInfo &ueInfo)
{
  // Trick to preserve alignment.
  std::ostringstream ipStr;
  ueInfo.GetAddr ().Print (ipStr);

  os << " " << setw (6)  << ueInfo.GetImsi ()
     << " " << setw (6)  << SliceIdStr (ueInfo.GetSliceId ())
     << " " << setw (11) << ipStr.str ();
  return os;
}

} // namespace ns3
