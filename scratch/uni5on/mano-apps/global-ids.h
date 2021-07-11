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

#ifndef INTER_TEID_H
#define INTER_TEID_H

#include <ns3/core-module.h>
#include "../uni5on-common.h"

namespace ns3 {

// TEID masks for OpenFlow matching.
#define TEID_STRICT_MASK    0xFFFFFFFF
#define TEID_SLICE_MASK     0x0F000000
#define TEID_IMSI_MASK      0x00FFFFF0
#define TEID_BID_MASK       0x0000000F

// Cookie masks for OpenFlow matching.
#define COOKIE_STRICT_MASK  0xFFFFFFFFFFFFFFFF
#define COOKIE_IFACE_MASK   0x000F000000000000
#define COOKIE_PRIO_MASK    0x0000FFFF00000000
#define COOKIE_TEID_MASK    0x00000000FFFFFFFF

#define COOKIE_IFACE_TEID_MASK \
  ((COOKIE_IFACE_MASK | COOKIE_TEID_MASK))
#define COOKIE_PRIO_TEID_MASK \
  ((COOKIE_PRIO_MASK | COOKIE_TEID_MASK))
#define COOKIE_IFACE_PRIO_TEID_MASK \
  ((COOKIE_IFACE_MASK | COOKIE_PRIO_MASK | COOKIE_TEID_MASK))

// Meter ID masks.
#define METER_SLC_TYPE      0xC0000000
#define METER_MBR_TYPE      0x80000000
#define METER_IFACE_MASK    0x30000000
#define METER_SLICE_MASK    0x0F000000

/**
 * \ingroup uni5onMano
 * The inter-slice application for global TEID allocation.
 */
class GlobalIds
{
public:
  /**
   * Compute the TEID value globally used in the UNI5ON architecture for a EPS
   * bearer considering the slice ID, the UE ISMI and bearer ID.
   * \param sliceId The logical slice ID.
   * \param ueImsi The UE ISMI.
   * \param bearerId The UE bearer ID.
   * \return The TEID value.
   *
   * \internal
   * We are using the following TEID allocation strategy:
   * \verbatim
   * TEID has 32 bits length: 0x 0 0 00000 0
   *                            |-|-|-----|-|
   *                             A B C     D
   *
   *  4 (A) bits are used to identify a valid TEID, here fixed at 0x0.
   *  4 (B) bits are used to identify the logical slice (slice ID).
   * 20 (C) bits are used to identify the UE (IMSI).
   *  4 (D) bits are used to identify the bearer withing the UE (bearer ID).
   * \endverbatim
   */
  static uint32_t TeidCreate (SliceId sliceId, uint32_t ueImsi,
                              uint32_t bearerId);

  /**
   * Get a TEID value only with the sliceID for masked matching purposes.
   * \param sliceId The logical slice ID.
   * \return The TEID value.
   */
  static uint32_t TeidSliceMask (SliceId sliceId);

  /**
   * Decompose the TEID to get the UE bearer ID.
   * \param teid The GTP tunnel ID.
   * \return The UE bearer ID for this tunnel.
   */
  static uint8_t TeidGetBearerId (uint32_t teid);

  /**
   * Decompose the TEID to get the slice ID.
   * \param teid The GTP tunnel ID.
   * \return The slice ID for this tunnel.
   */
  static SliceId TeidGetSliceId (uint32_t teid);

  /**
   * Decompose the TEID to get the UE IMSI.
   * \param teid The GTP tunnel ID.
   * \return The UE IMSI for this tunnel.
   */
  static uint64_t TeidGetUeImsi (uint32_t teid);

  /**
   * Compute the cookie value globally used in the UNI5ON architecture for
   * OpenFlow rules considering the bearer TEID, the rule priority, and the
   * logical interface.
   * \param teid The TEID value.
   * \param prio The OpenFlow rule priority.
   * \param iface The logical interface.
   * \return The cookie value.
   *
   * \internal
   * We are using the following cookie allocation strategy:
   * \verbatim
   * Cookie has 64 bits length: 0x 000 0 0000 00000000
   *                              |---|-|----|--------|
   *                               A   B C    D
   *
   * 12 (A) bits are currently unused, here fixed at 0x000.
   *  4 (B) bits are used to identify the logical interface.
   * 16 (C) bits are used to identify the rule priority.
   * 32 (D) bits are used to identify the bearer TEID.
   * \endverbatim
   */
  static uint64_t CookieCreate (EpsIface iface, uint16_t prio, uint32_t teid);

  /**
   * Decompose the cookie to get the bearer TEID.
   * \param cookie The OpenFlow cookie.
   * \return The bearer TEID.
   */
  static uint32_t CookieGetTeid (uint64_t cookie);

  /**
   * Decompose the cookie to get the rule priority.
   * \param cookie The OpenFlow cookie.
   * \return The OpenFlow rule priority.
   */
  static uint16_t CookieGetPriority (uint64_t cookie);

  /**
   * Decompose the cookie to get the logical interface.
   * \param cookie The OpenFlow cookie.
   * \return The logical interface.
   */
  static EpsIface CookieGetIface (uint64_t cookie);

  /**
   * Compute the meter ID value globally used in the UNI5ON architecture for
   * infrastructure MBR meters.
   * \param iface The logical interface.
   * \param teid The GTP tunnel ID.
   * \return The meter ID value.
   *
   * \internal
   * We are using the following meter ID allocation strategy:
   * \verbatim
   * Meter ID has 32 bits length: 0x 0 0000000
   *                                |-|-------|
   *                                 A B
   *
   *  4 (A) bits are used to identify a MBR meter: the first 2 bits are fixed
   *        here at 10 and the next 2 bits are used to identify the logical
   *        interface.
   * 28 (B) bits are used to identify the GTP tunnel ID (TEID).
   * \endverbatim
   */
  static uint32_t MeterIdMbrCreate (EpsIface iface, uint32_t teid);

  /**
   * Compute the meter ID value globally used in the UNI5ON architecture for
   * infrastructure slicing meters.
   * \param sliceId The logical slice ID.
   * \param linkdir The link direction.
   * \return The meter ID value.
   *
   * \internal
   * We are using the following meter ID allocation strategy:
   * \verbatim
   * Meter ID has 32 bits length: 0x 0 0 00000 0
   *                                |-|-|-----|-|
   *                                 A B C     D
   *
   *  4 (A) bits are used to identify a slicing meter, here fixed at 0xC.
   *  4 (B) bits are used to identify the logical slice (slice ID).
   * 20 (C) bits are unused, here fixed at 0x00000.
   *  4 (D) bits are used to identify the link direction.
   * \endverbatim
   */
  static uint32_t MeterIdSlcCreate (SliceId sliceId, uint32_t linkdir);
};

} // namespace ns3
#endif // INTER_TEID_H
