/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
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

#include <algorithm>
#include <ns3/ofswitch13-module.h>
#include "link-sharing.h"
#include "../uni5on-common.h"
#include "../infrastructure/transport-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkSharing");
NS_OBJECT_ENSURE_REGISTERED (LinkSharing);

LinkSharing::LinkSharing (Ptr<TransportController> transpCtrl)
  : m_controller (transpCtrl)
{
  NS_LOG_FUNCTION (this);
}

LinkSharing::~LinkSharing ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LinkSharing::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkSharing")
    .SetParent<Object> ()

    .AddAttribute ("ExtraStep",
                   "Extra bit rate adjustment step.",
                   DataRateValue (DataRate ("12Mbps")),
                   MakeDataRateAccessor (&LinkSharing::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("GuardStep",
                   "Link guard bit rate.",
                   DataRateValue (DataRate ("10Mbps")),
                   MakeDataRateAccessor (&LinkSharing::m_guardStep),
                   MakeDataRateChecker ())
    .AddAttribute ("SharingMode",
                   "Inter-slice operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (ShareMode::NONE),
                   MakeEnumAccessor (&LinkSharing::m_sharingMode),
                   MakeEnumChecker (ShareMode::NONE,
                                    ShareModeStr (ShareMode::NONE),
                                    ShareMode::SHAR,
                                    ShareModeStr (ShareMode::SHAR),
                                    ShareMode::STAT,
                                    ShareModeStr (ShareMode::STAT),
                                    ShareMode::DYNA,
                                    ShareModeStr (ShareMode::DYNA)))
    .AddAttribute ("SpareUse",
                   "Use spare link bit rate for sharing purposes.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&LinkSharing::m_spareUse),
                   MakeBooleanChecker ())
    .AddAttribute ("Timeout",
                   "The interval between adjustment operations.",
                   TimeValue (Seconds (20)),
                   MakeTimeAccessor (&LinkSharing::m_timeout),
                   MakeTimeChecker ())
  ;
  return tid;
}

DataRate
LinkSharing::GetExtraStep (void) const
{
  NS_LOG_FUNCTION (this);

  return m_extraStep;
}

DataRate
LinkSharing::GetGuardStep (void) const
{
  NS_LOG_FUNCTION (this);

  return m_guardStep;
}

LinkSharing::ShareMode
LinkSharing::GetSharingMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sharingMode;
}

bool
LinkSharing::GetSpareUse (void) const
{
  NS_LOG_FUNCTION (this);

  return m_spareUse;
}

Time
LinkSharing::GetTimeout (void) const
{
  NS_LOG_FUNCTION (this);

  return m_timeout;
}

std::string
LinkSharing::ShareModeStr (ShareMode mode)
{
  switch (mode)
    {
    case ShareMode::NONE:
      return "none";
    case ShareMode::SHAR:
      return "shared";
    case ShareMode::STAT:
      return "static";
    case ShareMode::DYNA:
      return "dynamic";
    default:
      NS_LOG_ERROR ("Invalid inter-slice operation mode.");
      return std::string ();
    }
}

void
LinkSharing::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_controller = 0;
  Object::DoDispose ();
}

void
LinkSharing::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Schedule the first timeout operation only when in dynamic operation mode.
  if (GetSharingMode () == ShareMode::DYNA)
    {
      Simulator::Schedule (m_timeout, &LinkSharing::DynamicTimeout, this);
    }

  Object::NotifyConstructionCompleted ();
}

const SliceControllerList_t&
LinkSharing::GetSliceControllerList (bool sharing) const
{
  NS_LOG_FUNCTION (this << sharing);

  return (sharing ? m_sliceCtrlsSha : m_sliceCtrlsAll);
}

