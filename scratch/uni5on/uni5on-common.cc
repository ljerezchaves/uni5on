/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#include <iomanip>
#include <iostream>
#include <string>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "uni5on-common.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Uni5onCommon");

std::string
DirectionStr (Direction dir)
{
  switch (dir)
    {
    case Direction::DLINK:
      return "Dlink";
    case Direction::ULINK:
      return "Ulink";
    default:
      NS_LOG_ERROR ("Invalid traffic direction.");
      return std::string ();
    }
}

std::string
LteIfaceStr (LteIface iface)
{
  switch (iface)
    {
    case LteIface::S1:
      return "s1u";
    case LteIface::S5:
      return "s5";
    case LteIface::X2:
      return "x2";
    case LteIface::SGI:
      return "sgi";
    default:
      NS_LOG_ERROR ("Invalid LTE logical interface.");
      return std::string ();
    }
}

std::string
OpModeStr (OpMode mode)
{
  switch (mode)
    {
    case OpMode::OFF:
      return "off";
    case OpMode::ON:
      return "on";
    case OpMode::AUTO:
      return "auto";
    default:
      NS_LOG_ERROR ("Invalid operation mode.");
      return std::string ();
    }
}

std::string
QosTypeStr (QosType type)
{
  switch (type)
    {
    case QosType::NON:
      return "NonGBR";
    case QosType::GBR:
      return "GBR";
    case QosType::BOTH:
      return "Both";
    default:
      NS_LOG_ERROR ("Invalid LTE QoS traffic type.");
      return std::string ();
    }
}

std::string
SliceIdStr (SliceId slice)
{
  switch (slice)
    {
    case SliceId::HTC:
      return "htc";
    case SliceId::MTC:
      return "mtc";
    case SliceId::TMP:
      return "tmp";
    case SliceId::ALL:
      return "all";
    case SliceId::UNKN:
      return "unknown";
    default:
      NS_LOG_ERROR ("Invalid logical slice ID.");
      return std::string ();
    }
}

std::string
SliceModeStr (SliceMode mode)
{
  switch (mode)
    {
    case SliceMode::NONE:
      return "none";
    case SliceMode::SHAR:
      return "shared";
    case SliceMode::STAT:
      return "static";
    case SliceMode::DYNA:
      return "dynamic";
    default:
      NS_LOG_ERROR ("Invalid inter-slice operation mode.");
      return std::string ();
    }
}

double
Bps2Kbps (int64_t bitrate)
{
  return static_cast<double> (bitrate) / 1000;
}

double
Bps2Kbps (DataRate datarate)
{
  return Bps2Kbps (datarate.GetBitRate ());
}

const DscpQueueMap_t&
Dscp2QueueMap ()
{
  static DscpQueueMap_t queueByDscp;

  // Populating the map at the first time.
  if (queueByDscp.empty ())
    {
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_EF,     0));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF41,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF31,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF32,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF21,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF11,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DscpDefault, 2));
    }

  return queueByDscp;
}

uint8_t
Dscp2Tos (Ipv4Header::DscpType dscp)
{
  switch (dscp)
    {
    case Ipv4Header::DSCP_EF:
      return 0x10;
    case Ipv4Header::DSCP_AF41:
      return 0x18;
    case Ipv4Header::DSCP_AF32:
    case Ipv4Header::DSCP_AF31:
    case Ipv4Header::DSCP_AF21:
    case Ipv4Header::DSCP_AF11:
      return 0x00;
    case Ipv4Header::DscpDefault:
      return 0x08;
    default:
      NS_ABORT_MSG ("No ToS mapped value for DSCP " << dscp);
      return 0x00;
    }
}

Ipv4Header::DscpType
Qci2Dscp (EpsBearer::Qci qci)
{
  switch (qci)
    {
    // QCI 1: VoIP.
    case EpsBearer::GBR_CONV_VOICE:
      return Ipv4Header::DSCP_EF;

    // QCI 2:
    case EpsBearer::GBR_CONV_VIDEO:
      return Ipv4Header::DSCP_EF;

    // QCI 3: Auto pilot.
    case EpsBearer::GBR_GAMING:
      return Ipv4Header::DSCP_EF;

    // QCI 4: Live video.
    case EpsBearer::GBR_NON_CONV_VIDEO:
      return Ipv4Header::DSCP_AF41;

    // QCI 5: Auto pilot.
    case EpsBearer::NGBR_IMS:
      return Ipv4Header::DSCP_AF31;

    // QCI 6: Buffered video.
    case EpsBearer::NGBR_VIDEO_TCP_OPERATOR:
      return Ipv4Header::DSCP_AF32;

    // QCI 7: Live video.
    case EpsBearer::NGBR_VOICE_VIDEO_GAMING:
      return Ipv4Header::DSCP_AF21;

    // QCI 8: HTTP.
    case EpsBearer::NGBR_VIDEO_TCP_PREMIUM:
      return Ipv4Header::DSCP_AF11;

    // QCI 9:
    case EpsBearer::NGBR_VIDEO_TCP_DEFAULT:
      return Ipv4Header::DscpDefault;

    default:
      NS_ABORT_MSG ("No DSCP mapped value for QCI " << qci);
      return Ipv4Header::DscpDefault;
    }
}

