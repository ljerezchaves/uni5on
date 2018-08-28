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

#include <string>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "svelte-common.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteCommon");

std::string
LteIfaceStr (LteIface iface)
{
  switch (iface)
    {
    case LteIface::S1U:
      return "s1-u";
    case LteIface::S5:
      return "s5";
    case LteIface::X2:
      return "x2";
    case LteIface::SGI:
      return "sgi";
    default:
      NS_LOG_ERROR ("Invalid LTE interface.");
      return "";
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
      return "";
    }
}

std::string
SliceIdStr (SliceId slice)
{
  switch (slice)
    {
    case SliceId::NONE:
      return "none";
    case SliceId::MTC:
      return "mtc";
    case SliceId::HTC:
      return "htc";
    case SliceId::TMP:
      return "tmp";
    case SliceId::ALL:
      return "all";
    default:
      NS_LOG_ERROR ("Invalid slice ID.");
      return "";
    }
}

uint8_t
Dscp2Tos (Ipv4Header::DscpType dscp)
{
  switch (dscp)
    {
    // Mapping default traffic to low priority queues.
    case Ipv4Header::DscpDefault:
      return 0x08;

    // Mapping HTC VoIP and MTC auto pilot traffic to high priority queues.
    case Ipv4Header::DSCP_EF:
      return 0x10;

    // Mapping MTC Non-GBR traffic to normal priority queues.
    case Ipv4Header::DSCP_AF31:
      return 0x18;

    // Mapping other HTC traffics to normal priority queues.
    case Ipv4Header::DSCP_AF41:
    case Ipv4Header::DSCP_AF11:
      return 0x00;

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
    // QCI 1: used by the HTC VoIP application.
    case EpsBearer::GBR_CONV_VOICE:
      return Ipv4Header::DSCP_EF;

    // QCI 2: not in use.
    case EpsBearer::GBR_CONV_VIDEO:
      return Ipv4Header::DSCP_EF;

    // QCI 3: used by the MTC auto pilot application.
    case EpsBearer::GBR_GAMING:
      return Ipv4Header::DSCP_EF;

    // QCI 4: used by the HTC live video application.
    case EpsBearer::GBR_NON_CONV_VIDEO:
      return Ipv4Header::DSCP_AF41;

    // QCI 5: used by the MTC auto pilot application.
    case EpsBearer::NGBR_IMS:
      return Ipv4Header::DSCP_AF31;

    // QCI 6: used by the HTC buffered video application.
    case EpsBearer::NGBR_VIDEO_TCP_OPERATOR:
      return Ipv4Header::DSCP_AF11;

    // QCI 7: used by the HTC live video application.
    case EpsBearer::NGBR_VOICE_VIDEO_GAMING:
      return Ipv4Header::DSCP_AF11;

    // QCI 8: used by the HTC HTTP application.
    case EpsBearer::NGBR_VIDEO_TCP_PREMIUM:
      return Ipv4Header::DSCP_AF11;

    // QCI 9: used by default bearers and by aggregated traffic.
    case EpsBearer::NGBR_VIDEO_TCP_DEFAULT:
      return Ipv4Header::DscpDefault;

    default:
      NS_ABORT_MSG ("No DSCP mapped value for QCI " << qci);
      return Ipv4Header::DscpDefault;
    }
}

uint32_t
GetSvelteTeid (SliceId sliceId, uint32_t ueImsi, uint8_t bearerId)
{
  NS_ABORT_MSG_IF (static_cast<uint32_t> (sliceId) > 0xF,
                   "Slice ID cannot exceed 4 bits in SVELTE.");
  NS_ABORT_MSG_IF (ueImsi > 0xFFFFF,
                   "UE IMSI cannot exceed 20 bits in SVELTE.");

  uint32_t teid = static_cast<uint32_t> (sliceId);
  teid <<= 20;
  teid |= static_cast<uint32_t> (ueImsi);
  teid <<= 4;
  teid |= static_cast<uint32_t> (bearerId);
  return teid;
}

std::string
GetTunnelIdStr (uint32_t teid, Ipv4Address dstIp)
{
  uint64_t tunnelId = static_cast<uint64_t> (dstIp.Get ());
  tunnelId <<= 32;
  tunnelId |= static_cast<uint64_t> (teid);

  char tunnelIdStr [19];
  sprintf (tunnelIdStr, "0x%016lx", tunnelId);
  return std::string (tunnelIdStr);
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
