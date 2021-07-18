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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <ns3/csma-module.h>
#include "gtpu-tunnel-app.h"
#include "pgwu-tunnel-app.h"
#include "slice-controller.h"
#include "slice-network.h"
#include "../infrastructure/radio-network.h"
#include "../infrastructure/switch-helper.h"
#include "../infrastructure/transport-network.h"
#include "../metadata/pgw-info.h"
#include "../metadata/sgw-info.h"
#include "../metadata/ue-info.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[Slice " << m_sliceIdStr << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceNetwork");
NS_OBJECT_ENSURE_REGISTERED (SliceNetwork);

SliceNetwork::SliceNetwork ()
  : m_pgwInfo (0),
  m_sgwInfo (0)
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
    .AddAttribute ("SliceId", "The logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceId::UNKN),
                   MakeEnumAccessor (&SliceNetwork::m_sliceId),
                   MakeEnumChecker (SliceId::MBB, SliceIdStr (SliceId::MBB),
                                    SliceId::MTC, SliceIdStr (SliceId::MTC),
                                    SliceId::TMP, SliceIdStr (SliceId::TMP)))
    .AddAttribute ("SliceCtrl", "The logical slice controller pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_controllerApp),
                   MakePointerChecker<SliceController> ())

    // Infrastructure.
    .AddAttribute ("TransportNet", "The OpenFlow transport network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceNetwork::m_transport),
                   MakePointerChecker<TransportNetwork> ())
    .AddAttribute ("RadioNet", "The RAN network pointer.",
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
                   DataRateValue (DataRate ("10Gbps")),
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
                   MakeUintegerChecker<uint16_t> (1, 32))
    .AddAttribute ("PgwInfraSwitch",
                   "The transport switch index to connect the P-GW.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceNetwork::m_pgwInfraSwIdx),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PgwUlDlCpuCapacity",
                   "CPU capacity for the P-GW UL/DL switch.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("2Gbps")),
                   MakeDataRateAccessor (&SliceNetwork::m_ulDlCpuCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwUlDlTableSize",
                   "Flow table size for the P-GW UL/DL switch.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (8192),
                   MakeUintegerAccessor (&SliceNetwork::m_ulDlTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("PgwUlDlTcamDelay",
                   "Average time for a TCAM operation in P-GW UL/DL switches.",
                   TimeValue (MicroSeconds (20)),
                   MakeTimeAccessor (&SliceNetwork::m_ulDlTcamDelay),
                   MakeTimeChecker (Time (0)))
    .AddAttribute ("PgwTftCpuCapacity",
                   "CPU capacity for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("2Gbps")),
                   MakeDataRateAccessor (&SliceNetwork::m_tftCpuCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwTftTableSize",
                   "Flow table size for the P-GW TFT switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (8192),
                   MakeUintegerAccessor (&SliceNetwork::m_tftTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("PgwTftTcamDelay",
                   "Average time for a TCAM operation in P-GW TFT switches.",
                   TimeValue (MicroSeconds (20)),
                   MakeTimeAccessor (&SliceNetwork::m_tftTcamDelay),
                   MakeTimeChecker (Time (0)))
    .AddAttribute ("PgwLinkDataRate",
                   "The data rate for the internal P-GW links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("1Gbps")),
                   MakeDataRateAccessor (&SliceNetwork::m_pgwLinkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PgwLinkDelay",
                   "The delay for the internal P-GW links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (50)),
                   MakeTimeAccessor (&SliceNetwork::m_pgwLinkDelay),
                   MakeTimeChecker ())

    // S-GW.
    .AddAttribute ("SgwInfraSwitch",
                   "The transport switch index to connect the S-GW.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceNetwork::m_sgwInfraSwIdx),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("SgwCpuCapacity",
                   "Pipeline capacity for the S-GW switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("2Gbps")),
                   MakeDataRateAccessor (&SliceNetwork::m_sgwCpuCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("SgwTableSize",
                   "Flow table size for the S-GW switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (8192),
                   MakeUintegerAccessor (&SliceNetwork::m_sgwTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))

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
SliceNetwork::EnablePcap (std::string prefix, bool promiscuous, bool ofchannel,
                          bool pgwDevices, bool sgiDevices)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous << ofchannel <<
                   pgwDevices << sgiDevices);

  if (ofchannel)
    {
      m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);
    }

  CsmaHelper helper;
  if (sgiDevices)
    {
      helper.EnablePcap (prefix + "sgi", m_webDevices, promiscuous);
    }
  if (pgwDevices)
    {
      helper.EnablePcap (prefix + "pgw", m_pgwIntDevices, promiscuous);
    }
}

std::vector<uint64_t>
SliceNetwork::GetUeImsiList (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ueImsiList;
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

  m_transport = 0;
  m_radio = 0;
  m_switchHelper = 0;
  m_controllerApp = 0;
  m_controllerNode = 0;
  m_webNode = 0;
  m_pgwInfo = 0;
  m_sgwInfo = 0;
  m_sgwNode = 0;
  m_sgwDevice = 0;
  Object::DoDispose ();
}

void
SliceNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_sliceId == SliceId::UNKN, "Unknown slice ID.");
  NS_ABORT_MSG_IF (!m_controllerApp, "No slice controller application.");
  NS_ABORT_MSG_IF (!m_transport, "No transport network.");
  NS_ABORT_MSG_IF (!m_radio, "No RAN network.");
  NS_ABORT_MSG_IF (m_controllerApp->GetSliceId () != m_sliceId,
                   "Incompatible slice IDs for controller and network.");

  m_sliceIdStr = SliceIdStr (m_sliceId);
  NS_LOG_INFO ("Creating logical network " << m_sliceIdStr <<
               " slice with " << m_nUes << " UEs.");

  // Configure IP address helpers.
  m_ueAddrHelper.SetBase (m_ueAddr, m_ueMask);
  m_webAddrHelper.SetBase (m_webAddr, m_webMask);

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<SwitchHelper> (
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

  // Create and configure the logical network.
  CreatePgw ();
  CreateSgw ();
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
  const uint16_t pgwId = 1; // A single P-GW in current implementation.

  // Create the P-GW metadata.
  m_pgwInfo = CreateObject<PgwInfo> (pgwId, m_nTfts);

  // Create and name the P-GW nodes.
  m_ulDlNodes.Create (2);
  m_tftNodes.Create (m_nTfts);
  std::ostringstream nodeName;
  nodeName << m_sliceIdStr << "_pgw" << pgwId;
  Names::Add (nodeName.str () + "_dl", m_ulDlNodes.Get (PGW_DL_IDX));
  Names::Add (nodeName.str () + "_ul", m_ulDlNodes.Get (PGW_UL_IDX));
  for (uint16_t tftIdx = 0; tftIdx < m_nTfts; tftIdx++)
    {
      std::ostringstream name;
      name << nodeName.str () << "_tft" << tftIdx;
      Names::Add (name.str (), m_tftNodes.Get (tftIdx));
    }
  NS_LOG_INFO ("P-GW with " << m_nTfts << " TFT switches.");

  // Set the default P-GW gateway logical address, which will be used to set
  // the static route at all UEs.
  m_pgwAddress = m_ueAddrHelper.NewAddress ();
  NS_LOG_INFO ("P-GW default IP address: " << m_pgwAddress);

  // Configure CSMA helper for connecting the P-GW node to the web server node.
  m_csmaHelper.SetDeviceAttribute  ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_webLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_webLinkDelay));

  // Configuring OpenFlow helper for P-GW UL/DL switches.
  // 7 pipeline table (1 + the maximum number of TFT adaptive levels,
  // considering the maximum of 32 TFT switches).
  m_switchHelper->SetDeviceAttribute (
    "CpuCapacity", DataRateValue (m_ulDlCpuCapacity));
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_ulDlTableSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (m_ulDlTableSize));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_ulDlTableSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (7));
  m_switchHelper->SetDeviceAttribute (
    "TcamDelay", TimeValue (m_ulDlTcamDelay));

  // Configure the P-GW UL/DL nodes as OpenFlow switches.
  m_ulDlDevices = m_switchHelper->InstallSwitch (m_ulDlNodes);

  // Connect the P-GW DL switch to the SGi interfaces. On the uplink direction,
  // the traffic will flow directly to the SGi interface thought this switch. On
  // the downlink direction, this switch will send the traffic to the TFT
  // switches.
  Ptr<Node> pgwDlNode = m_ulDlNodes.Get (PGW_DL_IDX);
  Ptr<OFSwitch13Device> pgwDlOfDev = m_ulDlDevices.Get (PGW_DL_IDX);

  // Connect the P-GW DL node to the web server node (SGi interface).
  NetDeviceContainer devices = m_csmaHelper.Install (pgwDlNode, m_webNode);
  Ptr<CsmaNetDevice> pgwSgiDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  Ptr<CsmaNetDevice> webSgiDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
  m_webDevices.Add (devices);

  // Set device names for pcap files.
  std::string tempStr = "~" + EpsIfaceStr (EpsIface::SGI) + "~";
  SetDeviceNames (pgwSgiDev, webSgiDev, tempStr);

  // Add the pgwSgiDev as physical port on the P-GW main OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = pgwDlOfDev->AddSwitchPort (pgwSgiDev);

  // Set the IP address on the Internet network.
  m_webAddrHelper.Assign (m_webDevices);
  Ipv4Address pgwSgiAddr = Ipv4AddressHelper::GetAddress (pgwSgiDev);
  NS_LOG_INFO ("Web node " << m_webNode << " attached to the sgi interface " <<
               "with IP " << Ipv4AddressHelper::GetAddress (webSgiDev));
  NS_LOG_INFO ("P-GW " << pgwId << " attached to the sgi interface " <<
               "with IP " << pgwSgiAddr);

  // Define static routes at the web server to the logical network.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (m_ueAddr, m_ueMask, pgwSgiAddr, 1);

  // Connect the P-GW UL switch to the S5 interfaces. On the downlink direction,
  // the traffic will flow directly to the S5 interface thought this switch. On
  // the uplink direction, this switch will send the traffic to the TFT
  Ptr<Node> pgwUlNode = m_ulDlNodes.Get (PGW_UL_IDX);
  Ptr<OFSwitch13Device> pgwUlOfDev = m_ulDlDevices.Get (PGW_UL_IDX);

  Ptr<CsmaNetDevice> pgwS5Dev;
  Ptr<OFSwitch13Port> infraSwS5Port;
  std::tie (pgwS5Dev, infraSwS5Port) = m_transport->AttachEpcNode (
      pgwUlNode, m_pgwInfraSwIdx, EpsIface::S5);
  Ipv4Address pgwS5Addr = Ipv4AddressHelper::GetAddress (pgwS5Dev);
  NS_LOG_INFO ("P-GW " << pgwId << " attached to the s5 interface " <<
               "with IP " << pgwS5Addr);

  // Create the logical port on the P-GW S5 interface.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = pgwUlOfDev->AddSwitchPort (pgwS5PortDev);
  pgwUlNode->AddApplication (
    CreateObject<PgwuTunnelApp> (pgwS5PortDev, pgwS5Dev));

  // Saving P-GW DL/UL metadata.
  m_pgwInfo->SaveUlDlInfo (pgwDlOfDev, pgwUlOfDev, pgwSgiPort->GetPortNo (),
                           pgwSgiAddr, pgwS5Port->GetPortNo (), pgwS5Addr,
                           m_pgwInfraSwIdx, infraSwS5Port->GetPortNo ());

  // Reconfigure CSMA helper for internal P-GW connections.
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_pgwLinkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_pgwLinkDelay));

  // Configuring OpenFlow helper for P-GW TFT switches.
  m_switchHelper->SetDeviceAttribute (
    "CpuCapacity", DataRateValue (m_tftCpuCapacity));
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_tftTableSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (m_tftTableSize));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_tftTableSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (1));
  m_switchHelper->SetDeviceAttribute (
    "TcamDelay", TimeValue (m_tftTcamDelay));

  // Configure the P-GW TFT nodes as OpenFlow switches.
  m_tftDevices = m_switchHelper->InstallSwitch (m_tftNodes);

  // Connect all P-GW TFT switches to the P-GW DL and UL switches
  for (uint16_t tftIdx = 0; tftIdx < m_nTfts; tftIdx++)
    {
      Ptr<Node> pgwTftNode = m_tftNodes.Get (tftIdx);
      Ptr<OFSwitch13Device> pgwTftOfDev = m_tftDevices.Get (tftIdx);

      // Connect the P-GW TFT node to the DL node.
      devices = m_csmaHelper.Install (pgwTftNode, pgwDlNode);
      Ptr<CsmaNetDevice> tftDlDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
      Ptr<CsmaNetDevice> dlTftDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
      Ptr<OFSwitch13Port> tftDlPort = pgwTftOfDev->AddSwitchPort (tftDlDev);
      Ptr<OFSwitch13Port> dlTftPort = pgwDlOfDev->AddSwitchPort (dlTftDev);
      m_pgwIntDevices.Add (devices);

      // Connect the P-GW TFT node to the UL node.
      devices = m_csmaHelper.Install (pgwTftNode, pgwUlNode);
      Ptr<CsmaNetDevice> tftUlDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
      Ptr<CsmaNetDevice> ulTftDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
      Ptr<OFSwitch13Port> tftUlPort = pgwTftOfDev->AddSwitchPort (tftUlDev);
      Ptr<OFSwitch13Port> ulTftPort = pgwUlOfDev->AddSwitchPort (ulTftDev);
      m_pgwIntDevices.Add (devices);

      // Saving P-GW TFT metadata.
      m_pgwInfo->SaveTftInfo (
        pgwTftOfDev, tftDlPort->GetPortNo (), tftUlPort->GetPortNo (),
        dlTftPort->GetPortNo (), ulTftPort->GetPortNo ());
    }

  // Notify the controller of the new P-GW entity.
  m_controllerApp->NotifyPgwAttach (m_pgwInfo, webSgiDev);
}

