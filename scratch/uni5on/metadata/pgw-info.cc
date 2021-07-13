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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <iomanip>
#include <iostream>
#include "pgw-info.h"
#include "../slices/slice-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwInfo");
NS_OBJECT_ENSURE_REGISTERED (PgwInfo);

PgwInfo::PgwInfo (uint32_t pgwId, uint16_t nTfts, Ptr<SliceController> ctrlApp)
  : m_pgwId (pgwId),
  m_sliceCtrl (ctrlApp),
  m_tftSwitches (nTfts)
{
  NS_LOG_FUNCTION (this);
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
PgwInfo::GetInfraSwS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_infraSwS5PortNo;
}

uint32_t
PgwInfo::GetPgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwId;
}

Ipv4Address
PgwInfo::GetS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5Addr;
}

Ipv4Address
PgwInfo::GetSgiAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgiAddr;
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

uint64_t
PgwInfo::GetDlDpId (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_ulDlDevices.GetN () == 2, "No P-GW DL switch registered");
  return m_ulDlDevices.Get (PGW_DL_IDX)->GetDatapathId ();
}

uint64_t
PgwInfo::GetUlDpId (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_ulDlDevices.GetN () == 2, "No P-GW UL switch registered");
  return m_ulDlDevices.Get (PGW_UL_IDX)->GetDatapathId ();
}

uint32_t
PgwInfo::GetDlSgiPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgiPortNo;
}

uint32_t
PgwInfo::GetUlS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5PortNo;
}

uint32_t
PgwInfo::GetDlToTftPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_dlToTftPortNos.size (), "Invalid TFT index.");
  return m_dlToTftPortNos.at (idx);
}

uint32_t
PgwInfo::GetUlToTftPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_ulToTftPortNos.size (), "Invalid TFT index.");
  return m_ulToTftPortNos.at (idx);
}

uint32_t
PgwInfo::GetTftToDlPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftToDlPortNos.size (), "Invalid TFT index.");
  return m_tftToDlPortNos.at (idx);
}

uint32_t
PgwInfo::GetTftToUlPortNo (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftToUlPortNos.size (), "Invalid TFT index.");
  return m_tftToUlPortNos.at (idx);
}

uint32_t
PgwInfo::GetTftFlowTableCur (uint16_t idx, uint8_t tableId) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid index.");
  return m_tftDevices.Get (idx)->GetFlowTableEntries (tableId);
}

uint32_t
PgwInfo::GetTftFlowTableMax (uint16_t idx, uint8_t tableId) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid index.");
  return m_tftDevices.Get (idx)->GetFlowTableSize (tableId);
}

double
PgwInfo::GetTftFlowTableUse (uint16_t idx, uint8_t tableId) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid index.");
  return m_tftDevices.Get (idx)->GetFlowTableUsage (tableId);
}

DataRate
PgwInfo::GetTftEwmaCpuCur (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid index.");
  return GetTftStats (idx)->GetEwmaCpuLoad ();
}

double
PgwInfo::GetTftEwmaCpuUse (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  return static_cast<double> (GetTftEwmaCpuCur (idx).GetBitRate ()) /
         static_cast<double> (GetTftCpuMax (idx).GetBitRate ());
}

DataRate
PgwInfo::GetTftCpuMax (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid index.");
  return m_tftDevices.Get (idx)->GetCpuCapacity ();
}

uint64_t
PgwInfo::GetTftDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_tftDevices.GetN (), "Invalid TFT index.");
  return m_tftDevices.Get (idx)->GetDatapathId ();
}

uint32_t
PgwInfo::GetTftAvgFlowTableCur (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftFlowTableCur (idx, tableId);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwInfo::GetTftAvgFlowTableMax (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftFlowTableMax (idx, tableId);
    }
  return value / GetCurTfts ();
}

double
PgwInfo::GetTftAvgFlowTableUse (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftFlowTableUse (idx, tableId);
    }
  return value / GetCurTfts ();
}

