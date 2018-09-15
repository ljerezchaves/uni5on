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
#include "pgw-tft-stats-calculator.h"
#include "../metadata/pgw-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwTftStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (PgwTftStatsCalculator);

PgwTftStatsCalculator::PgwTftStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::SliceController/PgwTftAdaptive",
    MakeCallback (&PgwTftStatsCalculator::NotifyPgwTftStats, this));
}

PgwTftStatsCalculator::~PgwTftStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwTftStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwTftStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<PgwTftStatsCalculator> ()
    .AddAttribute ("LbmStatsFilename",
                   "Filename for EPC P-GW TFT statistics.",
                   StringValue ("pgw-tft-stats"),
                   MakeStringAccessor (&PgwTftStatsCalculator::m_tftFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
PgwTftStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_tftWrapper = 0;
  Object::DoDispose ();
}

void
PgwTftStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("LbmStatsFilename", StringValue (prefix + m_tftFilename));

  m_tftWrapper = Create<OutputStreamWrapper> (
      m_tftFilename + ".log", std::ios::out);
  *m_tftWrapper->GetStream ()
    << fixed << setprecision (3) << boolalpha
    << left
    << setw (12) << "Time(s)"
    << right
    << setw (8)  << "CurLev"
    << setw (8)  << "NexLev"
    << setw (8)  << "MaxLev"
    << setw (8)  << "NoTFTs"
    << setw (8)  << "BeaMov"
    << setw (8)  << "BloThs"
    << setw (8)  << "SplThs"
    << setw (8)  << "JoiThs"
    << setw (8)  << "TabUse"
    << setw (8)  << "LoaUse"
    << setw (8)  << "TabSiz"
    << setw (8)  << "MaxEnt"
    << setw (8)  << "SumEnt"
    << setw (8)  << "AvgEnt"
    << setw (14) << "PipCap(Kbps)"
    << setw (14) << "MaxLoa(Kbps)"
    << setw (14) << "SumLoa(Kbps)"
    << setw (14) << "AvgLoa(Kbps)"
    << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
PgwTftStatsCalculator::NotifyPgwTftStats (
  std::string context, Ptr<const PgwInfo> pgwInfo, uint32_t currentLevel,
  uint32_t nextLevel, uint32_t maxLevel, uint32_t bearersMoved,
  double blockThrs, double joinThrs, double splitThrs)
{
  NS_LOG_FUNCTION (this << context);

  uint16_t numTfts = 1 << currentLevel;
  *m_tftWrapper->GetStream ()
    << left
    << setw (11) << Simulator::Now ().GetSeconds ()
    << right
    << " " << setw (8) << currentLevel
    << " " << setw (7) << nextLevel
    << " " << setw (7) << maxLevel
    << " " << setw (7) << numTfts
    << " " << setw (7) << bearersMoved
    << " " << setw (7) << blockThrs
    << " " << setw (7) << splitThrs
    << " " << setw (7) << joinThrs
//    << " " << setw (7) << stats.maxEntries / stats.tableSize
//    << " " << setw (7) << stats.maxLoad / stats.pipeCapacity
//    << " " << setw (7) << static_cast<uint32_t> (stats.tableSize)
//    << " " << setw (7) << static_cast<uint32_t> (stats.maxEntries)
//    << " " << setw (7) << static_cast<uint32_t> (stats.sumEntries)
//    << " " << setw (7) << static_cast<uint32_t> (stats.sumEntries / numTfts)
//    << " " << setw (13) << stats.pipeCapacity / 1000
//    << " " << setw (13) << stats.maxLoad / 1000
//    << " " << setw (13) << stats.sumLoad / 1000
//    << " " << setw (13) << stats.sumLoad / 1000 / numTfts
    << std::endl;
}

} // Namespace ns3
