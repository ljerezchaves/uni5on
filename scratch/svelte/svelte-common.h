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

#ifndef SVELTE_COMMON_H
#define SVELTE_COMMON_H

#include <string>

namespace ns3 {

/**
 * \ingroup svelte
 * Enumeration of LTE logical interfaces.
 */
typedef enum
{
  // Don't change enum order. S1U and S5 are used as array indices in RingInfo.
  S1U  = 0,   //!< S1-U interface connecting eNB to S-GW.
  S5   = 1,   //!< S5 interface connecting S-GW to P-GW.
  X2   = 2,   //!< X2 interface connecting eNB to eNB.
  SGI  = 3    //!< SGi interface connecting P-GW to Internet.
} LteIface;

/**
 * \ingroup svelte
 * Get the LTE interface name.
 * \param iface The LTE interface.
 * \return The string with the LTE interface name.
 */
std::string LteIfaceStr (LteIface iface);

/**
 * \ingroup svelte
 * Enumeration of available operation modes.
 */
typedef enum
{
  OFF  = 0,   //!< Always off.
  ON   = 1,   //!< Always on.
  AUTO = 2    //!< Automatic.
} OpMode;

/**
 * \ingroup svelte
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OpModeStr (OpMode mode);

/**
 * \ingroup svelte
 * Enumeration of available SVELTE logical slices IDs.
 * \internal Slice IDs are restricted to the range [1, 15] by the current
 * TEID allocation strategy.
 */
typedef enum
{
  NONE = 0,   //!< Undefined slice.
  HTC  = 1,   //!< Slice for HTC UEs.
  MTC  = 2    //!< Slice for MTC UEs.
} SliceId;

/**
 * \ingroup svelte
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

} // namespace ns3
#endif // SVELTE_COMMON_H
