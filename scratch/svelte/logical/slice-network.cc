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
#include "slice-network.h"
#include "slice-controller.h"
// #include "gtp-tunnel-app.h"
// #include "pgw-tunnel-app.h"
// #include "../sdran/sdran-cloud.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceNetwork");
NS_OBJECT_ENSURE_REGISTERED (SliceNetwork);

// Initializing SliceNetwork static members.
// FIXME O endereço IP de cada slice deve ser diferente pra não dar conflito no gateway
const Ipv4Address SliceNetwork::m_ueAddr   = Ipv4Address ("7.0.0.0");
const Ipv4Address SliceNetwork::m_sgiAddr  = Ipv4Address ("8.0.0.0");
const Ipv4Mask    SliceNetwork::m_ueMask   = Ipv4Mask ("255.0.0.0");
const Ipv4Mask    SliceNetwork::m_sgiMask  = Ipv4Mask ("255.0.0.0");

SliceNetwork::SliceNetwork ()
  : m_controllerApp (0),
  m_controllerNode (0),
  m_switchHelper (0),
  m_webNode (0)
{
  NS_LOG_FUNCTION (this);
}

SliceNetwork::~SliceNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SliceNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SliceNetwork")
    .SetParent<Object> ()
    .AddAttribute ("NumUes", "The total number of UEs for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceNetwork::m_nUes),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("UeMobility", "Enable UE random mobility.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&SliceNetwork::m_ueMobility),
                   MakeBooleanChecker ())

    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&SliceNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NumPgwTftSwitches",
                   "The number of P-GW TFT user-plane OpenFlow switches "
                   "(must be a power of 2).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceNetwork::m_pgwNumTftNodes),
                   MakeUintegerChecker<uint16_t> (1))
    .AddAttribute ("PgwTftPipelineCapacity",
                   "Pipeline capacity for P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_tftPipeCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwTftTableSize",
                   "Flow/Group/Meter table sizes for P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_tftTableSize),
                   MakeUintegerChecker<uint16_t> (1, 65535))
    .AddAttribute ("SgiLinkDataRate",
                   "The data rate for the link connecting the P-GW "
                   "to the Internet web server.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("SgiLinkDelay",
                   "The delay for the link connecting the P-GW "
                   "to the Internet web server.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MilliSeconds (15)),
                   MakeTimeAccessor (&SliceNetwork::m_linkDelay),
                   MakeTimeChecker ())
  ;
  return tid;
}

Ptr<Node>
SliceNetwork::GetWebNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_webNode;
}

void
SliceNetwork::SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);

  m_switchHelper->SetDeviceAttribute (n1, v1);
}

void
SliceNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel.
  m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  helper.EnablePcap (prefix + "pgw-int",  m_pgwIntDevices, promiscuous);
  helper.EnablePcap (prefix + "web-sgi",  m_sgiDevices, promiscuous);
}

Ipv4InterfaceContainer
SliceNetwork::AssignUeAddress (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  // TODO
  return Ipv4InterfaceContainer ();
}

NodeContainer
SliceNetwork::GetUeNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueNodes;
}

NetDeviceContainer
SliceNetwork::GetUeDevices (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueDevices;
}


