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
#include "svelte-enum.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteEnum");

// Implementing the lte-interface.h
std::string LteIfaceStr (LteIface iface)
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

// Implementing the operation-mode.h
std::string OpModeStr (OpMode mode)
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

// Implementing the slice-id.h
std::string SliceIdStr (SliceId slice)
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

} // namespace ns3