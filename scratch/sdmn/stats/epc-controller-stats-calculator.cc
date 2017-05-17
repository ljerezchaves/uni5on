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
#include "epc-controller-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcCtrlStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (EpcCtrlStatsCalculator);

EpcCtrlStatsCalculator::EpcCtrlStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcController/LoadBalancing",
    MakeCallback (&EpcCtrlStatsCalculator::NotifyLoadBalancing, this));
}

EpcCtrlStatsCalculator::~EpcCtrlStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcCtrlStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcCtrlStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<EpcCtrlStatsCalculator> ()
    .AddAttribute ("LbmStatsFilename",
                   "Filename for EPC P-GW load balancing statistics.",
                   StringValue ("epc-controller-loadbal.log"),
                   MakeStringAccessor (&EpcCtrlStatsCalculator::m_lbmFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
EpcCtrlStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_lbmWrapper = 0;
}

void
EpcCtrlStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("LbmStatsFilename", StringValue (prefix + m_lbmFilename));

  m_lbmWrapper = Create<OutputStreamWrapper> (m_lbmFilename, std::ios::out);
  *m_lbmWrapper->GetStream ()
  << fixed << setprecision (3) << boolalpha
  << left
  << setw (12) << "Time(s)"
  << right
  << setw (8)  << "MaxLev"
  << setw (8)  << "NoTFTs"
  << setw (8)  << "CurLev"
  << setw (8)  << "NexLev"
  << setw (8)  << "BeaMov"
  << setw (8)  << "TabSiz"
  << setw (8)  << "MaxEnt"
  << setw (8)  << "AvgEnt"
  << setw (12) << "PipCap"
  << setw (12) << "MaxLoa"
  << setw (12) << "AvgLoa"
  << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
EpcCtrlStatsCalculator::NotifyLoadBalancing (
  std::string context, EpcController::LoadBalancingStats stats)
{
  NS_LOG_FUNCTION (this << context);

  uint16_t numTfts = 1 << stats.currentLevel;
  *m_lbmWrapper->GetStream ()
  << left
  << setw (11) << Simulator::Now ().GetSeconds ()
  << right
  << " " << setw (8) << stats.maxLevel
  << " " << setw (7) << numTfts
  << " " << setw (7) << stats.currentLevel
  << " " << setw (7) << stats.nextLevel
  << " " << setw (7) << stats.bearersMoved
  << " " << setw (7) << stats.tableSize
  << " " << setw (7) << stats.maxEntries
  << " " << setw (7) << stats.avgEntries
  << " " << setw (11) << (double)(stats.pipeCapacity.GetBitRate ()) / 1000
  << " " << setw (11) << (double)(stats.maxLoad.GetBitRate ()) / 1000
  << " " << setw (11) << (double)(stats.avgLoad.GetBitRate ()) / 1000
  << std::endl;
}

} // Namespace ns3