// FIXME Esse aqui é para attach S-GW
// void
// SliceNetwork::AttachSdranCloud (Ptr<SdranCloud> sdranCloud)
// {
//   NS_LOG_FUNCTION (this << sdranCloud);
//
//   Ptr<Node> sgwNode = sdranCloud->GetSgwNode ();
//   Ptr<OFSwitch13Device> sgwSwitchDev = sdranCloud->GetSgwSwitchDevice ();
//   Ptr<SliceController> sdranCtrlApp = sdranCloud->GetSdranCtrlApp ();
//   sdranCtrlApp->SetEpcCtlrApp (m_controllerApp);
//
//   // Get the switch datapath ID on the backhaul network to attach the S-GW.
//   uint64_t swDpId = TopologyGetSgwSwitch (sdranCloud);
//   Ptr<Node> swNode = GetSwitchNode (swDpId);
//
//   // Connect the S-GW to the backhaul network over S5 interface.
//   NetDeviceContainer devices = m_csmaHelper.Install (swNode, sgwNode);
//   m_s5Devices.Add (devices.Get (1));
//
//   Ptr<CsmaNetDevice> swS5Dev, sgwS5Dev;
//   swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
//   sgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//   Names::Add (Names::FindName (swNode) + "_to_" +
//               Names::FindName (sgwNode), swS5Dev);
//   Names::Add (Names::FindName (sgwNode) + "_to_" +
//               Names::FindName (swNode), sgwS5Dev);
//
//   // Add the swS5Dev device as OpenFlow switch port on the backhaul switch.
//   Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
//   Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
//   uint32_t swS5PortNo = swS5Port->GetPortNo ();
//
//   // Add the sgwS5Dev as standard device on S-GW node.
//   // It will be connected to a logical port through the GtpTunnelApp.
//   m_s5AddrHelper.Assign (NetDeviceContainer (sgwS5Dev));
//   NS_LOG_INFO ("S-GW S5 address: " << SliceNetwork::GetIpv4Addr (sgwS5Dev));
//
//   // Create the virtual net device to work as the logical port on the S-GW S5
//   // interface. This logical ports will connect to the S-GW user-plane
//   // application, which will forward packets to/from this logical port and the
//   // S5 UDP socket binded to the sgwS5Dev.
//   Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
//   sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
//   Ptr<OFSwitch13Port> sgwS5Port = sgwSwitchDev->AddSwitchPort (sgwS5PortDev);
//   uint32_t sgwS5PortNo = sgwS5Port->GetPortNo ();
//
//   // Create the S-GW S5 user-plane application.
//   sgwNode->AddApplication (
//     CreateObject<GtpTunnelApp> (sgwS5PortDev, sgwS5Dev));
//
//   // Notify the EPC and SDRAN controllers of the new S-GW device attached
//   // OpenFlow backhaul network.
//   std::pair<uint32_t, uint32_t> mtcAggTeids;
//   m_controllerApp->NotifyS5Attach (swDev, swS5PortNo, sgwS5Dev);
//   mtcAggTeids = m_controllerApp->NotifySgwAttach (sgwS5Dev);
//   sdranCtrlApp->NotifySgwAttach (sgwS5PortNo, sgwS5Dev, mtcAggTeids.first,
//                                  mtcAggTeids.second);
// }

void
SliceNetwork::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_controllerApp = 0;
  m_controllerNode = 0;
  m_switchHelper = 0;
  m_webNode = 0;
  m_pgwNodes = NodeContainer ();

  Object::DoDispose ();
}

void
SliceNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Check the number of P-GW nodes (must have a power of 2 P-GW TFT nodes).
  NS_ASSERT_MSG ((GetNTftNodes () & (GetNTftNodes () - 1)) == 0,
                 "Invalid number of P-GW nodes.");

  // Configure IP address helpers.
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_sgiAddrHelper.SetBase (m_sgiAddr, m_sgiMask);

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Create the Internet network and the P-GW user-plane.
  InternetCreate ();
  PgwCreate ();

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

uint32_t
SliceNetwork::GetNTftNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwNumTftNodes;
}

void
SliceNetwork::InstallController (Ptr<SliceController> controller)
{
  NS_LOG_FUNCTION (this << controller);

  NS_ASSERT_MSG (!m_controllerApp, "Controller application already set.");

  // Create the controller node.
  m_controllerNode = CreateObject<Node> ();
  Names::Add ("slice_ctrl", m_controllerNode); // FIXME Vai dar pau por nome repetido quando tiver 2 slices.

  // Installing the controller application into controller node.
  m_controllerApp = controller;
  m_switchHelper->InstallController (m_controllerNode, m_controllerApp);
}

void
SliceNetwork::InternetCreate (void)
{
  NS_LOG_FUNCTION (this);

  // Create the single web server node.
  m_webNode = CreateObject<Node> ();
  Names::Add ("web", m_webNode);  // FIXME Vai dar pau por causa de nome repetido

  // Install the Internet stack into web node.
  InternetStackHelper internet;
  internet.Install (m_webNode);
}

