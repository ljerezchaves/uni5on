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

#include "pgwu-scaling.h"
#include "../slices/slice-controller.h"
#include "../metadata/bearer-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwuScaling");
NS_OBJECT_ENSURE_REGISTERED (PgwuScaling);

PgwuScaling::PgwuScaling (Ptr<PgwInfo> pgwInfo, Ptr<SliceController> slcCtrl)
  : m_controller (slcCtrl),
  m_pgwInfo (pgwInfo)
{
  NS_LOG_FUNCTION (this);
}

PgwuScaling::~PgwuScaling ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwuScaling::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwuScaling")
    .SetParent<Object> ()

    .AddAttribute ("ScalingMode",
                   "P-GW TFT scaling operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::OFF),
                   MakeEnumAccessor (&PgwuScaling::m_scalingMode),
                   MakeEnumChecker (OpMode::OFF,  OpModeStr (OpMode::OFF),
                                    OpMode::ON,   OpModeStr (OpMode::ON),
                                    OpMode::AUTO, OpModeStr (OpMode::AUTO)))
    .AddAttribute ("JoinThs",
                   "The P-GW TFT join threshold.",
                   DoubleValue (0.30),
                   MakeDoubleAccessor (&PgwuScaling::m_joinThs),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("SplitThs",
                   "The P-GW TFT split threshold.",
                   DoubleValue (0.80),
                   MakeDoubleAccessor (&PgwuScaling::m_splitThs),
                   MakeDoubleChecker<double> (0.5, 1.0))
    .AddAttribute ("StartMax",
                   "When in auto mode, start with maximum number of P-GW TFTs.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&PgwuScaling::m_startMax),
                   MakeBooleanChecker ())
    .AddAttribute ("Timeout",
                   "The interval between P-GW TFT scaling operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&PgwuScaling::m_timeout),
                   MakeTimeChecker (Seconds (1)))

    .AddTraceSource ("ScalingStats", "P-GW TFT scaling trace source.",
                     MakeTraceSourceAccessor (&PgwuScaling::m_pgwScalingTrace),
                     "ns3::PgwuScaling::PgwTftScalingTracedCallback")
  ;
  return tid;
}

Ptr<SliceController>
PgwuScaling::GetSliceCtrl (void) const
{
  NS_LOG_FUNCTION (this);

  return m_controller;
}

Ptr<PgwInfo>
PgwuScaling::GetPgwInfo (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwInfo;
}

uint16_t
PgwuScaling::GetCurLevel (void) const
{
  NS_LOG_FUNCTION (this);

  return m_level;
}

uint16_t
PgwuScaling::GetCurTfts (void) const
{
  NS_LOG_FUNCTION (this);

  return 1 << m_level;
}

uint16_t
PgwuScaling::GetMaxLevel (void) const
{
  NS_LOG_FUNCTION (this);

  return static_cast<uint16_t> (log2 (m_pgwInfo->GetNumTfts ()));
}

OpMode
PgwuScaling::GetScalingMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_scalingMode;
}

double
PgwuScaling::GetJoinThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_joinThs;
}

double
PgwuScaling::GetSplitThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_splitThs;
}

bool
PgwuScaling::GetStartMax (void) const
{
  NS_LOG_FUNCTION (this);

  return m_startMax;
}

Time
PgwuScaling::GetTimeout (void) const
{
  NS_LOG_FUNCTION (this);

  return m_timeout;
}

uint32_t
PgwuScaling::GetTftAvgFlowTableCur (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftFlowTableCur (idx, tableId);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwuScaling::GetTftAvgFlowTableMax (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftFlowTableMax (idx, tableId);
    }
  return value / GetCurTfts ();
}

double
PgwuScaling::GetTftAvgFlowTableUse (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftFlowTableUse (idx, tableId);
    }
  return value / GetCurTfts ();
}

DataRate
PgwuScaling::GetTftAvgEwmaCpuCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftEwmaCpuCur (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

DataRate
PgwuScaling::GetTftAvgCpuMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftCpuMax (idx).GetBitRate ();
    }
  return DataRate (value / GetCurTfts ());
}

double
PgwuScaling::GetTftAvgEwmaCpuUse (void) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value += m_pgwInfo->GetTftEwmaCpuUse (idx);
    }
  return value / GetCurTfts ();
}

uint32_t
PgwuScaling::GetTftMaxFlowTableMax (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftFlowTableMax (idx, tableId));
    }
  return value;
}

uint32_t
PgwuScaling::GetTftMaxFlowTableCur (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  uint32_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftFlowTableCur (idx, tableId));
    }
  return value;
}

double
PgwuScaling::GetTftMaxFlowTableUse (uint8_t tableId) const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftFlowTableUse (idx, tableId));
    }
  return value;
}

DataRate
PgwuScaling::GetTftMaxEwmaCpuCur (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftEwmaCpuCur (idx).GetBitRate ());
    }
  return DataRate (value);
}

DataRate
PgwuScaling::GetTftMaxCpuMax (void) const
{
  NS_LOG_FUNCTION (this);

  uint64_t value = 0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftCpuMax (idx).GetBitRate ());
    }
  return DataRate (value);
}