void
SliceNetwork::CreateSgw (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!m_sgwInfo, "S-GW already configured.");
  const uint16_t sgwId = 1; // A single S-GW in current implementation.

  // Create and name the S-GW node.
  m_sgwNode = CreateObject<Node> ();
  std::ostringstream name;
  name << m_sliceIdStr << "_sgw" << sgwId;
  Names::Add (name.str (), m_sgwNode);

  // Configuring OpenFlow helper for S-GW switches.
  // No group entries and 3 pipeline tables.
  m_switchHelper->SetDeviceAttribute (
    "CpuCapacity", DataRateValue (m_sgwCpuCapacity));
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_sgwTableSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (m_sgwTableSize));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_sgwTableSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (3));

  // Configure the S-GW node as an OpenFlow switch.
  m_sgwDevice = m_switchHelper->InstallSwitch (m_sgwNode);
  uint64_t sgwDpId = m_sgwDevice->GetDatapathId ();

  // Connect the S-GW node to the OpenFlow transport network.
  Ptr<CsmaNetDevice> sgwS1Dev;
  Ptr<OFSwitch13Port> infraSwS1Port;
  std::tie (sgwS1Dev, infraSwS1Port) = m_transport->AttachEpcNode (
      m_sgwNode, m_sgwInfraSwIdx, EpsIface::S1);
  NS_LOG_INFO ("S-GW " << sgwId << " switch dpId " << sgwDpId <<
               " attached to the s1u interface with IP " <<
               Ipv4AddressHelper::GetAddress (sgwS1Dev));

  Ptr<CsmaNetDevice> sgwS5Dev;
  Ptr<OFSwitch13Port> infraSwS5Port;
  std::tie (sgwS5Dev, infraSwS5Port) = m_transport->AttachEpcNode (
      m_sgwNode, m_sgwInfraSwIdx, EpsIface::S5);
  NS_LOG_INFO ("S-GW " << sgwId << " switch dpId " << sgwDpId <<
               " attached to the s5 interface with IP " <<
               Ipv4AddressHelper::GetAddress (sgwS5Dev));

  // Create the logical ports on the S-GW S1-U and S5 interfaces.
  Ptr<VirtualNetDevice> sgwS1PortDev = CreateObject<VirtualNetDevice> ();
  sgwS1PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> sgwS1Port = m_sgwDevice->AddSwitchPort (sgwS1PortDev);
  m_sgwNode->AddApplication (
    CreateObject<GtpuTunnelApp> (sgwS1PortDev, sgwS1Dev));

  Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
  sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> sgwS5Port = m_sgwDevice->AddSwitchPort (sgwS5PortDev);
  m_sgwNode->AddApplication (
    CreateObject<GtpuTunnelApp> (sgwS5PortDev, sgwS5Dev));

  // Saving S-GW metadata.
  m_sgwInfo = CreateObject<SgwInfo> (
      sgwId, m_sgwDevice, Ipv4AddressHelper::GetAddress (sgwS1Dev),
      Ipv4AddressHelper::GetAddress (sgwS5Dev), sgwS1Port->GetPortNo (),
      sgwS5Port->GetPortNo (), m_sgwInfraSwIdx, infraSwS1Port->GetPortNo (),
      infraSwS5Port->GetPortNo ());

  // Notify the controller of the new S-GW entity.
  m_controllerApp->NotifySgwAttach (m_sgwInfo);
}