void
LinkSharing::DynamicExtraAdjust (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << lInfo << dir);

  // TODO Review this entire method.
  NS_ASSERT_MSG (GetSharingMode () == ShareMode::DYNA,
                 "Invalid inter-slice operation mode.");

  const LinkInfo::EwmaTerm lTerm = LinkInfo::LTERM;
  int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());
  NS_ASSERT_MSG (stepRate > 0, "Invalid ExtraStep attribute value.");

  // Iterate over slices with enabled link sharing
  // to sum the quota bit rate and the used bit rate.
  int64_t maxShareBitRate = 0;
  int64_t useShareBitRate = 0;
  for (auto const &ctrl : GetSliceControllerList (true))
    {
      SliceId slice = ctrl->GetSliceId ();
      maxShareBitRate += lInfo->GetQuoBitRate (dir, slice);
      useShareBitRate += lInfo->GetUseBitRate (lTerm, dir, slice);
    }
  // When enable, sum the spare bit rate too.
  if (GetSpareUse ())
    {
      maxShareBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
    }

  // Get the idle bit rate (apart from the guard bit rate) that can be used as
  // extra bit rate by overloaded slices.
  int64_t guardBitRate = static_cast<int64_t> (m_guardStep.GetBitRate ());
  int64_t idlShareBitRate = maxShareBitRate - guardBitRate - useShareBitRate;

  if (idlShareBitRate > 0)
    {
      // We have some unused bit rate step that can be distributed as extra to
      // any overloaded slice. Iterate over slices with enabled link sharing in
      // decreasing priority order, assigning one extra bit rate to those slices
      // that may benefit from it. Also, gets back one extra bit rate from
      // underloaded slices to reduce unnecessary overbooking.
      for (auto it = m_sliceCtrlsSha.rbegin ();
           it != m_sliceCtrlsSha.rend (); ++it)
        {
          // Get the idle and extra bit rates for this slice.
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          if (sliceIdl < (stepRate / 2) && idlShareBitRate >= stepRate)
            {
              // This is an overloaded slice and we have idle bit rate.
              // Increase the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Increase extra bit rate.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate -= stepRate;
            }
          else if (sliceIdl >= (stepRate * 2) && sliceExt >= stepRate)
            {
              // This is an underloaded slice with some extra bit rate.
              // Decrease the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
            }
        }
    }
  else
    {
      // Link usage is over the safeguard threshold. First, iterate over slices
      // with enable link sharing and get back any unused extra bit rate to
      // reduce unnecessary overbooking.
      for (auto const &ctrl : GetSliceControllerList (true))
        {
          // Get the idle and extra bit rates for this slice.
          SliceId slice = ctrl->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // Remove all unused extra bit rate (step by step) from this slice.
          while (sliceIdl >= stepRate && sliceExt >= stepRate)
            {
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              sliceIdl -= stepRate;
              sliceExt -= stepRate;
            }
        }

      // At this point there is no slices with more than one step of unused
      // extra bit rate. Now, iterate again over slices with enabled link
      // sharing in increasing priority order, removing some extra bit rate from
      // those slices that are using more than its quota to get the link usage
      // below the safeguard threshold again.
      bool removedFlag = false;
      auto it = m_sliceCtrlsSha.begin ();
      auto sp = m_sliceCtrlsSha.begin ();
      while (it != m_sliceCtrlsSha.end () && idlShareBitRate < 0)
        {
          // Check if the slice priority has increased to update the sp.
          if ((*it)->GetPriority () > (*sp)->GetPriority ())
            {
              NS_ASSERT_MSG (!removedFlag, "Inconsistent removed flag.");
              sp = it;
            }

          // Get the idle and extra bit rates for this slice.
          SliceId slice = (*it)->GetSliceId ();
          int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
          int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
          NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // If possible, decrease the slice extra bit rate by one step.
          if (sliceExt >= stepRate)
            {
              removedFlag = true;
              NS_ASSERT_MSG (sliceIdl < stepRate, "Inconsistent bit rate.");
              NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
              bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate += stepRate - sliceIdl;
            }

          // Select the slice for the next loop iteration.
          auto nextIt = std::next (it);
          bool isLast = (nextIt == m_sliceCtrlsSha.end ());
          if ((!isLast && (*nextIt)->GetPriority () == (*it)->GetPriority ())
              || (removedFlag == false))
            {
              // Go to the next slice if it has the same priority of the
              // current one or if no more extra bit rate can be recovered from
              // slices with the current priority.
              it = nextIt;
            }
          else
            {
              // Go back to the first slice with the current priority (can be
              // the current slice) and reset the removed flag.
              NS_ASSERT_MSG (removedFlag, "Inconsistent removed flag.");
              it = sp;
              removedFlag = false;
            }
        }
    }

  // Update the slicing meters for all slices over this link.
  for (auto const &ctrl : GetSliceControllerList (true))
    {
      MeterAdjust (lInfo, ctrl->GetSliceId ());
    }
}

void
LinkSharing::DynamicTimeout (void)
{
  NS_LOG_FUNCTION (this);

  // Adjust the extra bit rates in both directions for each transport link.
  for (auto &lInfo : LinkInfo::GetList ())
    {
      for (auto &dir : LinkInfo::GetDirs ())
        {
          DynamicExtraAdjust (lInfo, dir);
        }
    }

  // Schedule the next sharing timeout operation.
  Simulator::Schedule (m_timeout, &LinkSharing::DynamicTimeout, this);
}

void
LinkSharing::NotifyHandshakeSuccessful (uint64_t swDpId)
{
  NS_LOG_FUNCTION (this << swDpId);


  switch (GetSharingMode ())
    {
    case ShareMode::NONE:
      // Nothing to do when link sharing is disabled.
      break;

    case ShareMode::SHAR:
      // Apply high-priority individual Non-GBR meter entries for slices with
      // disabled link sharing and the low-priority shared Non-GBR meter entry
      // for other slices.
      MeterApply (swDpId, SliceId::ALL);
      for (auto const &ctrl : GetSliceControllerList ())
        {
          if (ctrl->GetSharing () == OpMode::OFF)
            {
              MeterApply (swDpId, ctrl->GetSliceId ());
            }
        }
      break;

    case ShareMode::STAT:
    case ShareMode::DYNA:
      // Apply individual Non-GBR meter entries for each slice.
      for (auto const &ctrl : GetSliceControllerList ())
        {
          MeterApply (swDpId, ctrl->GetSliceId ());
        }
      break;

    default:
      NS_LOG_WARN ("Undefined link sharing operation mode.");
      break;
    }
}

