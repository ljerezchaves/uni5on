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
#include "../svelte-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BackhaulNetwork");
NS_OBJECT_ENSURE_REGISTERED (BackhaulNetwork);

BackhaulNetwork::BackhaulNetwork ()
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
    .SetParent<EpcHelper> ()
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
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

Ptr<Node>
BackhaulNetwork::GetSwitchNode (uint64_t dpId) const
{
  NS_LOG_FUNCTION (this << dpId);

  Ptr<Node> node = OFSwitch13Device::GetDevice (dpId)->GetObject<Node> ();
  NS_ASSERT_MSG (node, "No node found for this datapath ID");

  return node;
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
  helper.EnablePcap (prefix + "backhaul-s1", m_s1Devices,  promiscuous);
  helper.EnablePcap (prefix + "backhaul-s5", m_s5Devices,   promiscuous);
  helper.EnablePcap (prefix + "backhaul-x2", m_x2Devices,   promiscuous);
  helper.EnablePcap (prefix + "backhaul",    m_switchNodes, promiscuous);
}

void
BackhaulNetwork::AttachEnb (Ptr<Node> enbNode, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enbNode << cellId);

  // Get the switch datapath ID on the backhaul network to attach the eNB.
  uint64_t swDpId = TopologyGetEnbSwitch (cellId);
  Ptr<Node> swNode = GetSwitchNode (swDpId);

  // Connect the eNB to the backhaul network over S1-U interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, enbNode);
  m_s1Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS1Dev, enbS1Dev;
  swS1Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  enbS1Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "_to_" +
              Names::FindName (enbNode), swS1Dev);
  Names::Add (Names::FindName (enbNode) + "_to_" +
              Names::FindName (swNode), enbS1Dev);

  // Add the swS1Dev device as OpenFlow switch port on the backhaul switch.
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<OFSwitch13Port> swS1Port = swDev->AddSwitchPort (swS1Dev);
  uint32_t swS1PortNo = swS1Port->GetPortNo ();

  // Add the enbS1Dev as standard device on eNB node.
  // m_s1uAddrHelper.Assign (NetDeviceContainer (enbS1Dev)); // FIXME Usar o SvelteHelper???
  NS_LOG_INFO ("eNB S1-U address: " << SvelteHelper::GetIpv4Addr (enbS1Dev));

  // Notify the backhaul controller of the new EPC device attached to the
  // OpenFlow backhaul network.
  m_controllerApp->NotifyEpcAttach (swDev, swS1PortNo, enbS1Dev);
}

void
BackhaulNetwork::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_switchHelper = 0;
  m_controllerNode = 0;
  m_controllerApp = 0;
  m_svelteHelper = 0;

  Object::DoDispose ();
}

void
BackhaulNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG  (m_svelteHelper, "Create the object with SVELTE helper");

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
