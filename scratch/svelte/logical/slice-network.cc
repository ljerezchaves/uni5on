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
#include "../infrastructure/backhaul-network.h"
#include "../infrastructure/radio-network.h"
#include "../metadata/pgw-info.h"
#include "../metadata/sgw-info.h"
#include "../metadata/ue-info.h"
#include "gtp-tunnel-app.h"
#include "pgw-tunnel-app.h"
#include "slice-controller.h"
#include "slice-network.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[Slice " << m_sliceIdStr << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceNetwork");
NS_OBJECT_ENSURE_REGISTERED (SliceNetwork);

SliceNetwork::SliceNetwork ()
  : m_pgwInfo (0)
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
                   MakeEnumChecker (SliceId::MTC, "mtc",
                                    SliceId::HTC, "htc",
                                    SliceId::TMP, "tmp"))
    .AddAttribute ("SliceCtrl", "The slice controller application pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_controllerApp),
                   MakePointerChecker<SliceController> ())

    // Infrastructure.
    .AddAttribute ("BackhaulNet", "The OpenFlow backhaul network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_backhaul),
                   MakePointerChecker<BackhaulNetwork> ())
    .AddAttribute ("RadioNet", "The LTE RAN network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_radio),
                   MakePointerChecker<RadioNetwork> ())

    // UEs.
    .AddAttribute ("NumUes", "The total number of UEs for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
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
    .AddAttribute ("UeCellSiteCoverage", "Restrict UE positioning to a "
                   "specific cell site coverage. When left to 0, the entire "
                   "RAN coverage is used.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceNetwork::m_ueCellSiteCover),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("UeMobility", "Enable UE random mobility.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&SliceNetwork::m_ueMobility),
                   MakeBooleanChecker ())
    .AddAttribute ("UeMobilityPause", "A random variable used to pick the UE "
                   "pause time in the random waypoint mobility model.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=25.0]"),
                   MakePointerAccessor (&SliceNetwork::m_ueMobPause),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("UeMobilitySpeed", "A random variable used to pick the UE "
                   "speed in the random waypoint mobility model.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue (
                     "ns3::NormalRandomVariable[Mean=1.4|Variance=0.09]"),
                   MakePointerAccessor (&SliceNetwork::m_ueMobSpeed),
                   MakePointerChecker<RandomVariableStream> ())

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
                   UintegerValue (2),
                   MakeUintegerAccessor (&SliceNetwork::GetPgwTftNumNodes,
                                         &SliceNetwork::SetPgwTftNumNodes),
                   MakeUintegerChecker<uint16_t> (1, 32))
    .AddAttribute ("PgwBackhaulSwitch",
                   "The backhaul switch index to connect the P-GW.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceNetwork::m_pgwInfraSwIdx),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PgwMainFlowTableSize",
                   "Flow table size for the P-GW main switch.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_mainFlowSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("PgwMainPipelineCapacity",
                   "Pipeline capacity for the P-GW main switch.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_mainPipeCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwTftFlowTableSize",
                   "Flow table size for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_tftFlowSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("PgwTftMeterTableSize",
                   "Meter table size for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_tftMeterSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("PgwTftPipelineCapacity",
                   "Pipeline capacity for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_tftPipeCapacity),
                   MakeDataRateChecker ())
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

    // S-GW.
    .AddAttribute ("SgwBackhaulSwitches",
                   "The backhaul switch indexes to connect S-GWs.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("1:0"),
                   MakeStringAccessor (&SliceNetwork::m_sgwInfraSwIdxStr),
                   MakeStringChecker ())
    .AddAttribute ("SgwFlowTableSize",
                   "Flow table size for the S-GW switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_sgwFlowSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("SgwMeterTableSize",
                   "Meter table size for the S-GW switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&SliceNetwork::m_sgwMeterSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("SgwPipelineCapacity",
                   "Pipeline capacity for the S-GW switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gb/s")),
                   MakeDataRateAccessor (&SliceNetwork::m_sgwPipeCapacity),
                   MakeDataRateChecker ())

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

Ptr<Node>
SliceNetwork::GetWebNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_webNode;
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
  m_pgwInfo = 0;
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
  NS_ABORT_MSG_IF (m_controllerApp->GetSliceId () != m_sliceId,
                   "Incompatible slice IDs for controller and network.");

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
  m_controllerApp->SetNetworkAttributes (m_ueAddr, m_ueMask,
                                         m_webAddr, m_webMask);
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

  Object::NotifyConstructionCompleted ();
}

