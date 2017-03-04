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

UeInfo::UeInfo (uint64_t imsi)
  : m_imsi (imsi)
{
  NS_LOG_FUNCTION (this);

  m_ueAddr  = Ipv4Address ();
  m_enbAddr = Ipv4Address ();

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

uint64_t
UeInfo::GetImsi (void) const
{
  NS_LOG_FUNCTION (this);

  return m_imsi;
}

Ipv4Address
UeInfo::GetUeAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueAddr;
}

Ipv4Address
UeInfo::GetEnbAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbAddr;
}

void
UeInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_teidByBearerIdMap.clear ();
}

void
UeInfo::AddBearer (uint8_t bearerId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << bearerId << teid);

  m_teidByBearerIdMap [bearerId] = teid;
}

void
UeInfo::RemoveBearer (uint8_t bearerId)
{
  NS_LOG_FUNCTION (this << bearerId);

  m_teidByBearerIdMap.erase (bearerId);
}

void
UeInfo::SetUeAddress (Ipv4Address ueAddr)
{
  NS_LOG_FUNCTION (this << ueAddr);

  m_ueAddr = ueAddr;
}

void
UeInfo::SetEnbAddress (Ipv4Address enbAddr)
{
  NS_LOG_FUNCTION (this << enbAddr);

  m_enbAddr = enbAddr;
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

void
UeInfo::RegisterUeInfo (Ptr<UeInfo> ueInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t imsi = ueInfo->GetImsi ();
  std::pair<uint64_t, Ptr<UeInfo> > entry (imsi, ueInfo);
  std::pair<ImsiUeInfoMap_t::iterator, bool> ret;
  ret = UeInfo::m_ueInfoByImsiMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing UE information for ISMI " << imsi);
    }
}

};  // namespace ns3
