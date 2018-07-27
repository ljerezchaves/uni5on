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

#include "pgw-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwInfo");
NS_OBJECT_ENSURE_REGISTERED (PgwInfo);

// Initializing PgwInfo static members.
PgwInfo::PgwIdPgwInfo_t PgwInfo::m_pgwInfoByPgwId;

PgwInfo::PgwInfo (uint64_t pgwId)
  : m_pgwId (pgwId),
  m_sliceId (SliceId::NONE),
  m_s1uPortNo (0),
  m_s5PortNo (0),
  m_infraSwIdx (0),
  m_infraSwS1uPortNo (0),
  m_infraSwS5PortNo (0)
{
  NS_LOG_FUNCTION (this);

  RegisterPgwInfo (Ptr<PgwInfo> (this));
}

PgwInfo::~PgwInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint64_t
PgwInfo::GetPgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwId;
}

SliceId
PgwInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

Ipv4Address
PgwInfo::GetS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddr;
}


Ipv4Address
PgwInfo::GetS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5Addr;
}

uint32_t
PgwInfo::GetS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uPortNo;
}

uint32_t
PgwInfo::GetS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5PortNo;
}

uint16_t
PgwInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint32_t
PgwInfo::GetInfraSwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS1uPortNo;
}

uint32_t
PgwInfo::GetInfraSwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS5PortNo;
}

Ptr<PgwInfo>
PgwInfo::GetPointer (uint64_t pgwId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<PgwInfo> pgwInfo = 0;
  PgwIdPgwInfo_t::iterator ret;
  ret = PgwInfo::m_pgwInfoByPgwId.find (pgwId);
  if (ret != PgwInfo::m_pgwInfoByPgwId.end ())
    {
      pgwInfo = ret->second;
    }
  return pgwInfo;
}

void
PgwInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
PgwInfo::SetSliceId (SliceId value)
{
  NS_LOG_FUNCTION (this << value);

  m_sliceId = value;
}

void
PgwInfo::SetS1uAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_s1uAddr = value;
}

void
PgwInfo::SetS5Addr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_s5Addr = value;
}

void
PgwInfo::SetS1uPortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_s1uPortNo = value;
}

void
PgwInfo::SetS5PortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_s5PortNo = value;
}

void
PgwInfo::SetInfraSwIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwIdx = value;
}

void
PgwInfo::SetInfraSwS1uPortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwS1uPortNo = value;
}

void
PgwInfo::SetInfraSwS5PortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwS5PortNo = value;
}

void
PgwInfo::RegisterPgwInfo (Ptr<PgwInfo> pgwInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t pgwId = pgwInfo->GetPgwId ();
  std::pair<uint64_t, Ptr<PgwInfo> > entry (pgwId, pgwInfo);
  std::pair<PgwIdPgwInfo_t::iterator, bool> ret;
  ret = PgwInfo::m_pgwInfoByPgwId.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing P-GW info for this ID.");
}

} // namespace ns3
