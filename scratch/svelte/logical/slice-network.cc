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
#include "metadata/ue-info.h"
#include "metadata/sgw-info.h"
#include "../infrastructure/backhaul-network.h"
#include "../infrastructure/radio-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceNetwork");
NS_OBJECT_ENSURE_REGISTERED (SliceNetwork);

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
                   EnumValue (SliceId::NONE),
                   MakeEnumAccessor (&SliceNetwork::m_sliceId),
                   MakeEnumChecker (SliceId::HTC, "htc",
                                    SliceId::MTC, "mtc"))
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
                   MakePointerAccessor (&SliceNetwork::m_radio),
                   MakePointerChecker<RadioNetwork> ())

    // UEs.
    .AddAttribute ("NumUes", "The total number of UEs for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceNetwork::m_nUes),
                   MakeUintegerChecker<uint32_t> (0, 4095))
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
                   MakeUintegerAccessor (&SliceNetwork::m_pgwInfraSwIdx),
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

void
SliceNetwork::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_backhaul = 0;
  m_radio = 0;
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

  NS_ABORT_MSG_IF (m_sliceId == SliceId::NONE, "Undefined slice ID.");
  NS_ABORT_MSG_IF (!m_controllerApp, "No slice controller application.");
  NS_ABORT_MSG_IF (!m_backhaul, "No backhaul network.");
  NS_ABORT_MSG_IF (!m_radio, "No LTE RAN network.");

  EnumValue enumValue;
  m_controllerApp->GetAttribute ("SliceId", enumValue);
  NS_ABORT_MSG_IF (enumValue.Get () != m_sliceId, "Incompatible slice IDs");

  m_sliceIdStr = SliceIdStr (m_sliceId);
  NS_LOG_INFO ("Creating LTE logical network " << m_sliceIdStr <<
               " slice with " << m_nUes << " UEs.");

  // Configure IP address helpers.
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_webAddrHelper.SetBase (m_webAddr, m_webMask);

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Configure and install the slice controller application.
  m_controllerApp->SetNetworkAttributes (
    m_nTftNodes, m_ueAddr, m_ueMask, m_webAddr, m_webMask);
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

