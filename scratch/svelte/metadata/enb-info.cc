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
#include "enb-info.h"
#include "../infrastructure/uni5on-enb-application.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EnbInfo");
NS_OBJECT_ENSURE_REGISTERED (EnbInfo);

// Initializing EnbInfo static members.
EnbInfo::CellIdEnbInfoMap_t EnbInfo::m_enbInfoByCellId;

EnbInfo::EnbInfo (uint16_t cellId, Ipv4Address s1uAddr, uint16_t infraSwIdx,
                  uint32_t infraSwS1uPortNo, Ptr<Uni5onEnbApplication> enbApp)
  : m_application (enbApp),
  m_cellId (cellId),
  m_infraSwIdx (infraSwIdx),
  m_infraSwS1uPortNo (infraSwS1uPortNo),
  m_s1uAddr (s1uAddr)
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

Ptr<Uni5onEnbApplication>
EnbInfo::GetApplication (void) const
{
  NS_LOG_FUNCTION (this);

  return m_application;
}

uint16_t
EnbInfo::GetCellId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_cellId;
}

uint16_t
EnbInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint32_t
EnbInfo::GetInfraSwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS1uPortNo;
}

EpcS1apSapEnb*
EnbInfo::GetS1apSapEnb (void) const
{
  NS_LOG_FUNCTION (this);

  return m_application->GetS1apSapEnb ();
}

Ipv4Address
EnbInfo::GetS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddr;
}

Ptr<EnbInfo>
EnbInfo::GetPointer (uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<EnbInfo> enbInfo = 0;
  auto ret = EnbInfo::m_enbInfoByCellId.find (cellId);
  if (ret != EnbInfo::m_enbInfoByCellId.end ())
    {
      enbInfo = ret->second;
    }
  return enbInfo;
}

std::ostream &
EnbInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (6)  << "EnbId"
     << " " << setw (6)  << "EnbSw"
     << " " << setw (11) << "EnbS1Addr";
  return os;
}

std::ostream &
EnbInfo::PrintNull (std::ostream &os)
{
  os << " " << setw (6)  << "-"
     << " " << setw (6)  << "-"
     << " " << setw (11) << "-";
  return os;
}

void
EnbInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_application = 0;
  Object::DoDispose ();
}

void
EnbInfo::RegisterEnbInfo (Ptr<EnbInfo> enbInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t cellId = enbInfo->GetCellId ();
  std::pair<uint16_t, Ptr<EnbInfo> > entry (cellId, enbInfo);
  auto ret = EnbInfo::m_enbInfoByCellId.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing eNB info for this cell ID.");
}

std::ostream & operator << (std::ostream &os, const EnbInfo &enbInfo)
{
  // Trick to preserve alignment.
  std::ostringstream ipStr;
  enbInfo.GetS1uAddr ().Print (ipStr);

  os << " " << setw (6)  << enbInfo.GetCellId ()
     << " " << setw (6)  << enbInfo.GetInfraSwIdx ()
     << " " << setw (11) << ipStr.str ();
  return os;
}

} // namespace ns3
