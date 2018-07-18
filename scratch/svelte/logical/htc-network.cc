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

#include "htc-network.h"
// #include "ring-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HtcNetwork");
NS_OBJECT_ENSURE_REGISTERED (HtcNetwork);

HtcNetwork::HtcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

HtcNetwork::~HtcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
HtcNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HtcNetwork")
    .SetParent<SliceNetwork> ()
    .AddConstructor<HtcNetwork> ()
  ;
  return tid;
}

void
HtcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  SliceNetwork::DoDispose ();
}

void
HtcNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Chain up (the slice creation will be triggered by base class).
  SliceNetwork::NotifyConstructionCompleted ();
}

void
HtcNetwork::SliceCreate (void)
{
  NS_LOG_FUNCTION (this);

}

} // namespace ns3
