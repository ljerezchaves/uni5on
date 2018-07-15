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

#include <ns3/csma-module.h>
#include "backhaul-network.h"
#include "backhaul-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulNetwork");
NS_OBJECT_ENSURE_REGISTERED (BackhaulNetwork);

// Initializing BackhaulNetwork static members.
const uint16_t    BackhaulNetwork::m_gtpuPort = 2152;
const Ipv4Address BackhaulNetwork::m_s1uAddr  = Ipv4Address ("10.1.0.0");
const Ipv4Address BackhaulNetwork::m_s5Addr   = Ipv4Address ("10.2.0.0");
const Ipv4Address BackhaulNetwork::m_x2Addr   = Ipv4Address ("10.3.0.0");
const Ipv4Mask    BackhaulNetwork::m_s1uMask  = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    BackhaulNetwork::m_s5Mask   = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    BackhaulNetwork::m_x2Mask   = Ipv4Mask ("255.255.255.0");

BackhaulNetwork::BackhaulNetwork ()
  : m_controllerApp (0),
  m_controllerNode (0),
  m_switchHelper (0)
{
  NS_LOG_FUNCTION (this);
}

BackhaulNetwork::~BackhaulNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
BackhaulNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BackhaulNetwork")
    .SetParent<Object> ()
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&BackhaulNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EpcLinkDataRate",
                   "The data rate for the link connecting any EPC entity to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&BackhaulNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("EpcLinkDelay",
                   "The delay for the link connecting any EPC entity to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   // The default value is for 10km fiber cable latency.
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&BackhaulNetwork::m_linkDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
BackhaulNetwork::SetSwitchDeviceAttribute (std::string n1,
                                           const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);

  m_switchHelper->SetDeviceAttribute (n1, v1);
}

void
BackhaulNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel.
  m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  helper.EnablePcap (prefix + "epc-s1u",  m_s1uDevices,  promiscuous);
  helper.EnablePcap (prefix + "epc-s5",   m_s5Devices,   promiscuous);
  helper.EnablePcap (prefix + "epc-x2",   m_x2Devices,   promiscuous);
  helper.EnablePcap (prefix + "backhaul", m_switchNodes, promiscuous);
}

Ipv4Address
BackhaulNetwork::AttachEnb (Ptr<Node> enbNode, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enbNode << cellId);

  // Add an IPv4 stack to the previously created eNB node.
  InternetStackHelper internet;
  internet.Install (enbNode);

  // Get the switch on the backhaul network to attach the eNB.
  uint64_t swDpId = TopologyGetEnbSwitch (cellId);
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<Node> swNode = swDev->GetObject<Node> ();

  // Connect the eNB to the backhaul network over S1-U interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, enbNode);
  m_s1uDevices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS1uDev, enbS1uDev;
  swS1uDev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  enbS1uDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "_to_" +
              Names::FindName (enbNode), swS1uDev);
  Names::Add (Names::FindName (enbNode) + "_to_" +
              Names::FindName (swNode), enbS1uDev);

  // Add the swS1uDev device as OpenFlow switch port on the backhaul switch.
  Ptr<OFSwitch13Port> swS1Port = swDev->AddSwitchPort (swS1uDev);
  uint32_t swS1PortNo = swS1Port->GetPortNo ();

  // Add the enbS1uDev as standard device on eNB node.
  Ipv4InterfaceContainer s1uIpIfaces;
  s1uIpIfaces = m_s1uAddrHelper.Assign (NetDeviceContainer (enbS1uDev));
  Ipv4Address enbS1uAddr = s1uIpIfaces.GetAddress (0);
  NS_LOG_INFO ("eNB S1-U address: " << enbS1uAddr);

  // Notify the backhaul controller of the new EPC device attached to the
  // OpenFlow backhaul network.
  m_controllerApp->NotifyEpcAttach (swDev, swS1PortNo, enbS1uDev);

  return enbS1uAddr;
}

void
BackhaulNetwork::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_controllerApp = 0;
  m_controllerNode = 0;
  m_switchHelper = 0;

  Object::DoDispose ();
}

void
BackhaulNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure IP address helpers.
  m_s1uAddrHelper.SetBase (m_s1uAddr, m_s1uMask);
  m_s5AddrHelper.SetBase (m_s5Addr, m_s5Mask);
  m_x2AddrHelper.SetBase (m_x2Addr, m_x2Mask);

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Create the OpenFlow backhaul network.
  TopologyCreate ();

  // Let's connect the OpenFlow switches to the EPC controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  // Enable OpenFlow switch statistics.
  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  m_switchHelper->EnableDatapathStats (prefix + "ofswitch-stats", true);

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
BackhaulNetwork::InstallController (Ptr<BackhaulController> controller)
{
  NS_LOG_FUNCTION (this << controller);

  NS_ASSERT_MSG (!m_controllerApp, "Controller application already set.");

  // Create the controller node.
  m_controllerNode = CreateObject<Node> ();
  Names::Add ("backhaul_ctrl", m_controllerNode);

  // Installing the controller application into controller node.
  m_controllerApp = controller;
  m_switchHelper->InstallController (m_controllerNode, m_controllerApp);
}

} // namespace ns3
