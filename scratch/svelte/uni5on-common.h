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

#ifndef UNI5ON_COMMON_H
#define UNI5ON_COMMON_H

#include <string>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

// SVELTE TEID masks for OpenFlow matching.
#define TEID_STRICT_MASK    0xFFFFFFFF
#define TEID_SLICE_MASK     0x0F000000
#define TEID_IMSI_MASK      0x00FFFFF0
#define TEID_BID_MASK       0x0000000F

// SVELTE cookie masks for OpenFlow matching.
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

// SVELTE meter ID masks.
#define METER_SLC_TYPE      0xC0000000
#define METER_MBR_TYPE      0x80000000
#define METER_IFACE_MASK    0x30000000
#define METER_SLICE_MASK    0x0F000000

// UDP port numbers.
#define GTPU_PORT         2152
#define X2C_PORT          4444

// Protocol numbers.
#define IPV4_PROT_NUM     (static_cast<uint16_t> (Ipv4L3Protocol::PROT_NUMBER))
#define UDP_PROT_NUM      (static_cast<uint16_t> (UdpL4Protocol::PROT_NUMBER))
#define TCP_PROT_NUM      (static_cast<uint16_t> (TcpL4Protocol::PROT_NUMBER))

// OpenFlow flow-mod flags.
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

/** Map saving IP DSCP value / OpenFlow queue id. */
typedef std::map<Ipv4Header::DscpType, uint32_t> DscpQueueMap_t;

/**
 * \ingroup svelte
 * Enumeration of available traffic directions.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  DLINK = 0,  //!< Downlink traffic.
  ULINK = 1   //!< Uplink traffic.
} Direction;

// Total number of valid Direction items.
#define N_DIRECTIONS (static_cast<int> (Direction::ULINK) + 1)

/**
 * \ingroup svelte
 * Enumeration of LTE logical interfaces.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  S1  = 0,   //!< S1-U interface connecting eNB to S-GW.
  S5  = 1,   //!< S5 interface connecting S-GW to P-GW.
  X2  = 2,   //!< X2 interface connecting eNB to eNB.
  SGI = 3    //!< SGi interface connecting P-GW to Internet.
} LteIface;

// Total number of valid LteIface items.
#define N_IFACES (static_cast<int> (LteIface::SGI) + 1)
#define N_IFACES_EPC (static_cast<int> (LteIface::S5) + 1)

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

// Total number of valid OpMode items.
#define N_OP_MODES (static_cast<int> (OpMode::AUTO) + 1)

/**
 * \ingroup svelte
 * Enumeration of available LTE QoS traffic types.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  NON  = 0,  //!< Non-GBR traffic.
  GBR  = 1,  //!< GBR traffic.
  BOTH = 2   //!< Both GBR and Non-GBR traffic.
} QosType;

// Total number of valid QosType items.
#define N_QOS_TYPES (static_cast<int> (QosType::BOTH))
#define N_QOS_TYPES_BOTH (static_cast<int> (QosType::BOTH) + 1)

/**
 * \ingroup svelte
 * Enumeration of available SVELTE logical slices IDs.
 * \internal Slice IDs are restricted to the range [0, 14] by the current
 * TEID allocation strategy.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  HTC  = 0,  //!< Slice for HTC UEs.
  MTC  = 1,  //!< Slice for MTC UEs.
  TMP  = 2,  //!< Slice for TMP UEs.
  ALL  = 3,  //!< ALL previous slices.
  UNKN = 4   //!< Unknown slice.
} SliceId;

// Total number of valid SliceId items.
#define N_SLICE_IDS (static_cast<int> (SliceId::ALL))
#define N_SLICE_IDS_ALL (static_cast<int> (SliceId::ALL) + 1)
#define N_SLICE_IDS_UNKN (static_cast<int> (SliceId::UNKN) + 1)

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

// Total number of valid SliceMode items.
#define N_SLICE_MODES (static_cast<int> (SliceMode::DYNA) + 1)

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
double Bps2Kbps (int64_t bitrate);

/**
 * \ingroup svelte
 * Convert DataRate BPS to KBPS without precision loss.
 * \param bitrate The DataRate.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (DataRate datarate);

/**
 * \ingroup svelte
 * Get the mapped OpenFlow output queue ID for all DSCP used values.
 * \return The OpenFlow queue ID mapped values.
 *
 * \internal
 * Mapping the IP DSCP to the OpenFlow output queue ID.
 * \verbatim
 * DSCP_EF   --> OpenFlow queue 0 (priority)
 * DSCP_AF41 --> OpenFlow queue 1 (WRR)
 * DSCP_AF31 --> OpenFlow queue 1 (WRR)
 * DSCP_AF32 --> OpenFlow queue 1 (WRR)
 * DSCP_AF21 --> OpenFlow queue 1 (WRR)
 * DSCP_AF11 --> OpenFlow queue 1 (WRR)
 * DSCP_BE   --> OpenFlow queue 2 (WRR)
 * \endverbatim
 */
