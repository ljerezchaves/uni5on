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

#include "sgw-info.h"
#include "../logical/slice-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SgwInfo");
NS_OBJECT_ENSURE_REGISTERED (SgwInfo);

// Initializing SgwInfo static members.
SgwInfo::SgwIdSgwInfo_t SgwInfo::m_sgwInfoBySgwId;

SgwInfo::SgwInfo (
  uint64_t sgwId, Ipv4Address s1uAddr, Ipv4Address s5Addr, uint32_t s1uPortNo,
  uint32_t s5PortNo, uint16_t infraSwIdx, uint32_t infraSwS1uPortNo,
  uint32_t infraSwS5PortNo, Ptr<SliceController> ctrlApp)
  : m_sgwId (sgwId),
  m_s1uAddr (s1uAddr),
  m_s5Addr (s5Addr),
  m_s1uPortNo (s1uPortNo),
  m_s5PortNo (s5PortNo),
  m_infraSwIdx (infraSwIdx),
  m_infraSwS1uPortNo (infraSwS1uPortNo),
  m_infraSwS5PortNo (infraSwS5PortNo),
  m_sliceCtrl (ctrlApp)
{
  NS_LOG_FUNCTION (this);

  RegisterSgwInfo (Ptr<SgwInfo> (this));
}

SgwInfo::~SgwInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SgwInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SgwInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint64_t
SgwInfo::GetSgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwId;
}

uint64_t
SgwInfo::GetDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwId;
}

SliceId
SgwInfo::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl->GetSliceId ();
}

Ipv4Address
SgwInfo::GetS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddr;
}

Ipv4Address
SgwInfo::GetS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5Addr;
}

uint32_t
SgwInfo::GetS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uPortNo;
}

uint32_t
SgwInfo::GetS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5PortNo;
}

uint16_t
SgwInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint32_t
SgwInfo::GetInfraSwS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS1uPortNo;
}

uint32_t
SgwInfo::GetInfraSwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS5PortNo;
}

Ptr<SliceController>
SgwInfo::GetSliceCtrl (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl;
}

Ptr<SgwInfo>
SgwInfo::GetPointer (uint64_t sgwId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<SgwInfo> sgwInfo = 0;
  SgwIdSgwInfo_t::iterator ret;
  ret = SgwInfo::m_sgwInfoBySgwId.find (sgwId);
  if (ret != SgwInfo::m_sgwInfoBySgwId.end ())
    {
      sgwInfo = ret->second;
    }
  return sgwInfo;
}

void
SgwInfo::DoDispose ()
{
  m_sliceCtrl = 0;
  NS_LOG_FUNCTION (this);
}

void
SgwInfo::RegisterSgwInfo (Ptr<SgwInfo> sgwInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t sgwId = sgwInfo->GetSgwId ();
  std::pair<uint64_t, Ptr<SgwInfo> > entry (sgwId, sgwInfo);
  std::pair<SgwIdSgwInfo_t::iterator, bool> ret;
  ret = SgwInfo::m_sgwInfoBySgwId.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing S-GW info for this ID.");
}

} // namespace ns3
