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
#define TEID_SLICE_MASK   0x0F000000
#define TEID_IMSI_MASK    0x00FFFFF0
#define TEID_BID_MASK     0x0000000F

// UDP port numbers.
#define GTPU_PORT         2152
#define X2C_PORT          4444

// Protocol numbers.
#define IPV4_PROT_NUM     (static_cast<uint16_t> (Ipv4L3Protocol::PROT_NUMBER))
#define UDP_PROT_NUM      (static_cast<uint16_t> (UdpL4Protocol::PROT_NUMBER))
#define TCP_PROT_NUM      (static_cast<uint16_t> (TcpL4Protocol::PROT_NUMBER))

// OpenFlow cookie strick mask.
#define COOKIE_STRICT_MASK_STR  "0xFFFFFFFFFFFFFFFF"

// Flow-mod flags.
#define FLAGS_REMOVED_OVERLAP_RESET \
  ((OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))
#define FLAGS_OVERLAP_RESET \
  ((OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))

class Ipv4Address;
class NetDevice;

/** EPS bearer context created. */
typedef EpcS11SapMme::BearerContextCreated BearerCreated_t;

/** List of EPS bearer contexts created. */
typedef std::list<BearerCreated_t> BearerCreatedList_t;

/** EPS bearer context modified. */
typedef EpcS11SapMme::BearerContextModified BearerModified_t;

/** List of EPS bearer contexts modified. */
typedef std::list<BearerModified_t> BearerModifiedList_t;

/**
 * \ingroup svelte
 * Enumeration of available traffic directions.
 */
typedef enum
{
  DLINK = 0,  //!< Downlink traffic.
  ULINK = 1   //!< Uplink traffic.
} Direction;

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
 * Enumeration of available LTE QoS traffic types.
 */
typedef enum
{
  NON  = 0,  //!< Non-GBR traffic.
  GBR  = 1,  //!< GBR traffic.
  BOTH = 2   //!< Both GBR and Non-GBR traffic.
} QosType;

/**
 * \ingroup svelte
 * Enumeration of available SVELTE logical slices IDs.
 * \internal Slice IDs are restricted to the range [0, 14] by the current
 * TEID allocation strategy. The NONE value must be fixed to 0xF (15).
 */
typedef enum
{
  HTC  = 0,  //!< Slice for HTC UEs.
  MTC  = 1,  //!< Slice for MTC UEs.
  TMP  = 2,  //!< Slice for TMP UEs.
  ALL  = 3,  //!< ALL previous slices.
  UNKN = 15  //!< Unknown slice.
} SliceId;

/**
 * \ingroup svelte
 * Enumeration of available inter-slicing operation modes.
 */
typedef enum
{
  NONE = 0,   //!< No inter-slicing.
  SHAR = 1,   //!< Partial Non-GBR shared inter-slicing.
  STAT = 2,   //!< Full static inter-slicing.
  DYNA = 3    //!< Full dinaymic inter-slicing.
} SliceMode;

// Total number of slices + 1 for aggregated metadata.
#define N_SLICES_ALL (static_cast<uint8_t> (SliceId::ALL) + 1)

// Total number of types + 1 for aggregated metadata.
#define N_TYPES_ALL (static_cast<uint8_t> (QosType::BOTH) + 1)

/**
 * \ingroup svelte
 * Get the direction name.
 * \param dir The direction.
 * \return The string with the direction string.
 */
std::string DirectionStr (Direction dir);

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
 * Get the LTE QoS traffic type name.
 * \param type The LTE QoS traffic type.
 * \return The string with the LTE QoS traffic type name.
 */
std::string QosTypeStr (QosType type);

/**
 * \ingroup svelte
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

/**
 * \ingroup svelte
 * Get the inter-slicing operation mode name.
 * \param mode The inter-slicing operation mode.
 * \return The string with the inter-slicing operation mode name.
 */
std::string SliceModeStr (SliceMode mode);

/**
 * \ingroup svelte
 * Convert the BPS to KBPS without precision loss.
 * \param bitrate The bit rate in BPS.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (uint64_t bitrate);

/**
 * \ingroup svelte
 * Convert DataRate BPS to KBPS without precision loss.
 * \param bitrate The DataRate.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (DataRate datarate);

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
 * DSCP_EF   --> ToS 0x10 --> prio 6 --> band 0 --> queue 0 (high).
 * DSCP_AF41 --> ToS 0x18 --> prio 4 --> band 1 --> queue 1 (normal).
 * DSCP_AF31 --> ToS 0x00 --> prio 0 --> band 1 --> queue 1 (normal).
 * DSCP_AF32 --> ToS 0x00 --> prio 0 --> band 1 --> queue 1 (normal).
 * DSCP_AF11 --> ToS 0x00 --> prio 0 --> band 1 --> queue 1 (normal).
 * DSCP_AF11 --> ToS 0x00 --> prio 0 --> band 1 --> queue 1 (normal).
 * DSCP_BE   --> ToS 0x08 --> prio 2 --> band 2 --> queue 2 (low).
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
 * May 2013." This same mapping can also be found in "Cox, Christopher.
 * An Introduction to LTE: LTE, LTE-Advanced, SAE, VoLTE and 4G Mobile
 * Communications (2nd editio), Section 13.4.3, 2014."
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
 * Get the DSCP type name.
 * \param dscp The DSCP type value.
 * \return The string with the DSCP type name.
 */
std::string DscpTypeStr (Ipv4Header::DscpType dscp);

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
 * Decompose the TEID to get the UE bearer ID.
 * \param teid The GTP tunnel ID.
 * \return The UE bearer ID for this tunnel.
 */
uint8_t ExtractBearerId (uint32_t teid);

/**
 * Decompose the TEID to get the slice ID.
 * \param teid The GTP tunnel ID.
 * \return The slice ID for this tunnel.
 */
SliceId ExtractSliceId (uint32_t teid);

/**
 * Decompose the TEID to get the UE IMSI.
 * \param teid The GTP tunnel ID.
 * \return The UE IMSI for this tunnel.
 */
uint64_t ExtractUeImsi (uint32_t teid);

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
