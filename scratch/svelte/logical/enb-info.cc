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
  : m_cellId (cellId)
{
  NS_LOG_FUNCTION (this);

  m_enbS1uAddr = Ipv4Address ();

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
EnbInfo::GetEnbS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbS1uAddr;
}

EpcS1apSapEnb*
EnbInfo::GetS1apSapEnb (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1apSapEnb;
}

void
EnbInfo::SetEnbS1uAddr (Ipv4Address value)
{
  NS_LOG_FUNCTION (this << value);

  m_enbS1uAddr = value;
}

void
EnbInfo::SetS1apSapEnb (EpcS1apSapEnb* value)
{
  NS_LOG_FUNCTION (this);

  m_s1apSapEnb = value;
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
EnbInfo::RegisterEnbInfo (Ptr<EnbInfo> enbInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t cellId = enbInfo->GetCellId ();
  std::pair<uint16_t, Ptr<EnbInfo> > entry (cellId, enbInfo);
  std::pair<CellIdEnbInfo_t::iterator, bool> ret;
  ret = EnbInfo::m_enbInfoByCellId.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing eNB information for cell ID " << cellId);
    }
}

};  // namespace ns3
