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
  m_infraSwIdx (0),
  m_sgiPortNo (0),
  m_nTfts (0),
  m_tftPipeCapacity (DataRate (std::numeric_limits<uint64_t>::max ())),
  m_tftFlowTableSize (std::numeric_limits<uint32_t>::max ())
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

uint16_t
PgwInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint16_t
PgwInfo::GetNumTfts (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nTfts;
}

uint32_t
PgwInfo::GetMainSgiPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgiPortNo;
}

uint64_t
PgwInfo::GetMainDpId (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_dpIds.size (), "No P-GW main switch registered.");
  NS_ASSERT_MSG (m_dpIds.at (0) == m_pgwId, "Inconsistent metadata.");
  return m_dpIds.at (0);
}

Ipv4Address
PgwInfo::GetMainS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_s5Addrs.size (), "No P-GW main switch registered.");
  return m_s5Addrs.at (0);
}

uint32_t
PgwInfo::GetMainS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_s5PortNos.size (), "No P-GW main switch registered.");
  return m_s5PortNos.at (0);
}

uint32_t
PgwInfo::GetMainInfraSwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_infraSwS5PortNos.size (), "No P-GW main switch registered.");
  return m_infraSwS5PortNos.at (0);
}

uint32_t
PgwInfo::GetMainToTftPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_mainToTftPortNos.size (), "Invalid TFT index.");
  return m_mainToTftPortNos.at (idx);
}

uint64_t
PgwInfo::GetTftDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_dpIds.size (), "Invalid TFT index.");
  return m_dpIds.at (idx);
}

Ipv4Address
PgwInfo::GetTftS5Addr (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_s5Addrs.size (), "Invalid TFT index.");
  return m_s5Addrs.at (idx);
}

uint32_t
PgwInfo::GetTftS5PortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_s5PortNos.size (), "Invalid TFT index.");
  return m_s5PortNos.at (idx);
}

uint32_t
PgwInfo::GetTftInfraSwS5PortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_infraSwS5PortNos.size (), "Invalid TFT index.");
  return m_infraSwS5PortNos.at (idx);
}

uint32_t
PgwInfo::GetTftFlowTableSize () const
{
  NS_LOG_FUNCTION (this);

  return m_tftFlowTableSize;
}

DataRate
PgwInfo::GetTftPipelineCapacity () const
{
  NS_LOG_FUNCTION (this);

  return m_tftPipeCapacity;
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
PgwInfo::SetInfraSwIdx (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_infraSwIdx = value;
}

void
PgwInfo::SetNumTfts (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_nTfts = value;
}

void
PgwInfo::SetMainSgiPortNo (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgiPortNo = value;
}

void
PgwInfo::SaveSwitchInfo (uint64_t dpId, Ipv4Address s5Addr, uint32_t s5PortNo,
                         uint32_t infraSwS5PortNo, uint32_t mainToTftPortNo)
{
  NS_LOG_FUNCTION (this << dpId << s5Addr << s5PortNo << infraSwS5PortNo);

  m_dpIds.push_back (dpId);
  m_s5Addrs.push_back (s5Addr);
  m_s5PortNos.push_back (s5PortNo);
  m_infraSwS5PortNos.push_back (infraSwS5PortNo);
  m_mainToTftPortNos.push_back (mainToTftPortNo);

  // Nothing to do for the P-GW MAIN switch.
  if (m_dpIds.size () == 1) return;

  // Get the minimum table size and pipeline capacity of P-GW TFT switches.
  Ptr<OFSwitch13Device> dev = OFSwitch13Device::GetDevice (dpId);
  uint32_t tableSize = dev->GetFlowTableSize ();
  DataRate pipeCapacity = dev->GetPipelineCapacity ();

  m_tftFlowTableSize = std::min (m_tftFlowTableSize, tableSize);
  m_tftPipeCapacity = std::min (m_tftPipeCapacity, pipeCapacity);
  NS_LOG_DEBUG ("P-GW TFT min flow table size set to " << m_tftFlowTableSize);
  NS_LOG_DEBUG ("P-GW TFT min pipeline capacity set to " << m_tftPipeCapacity);
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
