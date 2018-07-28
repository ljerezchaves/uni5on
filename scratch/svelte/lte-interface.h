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

#ifndef LTE_INTERFACE_H
#define LTE_INTERFACE_H

#include <string>

namespace ns3 {

/**
 * \ingroup svelteInfra
 * Enumeration of LTE logical interfaces.
 */
typedef enum
{
  // Don't change enum order.
  // These values are used as array indices in RingRoutingInfo.

  S1U  = 0,   //!< S1-U interface connecting eNB to S-GW.
  S5   = 1,   //!< S5 interface connecting S-GW to P-GW.
  X2   = 2,   //!< X2 interface connecting eNB to eNB.
  SGI  = 3    //!< SGi interface connecting P-GW to Internet.
} LteIface;

/**
 * \ingroup svelteInfra
 * Get the LTE interface name.
 * \param iface The LTE interface.
 * \return The string with the LTE interface name.
 */
std::string LteIfaceStr (LteIface iface);

} // namespace ns3
#endif // LTE_INTERFACE_H
