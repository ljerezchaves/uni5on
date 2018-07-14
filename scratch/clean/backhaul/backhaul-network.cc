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
    .AddAttribute ("S1LinkDataRate",
                   "The data rate for the link connecting an S1 interface to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&BackhaulNetwork::m_s1LinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("S1LinkDelay",
                   "The delay for the link connecting an S1 interface to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   // The default value is for 10km fiber cable latency.
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&BackhaulNetwork::m_s1LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("S5LinkDataRate",
                   "The data rate for the link connecting an S5 interface to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&BackhaulNetwork::m_s5LinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("S5LinkDelay",
                   "The delay for the link connecting an S5 interface to  "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&BackhaulNetwork::m_s5LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("X2LinkDataRate",
                   "The data rate for the link connecting a X2 interface to  "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&BackhaulNetwork::m_x2LinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("X2LinkDelay",
                   "The delay for the link connecting a X2 interface to  "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&BackhaulNetwork::m_x2LinkDelay),
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
  helper.EnablePcap (prefix + "backhaul-s1", m_s1Devices,  promiscuous);
  helper.EnablePcap (prefix + "backhaul-s5", m_s5Devices,   promiscuous);
  helper.EnablePcap (prefix + "backhaul-x2", m_x2Devices,   promiscuous);
  helper.EnablePcap (prefix + "backhaul",    m_switchNodes, promiscuous);
}

void
BackhaulNetwork::AttachEnb (Ptr<Node> enb)
{
  NS_LOG_FUNCTION (this << enb);

// TODO
//  Ptr<OFSwitch13Device> sgwSwitchDev = sdranCloud->GetSgwSwitchDevice ();
//  Ptr<SdranController> sdranCtrlApp = sdranCloud->GetSdranCtrlApp ();
//  sdranCtrlApp->SetEpcCtlrApp (m_epcCtrlApp);
//
//  // Get the switch datapath ID on the backhaul network to attach the S-GW.
//  uint64_t swDpId = TopologyGetSgwSwitch (sdranCloud);
//  Ptr<Node> swNode = GetSwitchNode (swDpId);
//
//  // Connect the S-GW to the backhaul network over S5 interface.
//  NetDeviceContainer devices = m_csmaHelper.Install (swNode, sgwNode);
//  m_s5Devices.Add (devices.Get (1));
//
//  Ptr<CsmaNetDevice> swS5Dev, sgwS5Dev;
//  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
//  sgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//  Names::Add (Names::FindName (swNode) + "_to_" +
//              Names::FindName (sgwNode), swS5Dev);
//  Names::Add (Names::FindName (sgwNode) + "_to_" +
//              Names::FindName (swNode), sgwS5Dev);
//
//  // Add the swS5Dev device as OpenFlow switch port on the backhaul switch.
//  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
//  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
//  uint32_t swS5PortNo = swS5Port->GetPortNo ();
//
//  // Add the sgwS5Dev as standard device on S-GW node.
//  // It will be connected to a logical port through the GtpTunnelApp.
//  m_s5AddrHelper.Assign (NetDeviceContainer (sgwS5Dev));
//  NS_LOG_INFO ("S-GW S5 address: " << EpcNetwork::GetIpv4Addr (sgwS5Dev));
//
//  // Create the virtual net device to work as the logical port on the S-GW S5
//  // interface. This logical ports will connect to the S-GW user-plane
//  // application, which will forward packets to/from this logical port and the
//  // S5 UDP socket binded to the sgwS5Dev.
//  Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
//  sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
//  Ptr<OFSwitch13Port> sgwS5Port = sgwSwitchDev->AddSwitchPort (sgwS5PortDev);
//  uint32_t sgwS5PortNo = sgwS5Port->GetPortNo ();
//
//  // Create the S-GW S5 user-plane application.
//  sgwNode->AddApplication (
//    CreateObject<GtpTunnelApp> (sgwS5PortDev, sgwS5Dev));
//
//  // Notify the EPC and SDRAN controllers of the new S-GW device attached
//  // OpenFlow backhaul network.
//  std::pair<uint32_t, uint32_t> mtcAggTeids;
//  m_epcCtrlApp->NotifyS5Attach (swDev, swS5PortNo, sgwS5Dev);
//  mtcAggTeids = m_epcCtrlApp->NotifySgwAttach (sgwS5Dev);
//  sdranCtrlApp->NotifySgwAttach (sgwS5PortNo, sgwS5Dev, mtcAggTeids.first,
//                                 mtcAggTeids.second);
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
