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
  : m_imsi (imsi),
    m_mmeUeS1Id (imsi),
    m_enbUeS1Id (0),
    m_cellId (0),
    m_bearerCounter (0)
{
  NS_LOG_FUNCTION (this);

  m_ueAddr  = Ipv4Address ();
  m_enbS1uAddr = Ipv4Address ();

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
UeInfo::GetEnbS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbS1uAddr;
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

uint16_t
UeInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_cellId;
}

void
UeInfo::SetUeAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_ueAddr = value;
}

void
UeInfo::SetEnbS1uAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbS1uAddr = value;
}

void
UeInfo::SetMmeUeS1Id (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_mmeUeS1Id = value;
}

void
UeInfo::SetEnbUeS1Id (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbUeS1Id = value;
}

void
UeInfo::SetCellId (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_cellId = value;
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
UeInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_bearersList.clear ();
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