DataRate
PgwInfo::GetTftAvgEwmaCpuCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftEwmaCpuCur (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

DataRate
PgwInfo::GetTftAvgCpuMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftCpuMax (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

double
PgwInfo::GetTftAvgEwmaCpuUse (void) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += GetTftEwmaCpuUse (idx);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwInfo::GetTftMaxFlowTableMax (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftFlowTableMax (idx, tableId));
    }
  return value;
}

uint32_t
PgwInfo::GetTftMaxFlowTableCur (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftFlowTableCur (idx, tableId));
    }
  return value;
}

double
PgwInfo::GetTftMaxFlowTableUse (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftFlowTableUse (idx, tableId));
    }
  return value;
}

DataRate
PgwInfo::GetTftMaxEwmaCpuCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftEwmaCpuCur (idx).GetBitRate ());
    }
  return DataRate (value);
}

DataRate
PgwInfo::GetTftMaxCpuMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftCpuMax (idx).GetBitRate ());
    }
  return DataRate (value);
}

double
PgwInfo::GetTftMaxEwmaCpuUse () const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, GetTftEwmaCpuUse (idx));
    }
  return value;
}

std::ostream &
PgwInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (6)  << "PgwId"
     << " " << setw (6)  << "PgwSw"
     << " " << setw (11) << "PgwS5Addr";
  return os;
}

std::ostream &
PgwInfo::PrintNull (std::ostream &os)
{
  os << " " << setw (6)  << "-"
     << " " << setw (6)  << "-"
     << " " << setw (11) << "-";
  return os;
}

void
PgwInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sliceCtrl = 0;
  Object::DoDispose ();
}

Ptr<OFSwitch13StatsCalculator>
PgwInfo::GetTftStats (uint16_t idx) const
{
  Ptr<OFSwitch13StatsCalculator> stats;
  stats = m_tftDevices.Get (idx)->GetObject<OFSwitch13StatsCalculator> ();
  NS_ABORT_MSG_IF (!stats, "Enable OFSwitch13 datapath stats.");
  return stats;
}

void
PgwInfo::SaveTftInfo (
  Ptr<OFSwitch13Device> device, uint32_t tftToDlPortNo,
  uint32_t tftToUlPortNo, uint32_t dlToTftPortNo, uint32_t ulToTftPortNo)
{
  NS_LOG_FUNCTION (this << device << tftToDlPortNo << tftToUlPortNo <<
                   dlToTftPortNo << ulToTftPortNo);

  m_tftDevices.Add (device);
  m_tftToDlPortNos.push_back (tftToDlPortNo);
  m_tftToUlPortNos.push_back (tftToUlPortNo);
  m_dlToTftPortNos.push_back (dlToTftPortNo);
  m_ulToTftPortNos.push_back (ulToTftPortNo);
}

void
PgwInfo::SaveUlDlInfo (
  Ptr<OFSwitch13Device> dlDevice, Ptr<OFSwitch13Device> ulDevice,
  uint32_t sgiPortNo, Ipv4Address sgiAddr,
  uint32_t s5PortNo, Ipv4Address s5Addr,
  uint16_t infraSwIdx, uint32_t infraSwS5PortNo)
{
  NS_LOG_FUNCTION (this << dlDevice << ulDevice << sgiPortNo << sgiAddr <<
                   s5PortNo << s5Addr << infraSwIdx << infraSwS5PortNo);

  m_ulDlDevices.Add (dlDevice);
  m_ulDlDevices.Add (ulDevice);
  m_sgiPortNo = sgiPortNo;
  m_sgiAddr = sgiAddr;
  m_s5PortNo = s5PortNo;
  m_s5Addr = s5Addr;
  m_infraSwIdx = infraSwIdx;
  m_infraSwS5PortNo = infraSwS5PortNo;
}

void
PgwInfo::SetCurLevel (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_tftLevel = value;
}

std::ostream & operator << (std::ostream &os, const PgwInfo &pgwInfo)
{
  // Trick to preserve alignment.
  std::ostringstream ipS5Str;
  pgwInfo.GetS5Addr ().Print (ipS5Str);

  os << " " << setw (6)  << pgwInfo.GetPgwId ()
     << " " << setw (6)  << pgwInfo.GetInfraSwIdx ()
     << " " << setw (11) << ipS5Str.str ();
  return os;
}

} // namespace ns3
