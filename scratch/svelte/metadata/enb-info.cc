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

#include "enb-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EnbInfo");
NS_OBJECT_ENSURE_REGISTERED (EnbInfo);

// Initializing EnbInfo static members.
EnbInfo::CellIdEnbInfo_t EnbInfo::m_enbInfoByCellId;

EnbInfo::EnbInfo (uint16_t cellId)
  : m_cellId (cellId),
  m_infraSwIdx (0),
  m_infraSwPortNo (0)
{
  NS_LOG_FUNCTION (this);

  RegisterEnbInfo (Ptr<EnbInfo> (this));
}

EnbInfo::~EnbInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EnbInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EnbInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint16_t
EnbInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_cellId;
}

Ipv4Address
EnbInfo::GetS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddr;
}

uint16_t
EnbInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint32_t
EnbInfo::GetInfraSwPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwPortNo;
}

EpcS1apSapEnb*
EnbInfo::GetS1apSapEnb (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1apSapEnb;
}

Ptr<EnbInfo>
EnbInfo::GetPointer (uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EnbInfo> enbInfo = 0;
  CellIdEnbInfo_t::iterator ret;
  ret = EnbInfo::m_enbInfoByCellId.find (cellId);
  if (ret != EnbInfo::m_enbInfoByCellId.end ())
    {
      enbInfo = ret->second;
    }
  return enbInfo;
}

void
EnbInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
EnbInfo::SetS1uAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_s1uAddr = value;
}

void
EnbInfo::SetInfraSwIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwIdx = value;
}

void
EnbInfo::SetInfraSwPortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwPortNo = value;
}

void
EnbInfo::SetS1apSapEnb (EpcS1apSapEnb* value)
{
  NS_LOG_FUNCTION (this);

  m_s1apSapEnb = value;
}

void
EnbInfo::RegisterEnbInfo (Ptr<EnbInfo> enbInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t cellId = enbInfo->GetCellId ();
  std::pair<uint16_t, Ptr<EnbInfo> > entry (cellId, enbInfo);
  std::pair<CellIdEnbInfo_t::iterator, bool> ret;
  ret = EnbInfo::m_enbInfoByCellId.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing eNB info for this cell ID.");
}

} // namespace ns3