uint32_t
SliceNetwork::GetPgwTftNumNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nTfts;
}

void
SliceNetwork::SetPgwTftNumNodes (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  // Check the number of P-GW TFT nodes (must be a power of 2).
  NS_ABORT_MSG_IF ((value & (value - 1)) != 0, "Invalid number of P-GW TFTs.");

  m_nTfts = value;
}

void
SliceNetwork::CreatePgw (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!m_pgwInfo, "P-GW already configured.");
  uint16_t pgwId = 1; // A single P-GW in current implementation.

  // Create and name the P-GW nodes
  m_pgwNodes.Create (m_nTfts + 1);
  std::ostringstream mainName;
  mainName << m_sliceIdStr << "_pgw" << pgwId;
  Names::Add (mainName.str () + "_main", m_pgwNodes.Get (0));
  for (uint16_t tftIdx = 1; tftIdx <= m_nTfts; tftIdx++)
    {
      std::ostringstream name;
      name << mainName.str () << "_tft" << tftIdx;
      Names::Add (name.str (), m_pgwNodes.Get (tftIdx));
    }
  NS_LOG_INFO ("P-GW with main switch + " << m_nTfts << " TFT switches.");

  // Set the default P-GW gateway logical address, which will be used to set
  // the static route at all UEs.
  m_pgwAddress = m_ueAddrHelper.NewAddress ();
  NS_LOG_INFO ("P-GW default IP address: " << m_pgwAddress);

  // Configuring OpenFlow helper for P-GW main switch.
  // No meter/group entries and 7 pipeline table (1 + the maximum number of TFT
  // adaptive levels considering the maximum of 32 TFT switches).
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_mainFlowSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (0));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (0));
  m_switchHelper->SetDeviceAttribute (
    "PipelineCapacity", DataRateValue (m_mainPipeCapacity));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (7));

  // Configure the P-GW main node as an OpenFlow switch.
  Ptr<Node> pgwMainNode = m_pgwNodes.Get (0);
  m_pgwDevices = m_switchHelper->InstallSwitch (pgwMainNode);
  Ptr<OFSwitch13Device> pgwMainOfDev = m_pgwDevices.Get (0);
  uint64_t pgwDpId = pgwMainOfDev->GetDatapathId ();

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
  std::string tempStr = "~" + LteIfaceStr (LteIface::SGI) + "~";
  SetDeviceNames (pgwSgiDev, webSgiDev, tempStr);

  // Add the pgwSgiDev as physical port on the P-GW main OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = pgwMainOfDev->AddSwitchPort (pgwSgiDev);

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
  Ptr<OFSwitch13Port> infraSwS5Port;
  Ipv4Address pgwS5Addr;
  std::tie (pgwS5Dev, infraSwS5Port) = m_backhaul->AttachEpcNode (
      pgwMainNode, m_pgwInfraSwIdx, LteIface::S5);
  pgwS5Addr = Ipv4AddressHelper::GetAddress (pgwS5Dev);
  NS_LOG_INFO ("P-GW " << pgwId << " main switch dpId " << pgwDpId <<
               " attached to the s5 interface with IP " << pgwS5Addr);

  // Create the logical port on the P-GW S5 interface.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = pgwMainOfDev->AddSwitchPort (pgwS5PortDev);
  pgwMainNode->AddApplication (
    CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev));

  // Saving P-GW metadata.
  m_pgwInfo = CreateObject<PgwInfo> (
      pgwId, m_nTfts, pgwSgiPort->GetPortNo (),
      m_pgwInfraSwIdx, m_controllerApp);

  // Saving P-GW MAIN metadata first.
  m_pgwInfo->SaveSwitchInfo (pgwMainOfDev, pgwS5Addr, pgwS5Port->GetPortNo (),
                             infraSwS5Port->GetPortNo ());

  // Configure CSMA helper for connecting P-GW internal node.
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_pgwLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_pgwLinkDelay));

  // Configuring OpenFlow helper for P-GW TFT switches.
  // No group entries and 1 pipeline table.
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_tftFlowSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (0));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_tftMeterSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineCapacity", DataRateValue (m_tftPipeCapacity));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (1));

  // Connect all P-GW TFT switches to the P-GW main switch and to the S5
  // interface. Only downlink traffic will be sent to these switches.
  for (uint16_t tftIdx = 1; tftIdx <= m_nTfts; tftIdx++)
    {
      // Configure the P-GW TFT node as an OpenFlow switch.
      Ptr<Node> pgwTftNode = m_pgwNodes.Get (tftIdx);
      m_pgwDevices.Add (m_switchHelper->InstallSwitch (pgwTftNode));
      Ptr<OFSwitch13Device> pgwTftOfDev = m_pgwDevices.Get (tftIdx);
      pgwDpId = pgwTftOfDev->GetDatapathId ();

      // Connect the P-GW main node to the P-GW TFT node.
      devices = m_csmaHelper.Install (pgwTftNode, pgwMainNode);
      Ptr<CsmaNetDevice> tftDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
      Ptr<CsmaNetDevice> manDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
      m_pgwIntDevices.Add (devices);

      // Add the mainDev as physical port on the P-GW main OpenFlow switch.
      Ptr<OFSwitch13Port> mainPort = pgwMainOfDev->AddSwitchPort (manDev);

      // Add the tftDev as physical port on the P-GW TFT OpenFlow switch.
      Ptr<OFSwitch13Port> tftPort = pgwTftOfDev->AddSwitchPort (tftDev);

      // Connect the P-GW TFT node to the OpenFlow backhaul node.
      std::tie (pgwS5Dev, infraSwS5Port) = m_backhaul->AttachEpcNode (
          pgwTftNode, m_pgwInfraSwIdx, LteIface::S5);
      pgwS5Addr = Ipv4AddressHelper::GetAddress (pgwS5Dev);
      NS_LOG_INFO ("P-GW TFT " << tftIdx << " switch dpId " << pgwDpId <<
                   " attached to the s5 interface with IP " << pgwS5Addr);

      // Create the logical port on the P-GW S5 interface.
      pgwS5PortDev = CreateObject<VirtualNetDevice> ();
      pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
      pgwS5Port = pgwTftOfDev->AddSwitchPort (pgwS5PortDev);
      pgwTftNode->AddApplication (
        CreateObject<PgwTunnelApp> (pgwS5PortDev, pgwS5Dev));

      // Saving P-GW TFT metadata.
      m_pgwInfo->SaveSwitchInfo (
        pgwTftOfDev, pgwS5Addr, pgwS5Port->GetPortNo (),
        infraSwS5Port->GetPortNo (), mainPort->GetPortNo (),
        tftPort->GetPortNo ());
    }

  // Notify the controller of the new P-GW entity.
  m_controllerApp->NotifyPgwAttach (m_pgwInfo, webSgiDev);
}

