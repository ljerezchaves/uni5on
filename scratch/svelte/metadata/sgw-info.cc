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
#include "sgw-info.h"
#include "../logical/slice-controller.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SgwInfo");
NS_OBJECT_ENSURE_REGISTERED (SgwInfo);

SgwInfo::SgwInfo (
  uint32_t sgwId, Ptr<OFSwitch13Device> device, Ipv4Address s1uAddr,
  Ipv4Address s5Addr, uint32_t s1uPortNo, uint32_t s5PortNo,
  uint16_t infraSwIdx, uint32_t infraSwS1uPortNo, uint32_t infraSwS5PortNo,
  Ptr<SliceController> ctrlApp)
  : m_device (device),
  m_infraSwIdx (infraSwIdx),
  m_infraSwS1uPortNo (infraSwS1uPortNo),
  m_infraSwS5PortNo (infraSwS5PortNo),
  m_s1uAddr (s1uAddr),
  m_s1uPortNo (s1uPortNo),
  m_s5Addr (s5Addr),
  m_s5PortNo (s5PortNo),
  m_sgwId (sgwId),
  m_sliceCtrl (ctrlApp)
{
  NS_LOG_FUNCTION (this);
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
SgwInfo::GetDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_device->GetDatapathId ();
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

Ipv4Address
SgwInfo::GetS1uAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddr;
}

uint32_t
SgwInfo::GetS1uPortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s1uPortNo;
}

Ipv4Address
SgwInfo::GetS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5Addr;
}

uint32_t
SgwInfo::GetS5PortNo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5PortNo;
}

uint32_t
SgwInfo::GetSgwId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwId;
}

Ptr<SliceController>
SgwInfo::GetSliceController (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceCtrl;
}

uint32_t
SgwInfo::GetFlowTableCur (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  return GetStats ()->GetEwmaFlowTableEntries (tableId);
}

uint32_t
SgwInfo::GetFlowTableMax (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  return m_device->GetFlowTableSize (tableId);
}

double
SgwInfo::GetFlowTableUsage (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<double> (GetFlowTableCur (tableId)) /
         static_cast<double> (GetFlowTableMax (tableId));
}

DataRate
SgwInfo::GetPipeCapacityCur (void) const
{
  NS_LOG_FUNCTION (this);

  return GetStats ()->GetEwmaProcessingLoad ();
}

DataRate
SgwInfo::GetPipeCapacityMax (void) const
{
  NS_LOG_FUNCTION (this);

  return m_device->GetProcessingCapacity ();
}

double
SgwInfo::GetPipeCapacityUsage (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<double> (GetPipeCapacityCur ().GetBitRate ()) /
         static_cast<double> (GetPipeCapacityMax ().GetBitRate ());
}

std::ostream &
SgwInfo::PrintHeader (std::ostream &os)
{
  os << " " << setw (6)  << "SgwId"
     << " " << setw (6)  << "SgwDp"
     << " " << setw (6)  << "SgwSw"
     << " " << setw (11) << "SgwS1Addr"
     << " " << setw (11) << "SgwS5Addr";
  return os;
}

void
SgwInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sliceCtrl = 0;
  Object::DoDispose ();
}

Ptr<OFSwitch13StatsCalculator>
SgwInfo::GetStats (void) const
{
  Ptr<OFSwitch13StatsCalculator> stats;
  stats = m_device->GetObject<OFSwitch13StatsCalculator> ();
  NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");
  return stats;
}

std::ostream & operator << (std::ostream &os, const SgwInfo &sgwInfo)
{
  // Trick to preserve alignment.
  std::ostringstream ipS1Str, ipS5Str;
  sgwInfo.GetS1uAddr ().Print (ipS1Str);
  sgwInfo.GetS5Addr ().Print (ipS5Str);

  os << " " << setw (6)  << sgwInfo.GetSgwId ()
     << " " << setw (6)  << sgwInfo.GetDpId ()
     << " " << setw (6)  << sgwInfo.GetInfraSwIdx ()
     << " " << setw (11) << ipS1Str.str ()
     << " " << setw (11) << ipS5Str.str ();
  return os;
}

} // namespace ns3