void
SliceNetwork::CreateUes (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_pgwInfo, "P-GW not configured yet.");
  NS_ASSERT_MSG (m_sgwInfo, "S-GW not configured yet.");

  // Create the UE nodes and set their names.
  NodeContainer ueNodes;
  ueNodes.Create (m_nUes);
  for (uint32_t i = 0; i < m_nUes; i++)
    {
      std::ostringstream name;
      name << m_sliceIdStr + "_ue" << i + 1;
      Names::Add (name.str (), ueNodes.Get (i));
    }

  // Configure UE positioning and mobility, considering possible restrictions
  // on the RAN coverage area.
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
  NetDeviceContainer ueDevices;
  ueDevices = m_radio->InstallUeDevices (ueNodes, mobilityHelper);

  // Install TCP/IP protocol stack into UE nodes and assign IP address.
  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIfaces = m_ueAddrHelper.Assign (ueDevices);

  // Saving UE metadata.
  UintegerValue imsiValue;
  for (uint32_t i = 0; i < ueDevices.GetN (); i++)
    {
      ueDevices.Get (i)->GetAttribute ("Imsi", imsiValue);
      m_ueImsiList.push_back (imsiValue.Get ());
      Ptr<UeInfo> ueInfo = CreateObject<UeInfo> (
          imsiValue.Get (), ueIfaces.GetAddress (i), ueIfaces.GetMask (i),
          ueNodes.Get (i), ueDevices.Get (i), m_controllerApp);
      NS_LOG_DEBUG ("UE IMSI " << imsiValue.Get () <<
                    " configured with IP " << ueInfo->GetAddr ());
    }

  // Specify static routes for each UE to its default P-GW.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  NodeContainer::Iterator it;
  for (it = ueNodes.Begin (); it != ueNodes.End (); it++)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (m_pgwAddress, 1);
    }

  // Attach UE to the eNBs using initial cell selection.
  m_radio->AttachUeDevices (ueDevices);
}

} // namespace ns3
