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
#include "gtp-tunnel-app.h"
#include "pgw-tunnel-app.h"
#include "slice-controller.h"
#include "../infrastructure/backhaul-network.h"
#include "../infrastructure/radio-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceNetwork");
NS_OBJECT_ENSURE_REGISTERED (SliceNetwork);

std::string LogicalSliceStr (LogicalSlice slice)
{
  switch (slice)
    {
    case LogicalSlice::HTC:
      return "htc";
    case LogicalSlice::MTC:
      return "mtc";
    default:
      NS_LOG_ERROR ("Invalid logical slice.");
      return "";
    }
}

SliceNetwork::SliceNetwork ()
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
    .AddConstructor<SliceNetwork> ()

    // Slice.
    .AddAttribute ("SliceId", "The LTE logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (LogicalSlice::HTC),
                   MakeEnumAccessor (&SliceNetwork::m_sliceId),
                   MakeEnumChecker (LogicalSlice::HTC, "htc",
                                    LogicalSlice::MTC, "mtc"))
    .AddAttribute ("Controller", "The slice controller application pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_controllerApp),
                   MakePointerChecker<SliceController> ())

    // Infrastructure.
    .AddAttribute ("Backhaul", "The OpenFlow backhaul network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_backhaul),
                   MakePointerChecker<BackhaulNetwork> ())
    .AddAttribute ("Radio", "The LTE RAN network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_lteRan),
                   MakePointerChecker<RadioNetwork> ())

    // UEs.
    .AddAttribute ("NumUes", "The total number of UEs for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceNetwork::m_nUes),
                   MakeUintegerChecker<uint32_t> (0, 3000))
    .AddAttribute ("UeAddress", "The UE network address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue ("7.0.0.0"),
                   MakeIpv4AddressAccessor (&SliceNetwork::m_ueAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("UeMask", "The UE network mask.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4MaskValue ("255.0.0.0"),
                   MakeIpv4MaskAccessor (&SliceNetwork::m_ueMask),
                   MakeIpv4MaskChecker ())
    .AddAttribute ("UeMobility", "Enable UE random mobility.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&SliceNetwork::m_ueMobility),
                   MakeBooleanChecker ())

    // Internet.
    .AddAttribute ("WebAddress", "The Internet network address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue ("8.0.0.0"),
                   MakeIpv4AddressAccessor (&SliceNetwork::m_webAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("WebMask", "The Internet network mask.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4MaskValue ("255.0.0.0"),
                   MakeIpv4MaskAccessor (&SliceNetwork::m_webMask),
                   MakeIpv4MaskChecker ())
    .AddAttribute ("WebLinkDataRate",
                   "The data rate for the link connecting the P-GW "
                   "to the Internet web server.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_webLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("WebLinkDelay",
                   "The delay for the link connecting the P-GW "
                   "to the Internet web server.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MilliSeconds (15)),
                   MakeTimeAccessor (&SliceNetwork::m_webLinkDelay),
                   MakeTimeChecker ())

    // P-GW.
    .AddAttribute ("NumPgwTftSwitches",
                   "The number of P-GW TFT user-plane OpenFlow switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceNetwork::GetPgwTftNumNodes,
                                         &SliceNetwork::SetPgwTftNumNodes),
                   MakeUintegerChecker<uint16_t> (1))
    .AddAttribute ("PgwBackhaulSwitch",
                   "The backhaul switch index to connect the P-GW.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceNetwork::m_pgwSwitchIdx),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PgwTftPipelineCapacity",
                   "Pipeline capacity for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_tftPipeCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwTftTableSize",
                   "Flow/Group/Meter table sizes for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_tftTableSize),
                   MakeUintegerChecker<uint16_t> (1, 65535))
    .AddAttribute ("PgwLinkDataRate",
                   "The data rate for the internal P-GW links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_pgwLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwLinkDelay",
                   "The delay for the internal P-GW links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&SliceNetwork::m_pgwLinkDelay),
                   MakeTimeChecker ())

    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&SliceNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
SliceNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel.
  m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  helper.EnablePcap (prefix + "pgw_user", m_pgwIntDevices, promiscuous);
  helper.EnablePcap (prefix + "internet", m_webDevices, promiscuous);
}

uint32_t
SliceNetwork::GetPgwTftNumNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nTftNodes;
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
//   BackhaulNetwork::SetDeviceNames (swS5Dev, sgwS5Dev, "~");
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

  m_backhaul = 0;
  m_lteRan = 0;
  m_switchHelper = 0;
  m_controllerApp = 0;
  m_controllerNode = 0;
  m_webNode = 0;

  Object::DoDispose ();
}

void
SliceNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (!m_backhaul || !m_lteRan, "No infrastructure network.");
  NS_ABORT_MSG_IF (!m_controllerApp, "No slice controller application.");

  m_sliceIdStr = LogicalSliceStr (m_sliceId);
  NS_LOG_INFO ("LTE " << m_sliceIdStr << " slice with " << m_nUes << " UEs.");

  // Configure IP address helpers.
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_webAddrHelper.SetBase (m_webAddr, m_webMask);

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Configure and install the slice controller application.
  m_controllerApp->SetNetworkAttributes (m_ueAddr, m_ueMask, m_nTftNodes);
  m_controllerNode = CreateObject<Node> ();
  Names::Add (m_sliceIdStr + "_ctrl", m_controllerNode);
  m_switchHelper->InstallController (m_controllerNode, m_controllerApp);

  // Create the Internet web server node with Internet stack.
  m_webNode = CreateObject<Node> ();
  Names::Add (m_sliceIdStr + "_web", m_webNode);
  InternetStackHelper internet;
  internet.Install (m_webNode);

  // Create and configure the logical LTE network.
  CreatePgw ();
  CreateSgws ();
  CreateUes ();

  // Let's connect the OpenFlow switches to the controller. From this point
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
SliceNetwork::SetBackhaulNetwork (Ptr<BackhaulNetwork> value)
{
  NS_LOG_FUNCTION (this << value);

  m_backhaul = value;
}

void
SliceNetwork::SetRadioNetwork (Ptr<RadioNetwork> value)
{
  NS_LOG_FUNCTION (this << value);

  m_lteRan = value;
}

void
SliceNetwork::SetPgwTftNumNodes (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  // Check the number of P-GW TFT nodes (must be a power of 2).
  NS_ABORT_MSG_IF ((value & (value - 1)) != 0, "Invalid number of P-GW TFTs.");

  m_nTftNodes = value;
}

void
SliceNetwork::CreatePgw (void)
{
  NS_LOG_FUNCTION (this);

  // Create the P-GW nodes and configure them as OpenFlow switches.
  m_pgwNodes.Create (m_nTftNodes + 1);
  m_pgwDevices = m_switchHelper->InstallSwitch (m_pgwNodes);
  for (uint16_t i = 0; i < m_nTftNodes + 1; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr << "_pgw" << i + 1;
      Names::Add (name.str (), m_pgwNodes.Get (i));
    }

  // Set the default P-GW gateway logical address, which will be used to set
  // the static route at all UEs.
  m_pgwAddress = m_ueAddrHelper.NewAddress ();
  NS_LOG_INFO ("P-GW gateway S5 address: " << m_pgwAddress);

  // Get the P-GW main node and device.
  Ptr<Node> pgwMainNode = m_pgwNodes.Get (0);
  Ptr<OFSwitch13Device> pgwMainOfDev = m_pgwDevices.Get (0);

  //
  // Connect the P-GW main switch to the SGi and S5 interfaces. On the uplink
  // direction, the traffic will flow directly from the S5 to the SGi interface
  // thought this switch. On the downlink direction, this switch will send the
  // traffic to the TFT switches.
  //
  // Configure CSMA helper for connecting the P-GW node to the web server node.
  m_csmaHelper.SetDeviceAttribute  ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_webLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_webLinkDelay));

  // Connect the P-GW main node to the web server node (SGi interface).
  m_webDevices = m_csmaHelper.Install (pgwMainNode, m_webNode);

  Ptr<CsmaNetDevice> pgwSgiDev, webSgiDev;
  pgwSgiDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (0));
  webSgiDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (1));
  BackhaulNetwork::SetDeviceNames (pgwSgiDev, webSgiDev, "~sgi~");

  // Add the pgwSgiDev as physical port on the P-GW main OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = pgwMainOfDev->AddSwitchPort (pgwSgiDev);
  uint32_t pgwSgiPortNo = pgwSgiPort->GetPortNo ();

  // Set the IP address on the Internet network.
  m_webAddrHelper.Assign (NetDeviceContainer (m_webDevices));
  NS_LOG_INFO ("Web SGi: " << Ipv4AddressHelper::GetFirstAddress (webSgiDev));
  NS_LOG_INFO ("P-GW SGi: " << Ipv4AddressHelper::GetFirstAddress (pgwSgiDev));

  // Define static routes at the web server to the LTE network.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    m_ueAddr, m_ueMask, Ipv4AddressHelper::GetFirstAddress (pgwSgiDev), 1);

  // Connect the P-GW node to the OpenFlow backhaul network.
  Ptr<CsmaNetDevice> pgwS5Dev;
  pgwS5Dev = m_backhaul->AttachPgw (pgwMainNode, m_pgwSwitchIdx);

  // Create the virtual net device to work as the logical ports on the P-GW S5
  // interface. This logical ports will connect to the P-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the pgwS5Dev.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = pgwMainOfDev->AddSwitchPort (pgwS5PortDev);
  uint32_t pgwS5PortNo = pgwS5Port->GetPortNo ();

  // Create the P-GW S5 user-plane application.
  Ptr<PgwTunnelApp> tunnelApp;
  tunnelApp = CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev);
  pgwMainNode->AddApplication (tunnelApp);

  // Notify the controller of the P-GW main switch attached to the Internet and
  // to the OpenFlow backhaul network.
  m_controllerApp->NotifyPgwMainAttach (pgwMainOfDev, pgwS5PortNo, pgwSgiPortNo,
                                        pgwS5Dev, webSgiDev);

  // Configure CSMA helper for connecting P-GW internal node.
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_pgwLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_pgwLinkDelay));
  NetDeviceContainer devices;

  //
  // Connect all P-GW TFT switches to the P-GW main switch and to the S5
  // interface. Only downlink traffic will be sent to these switches.
  //
  for (uint16_t tftIdx = 0; tftIdx < GetPgwTftNumNodes (); tftIdx++)
    {
      Ptr<Node> pgwTftNode = m_pgwNodes.Get (tftIdx + 1);
      Ptr<OFSwitch13Device> pgwTftOfDev = m_pgwDevices.Get (tftIdx + 1);

      // Set P-GW TFT attributes.
      pgwTftOfDev->SetAttribute (
        "PipelineCapacity", DataRateValue (m_tftPipeCapacity));
      pgwTftOfDev->SetAttribute (
        "FlowTableSize", UintegerValue (m_tftTableSize));
      pgwTftOfDev->SetAttribute (
        "GroupTableSize", UintegerValue (m_tftTableSize));
      pgwTftOfDev->SetAttribute (
        "MeterTableSize", UintegerValue (m_tftTableSize));

      // Connect the P-GW main node to the P-GW TFT node.
      devices = m_csmaHelper.Install (pgwTftNode, pgwMainNode);
      m_pgwIntDevices.Add (devices);

      Ptr<CsmaNetDevice> tftDev, mainDev;
      tftDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
      mainDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

      // Add the mainDev as physical port on the P-GW main OpenFlow switch.
      Ptr<OFSwitch13Port> mainPort = pgwMainOfDev->AddSwitchPort (mainDev);
      uint32_t mainPortNo = mainPort->GetPortNo ();

      // Add the tftDev as physical port on the P-GW TFT OpenFlow switch.
      Ptr<OFSwitch13Port> tftPort = pgwTftOfDev->AddSwitchPort (tftDev);
      uint32_t tftPortNo = tftPort->GetPortNo ();
      NS_UNUSED (tftPortNo);

      // Connect the P-GW TFT node to the OpenFlow backhaul node.
      Ptr<CsmaNetDevice> pgwS5Dev;
      pgwS5Dev = m_backhaul->AttachPgw (pgwTftNode, m_pgwSwitchIdx);

      // Create the virtual net device to work as the logical ports on the P-GW
      // S5 interface.
      pgwS5PortDev = CreateObject<VirtualNetDevice> ();
      pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
      pgwS5Port = pgwTftOfDev->AddSwitchPort (pgwS5PortDev);
      pgwS5PortNo = pgwS5Port->GetPortNo ();

      // Create the P-GW S5 user-plane application.
      tunnelApp = CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev);
      pgwTftNode->AddApplication (tunnelApp);

      // Notify the EPC controller of the P-GW TFT switch attached to the P-GW
      // main switch and to the OpenFlow backhaul network.
      m_controllerApp->NotifyPgwTftAttach (tftIdx, pgwTftOfDev, pgwS5PortNo,
                                           mainPortNo);
    }
  m_controllerApp->NotifyPgwBuilt (m_pgwDevices);
}

