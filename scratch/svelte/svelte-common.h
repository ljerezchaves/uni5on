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

// SVELTE TEID masks for OpenFlow matching.
#define TEID_SLICE_MASK_STR   "0x0F000000"
#define TEID_IMSI_MASK_STR    "0x00FFFFF0"
#define TEID_BID_MASK_STR     "0x0000000F"

class Ipv4Address;
class NetDevice;

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
 * Get the LTE interface name.
 * \param iface The LTE interface.
 * \return The string with the LTE interface name.
 */
std::string LteIfaceStr (LteIface iface);

/**
 * \ingroup svelte
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OpModeStr (OpMode mode);

/**
 * \ingroup svelte
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

/**
 * \ingroup svelte
 * Compute the TEID value globally used in the SVELTE architecture for a EPS
 * bearer considering the slice ID, the UE ISMI and bearer ID.
 * \param sliceId The SVELTE logical slice ID.
 * \param ueImsi The UE ISMI.
 * \param bearerId The UE bearer ID.
 * \return The TEID for this bearer.
 *
 * \internal
 * \verbatim
 * We are using the following TEID allocation strategy:
 * TEID has 32 bits length: 0x 0 0 00000 0
 *                            |-|-|-----|-|
 *                             A B C    D
 *
 *  4 (A) bits are reserved for further usage.
 *  4 (B) bits are used to identify the logical slice.
 * 20 (C) bits are used to identify the UE (IMSI).
 *  4 (D) bits are used to identify the bearer withing the UE.
 * \endverbatim
 */
uint32_t GetSvelteTeid (SliceId sliceId, uint32_t ueImsi, uint8_t bearerId);

/**
 * \ingroup svelte
 * Encapsulate the destination address in the 32 MSB of tunnel ID and the
 * TEID in the 32 LSB of tunnel ID.
 * \param dstIp The destination IP address.
 * \param teid The tunnel TEID.
 * \return The string for this tunnel ID.
 */
std::string GetTunnelIdStr (uint32_t teid, Ipv4Address dstIp);

/**
 * \ingroup svelte
 * Set the devices names identifying the connection between the nodes.
 * \param src The network device in the source node.
 * \param dst The network device in the destination node.
 * \param desc The string describing this connection.
 */
void SetDeviceNames (Ptr<NetDevice> src, Ptr<NetDevice> dst, std::string desc);

} // namespace ns3
#endif // SVELTE_COMMON_H
