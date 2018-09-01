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
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

// SVELTE TEID masks for OpenFlow matching.
#define TEID_SLICE_MASK     0x0F000000
#define TEID_IMSI_MASK      0x00FFFFF0
#define TEID_BID_MASK       0x0000000F

// OpenFlow cookie strick mask.
#define COOKIE_STRICT_MASK_STR  "0xFFFFFFFFFFFFFFFF"

// GTP-U UDP port number.
#define GTPU_PORT   2152

class Ipv4Address;
class NetDevice;

/** EPS bearer context created. */
typedef EpcS11SapMme::BearerContextCreated BearerContext_t;

/** List of bearer context created. */
typedef std::list<BearerContext_t> BearerContextList_t;

/**
 * \ingroup svelte
 * Enumeration of LTE logical interfaces.
 */
typedef enum
{
  // Don't change enum order. S1U and S5 are used as array indexes in RingInfo.
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
 * \internal Slice IDs are restricted to the range [0, 14] by the current
 * TEID allocation strategy. The NONE value must be fixed to 0xF (15).
 */
typedef enum
{
  MTC  = 0,  //!< Slice for MTC UEs.
  HTC  = 1,  //!< Slice for HTC UEs.
  TMP  = 2,  //!< Slice for TMP UEs.
  ALL  = 3,  //!< ALL previous slices.
  NONE = 15  //!< Undefined slice.
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
 * Get the mapped IP ToS value for a specific DSCP.
 * \param dscp The IP DSCP value.
 * \return The IP ToS mapped for this DSCP.
 *
 * \internal
 * We are mapping the DSCP value (RFC 2474) to the IP Type of Service (ToS)
 * (RFC 1349) field because the pfifo_fast queue discipline from the traffic
 * control module still uses the old IP ToS definition. Thus, we are
 * 'translating' the DSCP values so we can keep the priority queuing
 * consistency both on traffic control module and OpenFlow port queues.
 * \verbatim
 * DSCP_EF   --> ToS 0x10 --> priority 6 --> queue 0 (high priority).
 * DSCP_AF41 --> ToS 0x18 --> priority 4 --> queue 1 (normal priority).
 * DSCP_AF31 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
 * DSCP_AF32 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
 * DSCP_AF11 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
 * DSCP_AF11 --> ToS 0x00 --> priority 0 --> queue 1 (normal priority).
 * DSCP_BE   --> ToS 0x08 --> priority 2 --> queue 2 (low priority).
 * \endverbatim
 * \see See the ns3::Socket::IpTos2Priority for details.
 */
uint8_t Dscp2Tos (Ipv4Header::DscpType dscp);

/**
 * \ingroup svelte
 * Get the mapped DSCP value for a specific EPS QCI.
 * \param qci The EPS bearer QCI.
 * \return The IP DSCP mapped for this QCI.
 *
 * \internal
 * The following EPS QCI --> IP DSCP mapping is specified in "GSM Association
 * IR.34 (2013) Guidelines for IPX Provider networks, Version 9.1, Section 6.2,
 * May 2013."
 * \verbatim
 *     GBR traffic: QCI 1, 2, 3 --> DSCP_EF
 *                  QCI 4       --> DSCP_AF41
 * Non-GBR traffic: QCI 5       --> DSCP_AF31
 *                  QCI 6       --> DSCP_AF32
 *                  QCI 7       --> DSCP_AF21
 *                  QCI 8       --> DSCP_AF11
 *                  QCI 9       --> DSCP_BE
 * \endverbatim
 */
Ipv4Header::DscpType Qci2Dscp (EpsBearer::Qci qci);

/**
 * \ingroup svelte
 * Compute the TEID value globally used in the SVELTE architecture for a EPS
 * bearer considering the slice ID, the UE ISMI and bearer ID.
 * \param sliceId The SVELTE logical slice ID.
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
 *  4 (A) bits are reserved for slicing meters.
 *        - For TEID these bits are fixed to 0x0.
 *  4 (B) bits are used to identify the logical slice (slice ID).
 * 20 (C) bits are used to identify the UE (IMSI).
 *  4 (D) bits are used to identify the bearer withing the UE (BID).
 * \endverbatim
 */
uint32_t GetSvelteTeid (SliceId sliceId, uint32_t ueImsi, uint32_t bearerId);

/**
 * \ingroup svelte
 * Compute the meter ID value globally used in the SVELTE architecture for
 * infrastructure slicing meters.
 * \param type The slicing meter type.
 * \param sliceId The SVELTE logical slice ID.
 * \param meterId The meter identification considering the network topology.
 * \return The meter ID value.
 *
 * \internal
 * When the network slicing operation mode is active, the traffic of each
 * slice will be independently monitored by slicing meters using the
 * following meter ID allocation strategy.
 * \verbatim
 * Meter ID has 32 bits length: 0x 0 0 000000
 *                                |-|-|------|
 *                                 A B C
 *
 *  4 (A) bits are used to identify the meter type.
 *        - For infrastructure slicing meters these bits are fixed to 0x1.
 *  4 (B) bits are used to identify the logical slice (slice ID).
 * 24 (C) bits are used to identify the meter considering the network topology.
 * \endverbatim
 */
uint32_t GetSvelteMeterId (SliceId sliceId, uint32_t meterId);

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
 * Convert the uint32_t parameter value to a hexadecimal string representation.
 * \param value The uint32_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint32Hex (uint32_t value);

/**
 * \ingroup svelte
 * Convert the uint64_t parameter value to a hexadecimal string representation.
 * \param value The uint64_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint64Hex (uint64_t value);

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