uint32_t
SliceNetwork::GetPgwTftNumNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nTftNodes;
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
  NS_LOG_INFO ("P-GW with main switch + " << m_nTftNodes << " TFT switches.");

  // Set the default P-GW gateway logical address, which will be used to set
  // the static route at all UEs.
  m_pgwAddress = m_ueAddrHelper.NewAddress ();
  NS_LOG_INFO ("P-GW default IP address: " << m_pgwAddress);

  // Get the P-GW main node and device.
  Ptr<Node> pgwMainNode = m_pgwNodes.Get (0);
  Ptr<OFSwitch13Device> pgwMainOfDev = m_pgwDevices.Get (0);

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
  NetDeviceContainer devices = m_csmaHelper.Install (pgwMainNode, m_webNode);
  Ptr<CsmaNetDevice> pgwSgiDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  Ptr<CsmaNetDevice> webSgiDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
  m_webDevices.Add (devices);

  // Set device names for pcap files.
  std::string ifaceName = "~" + LteInterfaceStr (LteInterface::SGI) + "~";
  BackhaulNetwork::SetDeviceNames (pgwSgiDev, webSgiDev, ifaceName);

  // Add the pgwSgiDev as physical port on the P-GW main OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = pgwMainOfDev->AddSwitchPort (pgwSgiDev);
  uint32_t pgwSgiPortNo = pgwSgiPort->GetPortNo ();

  // Set the IP address on the Internet network.
  m_webAddrHelper.Assign (m_webDevices);
  NS_LOG_INFO ("Web node " << m_webNode << " attached to the sgi interface " <<
               "with IP " << Ipv4AddressHelper::GetAddress (webSgiDev));
  NS_LOG_INFO ("P-GW " << pgwMainNode << " attached to the sgi interface " <<
               "with IP " << Ipv4AddressHelper::GetAddress (pgwSgiDev));

  // Define static routes at the web server to the LTE network.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    m_ueAddr, m_ueMask, Ipv4AddressHelper::GetAddress (pgwSgiDev), 1);

  // Connect the P-GW node to the OpenFlow backhaul network.
  Ptr<CsmaNetDevice> pgwS5Dev;
  Ptr<OFSwitch13Port> infraSwPort;
  std::tie (pgwS5Dev, infraSwPort) = m_backhaul->AttachEpcNode (
      pgwMainNode, m_pgwInfraSwIdx, LteInterface::S5);
  NS_LOG_INFO ("P-GW main switch " << pgwMainNode <<
               " attached to the s5 interface with IP " <<
               Ipv4AddressHelper::GetAddress (pgwS5Dev));

  // Create the logical port on the P-GW S5 interface.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = pgwMainOfDev->AddSwitchPort (pgwS5PortDev);
  uint32_t pgwS5PortNo = pgwS5Port->GetPortNo ();

  // Create the P-GW S5 user-plane application.
  pgwMainNode->AddApplication (
    CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev));

  // Notify the controller of the new P-GW main switch.
  m_controllerApp->NotifyPgwMainAttach (
    pgwMainOfDev, pgwS5Dev, pgwS5PortNo, pgwSgiDev, pgwSgiPortNo, webSgiDev);

  // Configure CSMA helper for connecting P-GW internal node.
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_pgwLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_pgwLinkDelay));

  // Connect all P-GW TFT switches to the P-GW main switch and to the S5
  // interface. Only downlink traffic will be sent to these switches.
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

      Ptr<CsmaNetDevice> tftDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
      Ptr<CsmaNetDevice> manDev = DynamicCast<CsmaNetDevice> (devices.Get (1));

      // Add the mainDev as physical port on the P-GW main OpenFlow switch.
      Ptr<OFSwitch13Port> mainPort = pgwMainOfDev->AddSwitchPort (manDev);
      uint32_t mainPortNo = mainPort->GetPortNo ();

      // Add the tftDev as physical port on the P-GW TFT OpenFlow switch.
      Ptr<OFSwitch13Port> tftPort = pgwTftOfDev->AddSwitchPort (tftDev);
      uint32_t tftPortNo = tftPort->GetPortNo ();
      NS_UNUSED (tftPortNo);

      // Connect the P-GW TFT node to the OpenFlow backhaul node.
      std::tie (pgwS5Dev, infraSwPort) = m_backhaul->AttachEpcNode (
          pgwTftNode, m_pgwInfraSwIdx, LteInterface::S5);
      NS_LOG_INFO ("P-GW TFT switch " << pgwTftNode <<
                   " attached to the s5 interface with IP " <<
                   Ipv4AddressHelper::GetAddress (pgwS5Dev));

      // Create the logical port on the P-GW S5 interface.
      pgwS5PortDev = CreateObject<VirtualNetDevice> ();
      pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
      pgwS5Port = pgwTftOfDev->AddSwitchPort (pgwS5PortDev);
      pgwS5PortNo = pgwS5Port->GetPortNo ();

      // Create the P-GW S5 user-plane application.
      pgwTftNode->AddApplication (
        CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev));

      // Notify the controller of the new P-GW TFT switch.
      m_controllerApp->NotifyPgwTftAttach (
        pgwTftOfDev, pgwS5Dev, pgwS5PortNo, mainPortNo, tftIdx);
    }
  m_controllerApp->NotifyPgwBuilt (m_pgwDevices);
}

