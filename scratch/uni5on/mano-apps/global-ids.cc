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

#include "global-ids.h"

namespace ns3 {

uint32_t
GlobalIds::TeidCreate (SliceId sliceId, uint32_t ueImsi, uint32_t bearerId)
{
  NS_ASSERT_MSG (sliceId <= 0xF, "Slice ID cannot exceed 4 bits.");
  NS_ASSERT_MSG (ueImsi <= 0xFFFFF, "UE IMSI cannot exceed 20 bits.");
  NS_ASSERT_MSG (bearerId <= 0xF, "Bearer ID cannot exceed 4 bits.");

  uint32_t teid = 0x0;
  teid <<= 4;
  teid |= static_cast<uint32_t> (sliceId);
  teid <<= 20;
  teid |= ueImsi;
  teid <<= 4;
  teid |= bearerId;
  return teid;
}

uint32_t
GlobalIds::TeidSliceMask (SliceId sliceId)
{
  return (TeidCreate (sliceId, 0, 0) & TEID_SLICE_MASK);
}

uint8_t
GlobalIds::TeidGetBearerId (uint32_t teid)
{
  teid &= TEID_BID_MASK;
  return static_cast<uint8_t> (teid);
}

SliceId
GlobalIds::TeidGetSliceId (uint32_t teid)
{
  teid &= TEID_SLICE_MASK;
  teid >>= 24;
  return static_cast<SliceId> (teid);
}

uint64_t
GlobalIds::TeidGetUeImsi (uint32_t teid)
{
  teid &= TEID_IMSI_MASK;
  teid >>= 4;
  return static_cast<uint64_t> (teid);
}

uint64_t
GlobalIds::CookieCreate (EpsIface iface, uint16_t prio, uint32_t teid)
{
  NS_ASSERT_MSG (iface <= 0xF, "Interface cannot exceed 4 bits.");
  NS_ASSERT_MSG (prio <= 0xFFFF, "Rule priority cannot exceed 16 bits.");
  NS_ASSERT_MSG (teid <= 0xFFFFFFFF, "TEID cannot exceed 32 bits.");

  uint64_t cookie = 0x0;
  cookie <<= 4;
  cookie |= static_cast<uint32_t> (iface);
  cookie <<= 16;
  cookie |= prio;
  cookie <<= 32;
  cookie |= teid;
  return cookie;
}

uint32_t
GlobalIds::CookieGetTeid (uint64_t cookie)
{
  cookie &= COOKIE_TEID_MASK;
  return static_cast<uint32_t> (cookie);
}

uint16_t
GlobalIds::CookieGetPriority (uint64_t cookie)
{
  cookie &= COOKIE_PRIO_MASK;
  cookie >>= 32;
  return static_cast<uint16_t> (cookie);
}

EpsIface
GlobalIds::CookieGetIface (uint64_t cookie)
{
  cookie &= COOKIE_IFACE_MASK;
  cookie >>= 48;
  return static_cast<EpsIface> (cookie);
}

uint32_t
GlobalIds::MeterIdMbrCreate (EpsIface iface, uint32_t teid)
{
  NS_ASSERT_MSG (iface <= 0x3, "Interface cannot exceed 2 bits.");
  NS_ASSERT_MSG (teid <= 0xFFFFFFF, "TEID cannot exceed 28 bits.");

  uint32_t meterId = 0x2;
  meterId <<= 2;
  meterId |= static_cast<uint32_t> (iface);
  meterId <<= 28;
  meterId |= teid;
  return meterId;
}

uint32_t
GlobalIds::MeterIdSlcCreate (SliceId sliceId, uint32_t linkdir)
{
  NS_ASSERT_MSG (sliceId <= 0xF, "Slice ID cannot exceed 4 bits.");
  NS_ASSERT_MSG (linkdir <= 0xF, "Link direction cannot exceed 4 bits.");

  uint32_t meterId = 0xC;
  meterId <<= 4;
  meterId |= static_cast<uint32_t> (sliceId);
  meterId <<= 24;
  meterId |= linkdir;
  return meterId;
}

} // namespace ns3