void
SliceNetwork::CreateSgws (void)
{
  NS_LOG_FUNCTION (this);

  ParseSgwInfraSwIdxs ();
  uint32_t nSgws = m_sgwInfraSwIdx.size ();

  // Create and name the S-GW nodes.
  m_sgwNodes.Create (nSgws);
  for (uint16_t i = 0; i < nSgws; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr << "_sgw" << i + 1;
      Names::Add (name.str (), m_sgwNodes.Get (i));
    }

  // Configuring OpenFlow helper for S-GW switches.
  // No group entries and 3 pipeline tables.
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_sgwFlowSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (0));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_sgwMeterSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineCapacity", DataRateValue (m_sgwPipeCapacity));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (3));

  // Configure the S-GW nodes as OpenFlow switches.
  m_sgwDevices = m_switchHelper->InstallSwitch (m_sgwNodes);

  // Connect all S-GW switches to the S1-U and S5 interfaces.
  for (uint16_t sgwIdx = 0; sgwIdx < nSgws; sgwIdx++)
    {
      uint16_t sgwId = sgwIdx + 1;
      uint16_t infraSwIdx = m_sgwInfraSwIdx.at (sgwIdx);

      Ptr<Node> sgwNode = m_sgwNodes.Get (sgwIdx);
      Ptr<OFSwitch13Device> sgwOfDev = m_sgwDevices.Get (sgwIdx);
      uint64_t sgwDpId = sgwOfDev->GetDatapathId ();

      // Connect the S-GW node to the OpenFlow backhaul node.
      Ptr<CsmaNetDevice> sgwS1uDev;
      Ptr<OFSwitch13Port> infraSwS1uPort;
      std::tie (sgwS1uDev, infraSwS1uPort) = m_backhaul->AttachEpcNode (
          sgwNode, infraSwIdx, LteIface::S1U);
      NS_LOG_INFO ("S-GW " << sgwId << " switch dpId " << sgwDpId <<
                   " attached to the s1u interface with IP " <<
                   Ipv4AddressHelper::GetAddress (sgwS1uDev));

      Ptr<CsmaNetDevice> sgwS5Dev;
      Ptr<OFSwitch13Port> infraSwS5Port;
      std::tie (sgwS5Dev, infraSwS5Port) = m_backhaul->AttachEpcNode (
          sgwNode, infraSwIdx, LteIface::S5);
      NS_LOG_INFO ("S-GW " << sgwId << " switch dpId " << sgwDpId <<
                   " attached to the s5 interface with IP " <<
                   Ipv4AddressHelper::GetAddress (sgwS5Dev));

      // Create the logical ports on the S-GW S1-U and S5 interfaces.
      Ptr<VirtualNetDevice> sgwS1uPortDev = CreateObject<VirtualNetDevice> ();
      sgwS1uPortDev->SetAddress (Mac48Address::Allocate ());
      Ptr<OFSwitch13Port> sgwS1uPort = sgwOfDev->AddSwitchPort (sgwS1uPortDev);
      sgwNode->AddApplication (
        CreateObject<GtpTunnelApp> (sgwS1uPortDev, sgwS1uDev));

      Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
      sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
      Ptr<OFSwitch13Port> sgwS5Port = sgwOfDev->AddSwitchPort (sgwS5PortDev);
      sgwNode->AddApplication (
        CreateObject<GtpTunnelApp> (sgwS5PortDev, sgwS5Dev));

      // Saving S-GW metadata.
      Ptr<SgwInfo> sgwInfo = CreateObject<SgwInfo> (
          sgwId, sgwOfDev, Ipv4AddressHelper::GetAddress (sgwS1uDev),
          Ipv4AddressHelper::GetAddress (sgwS5Dev), sgwS1uPort->GetPortNo (),
          sgwS5Port->GetPortNo (), infraSwIdx, infraSwS1uPort->GetPortNo (),
          infraSwS5Port->GetPortNo (), m_controllerApp);

      // Notify the controller of the new S-GW switch.
      m_controllerApp->NotifySgwAttach (sgwInfo);
    }
}

