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

#ifndef SLICE_ID_H
#define SLICE_ID_H

#include <string>

namespace ns3 {

/**
 * \ingroup svelteLogical
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
 * \ingroup svelteLogical
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

} // namespace ns3
#endif // SLICE_ID_H
