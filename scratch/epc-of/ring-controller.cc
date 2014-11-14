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

#include "ring-controller.h"

NS_LOG_COMPONENT_DEFINE ("RingController");

namespace ns3 {

RingController::RingController ()
{
  NS_LOG_FUNCTION (this);
  //SetConnectionCallback (MakeCallback (&RingController::ConnectionStarted, this));
}

RingController::~RingController ()
{
  NS_LOG_FUNCTION (this);
}

void
RingController::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
RingController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RingController")
    .SetParent (EpcSdnController::GetTypeId ())
  ;
  return tid;
}

};  // namespace ns3

