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
#include "../logical/slice-controller.h"
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
    << boolalpha << right << fixed << setprecision (3)
    << GetTimeHeader ()
    << " " << setw (7)  << "CurLev"
    << " " << setw (7)  << "NexLev"
    << " " << setw (7)  << "MaxLev"
    << " " << setw (7)  << "NumTFT"
    << " " << setw (7)  << "BeaMov"
    << " " << setw (7)  << "BloThs"
    << " " << setw (7)  << "SplThs"
    << " " << setw (7)  << "JoiThs"
    << " " << setw (7)  << "AvgSiz"
    << " " << setw (7)  << "MaxSiz"
    << " " << setw (7)  << "AvgEnt"
    << " " << setw (7)  << "MaxEnt"
    << " " << setw (9)  << "AvgUse:%"
    << " " << setw (9)  << "MaxUse:%"
    << " " << setw (13) << "AvgCap:kbps"
    << " " << setw (13) << "MaxCap:kbps"
    << " " << setw (13) << "AvgLoa:kbps"
    << " " << setw (13) << "MaxLoa:kbps"
    << " " << setw (9)  << "AvgUse:%"
    << " " << setw (9)  << "MaxUse:%"
    << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
PgwTftStatsCalculator::NotifyPgwTftStats (
  std::string context, Ptr<const PgwInfo> pgwInfo, uint32_t nextLevel,
  uint32_t bearersMoved)
{
  NS_LOG_FUNCTION (this << context << pgwInfo << nextLevel << bearersMoved);

  *m_tftWrapper->GetStream ()
    << GetTimeStr ()
    << " " << setw (7)  << pgwInfo->GetCurLevel ()
    << " " << setw (7)  << nextLevel
    << " " << setw (7)  << pgwInfo->GetMaxLevel ()
    << " " << setw (7)  << pgwInfo->GetCurTfts ()
    << " " << setw (7)  << bearersMoved
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwTftBlockThs ()
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwTftSplitThs ()
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwTftJoinThs ()
    << " " << setw (7)  << pgwInfo->GetTftAvgFlowTableMax ()
    << " " << setw (7)  << pgwInfo->GetTftMaxFlowTableMax ()
    << " " << setw (7)  << pgwInfo->GetTftAvgFlowTableCur ()
    << " " << setw (7)  << pgwInfo->GetTftMaxFlowTableCur ()
    << " " << setw (9)  << pgwInfo->GetTftAvgFlowTableUsage () * 100
    << " " << setw (9)  << pgwInfo->GetTftMaxFlowTableUsage () * 100
    << " " << setw (13) << Bps2Kbps (pgwInfo->GetTftAvgPipeCapacityMax ())
    << " " << setw (13) << Bps2Kbps (pgwInfo->GetTftMaxPipeCapacityMax ())
    << " " << setw (13) << Bps2Kbps (pgwInfo->GetTftAvgPipeCapacityCur ())
    << " " << setw (13) << Bps2Kbps (pgwInfo->GetTftMaxPipeCapacityCur ())
    << " " << setw (9)  << pgwInfo->GetTftAvgPipeCapacityUsage () * 100
    << " " << setw (9)  << pgwInfo->GetTftMaxPipeCapacityUsage () * 100
    << std::endl;
}

} // Namespace ns3