void
SliceNetwork::CreateSgws (void)
{
  NS_LOG_FUNCTION (this);

}

void
SliceNetwork::CreateUes (void)
{
  NS_LOG_FUNCTION (this);

  // Create the UE nodes and set their names.
  m_ueNodes.Create (m_nUes);
  for (uint32_t i = 0; i < m_nUes; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr + "_ue" << i + 1;
      Names::Add (name.str (), m_ueNodes.Get (i));
    }

  // Configure UE positioning and mobility.
  // TODO Use attributes for custom configuration.
  MobilityHelper mobilityHelper = m_lteRan->GetRandomInitialPositioning ();
  if (m_ueMobility)
    {
      // mobilityHelper.SetMobilityModel (
      //   "ns3::RandomWaypointMobilityModel",
      //   "Speed", StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=15.0]"),
      //   "Pause", StringValue ("ns3::ExponentialRandomVariable[Mean=25.0]"),
      //   "PositionAllocator", PointerValue (boxPosAllocator));
      // FIXME Como recuperar o PositionAllocator? Criar um novo aqui?
    }

  // Install LTE protocol stack into UE nodes.
  m_ueDevices = m_lteRan->InstallUeDevices (m_ueNodes, mobilityHelper);

  // Install TCP/IP protocol stack into UE nodes and assign IP address.
  InternetStackHelper internet;
  internet.Install (m_ueNodes);
  m_ueAddrHelper.Assign (m_ueDevices);

  // Specify static routes for each UE to its default S-GW.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  NodeContainer::Iterator it;
  for (it = m_ueNodes.Begin (); it != m_ueNodes.End (); it++)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (m_pgwAddress, 1);
    }

  // Attach UE to the eNBs using initial cell selection.
  m_lteRan->AttachUeDevices (m_ueDevices);
}

} // namespace ns3
