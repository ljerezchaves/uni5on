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

#include "ue-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UeInfo");
NS_OBJECT_ENSURE_REGISTERED (UeInfo);

// Initializing UeInfo static members.
UeInfo::ImsiUeInfoMap_t UeInfo::m_ueInfoByImsiMap;
UeInfo::Ipv4UeInfoMap_t UeInfo::m_ueInfoByIpv4Map;

UeInfo::UeInfo (uint64_t imsi)
  : m_imsi (imsi),
  m_sliceId (SliceId::NONE),
  m_cellId (0),
  m_mmeUeS1Id (imsi),
  m_enbUeS1Id (0),
  m_bearerCounter (0)
{
  NS_LOG_FUNCTION (this);

  RegisterUeInfoByImsi (Ptr<UeInfo> (this));
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

uint64_t
UeInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_imsi;
}

SliceId
UeInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

Ipv4Address
UeInfo::GetUeAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueAddr;
}

uint16_t
UeInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_cellId;
}

uint64_t
UeInfo::GetSgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwId;
}

uint64_t
UeInfo::GetMmeUeS1Id (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mmeUeS1Id;
}

uint64_t
UeInfo::GetEnbUeS1Id (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbUeS1Id;
}

EpcS11SapSgw*
UeInfo::GetS11SapSgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s11SapSgw;
}

std::list<UeInfo::BearerInfo>::const_iterator
UeInfo::GetBearerListBegin () const
{
  NS_LOG_FUNCTION (this);

  return m_bearersList.begin ();
}

std::list<UeInfo::BearerInfo>::const_iterator
UeInfo::GetBearerListEnd () const
{
  NS_LOG_FUNCTION (this);

  return m_bearersList.end ();
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
UeInfo::RemoveBearer (uint8_t bearerId)
{
  NS_LOG_FUNCTION (this << bearerId);

  std::list<BearerInfo>::iterator it;
  for (it = m_bearersList.begin (); it != m_bearersList.end (); ++it)
    {
      if (it->bearerId == bearerId)
        {
          m_bearersList.erase (it);
          break;
        }
    }
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

Ptr<UeInfo>
UeInfo::GetPointer (uint64_t imsi)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<UeInfo> ueInfo = 0;
  ImsiUeInfoMap_t::iterator ret;
  ret = UeInfo::m_ueInfoByImsiMap.find (imsi);
  if (ret != UeInfo::m_ueInfoByImsiMap.end ())
    {
      ueInfo = ret->second;
    }
  return ueInfo;
}

Ptr<UeInfo>
UeInfo::GetPointer (Ipv4Address ipv4)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<UeInfo> ueInfo = 0;
  Ipv4UeInfoMap_t::iterator ret;
  ret = UeInfo::m_ueInfoByIpv4Map.find (ipv4);
  if (ret != UeInfo::m_ueInfoByIpv4Map.end ())
    {
      ueInfo = ret->second;
    }
  return ueInfo;
}

void
UeInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_bearersList.clear ();
}

void
UeInfo::SetSliceId (SliceId value)
{
  NS_LOG_FUNCTION (this << value);

  m_sliceId = value;
}

void
UeInfo::SetUeAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_ueAddr = value;
  RegisterUeInfoByIpv4 (Ptr<UeInfo> (this));
}

void
UeInfo::SetCellId (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_cellId = value;
}

void
UeInfo::SetSgwId (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwId = value;
}

void
UeInfo::SetEnbUeS1Id (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbUeS1Id = value;
}

void
UeInfo::SetS11SapSgw (EpcS11SapSgw* value)
{
  NS_LOG_FUNCTION (this << value);

  m_s11SapSgw = value;
}

void
UeInfo::RegisterUeInfoByImsi (Ptr<UeInfo> ueInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t imsi = ueInfo->GetImsi ();
  std::pair<uint64_t, Ptr<UeInfo> > entry (imsi, ueInfo);
  std::pair<ImsiUeInfoMap_t::iterator, bool> ret;
  ret = UeInfo::m_ueInfoByImsiMap.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing UE info for this ISMI.");
}

void
UeInfo::RegisterUeInfoByIpv4 (Ptr<UeInfo> ueInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ipv4Address ipv4 = ueInfo->GetUeAddr ();
  std::pair<Ipv4Address, Ptr<UeInfo> > entry (ipv4, ueInfo);
  std::pair<Ipv4UeInfoMap_t::iterator, bool> ret;
  ret = UeInfo::m_ueInfoByIpv4Map.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing UE info for this IP.");
}

} // namespace ns3
