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

#include "epc-sdn-controller.h"

NS_LOG_COMPONENT_DEFINE ("EpcSdnController");

namespace ns3 {

EpcSdnController::EpcSdnController ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

EpcSdnController::~EpcSdnController ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
EpcSdnController::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

TypeId 
EpcSdnController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcSdnController")
    .SetParent (LearningController::GetTypeId ())
    .AddConstructor<EpcSdnController> ()
  ;
  return tid;
}

uint8_t
EpcSdnController::AddBearer (uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  return 0;
}

};  // namespace ns3

