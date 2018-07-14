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

#include <ns3/csma-module.h>
#include "svelte-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SvelteHelper");
NS_OBJECT_ENSURE_REGISTERED (SvelteHelper);

// Initializing SvelteHelper static members.
const uint16_t    SvelteHelper::m_gtpuPort = 2152;
const Ipv4Address SvelteHelper::m_htcAddr  = Ipv4Address ("7.64.0.0");
const Ipv4Address SvelteHelper::m_mtcAddr  = Ipv4Address ("7.128.0.0");
const Ipv4Address SvelteHelper::m_s1uAddr  = Ipv4Address ("10.2.0.0");
const Ipv4Address SvelteHelper::m_s5Addr   = Ipv4Address ("10.1.0.0");
const Ipv4Address SvelteHelper::m_sgiAddr  = Ipv4Address ("8.0.0.0");
const Ipv4Address SvelteHelper::m_ueAddr   = Ipv4Address ("7.0.0.0");
const Ipv4Address SvelteHelper::m_x2Addr   = Ipv4Address ("10.3.0.0");
const Ipv4Mask    SvelteHelper::m_htcMask  = Ipv4Mask ("255.192.0.0");
const Ipv4Mask    SvelteHelper::m_mtcMask  = Ipv4Mask ("255.192.0.0");
const Ipv4Mask    SvelteHelper::m_s1uMask  = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    SvelteHelper::m_s5Mask   = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    SvelteHelper::m_sgiMask  = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SvelteHelper::m_ueMask   = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SvelteHelper::m_x2Mask   = Ipv4Mask ("255.255.255.0");

SvelteHelper::SvelteHelper ()
{
  NS_LOG_FUNCTION (this);
}

SvelteHelper::~SvelteHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SvelteHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SvelteHelper")
    .SetParent<EpcHelper> ()
  ;
  return tid;
}

void
SvelteHelper::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

void
SvelteHelper::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure IP address helpers.
  m_htcUeAddrHelper.SetBase (m_htcAddr, m_htcMask);
  m_mtcUeAddrHelper.SetBase (m_mtcAddr, m_mtcMask);
  m_s1uAddrHelper.SetBase   (m_s1uAddr, m_s1uMask);
  m_s5AddrHelper.SetBase    (m_s5Addr,  m_s5Mask);
  m_sgiAddrHelper.SetBase   (m_sgiAddr, m_sgiMask);
  m_x2AddrHelper.SetBase    (m_x2Addr,  m_x2Mask);

  // Configure the default PG-W address.
  // FIXME This may not make sense with multiple P-GW instances. Check this!
  Ipv4AddressHelper m_ueAddrHelper;
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_pgwAddr = m_ueAddrHelper.NewAddress ();

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
SvelteHelper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  return 0;
//  FIXME
//  // Retrieve the IPv4 address of the UE and save it into UeInfo, if necessary.
//  Ptr<Node> ueNode = ueDevice->GetNode ();
//  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
//  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed.");
//
//  int32_t interface = ueIpv4->GetInterfaceForDevice (ueDevice);
//  NS_ASSERT (interface >= 0);
//  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);
//
//  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
//  NS_LOG_INFO ("Activate EPS bearer UE IP address: " << ueAddr);
//
//  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
//  if (ueInfo->GetUeAddr () != ueAddr)
//    {
//      ueInfo->SetUeAddr (ueAddr);
//
//      // Identify MTC UEs by the network address.
//      if (m_mtcAddr.IsEqual (ueAddr.CombineMask (m_mtcMask)))
//        {
//          ueInfo->SetMtc (true);
//        }
//    }
//
//  // Trick for default bearer.
//  if (tft->IsDefaultTft ())
//    {
//      // To avoid rules overlap on the P-GW, we are going to replace the
//      // default packet filter by two filters that includes the UE address and
//      // the protocol (TCP and UDP).
//      tft->RemoveFilter (0);
//
//      EpcTft::PacketFilter filterTcp;
//      filterTcp.protocol = TcpL4Protocol::PROT_NUMBER;
//      filterTcp.localAddress = ueAddr;
//      tft->Add (filterTcp);
//
//      EpcTft::PacketFilter filterUdp;
//      filterUdp.protocol = UdpL4Protocol::PROT_NUMBER;
//      filterUdp.localAddress = ueAddr;
//      tft->Add (filterUdp);
//    }
//
//  // Save the bearer context into UE info.
//  UeInfo::BearerInfo bearerInfo;
//  bearerInfo.tft = tft;
//  bearerInfo.bearer = bearer;
//  uint8_t bearerId = ueInfo->AddBearer (bearerInfo);
//
//  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
//  if (ueLteDevice)
//    {
//      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
//    }
//  return bearerId;
}

void
SvelteHelper::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  // TODO
}

void
SvelteHelper::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // TODO
}

void
SvelteHelper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice);

  // Create the UE info. FIXME
//  CreateObject<UeInfo> (imsi);
}

Ptr<Node>
SvelteHelper::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  // FIXME Should return the IP address for the correct slice.
  NS_FATAL_ERROR ("SVELTE has more than one P-GW node.");
}

Ipv4InterfaceContainer
SvelteHelper::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  NS_FATAL_ERROR ("Use the specific method for HTC or MTC UEs.");
}

Ipv4Address
SvelteHelper::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  // FIXME Deveria retornar o IP correto de acordo com o slice.
  // Fazer variações para cada tipo de UE.
  return m_pgwAddr;
}

Ipv4InterfaceContainer
SvelteHelper::AssignHtcUeIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_htcUeAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteHelper::AssignMtcUeIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_mtcUeAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteHelper::AssignS1Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_s1uAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteHelper::AssignS5Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_s5AddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteHelper::AssignSgiIpv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_sgiAddrHelper.Assign (devices);
}

Ipv4InterfaceContainer
SvelteHelper::AssignX2Ipv4Address (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  return m_x2AddrHelper.Assign (devices);
}

Ipv4Address
SvelteHelper::GetIpv4Addr (Ptr<const NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetLocal ();
}

Ipv4Mask
SvelteHelper::GetIpv4Mask (Ptr<const NetDevice> device)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = device->GetNode ();
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  int32_t idx = ipv4->GetInterfaceForDevice (device);
  return ipv4->GetAddress (idx, 0).GetMask ();
}

} // namespace ns3
