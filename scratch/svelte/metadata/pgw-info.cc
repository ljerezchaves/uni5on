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

#include <iomanip>
#include <iostream>
#include "pgw-info.h"
#include "../logical/slice-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwInfo");
NS_OBJECT_ENSURE_REGISTERED (PgwInfo);

// Initializing PgwInfo static members.
PgwInfo::PgwIdPgwInfoMap_t PgwInfo::m_pgwInfoByPgwId;

PgwInfo::PgwInfo (uint32_t pgwId, uint16_t nTfts, uint32_t sgiPortNo,
                  uint16_t infraSwIdx, Ptr<SliceController> ctrlApp)
  : m_infraSwIdx (infraSwIdx),
  m_pgwId (pgwId),
  m_sgiPortNo (sgiPortNo),
  m_sliceCtrl (ctrlApp),
  m_tftSwitches (nTfts)
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

uint16_t
PgwInfo::GetInfraSwIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwIdx;
}

uint32_t
PgwInfo::GetPgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwId;
}

Ptr<SliceController>
PgwInfo::GetSliceCtrl (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl;
}

uint16_t
PgwInfo::GetCurLevel (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftLevel;
}

uint16_t
PgwInfo::GetCurTfts (void) const
{
  NS_LOG_FUNCTION (this);

  return 1 << m_tftLevel;
}

uint16_t
PgwInfo::GetMaxLevel (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (log2 (GetMaxTfts ()));
}

uint16_t
PgwInfo::GetMaxTfts (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftSwitches;
}

uint32_t
PgwInfo::GetFlowTableCur (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_devices.size (), "Invalid index.");
  Ptr<OFSwitch13StatsCalculator> stats;
  stats = m_devices.at (idx)->GetObject<OFSwitch13StatsCalculator> ();
  NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");
  return stats->GetEwmaFlowEntries ();
}

uint32_t
PgwInfo::GetFlowTableMax (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_devices.size (), "Invalid index.");
  return m_devices.at (idx)->GetFlowTableSize ();
}

double
PgwInfo::GetFlowTableUsage (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  return static_cast<double> (GetFlowTableCur (idx)) /
         static_cast<double> (GetFlowTableMax (idx));
}

DataRate
PgwInfo::GetPipeCapacityCur (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_devices.size (), "Invalid index.");
  Ptr<OFSwitch13StatsCalculator> stats;
  stats = m_devices.at (idx)->GetObject<OFSwitch13StatsCalculator> ();
  NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");
  return stats->GetEwmaPipelineLoad ();
}

DataRate
PgwInfo::GetPipeCapacityMax (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_devices.size (), "Invalid index.");
  return m_devices.at (idx)->GetPipelineCapacity ();
}

double
PgwInfo::GetPipeCapacityUsage (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  return static_cast<double> (GetPipeCapacityCur (idx).GetBitRate ()) /
         static_cast<double> (GetPipeCapacityMax (idx).GetBitRate ());
}

uint64_t
PgwInfo::GetMainDpId (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_infraSwS5PortNos.size (), "No P-GW main switch registered");
  return m_devices.at (0)->GetDatapathId ();
}

uint32_t
PgwInfo::GetMainInfraSwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_infraSwS5PortNos.size (), "No P-GW main switch registered");
  return m_infraSwS5PortNos.at (0);
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
PgwInfo::GetMainSgiPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgiPortNo;
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
  NS_ASSERT_MSG (idx < m_devices.size (), "Invalid TFT index.");
  return m_devices.at (idx)->GetDatapathId ();
}

uint32_t
PgwInfo::GetTftInfraSwS5PortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_infraSwS5PortNos.size (), "Invalid TFT index.");
  return m_infraSwS5PortNos.at (idx);
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
PgwInfo::GetTftToMainPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx > 0, "Invalid TFT index.");
  NS_ASSERT_MSG (idx < m_tftToMainPortNos.size (), "Invalid TFT index.");
  return m_tftToMainPortNos.at (idx);
}

