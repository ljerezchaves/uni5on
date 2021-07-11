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

#ifndef UNI5ON_COMMON_H
#define UNI5ON_COMMON_H

#include <string>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>

namespace ns3 {

/**
 * \defgroup uni5on UNI5ON architecture
 * Network simulation scenario for the UNI5ON architecture.
 *
 * \ingroup uni5on
 * \defgroup uni5onApps Applications
 * Applications prepared to work with the UNI5ON architecture.
 *
 * \ingroup uni5on
 * \defgroup uni5onInfra Infrastructure
 * The UNI5ON architecture infrastructure.
 *
 * \ingroup uni5on
 * \defgroup uni5onMano Management and Orchestration
 * The UNI5ON architecture management and orchestration applications.
 *
 * \ingroup uni5on
 * \defgroup uni5onLogical Logical
 * The logical eEPC network slices.
 *
 * \ingroup uni5on
 * \defgroup uni5onMeta Metadata
 * The metadata for the UNI5ON architecture.
 *
 * \ingroup uni5on
 * \defgroup uni5onTraffic Traffic
 * Traffic configuration helpers and manager.
 *
 * \ingroup uni5on
 * \defgroup uni5onStats Statistics
 * Statistics calculators for monitoring the UNI5ON architecture.
 */

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
 * \ingroup uni5on
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
 * \ingroup uni5on
 * Enumeration of logical interfaces.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  S1  = 0,   //!< S1-U interface connecting eNB to S-GW.
  S5  = 1,   //!< S5 interface connecting S-GW to P-GW.
  X2  = 2,   //!< X2 interface connecting eNB to eNB.
  SGI = 3    //!< SGi interface connecting P-GW to Internet.
} EpsIface;

// Total number of valid EpsIface items.
#define N_IFACES (static_cast<int> (EpsIface::SGI) + 1)
#define N_IFACES_EPC (static_cast<int> (EpsIface::S5) + 1)

/**
 * \ingroup uni5on
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
 * \ingroup uni5on
 * Enumeration of available QoS traffic types.
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
 * \ingroup uni5on
 * Enumeration of available logical slices IDs.
 * \internal Slice IDs are restricted to the range [0, 14] by the current
 * TEID allocation strategy.
 */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  // The last two enum items must be ALL and UNKN, in this order.
  MBB  = 0,  //!< Slice for MBB UEs.
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
 * \ingroup uni5on
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
 * \ingroup uni5on
 * Get the direction name.
 * \param dir The direction.
 * \return The string with the direction string.
 */
std::string DirectionStr (Direction dir);

/**
 * \ingroup uni5on
 * Get the logical interface name.
 * \param iface The logical interface.
 * \return The string with the logical interface name.
 */
std::string EpsIfaceStr (EpsIface iface);

/**
 * \ingroup uni5on
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OpModeStr (OpMode mode);

/**
 * \ingroup uni5on
 * Get the QoS traffic type name.
 * \param type The QoS traffic type.
 * \return The string with the QoS traffic type name.
 */
std::string QosTypeStr (QosType type);

/**
 * \ingroup uni5on
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

/**
 * \ingroup uni5on
 * Get the inter-slicing operation mode name.
 * \param mode The inter-slicing operation mode.
 * \return The string with the inter-slicing operation mode name.
 */
std::string SliceModeStr (SliceMode mode);

/**
 * \ingroup uni5on
 * Convert the BPS to KBPS without precision loss.
 * \param bitrate The bit rate in BPS.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (int64_t bitrate);

/**
 * \ingroup uni5on
 * Convert DataRate BPS to KBPS without precision loss.
 * \param bitrate The DataRate.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (DataRate datarate);

/**
 * \ingroup uni5on
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
 * \ingroup uni5on
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
 * \ingroup uni5on
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
 * \ingroup uni5on
 * Get the DSCP type name.
 * \param dscp The DSCP type value.
 * \return The string with the DSCP type name.
 */
std::string DscpTypeStr (Ipv4Header::DscpType dscp);

/**
 * \ingroup uni5on
 * Encapsulate the destination address in the 32 MSB of tunnel ID and the
 * TEID in the 32 LSB of tunnel ID.
 * \param dstIp The destination IP address.
 * \param teid The tunnel TEID.
 * \return The string for this tunnel ID.
 */
std::string GetTunnelIdStr (uint32_t teid, Ipv4Address dstIp);

/**
 * \ingroup uni5on
 * Convert the uint32_t parameter value to a hexadecimal string representation.
 * \param value The uint32_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint32Hex (uint32_t value);

/**
 * \ingroup uni5on
 * Convert the uint64_t parameter value to a hexadecimal string representation.
 * \param value The uint64_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint64Hex (uint64_t value);

/**
 * \ingroup uni5on
 * Set the devices names identifying the connection between the nodes.
 * \param src The network device in the source node.
 * \param dst The network device in the destination node.
 * \param desc The string describing this connection.
 */
void SetDeviceNames (Ptr<NetDevice> src, Ptr<NetDevice> dst, std::string desc);

} // namespace ns3
#endif // UNI5ON_COMMON_H