std::string
DscpTypeStr (Ipv4Header::DscpType dscp)
{
  switch (dscp)
    {
    case Ipv4Header::DscpDefault:
      return "BE";
    case Ipv4Header::DSCP_CS1:
      return "CS1";
    case Ipv4Header::DSCP_AF11:
      return "AF11";
    case Ipv4Header::DSCP_AF12:
      return "AF12";
    case Ipv4Header::DSCP_AF13:
      return "AF13";
    case Ipv4Header::DSCP_CS2:
      return "CS2";
    case Ipv4Header::DSCP_AF21:
      return "AF21";
    case Ipv4Header::DSCP_AF22:
      return "AF22";
    case Ipv4Header::DSCP_AF23:
      return "AF23";
    case Ipv4Header::DSCP_CS3:
      return "CS3";
    case Ipv4Header::DSCP_AF31:
      return "AF31";
    case Ipv4Header::DSCP_AF32:
      return "AF32";
    case Ipv4Header::DSCP_AF33:
      return "AF33";
    case Ipv4Header::DSCP_CS4:
      return "CS4";
    case Ipv4Header::DSCP_AF41:
      return "AF41";
    case Ipv4Header::DSCP_AF42:
      return "AF42";
    case Ipv4Header::DSCP_AF43:
      return "AF43";
    case Ipv4Header::DSCP_CS5:
      return "CS5";
    case Ipv4Header::DSCP_EF:
      return "EF";
    case Ipv4Header::DSCP_CS6:
      return "CS6";
    case Ipv4Header::DSCP_CS7:
      return "CS7";
    default:
      NS_LOG_ERROR ("Invalid DSCP type.");
      return std::string ();
    }
}

uint64_t
CookieCreate (LteIface iface, uint16_t prio, uint32_t teid)
{
  NS_ASSERT_MSG (iface <= 0xF, "LTE interface cannot exceed 4 bits.");
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
CookieGetTeid (uint64_t cookie)
{
  cookie &= COOKIE_TEID_MASK;
  return static_cast<uint32_t> (cookie);
}

uint16_t
CookieGetPriority (uint64_t cookie)
{
  cookie &= COOKIE_PRIO_MASK;
  cookie >>= 32;
  return static_cast<uint16_t> (cookie);
}

LteIface
CookieGetIface (uint64_t cookie)
{
  cookie &= COOKIE_IFACE_MASK;
  cookie >>= 48;
  return static_cast<LteIface> (cookie);
}

uint32_t
TeidCreate (SliceId sliceId, uint32_t ueImsi, uint32_t bearerId)
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

uint8_t
TeidGetBearerId (uint32_t teid)
{
  teid &= TEID_BID_MASK;
  return static_cast<uint8_t> (teid);
}

SliceId
TeidGetSliceId (uint32_t teid)
{
  teid &= TEID_SLICE_MASK;
  teid >>= 24;
  return static_cast<SliceId> (teid);
}

uint64_t
TeidGetUeImsi (uint32_t teid)
{
  teid &= TEID_IMSI_MASK;
  teid >>= 4;
  return static_cast<uint64_t> (teid);
}

uint32_t
MeterIdMbrCreate (LteIface iface, uint32_t teid)
{
  NS_ASSERT_MSG (iface <= 0x3, "LTE interface cannot exceed 2 bits.");
  NS_ASSERT_MSG (teid <= 0xFFFFFFF, "TEID cannot exceed 28 bits.");

  uint32_t meterId = 0x2;
  meterId <<= 2;
  meterId |= static_cast<uint32_t> (iface);
  meterId <<= 28;
  meterId |= teid;
  return meterId;
}

uint32_t
MeterIdSlcCreate (SliceId sliceId, uint32_t linkdir)
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

std::string
GetTunnelIdStr (uint32_t teid, Ipv4Address dstIp)
{
  uint64_t tunnelId = static_cast<uint64_t> (dstIp.Get ());
  tunnelId <<= 32;
  tunnelId |= static_cast<uint64_t> (teid);
  return GetUint64Hex (tunnelId);
}

std::string
GetUint32Hex (uint32_t value)
{
  char valueStr [11];
  sprintf (valueStr, "0x%08x", value);
  return std::string (valueStr);
}

std::string
GetUint64Hex (uint64_t value)
{
  char valueStr [19];
  sprintf (valueStr, "0x%016lx", value);
  return std::string (valueStr);
}

void
SetDeviceNames (Ptr<NetDevice> src, Ptr<NetDevice> dst, std::string desc)
{
  Names::Add (Names::FindName (src->GetNode ()) + desc +
              Names::FindName (dst->GetNode ()), src);
  Names::Add (Names::FindName (dst->GetNode ()) + desc +
              Names::FindName (src->GetNode ()), dst);
}

} // namespace ns3
