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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
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
    "/NodeList/*/ApplicationList/*/$ns3::SliceController/PgwTftLoadBal",
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
                   StringValue ("pgw-tft-loadbal"),
                   MakeStringAccessor (&PgwTftStatsCalculator::m_tftFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
PgwTftStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  for (int s = 0; s < N_SLICE_IDS; s++)
    {
      m_slices [s].tftWrapper = 0;
    }
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

  for (int s = 0; s < N_SLICE_IDS; s++)
    {
      std::string sliceStr = SliceIdStr (static_cast<SliceId> (s));
      SliceMetadata &slData = m_slices [s];

      // Create the output file for this slice.
      slData.tftWrapper = Create<OutputStreamWrapper> (
          m_tftFilename + "-" + sliceStr + ".log", std::ios::out);

      // Print the header in output file.
      *slData.tftWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8)  << "TimeSec"
        << " " << setw (7)  << "CurLev"
        << " " << setw (7)  << "NexLev"
        << " " << setw (7)  << "MaxLev"
        << " " << setw (7)  << "NumTft"
        << " " << setw (7)  << "BeaMov"
        << " " << setw (7)  << "BloThs"
        << " " << setw (7)  << "SplThs"
        << " " << setw (7)  << "JoiThs"
        << " " << setw (9)  << "AvgTabSiz"
        << " " << setw (9)  << "MaxTabSiz"
        << " " << setw (9)  << "AvgTabEnt"
        << " " << setw (9)  << "MaxTabEnt"
        << " " << setw (9)  << "AvgTabUse"
        << " " << setw (9)  << "MaxTabUse"
        << " " << setw (11) << "AvgCpuMax"
        << " " << setw (11) << "MaxCpuMax"
        << " " << setw (11) << "AvgCpuLoa"
        << " " << setw (11) << "MaxCpuLoa"
        << " " << setw (9)  << "AvgCpuUse"
        << " " << setw (9)  << "MaxCpuUse"
        << std::endl;
    }

  Object::NotifyConstructionCompleted ();
}

void
PgwTftStatsCalculator::NotifyPgwTftStats (
  std::string context, Ptr<const PgwInfo> pgwInfo, uint32_t nextLevel,
  uint32_t bearersMoved)
{
  NS_LOG_FUNCTION (this << context << pgwInfo << nextLevel << bearersMoved);

  SliceId slice = pgwInfo->GetSliceCtrl ()->GetSliceId ();
  *m_slices [slice].tftWrapper->GetStream ()
    << " " << setw (8)  << Simulator::Now ().GetSeconds ()
    << " " << setw (7)  << pgwInfo->GetCurLevel ()
    << " " << setw (7)  << nextLevel
    << " " << setw (7)  << pgwInfo->GetMaxLevel ()
    << " " << setw (7)  << pgwInfo->GetCurTfts ()
    << " " << setw (7)  << bearersMoved
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwBlockThs ()
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwTftSplitThs ()
    << " " << setw (7)  << pgwInfo->GetSliceCtrl ()->GetPgwTftJoinThs ()
    << " " << setw (9)  << pgwInfo->GetTftAvgFlowTableMax ()
    << " " << setw (9)  << pgwInfo->GetTftMaxFlowTableMax ()
    << " " << setw (9)  << pgwInfo->GetTftAvgFlowTableCur ()
    << " " << setw (9)  << pgwInfo->GetTftMaxFlowTableCur ()
    << " " << setw (9)  << pgwInfo->GetTftAvgFlowTableUse () * 100
    << " " << setw (9)  << pgwInfo->GetTftMaxFlowTableUse () * 100
    << " " << setw (11) << Bps2Kbps (pgwInfo->GetTftAvgCpuMax ())
    << " " << setw (11) << Bps2Kbps (pgwInfo->GetTftMaxCpuMax ())
    << " " << setw (11) << Bps2Kbps (pgwInfo->GetTftAvgEwmaCpuCur ())
    << " " << setw (11) << Bps2Kbps (pgwInfo->GetTftMaxEwmaCpuCur ())
    << " " << setw (9)  << pgwInfo->GetTftAvgEwmaCpuUse () * 100
    << " " << setw (9)  << pgwInfo->GetTftMaxEwmaCpuUse () * 100
    << std::endl;
}

} // Namespace ns3