double
PgwuScaling::GetTftMaxEwmaCpuUse () const
{
  NS_LOG_FUNCTION (this);

  double value = 0.0;
  for (uint16_t idx = 0; idx < GetCurTfts (); idx++)
    {
      value = std::max (value, m_pgwInfo->GetTftEwmaCpuUse (idx));
    }
  return value;
}

uint16_t
PgwuScaling::GetTftIdx (Ptr<const BearerInfo> bInfo, uint16_t nTfts) const
{
  NS_LOG_FUNCTION (this << bInfo << nTfts);

  return (bInfo->GetUeAddr ().Get () % nTfts);
}

void
PgwuScaling::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_pgwInfo = 0;
  m_controller = 0;
  Object::DoDispose ();
}

void
PgwuScaling::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Set the P-GW TFT initial level.
  switch (GetScalingMode ())
    {
    case OpMode::OFF:
      {
        m_level = 0;
        break;
      }
    case OpMode::ON:
      {
        m_level = GetMaxLevel ();
        break;
      }
    case OpMode::AUTO:
      {
        m_level = m_startMax ? GetMaxLevel () : 0;
        break;
      }
    }

  // Schedule the first P-GW TFT scaling operation.
  Simulator::Schedule (m_timeout, &PgwuScaling::PgwTftScaling, this);
}

void
PgwuScaling::NotifyBearerCreated (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());

  // Set the initial TFT index for this bearer.
  bInfo->SetPgwTftIdx (GetTftIdx (bInfo, GetCurTfts ()));
}

void
PgwuScaling::PgwTftScaling (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_pgwInfo, "No P-GW attached to this slice.");

  // Check for valid P-GW TFT thresholds attributes.
  NS_ASSERT_MSG (m_splitThs < m_controller->GetPgwBlockThs ()
                 && m_splitThs > 2 * m_joinThs,
                 "The split threshold should be smaller than the block "
                 "threshold and two times larger than the join threshold.");

  uint16_t nextLevel = GetCurLevel ();
  if (GetScalingMode () == OpMode::AUTO)
    {
      double maxTabUse = GetTftMaxFlowTableUse ();
      double maxCpuUse = GetTftMaxEwmaCpuUse ();

      // We may increase the level when we hit the split threshold.
      if ((GetCurLevel () < GetMaxLevel ())
          && (maxTabUse >= GetSplitThs () || maxCpuUse >= GetSplitThs ()))
        {
          NS_LOG_INFO ("Increasing the P-GW scaling level.");
          nextLevel++;
        }

      // We may decrease the level when we hit the join threshold.
      else if ((GetCurLevel () > 0)
               && (maxTabUse < GetJoinThs ()) && (maxCpuUse < GetJoinThs ()))
        {
          NS_LOG_INFO ("Decreasing the P-GW scaling level.");
          nextLevel--;
        }
    }

  // Check if we need to update the level.
  uint32_t moved = 0;
  if (GetCurLevel () != nextLevel)
    {
      uint16_t futureTfts = 1 << nextLevel;

      // Random variable to avoid simultaneously moving all bearers.
      Ptr<RandomVariableStream> rand = CreateObject<UniformRandomVariable> ();
      rand->SetAttribute ("Min", DoubleValue (0));
      rand->SetAttribute ("Max", DoubleValue (250));

      // Iterate over all bearers for this slice, updating the P-GW TFT switch
      // index and moving the bearer when necessary.
      BearerInfoList_t bearerList;
      BearerInfo::GetList (bearerList, m_controller->GetSliceId ());
      for (auto const &bInfo : bearerList)
        {
          uint16_t currIdx = bInfo->GetPgwTftIdx ();
          uint16_t destIdx = GetTftIdx (bInfo, futureTfts);
          if (destIdx != currIdx)
            {
              if (!bInfo->IsGwInstalled ())
                {
                  // Update the P-GW TFT switch index so new rules will be
                  // installed in the new switch.
                  bInfo->SetPgwTftIdx (destIdx);
                }
              else
                {
                  // Schedule the rules transfer from old to new switch.
                  moved++;
                  NS_LOG_INFO ("Move bearer teid " << bInfo->GetTeidHex () <<
                               " from TFT " << currIdx << " to " << destIdx);
                  Simulator::Schedule (
                    MilliSeconds (rand->GetInteger ()),
                    &SliceController::PgwRulesMove,
                    m_controller, bInfo, currIdx, destIdx);
                }
            }
        }

      // Schedule the update on the P-GW DL and UL switches.
      Simulator::Schedule (
        MilliSeconds (500),
        &SliceController::PgwTftLevelUpdate,
        m_controller, nextLevel);
    }

  // Fire the P-GW scaling trace source.
  m_pgwScalingTrace (Ptr<PgwuScaling> (this), nextLevel, moved);

  // Update the current operation level.
  m_level = nextLevel;

  // Schedule the next P-GW TFT scaling operation.
  Simulator::Schedule (m_timeout, &PgwuScaling::PgwTftScaling, this);
}

} // namespace ns3