uint32_t
PgwInfo::GetTftAvgFlowTableCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetFlowTableCur (idx);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwInfo::GetTftAvgFlowTableMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetFlowTableMax (idx);
    }
  return value / GetCurTfts ();
}

double
PgwInfo::GetTftAvgFlowTableUsage (void) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetFlowTableUsage (idx);
    }
  return value / GetCurTfts ();
}

DataRate
PgwInfo::GetTftAvgPipeCapacityCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetPipeCapacityCur (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

DataRate
PgwInfo::GetTftAvgPipeCapacityMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetPipeCapacityMax (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

double
PgwInfo::GetTftAvgPipeCapacityUsage (void) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value += GetPipeCapacityUsage (idx);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwInfo::GetTftMaxFlowTableMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetFlowTableMax (idx));
    }
  return value;
}

uint32_t
PgwInfo::GetTftMaxFlowTableCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetFlowTableCur (idx));
    }
  return value;
}

double
PgwInfo::GetTftMaxFlowTableUsage () const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetFlowTableUsage (idx));
    }
  return value;
}

DataRate
PgwInfo::GetTftMaxPipeCapacityCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetPipeCapacityCur (idx).GetBitRate ());
    }
  return DataRate (value);
}

DataRate
PgwInfo::GetTftMaxPipeCapacityMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetPipeCapacityMax (idx).GetBitRate ());
    }
  return DataRate (value);
}

double
PgwInfo::GetTftMaxPipeCapacityUsage () const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 1; idx <= GetCurTfts (); idx++)
    {
      value = std::max (value, GetPipeCapacityUsage (idx));
    }
  return value;
}

Ptr<PgwInfo>
PgwInfo::GetPointer (uint64_t pgwId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<PgwInfo> pgwInfo = 0;
  PgwIdPgwInfoMap_t::iterator ret;
  ret = PgwInfo::m_pgwInfoByPgwId.find (pgwId);
  if (ret != PgwInfo::m_pgwInfoByPgwId.end ())
    {
      pgwInfo = ret->second;
    }
  return pgwInfo;
}

std::ostream &
PgwInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (6)  << "PgwID"
     << " " << setw (6)  << "PgwSw"
     << " " << setw (11) << "PgwS5Addr";
  return os;
}

void
PgwInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sliceCtrl = 0;
  m_devices.clear ();
  Object::DoDispose ();
}

void
PgwInfo::SaveSwitchInfo (Ptr<OFSwitch13Device> device, Ipv4Address s5Addr,
                         uint32_t s5PortNo, uint32_t infraSwS5PortNo,
                         uint32_t mainToTftPortNo, uint32_t tftToMainPortNo)
{
  NS_LOG_FUNCTION (this << device << s5Addr << s5PortNo << infraSwS5PortNo);

  m_devices.push_back (device);
  m_s5Addrs.push_back (s5Addr);
  m_s5PortNos.push_back (s5PortNo);
  m_infraSwS5PortNos.push_back (infraSwS5PortNo);
  m_mainToTftPortNos.push_back (mainToTftPortNo);
  m_tftToMainPortNos.push_back (tftToMainPortNo);
}

void
PgwInfo::SetTftLevel (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_tftLevel = value;
}

void
PgwInfo::RegisterPgwInfo (Ptr<PgwInfo> pgwInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t pgwId = pgwInfo->GetPgwId ();
  std::pair<uint64_t, Ptr<PgwInfo> > entry (pgwId, pgwInfo);
  auto ret = PgwInfo::m_pgwInfoByPgwId.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing P-GW info for this ID.");
}

std::ostream & operator << (std::ostream &os, const PgwInfo &pgwInfo)
{
  // Trick to preserve alignment.
  std::ostringstream ipS5Str;
  pgwInfo.GetMainS5Addr ().Print (ipS5Str);

  os << " " << setw (6)  << pgwInfo.GetPgwId ()
     << " " << setw (6)  << pgwInfo.GetInfraSwIdx ()
     << " " << setw (11) << ipS5Str.str ();
  return os;
}

} // namespace ns3