void
SliceNetwork::CreateSgws (void)
{
  NS_LOG_FUNCTION (this);

  // Create a S-GW switch for each OpenFlow backhaul switch.
  uint32_t nSgws = m_backhaul->GetNSwitches ();

  // Create the S-GW nodes and configure them as OpenFlow switches.
  m_sgwNodes.Create (nSgws);
  m_sgwDevices = m_switchHelper->InstallSwitch (m_sgwNodes);
  for (uint16_t i = 0; i < nSgws; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr << "_sgw" << i + 1;
      Names::Add (name.str (), m_sgwNodes.Get (i));
    }

  // Connect all S-GW switches to the S1-U and S5 interfaces.
  for (uint16_t sgwIdx = 0; sgwIdx < nSgws; sgwIdx++)
    {
      Ptr<Node> sgwNode = m_sgwNodes.Get (sgwIdx);
      Ptr<OFSwitch13Device> sgwOfDev = m_sgwDevices.Get (sgwIdx);
      uint64_t sgwDpId = sgwOfDev->GetDatapathId ();

      // Connect the S-GW node to the OpenFlow backhaul node.
      Ptr<CsmaNetDevice> sgwS1uDev;
      Ptr<OFSwitch13Port> infraSwS1uPort;
      std::tie (sgwS1uDev, infraSwS1uPort) = m_backhaul->AttachEpcNode (
          sgwNode, sgwIdx, LteInterface::S1U);
      NS_LOG_INFO ("S-GW " << sgwNode << " attached to the s1u interface " <<
                   "with IP " << Ipv4AddressHelper::GetAddress (sgwS1uDev));

      Ptr<CsmaNetDevice> sgwS5Dev;
      Ptr<OFSwitch13Port> infraSwS5Port;
      std::tie (sgwS5Dev, infraSwS5Port) = m_backhaul->AttachEpcNode (
          sgwNode, sgwIdx, LteInterface::S5);
      NS_LOG_INFO ("S-GW " << sgwNode << " attached to the s5 interface " <<
                   "with IP " << Ipv4AddressHelper::GetAddress (sgwS5Dev));

      // Create the logical ports on the S-GW S1-U and S5 interfaces.
      Ptr<VirtualNetDevice> sgwS1uPortDev = CreateObject<VirtualNetDevice> ();
      sgwS1uPortDev->SetAddress (Mac48Address::Allocate ());
      Ptr<OFSwitch13Port> sgwS1uPort = sgwOfDev->AddSwitchPort (sgwS1uPortDev);
      uint32_t sgwS1uPortNo = sgwS1uPort->GetPortNo ();

      Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
      sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
      Ptr<OFSwitch13Port> sgwS5Port = sgwOfDev->AddSwitchPort (sgwS5PortDev);
      uint32_t sgwS5PortNo = sgwS5Port->GetPortNo ();

      // Create the S-GW S1-U and S5 user-plane application.
      sgwNode->AddApplication (
        CreateObject<GtpTunnelApp> (sgwS1uPortDev, sgwS1uDev));
      sgwNode->AddApplication (
        CreateObject<GtpTunnelApp> (sgwS5PortDev, sgwS5Dev));

      // Saving S-GW metadata.
      Ptr<SgwInfo> sgwInfo = CreateObject<SgwInfo> (sgwDpId);
      sgwInfo->SetSliceId (m_sliceId);
      sgwInfo->SetS1uAddr (Ipv4AddressHelper::GetAddress (sgwS1uDev));
      sgwInfo->SetS5Addr (Ipv4AddressHelper::GetAddress (sgwS5Dev));
      sgwInfo->SetS1uPortNo (sgwS1uPortNo);
      sgwInfo->SetS5PortNo (sgwS5PortNo);
      sgwInfo->SetInfraSwIdx (sgwIdx);
      sgwInfo->SetInfraSwS1uPortNo (infraSwS1uPort->GetPortNo ());
      sgwInfo->SetInfraSwS5PortNo (infraSwS5Port->GetPortNo ());

      // Notify the controller of the new S-GW switch.
      m_controllerApp->NotifySgwAttach (
        sgwOfDev, sgwS1uDev, sgwS1uPortNo, sgwS5Dev, sgwS5PortNo);
    }
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
  Ptr<PositionAllocator> posAllocator = m_radio->GetRandomPositionAllocator ();
  MobilityHelper mobilityHelper;
  mobilityHelper.SetPositionAllocator (posAllocator);
  if (m_ueMobility)
    {
      // TODO Use attributes for custom mobility configuration.
      mobilityHelper.SetMobilityModel (
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=15.0]"),
        "Pause", StringValue ("ns3::ExponentialRandomVariable[Mean=25.0]"),
        "PositionAllocator", PointerValue (posAllocator));
    }

  // Install LTE protocol stack into UE nodes.
  m_ueDevices = m_radio->InstallUeDevices (m_ueNodes, mobilityHelper);

  // Install TCP/IP protocol stack into UE nodes and assign IP address.
  InternetStackHelper internet;
  internet.Install (m_ueNodes);
  Ipv4InterfaceContainer ueIfaces = m_ueAddrHelper.Assign (m_ueDevices);

  // Saving UE metadata.
  UintegerValue imsiValue;
  for (uint32_t i = 0; i < m_ueDevices.GetN (); i++)
    {
      m_ueDevices.Get (i)->GetAttribute ("Imsi", imsiValue);
      Ptr<UeInfo> ueInfo = CreateObject<UeInfo> (imsiValue.Get ());
      ueInfo->SetSliceId (m_sliceId);
      ueInfo->SetUeAddr (ueIfaces.GetAddress (i));
      ueInfo->SetS11SapSgw (m_controllerApp->GetS11SapSgw ());
      NS_LOG_DEBUG ("UE IMSI " << imsiValue.Get () <<
                    " configured with IP " << ueInfo->GetUeAddr ());
    }

  // Specify static routes for each UE to its default P-GW.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  NodeContainer::Iterator it;
  for (it = m_ueNodes.Begin (); it != m_ueNodes.End (); it++)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (m_pgwAddress, 1);
    }

  // Attach UE to the eNBs using initial cell selection.
  m_radio->AttachUeDevices (m_ueDevices);
}

} // namespace ns3
