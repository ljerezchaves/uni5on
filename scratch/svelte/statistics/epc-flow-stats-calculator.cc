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
NS_OBJECT_ENSURE_REGISTERED (EpcStatsCalculator); // FIXME

EpcStatsCalculator::EpcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  for (int r = 0; r <= DropReason::ALL; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
}

EpcStatsCalculator::~EpcStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcStatsCalculator")
    .SetParent<FlowStatsCalculator> ()
    .AddConstructor<EpcStatsCalculator> ()
  ;
  return tid;
}

void
EpcStatsCalculator::ResetCounters (void)
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
EpcStatsCalculator::GetDpBytes (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpBytes [reason];
}

uint32_t
EpcStatsCalculator::GetDpPackets (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpPackets [reason];
}

void
EpcStatsCalculator::NotifyDrop (uint32_t dpBytes, DropReason reason)
{
  NS_LOG_FUNCTION (this << dpBytes << reason);

  m_dpPackets [reason]++;
  m_dpBytes [reason] += dpBytes;

  m_dpPackets [DropReason::ALL]++;
  m_dpBytes [DropReason::ALL] += dpBytes;
}

std::ostream &
EpcStatsCalculator::PrintHeader (std::ostream &os)
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
EpcStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  FlowStatsCalculator::DoDispose ();
}

std::ostream & operator << (std::ostream &os, const EpcStatsCalculator &stats)
{
  os << static_cast<FlowStatsCalculator> (stats);
  for (int r = 0; r <= EpcStatsCalculator::ALL; r++)
    {
      os << " " << setw (6) << stats.GetDpPackets (
        static_cast<EpcStatsCalculator::DropReason> (r));
    }
  return os;
}

} // Namespace ns3