// Comparator for slice priorities.
static bool
PriComp (Ptr<SliceController> ctrl1, Ptr<SliceController> ctrl2)
{
  return ctrl1->GetPriority () < ctrl2->GetPriority ();
}

void
LinkSharing::NotifySlicesBuilt (ApplicationContainer &controllers)
{
  NS_LOG_FUNCTION (this);

  ApplicationContainer::Iterator it;
  for (it = controllers.Begin (); it != controllers.End (); ++it)
    {
      Ptr<SliceController> controller = DynamicCast<SliceController> (*it);
      m_sliceCtrlsAll.push_back (controller);
      if (controller->GetSharing () == OpMode::ON)
        {
          m_sliceCtrlsSha.push_back (controller);
        }
    }

  // Sort slice controllers in increasing priority order.
  std::stable_sort (m_sliceCtrlsAll.begin (), m_sliceCtrlsAll.end (), PriComp);
  std::stable_sort (m_sliceCtrlsSha.begin (), m_sliceCtrlsSha.end (), PriComp);

  // Install inter-slicing meters, depending on the InterShareMode attribute.
  switch (GetSharingMode ())
    {
    case ShareMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case ShareMode::SHAR:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install high-priority individual Non-GBR meter entries for slices
          // with disabled link sharing and the low-priority shared Non-GBR
          // meter entry for other slices.
          MeterInstall (lInfo, SliceId::ALL);
          for (auto const &ctrl : GetSliceControllerList ())
            {
              if (ctrl->GetSharing () == OpMode::OFF)
                {
                  MeterInstall (lInfo, ctrl->GetSliceId ());
                }
            }
        }
      break;

    case ShareMode::STAT:
    case ShareMode::DYNA:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install individual Non-GBR meter entries.
          for (auto const &ctrl : GetSliceControllerList ())
            {
              MeterInstall (lInfo, ctrl->GetSliceId ());
            }
        }
      break;

    default:
      NS_LOG_WARN ("Undefined link sharing operation mode.");
      break;
    }
}

void
LinkSharing::MeterAdjust (
  Ptr<LinkInfo> lInfo, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << slice);

  // Update inter-slicing meter, depending on the InterShareMode attribute.
  NS_ASSERT_MSG (slice < SliceId::ALL, "Invalid slice for this operation.");
  switch (GetSharingMode ())
    {
    case ShareMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case ShareMode::SHAR:
      // Identify the Non-GBR meter entry to adjust: individual or shared.
      if (m_controller->GetSliceController (slice)->GetSharing () == OpMode::ON)
        {
          slice = SliceId::ALL;
        }
      break;

    case ShareMode::STAT:
    case ShareMode::DYNA:
      // Update the individual Non-GBR meter entry.
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }

  // Check for updated slicing meters in both link directions.
  for (auto &dir : LinkInfo::GetDirs ())
    {
      int64_t meterBitRate = 0;
      if (slice == SliceId::ALL)
        {
          // Iterate over slices with enabled link sharing
          // to sum the quota bit rate.
          for (auto const &ctrl : GetSliceControllerList (true))
            {
              SliceId slc = ctrl->GetSliceId ();
              meterBitRate += lInfo->GetUnrBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (GetSpareUse ())
            {
              meterBitRate += lInfo->GetUnrBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetUnrBitRate (dir, slice);
        }

      m_controller->SharingMeterUpdate (lInfo, dir, slice, meterBitRate);
    }
}

void
LinkSharing::MeterInstall (Ptr<LinkInfo> lInfo, SliceId slice)
{
  NS_LOG_FUNCTION (this << lInfo << slice);

  NS_ASSERT_MSG (GetSharingMode () != ShareMode::NONE,
                 "Invalid link sharing operation mode.");

  // Install slicing meters in both link directions.
  for (auto &dir : LinkInfo::GetDirs ())
    {
      int64_t meterBitRate = 0;
      if (slice == SliceId::ALL)
        {
          NS_ASSERT_MSG (GetSharingMode () == ShareMode::SHAR,
                         "Invalid link sharing operation mode.");

          // Iterate over slices with enabled link sharing
          // to sum the quota bit rate.
          for (auto const &ctrl : GetSliceControllerList (true))
            {
              SliceId slc = ctrl->GetSliceId ();
              meterBitRate += lInfo->GetQuoBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (GetSpareUse ())
            {
              meterBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetQuoBitRate (dir, slice);
        }

      m_controller->SharingMeterInstall (lInfo, dir, slice, meterBitRate);
    }
}

void
LinkSharing::MeterApply (uint64_t swDpId, SliceId slice)
{
  NS_LOG_FUNCTION (this << swDpId << slice);

  // Apply slicing meters in both link directions.
  for (auto &dir : LinkInfo::GetDirs ())
    {
      m_controller->SharingMeterApply (swDpId, dir, slice);
    }
}

} // namespace ns3
