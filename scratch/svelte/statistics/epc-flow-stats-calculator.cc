/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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
#include "epc-flow-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcFlowStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (EpcFlowStatsCalculator);

EpcFlowStatsCalculator::EpcFlowStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  for (int r = 0; r <= DropReason::ALL; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
}

EpcFlowStatsCalculator::~EpcFlowStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcFlowStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcFlowStatsCalculator")
    .SetParent<FlowStatsCalculator> ()
    .AddConstructor<EpcFlowStatsCalculator> ()
  ;
  return tid;
}

void
EpcFlowStatsCalculator::ResetCounters (void)
{
  NS_LOG_FUNCTION (this);

  for (int r = 0; r <= DropReason::ALL; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
  FlowStatsCalculator::ResetCounters ();
}

uint32_t
EpcFlowStatsCalculator::GetDpBytes (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpBytes [reason];
}

uint32_t
EpcFlowStatsCalculator::GetDpPackets (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpPackets [reason];
}

void
EpcFlowStatsCalculator::NotifyDrop (uint32_t dpBytes, DropReason reason)
{
  NS_LOG_FUNCTION (this << dpBytes << reason);

  m_dpPackets [reason]++;
  m_dpBytes [reason] += dpBytes;

  m_dpPackets [DropReason::ALL]++;
  m_dpBytes [DropReason::ALL] += dpBytes;
}

std::ostream &
EpcFlowStatsCalculator::PrintHeader (std::ostream &os)
{
  FlowStatsCalculator::PrintHeader (os);
  os << " " << setw (6) << "DpLoa"
     << " " << setw (6) << "DpMbr"
     << " " << setw (6) << "DpSli"
     << " " << setw (6) << "DpQue"
     << " " << setw (6) << "DpAll";
  return os;
}

void
EpcFlowStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  FlowStatsCalculator::DoDispose ();
}

std::ostream & operator << (std::ostream &os,
                            const EpcFlowStatsCalculator &stats)
{
  os << static_cast<FlowStatsCalculator> (stats);
  for (int r = 0; r <= EpcFlowStatsCalculator::ALL; r++)
    {
      os << " " << setw (6) << stats.GetDpPackets (
        static_cast<EpcFlowStatsCalculator::DropReason> (r));
    }
  return os;
}

} // Namespace ns3
