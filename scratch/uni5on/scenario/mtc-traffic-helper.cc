/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include "mtc-traffic-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MtcTrafficHelper");
NS_OBJECT_ENSURE_REGISTERED (MtcTrafficHelper);

// ------------------------------------------------------------------------ //
MtcTrafficHelper::MtcTrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

MtcTrafficHelper::~MtcTrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MtcTrafficHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MtcTrafficHelper")
    .SetParent<TrafficHelper> ()
    .AddConstructor<MtcTrafficHelper> ()
  ;
  return tid;
}

void
MtcTrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  TrafficHelper::DoDispose ();
}

void
MtcTrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  TrafficHelper::NotifyConstructionCompleted ();
}

} // namespace ns3
