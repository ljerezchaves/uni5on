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
    .AddAttribute ("NumUes", "The total number of UEs for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&HtcNetwork::GetNumUes,
                                         &HtcNetwork::SetNumUes),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("UeAddress", "The UE network address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue ("7.64.0.0"),
                   MakeIpv4AddressAccessor (&HtcNetwork::SetUeAddress,
                                            &HtcNetwork::GetUeAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("UeMask", "The UE network mask.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4MaskValue ("255.192.0.0"),
                   MakeIpv4MaskAccessor (&HtcNetwork::SetUeMask,
                                         &HtcNetwork::GetUeMask),
                   MakeIpv4MaskChecker ())

    .AddAttribute ("SgiAddress", "The SGi interface network address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue ("8.1.0.0"),
                   MakeIpv4AddressAccessor (&HtcNetwork::SetSgiAddress,
                                            &HtcNetwork::GetSgiAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("SgiMask", "The SGi interface network mask.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4MaskValue ("255.255.0.0"),
                   MakeIpv4MaskAccessor (&HtcNetwork::SetSgiMask,
                                         &HtcNetwork::GetSgiMask),
                   MakeIpv4MaskChecker ())

    // P-GW configuration.
    .AddAttribute ("NumPgwTftSwitches",
                   "The number of P-GW TFT user-plane OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&HtcNetwork::GetPgwTftNumNodes,
                                         &HtcNetwork::SetPgwTftNumNodes),
                   MakeUintegerChecker<uint16_t> (1))
    .AddAttribute ("PgwTftPipelineCapacity",
                   "Pipeline capacity for P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&HtcNetwork::GetPgwTftPipeCapacity,
                                         &HtcNetwork::SetPgwTftPipeCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwTftTableSize",
                   "Flow/Group/Meter table sizes for P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&HtcNetwork::GetPgwTftTableSize,
                                         &HtcNetwork::SetPgwTftTableSize),
                   MakeUintegerChecker<uint16_t> (1, 65535))

    // TODO Fazer a configuração também dos parâmetros do S-GW.
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

  // TODO
}

} // namespace ns3