void
SliceNetwork::CreateUes (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_pgwInfo, "P-GW not configured yet.");

  // Create the UE nodes and set their names.
  m_ueNodes.Create (m_nUes);
  for (uint32_t i = 0; i < m_nUes; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr + "_ue" << i + 1;
      Names::Add (name.str (), m_ueNodes.Get (i));
    }

  // Configure UE positioning and mobility, considering possible restrictions
  // on the LTE RAN coverage area.
  Ptr<PositionAllocator> posAllocator =
    m_radio->GetRandomPositionAllocator (m_ueCellSiteCover);
  MobilityHelper mobilityHelper;
  mobilityHelper.SetPositionAllocator (posAllocator);
  if (m_ueMobility)
    {
      mobilityHelper.SetMobilityModel (
        "ns3::RandomWaypointMobilityModel",
        "Pause", PointerValue (m_ueMobPause),
        "Speed", PointerValue (m_ueMobSpeed),
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
      Ptr<UeInfo> ueInfo = CreateObject<UeInfo> (
          imsiValue.Get (), ueIfaces.GetAddress (i), m_controllerApp);
      NS_LOG_DEBUG ("UE IMSI " << imsiValue.Get () <<
                    " configured with IP " << ueInfo->GetAddr ());
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

void
SliceNetwork::ParseSgwInfraSwIdxs (void)
{
  NS_LOG_FUNCTION (this);

  char ch;
  size_t max;
  uint16_t temp;

  m_sgwInfraSwIdx.clear ();
  std::istringstream is (m_sgwInfraSwIdxStr);
  is >> max >> ch;
  if (ch != ':')
    {
      is.setstate (std::ios_base::failbit);
    }

  size_t count = 0;
  while (!is.eof () && !is.fail ())
    {
      is >> temp;
      m_sgwInfraSwIdx.push_back (temp);

      count++;
      if (count < max)
        {
          is >> ch;
          if (ch != '+')
            {
              is.setstate (std::ios_base::failbit);
            }
        }
    }
  NS_ABORT_MSG_IF (is.fail () || m_sgwInfraSwIdx.size () != max,
                   "Failure to parse SgwBackhaulSwitches.");
}

} // namespace ns3