void
SliceNetwork::PgwCreate (void)
{
  NS_LOG_FUNCTION (this);

//  // Create the P-GW nodes and configure them as OpenFlow switches.
//  m_pgwNodes.Create (m_pgwNumTftNodes + 1);
//  m_pgwOfDevices = m_switchHelper->InstallSwitch (m_pgwNodes);
//  for (uint16_t i = 0; i < m_pgwNumTftNodes + 1; i++)
//    {
//      std::ostringstream pgwNodeName;
//      pgwNodeName << "pgw" << i + 1;    // FIXME Vai dar pau com nome repetido
//      Names::Add (pgwNodeName.str (), m_pgwNodes.Get (i));
//    }
//
//  // Set the default P-GW gateway logical address, which will be used to set
//  // the static route at all UEs.
//  m_pgwAddr = m_ueAddrHelper.NewAddress ();
//  NS_LOG_INFO ("P-GW gateway address: " << GetUeDefaultGatewayAddress ());
//
//  // Get the backhaul node and device to attach the P-GW.
//  uint64_t backOfDpId = TopologyGetPgwSwitch ();
//  Ptr<Node> backNode = GetSwitchNode (backOfDpId);
//  Ptr<OFSwitch13Device> backOfDev = OFSwitch13Device::GetDevice (backOfDpId);
//
//  // Get the P-GW main node and device.
//  Ptr<Node> pgwMainNode = m_pgwNodes.Get (0);
//  Ptr<OFSwitch13Device> pgwMainOfDev = m_pgwOfDevices.Get (0);
//
//  //
//  // Connect the P-GW main switch to the SGi and S5 interfaces. On the uplink
//  // direction, the traffic will flow directly from the S5 to the SGi interface
//  // thought this switch. On the downlink direction, this switch will send the
//  // traffic to the TFT switches.
//  //
//  // Configure CSMA helper for connecting the P-GW node to the web server node.
//  m_csmaHelper.SetDeviceAttribute  ("Mtu", UintegerValue (m_linkMtu));
//  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
//  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));
//
//  // Connect the P-GW main node to the web server node (SGi interface).
//  m_sgiDevices = m_csmaHelper.Install (pgwMainNode, m_webNode);
//
//  Ptr<CsmaNetDevice> pgwSgiDev, webSgiDev;
//  pgwSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (0));
//  webSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (1));
//
//  Names::Add (Names::FindName (pgwMainNode) + "_to_" +
//              Names::FindName (m_webNode), pgwSgiDev);
//  Names::Add (Names::FindName (m_webNode) + "_to_" +
//              Names::FindName (pgwMainNode), webSgiDev);
//
//  // Add the pgwSgiDev as physical port on the P-GW main OpenFlow switch.
//  Ptr<OFSwitch13Port> pgwSgiPort = pgwMainOfDev->AddSwitchPort (pgwSgiDev);
//  uint32_t pgwSgiPortNo = pgwSgiPort->GetPortNo ();
//
//  // Set the IP address on SGi interfaces.
//  m_sgiAddrHelper.Assign (NetDeviceContainer (m_sgiDevices));
//  NS_LOG_INFO ("Web SGi address: " << SliceNetwork::GetIpv4Addr (webSgiDev));
//  NS_LOG_INFO ("P-GW SGi address: " << SliceNetwork::GetIpv4Addr (pgwSgiDev));
//
//  // Define static routes at the web server to the LTE network.
//  Ipv4StaticRoutingHelper ipv4RoutingHelper;
//  Ptr<Ipv4StaticRouting> webHostStaticRouting =
//    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
//  webHostStaticRouting->AddNetworkRouteTo (
//    SliceNetwork::m_ueAddr, SliceNetwork::m_ueMask,
//    SliceNetwork::GetIpv4Addr (pgwSgiDev), 1);
//
//  // Configure CSMA helper for connecting EPC nodes (P-GW and S-GWs) to the
//  // OpenFlow backhaul topology.
//  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_s5LinkRate));
//  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_s5LinkDelay));
//
//  // Connect the P-GW main node to the OpenFlow backhaul node (S5 interface).
//  NetDeviceContainer devices = m_csmaHelper.Install (pgwMainNode, backNode);
//  m_s5Devices.Add (devices.Get (0));
//
//  Ptr<CsmaNetDevice> pgwS5Dev, backS5Dev;
//  pgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (0));
//  backS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//  Names::Add (Names::FindName (backNode) + "_to_" +
//              Names::FindName (pgwMainNode), backS5Dev);
//  Names::Add (Names::FindName (pgwMainNode) + "_to_" +
//              Names::FindName (backNode), pgwS5Dev);
//
//  // Add the backS5Dev as physical port on the backhaul OpenFlow switch.
//  Ptr<OFSwitch13Port> backS5Port = backOfDev->AddSwitchPort (backS5Dev);
//  uint32_t backS5PortNo = backS5Port->GetPortNo ();
//
//  // Set the IP address on pgwS5Dev interface. It will be left as standard
//  // device on P-GW main node and will be connected to a logical port.
//  m_s5AddrHelper.Assign (NetDeviceContainer (pgwS5Dev));
//  NS_LOG_INFO ("P-GW S5 address: " << SliceNetwork::GetIpv4Addr (pgwS5Dev));
//
//  // Create the virtual net device to work as the logical ports on the P-GW S5
//  // interface. This logical ports will connect to the P-GW user-plane
//  // application, which will forward packets to/from this logical port and the
//  // S5 UDP socket binded to the pgwS5Dev.
//  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
//  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
//  Ptr<OFSwitch13Port> pgwS5Port = pgwMainOfDev->AddSwitchPort (pgwS5PortDev);
//  uint32_t pgwS5PortNo = pgwS5Port->GetPortNo ();
//
//  // Create the P-GW S5 user-plane application.
//  Ptr<PgwTunnelApp> tunnelApp;
//  tunnelApp = CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev);
//  pgwMainNode->AddApplication (tunnelApp);
//
//  // Notify the EPC controller of the P-GW main switch attached to the Internet
//  // and to the OpenFlow backhaul network.
//  m_controllerApp->NotifyS5Attach (backOfDev, backS5PortNo, pgwS5Dev);
//  m_controllerApp->NotifyPgwMainAttach (pgwMainOfDev, pgwS5PortNo, pgwSgiPortNo,
//                                     pgwS5Dev, webSgiDev);
//
//  //
//  // Connect all P-GW TFT switches to the P-GW main switch and to the S5
//  // interface. Only downlink traffic will be sent to these switches.
//  //
//  for (uint16_t tftIdx = 0; tftIdx < GetNTftNodes (); tftIdx++)
//    {
//      Ptr<Node> pgwTftNode = m_pgwNodes.Get (tftIdx + 1);
//      Ptr<OFSwitch13Device> pgwTftOfDev = m_pgwOfDevices.Get (tftIdx + 1);
//
//      // Set P-GW TFT attributes.
//      pgwTftOfDev->SetAttribute (
//        "PipelineCapacity", DataRateValue (m_tftPipeCapacity));
//      pgwTftOfDev->SetAttribute (
//        "FlowTableSize", UintegerValue (m_tftTableSize));
//      pgwTftOfDev->SetAttribute (
//        "GroupTableSize", UintegerValue (m_tftTableSize));
//      pgwTftOfDev->SetAttribute (
//        "MeterTableSize", UintegerValue (m_tftTableSize));
//
//      // Connect the P-GW main node to the P-GW TFT node.
//      devices = m_csmaHelper.Install (pgwTftNode, pgwMainNode);
//      m_pgwIntDevices.Add (devices);
//
//      Ptr<CsmaNetDevice> tftDev, mainDev;
//      tftDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
//      mainDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//      // Add the mainDev as physical port on the P-GW main OpenFlow switch.
//      Ptr<OFSwitch13Port> mainPort = pgwMainOfDev->AddSwitchPort (mainDev);
//      uint32_t mainPortNo = mainPort->GetPortNo ();
//
//      // Add the tftDev as physical port on the P-GW TFT OpenFlow switch.
//      Ptr<OFSwitch13Port> tftPort = pgwTftOfDev->AddSwitchPort (tftDev);
//      uint32_t tftPortNo = tftPort->GetPortNo ();
//      NS_UNUSED (tftPortNo);
//
//      // Connect the P-GW TFT node to the OpenFlow backhaul node (S5 interf).
//      devices = m_csmaHelper.Install (pgwTftNode, backNode);
//      m_s5Devices.Add (devices.Get (0));
//
//      pgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (0));
//      backS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));
//
//      Names::Add (Names::FindName (backNode) + "_to_" +
//                  Names::FindName (pgwTftNode), backS5Dev);
//      Names::Add (Names::FindName (pgwTftNode) + "_to_" +
//                  Names::FindName (backNode), pgwS5Dev);
//
//      // Add the backS5Dev as physical port on the backhaul OpenFlow switch.
//      backS5Port = backOfDev->AddSwitchPort (backS5Dev);
//      backS5PortNo = backS5Port->GetPortNo ();
//
//      // Set the IP address on pgwS5Dev interface. It will be left as standard
//      // device on P-GW TFT node and will be connected to a logical port.
//      m_s5AddrHelper.Assign (NetDeviceContainer (pgwS5Dev));
//      NS_LOG_INFO ("P-GW TFT S5 addr: " << SliceNetwork::GetIpv4Addr (pgwS5Dev));
//
//      // Create the virtual net device to work as the logical ports on the P-GW
//      // S5 interface.
//      pgwS5PortDev = CreateObject<VirtualNetDevice> ();
//      pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
//      pgwS5Port = pgwTftOfDev->AddSwitchPort (pgwS5PortDev);
//      pgwS5PortNo = pgwS5Port->GetPortNo ();
//
//      // Create the P-GW S5 user-plane application.
//      tunnelApp = CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev);
//      pgwTftNode->AddApplication (tunnelApp);
//
//      // Notify the EPC controller of the P-GW TFT switch attached to the P-GW
//      // main switch and to the OpenFlow backhaul network.
//      m_controllerApp->NotifyS5Attach (backOfDev, backS5PortNo, pgwS5Dev);
//      m_controllerApp->NotifyPgwTftAttach (tftIdx, pgwTftOfDev, pgwS5PortNo,
//                                        mainPortNo);
//    }
//  m_controllerApp->NotifyPgwBuilt (m_pgwOfDevices);
}

} // namespace ns3
