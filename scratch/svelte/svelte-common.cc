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
      return "s1u";
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
    case SliceId::HTC:
      return "htc";
    case SliceId::MTC:
      return "mtc";
    default:
      NS_LOG_ERROR ("Invalid logical slice.");
      return "";
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