const DscpQueueMap_t& Dscp2QueueMap (void);

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
 * 'translating' the DSCP values so we can keep the queuing consistency
 * both on traffic control module and OpenFlow port queues.
 * \verbatim
 * DSCP_EF   --> ToS 0x10 --> prio 6 --> pfifo band 0
 * DSCP_AF41 --> ToS 0x18 --> prio 4 --> pfifo band 1
 * DSCP_AF31 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF32 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF21 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF11 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_BE   --> ToS 0x08 --> prio 2 --> pfifo band 2
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
 * Compute the cookie value globally used in the SVELTE architecture for
 * OpenFlow rules considering the bearer TEID, the rule priority, and the LTE
 * logical interface.
 * \param teid The TEID value.
 * \param prio The OpenFlow rule priority.
 * \param iface The LTE logical interface.
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
 *  4 (B) bits are used to identify the LTE logical interface.
 * 16 (C) bits are used to identify the rule priority.
 * 32 (D) bits are used to identify the bearer TEID.
 * \endverbatim
 */
uint64_t CookieCreate (LteIface iface, uint16_t prio, uint32_t teid);

/**
 * Decompose the cookie to get the bearer TEID.
 * \param cookie The OpenFlow cookie.
 * \return The bearer TEID.
 */
uint32_t CookieGetTeid (uint64_t cookie);

/**
 * Decompose the cookie to get the rule priority.
 * \param cookie The OpenFlow cookie.
 * \return The OpenFlow rule priority.
 */
uint16_t CookieGetPriority (uint64_t cookie);

/**
 * Decompose the cookie to get the LTE logical interface.
 * \param cookie The OpenFlow cookie.
 * \return The LTE logical interface.
 */
LteIface CookieGetIface (uint64_t cookie);

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
 *  4 (A) bits are used to identify a valid TEID, here fixed at 0x0.
 *  4 (B) bits are used to identify the logical slice (slice ID).
 * 20 (C) bits are used to identify the UE (IMSI).
 *  4 (D) bits are used to identify the bearer withing the UE (bearer ID).
 * \endverbatim
 */
uint32_t TeidCreate (SliceId sliceId, uint32_t ueImsi, uint32_t bearerId);

/**
 * Decompose the TEID to get the UE bearer ID.
 * \param teid The GTP tunnel ID.
 * \return The UE bearer ID for this tunnel.
 */
uint8_t TeidGetBearerId (uint32_t teid);

/**
 * Decompose the TEID to get the slice ID.
 * \param teid The GTP tunnel ID.
 * \return The slice ID for this tunnel.
 */
SliceId TeidGetSliceId (uint32_t teid);

/**
 * Decompose the TEID to get the UE IMSI.
 * \param teid The GTP tunnel ID.
 * \return The UE IMSI for this tunnel.
 */
uint64_t TeidGetUeImsi (uint32_t teid);

/**
 * \ingroup svelte
 * Compute the meter ID value globally used in the SVELTE architecture for
 * infrastructure MBR meters.
 * \param iface The LTE logical interface.
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
 *        here at 10 and the next 2 bits are used to identify the LTE logical
 *        interface.
 * 28 (B) bits are used to identify the GTP tunnel ID (TEID).
 * \endverbatim
 */
uint32_t MeterIdMbrCreate (LteIface iface, uint32_t teid);

/**
 * \ingroup svelte
 * Compute the meter ID value globally used in the SVELTE architecture for
 * infrastructure slicing meters.
 * \param sliceId The SVELTE logical slice ID.
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
uint32_t MeterIdSlcCreate (SliceId sliceId, uint32_t linkdir);

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
#endif // UNI5ON_COMMON_H
