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
#include "epc-network.h"
#include "epc-controller.h"
#include "pgw-user-app.h"
#include "sdran-cloud.h"
#include "stats/backhaul-stats-calculator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcNetwork");
NS_OBJECT_ENSURE_REGISTERED (EpcNetwork);

const uint16_t EpcNetwork::m_gtpuPort = 2152;

EpcNetwork::EpcNetwork ()
  : m_epcCtrlApp (0),
    m_epcCtrlNode (0),
    m_ofSwitchHelper (0),
    m_pgwNode (0),
    m_pgwSwitchDev (0),
    m_webNode (0),
    m_epcStats (0)
{
  NS_LOG_FUNCTION (this);

  // Let's use point to point connections for OpenFlow channel.
  m_ofSwitchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Creating network nodes.
  m_webNode = CreateObject<Node> ();
  Names::Add ("web", m_webNode);

  m_pgwNode = CreateObject<Node> ();
  Names::Add ("pgw", m_pgwNode);

  m_epcCtrlNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_epcCtrlNode);

  // Create the commom LTE MME element.
  m_mme = CreateObject<EpcMme> ();

  // Creating stats calculators.
  m_epcStats = CreateObject<BackhaulStatsCalculator> ();
}

EpcNetwork::~EpcNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcNetwork")
    .SetParent<EpcHelper> ()

    // Attributes for connecting the EPC entities to the backhaul network.
    .AddAttribute ("EpcLinkDataRate",
                   "The data rate for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&EpcNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("EpcLinkDelay",
                   "The delay for the link connecting a gateway to the "
                   "OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&EpcNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA OpenFlow links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&EpcNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())

    // IP network addresses.
    .AddAttribute ("UeNetworkAddr",
                   "The IPv4 network address used for UE devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("7.0.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_ueNetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("WebNetworkAddr",
                   "The IPv4 network address used for web devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("8.0.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_webNetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("S5NetworkAddr",
                   "The IPv4 network address used for S5 devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("10.1.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_s5NetworkAddr),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("X2NetworkAddr",
                   "The IPv4 network address used for X2 devices.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (Ipv4Address ("10.3.0.0")),
                   MakeIpv4AddressAccessor (&EpcNetwork::m_x2NetworkAddr),
                   MakeIpv4AddressChecker ())

    // Trace sources used by stats calculators to be aware of backhaul network.
    .AddTraceSource ("NewSwitchConnection",
                     "New connection between two OpenFlow switches during "
                     "backhaul topology creation.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_newConnTrace),
                     "ns3::ConnectionInfo::ConnTracedCallback")
    .AddTraceSource ("TopologyBuilt",
                     "OpenFlow backhaul network topology is built and no more "
                     "connections between OpenFlow switches will be created.",
                     MakeTraceSourceAccessor (&EpcNetwork::m_topoBuiltTrace),
                     "ns3::EpcNetwork::TopologyTracedCallback")
  ;
  return tid;
}

uint16_t
EpcNetwork::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ofSwitches.GetN ();
}

Ptr<Node>
EpcNetwork::GetWebNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_webNode;
}

Ipv4Address
EpcNetwork::GetWebIpAddress (void) const
{
  NS_LOG_FUNCTION (this);

  return m_webSgiIpAddr;
}

Ptr<Node>
EpcNetwork::GetControllerNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_epcCtrlNode;
}

Ptr<EpcController>
EpcNetwork::GetControllerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_epcCtrlApp;
}

Ptr<Node>
EpcNetwork::GetSwitchNode (uint64_t dpId) const
{
  NS_LOG_FUNCTION (this << dpId);

  Ptr<Node> node = OFSwitch13Device::GetDevice (dpId)->GetObject<Node> ();
  NS_ASSERT_MSG (node, "No node found for this datapath ID");

  return node;
}

Ptr<SdranCloud>
EpcNetwork::GetSdranCloud (Ptr<Node> enb)
{
  NS_LOG_FUNCTION (this << enb);

  NodeSdranMap_t::iterator ret;
  ret = m_enbSdranMap.find (enb);
  if (ret != m_enbSdranMap.end ())
    {
      NS_LOG_DEBUG ("Found SDRAN " << ret->second << " for eNB " << enb);
      return ret->second;
    }
  NS_FATAL_ERROR ("eNB node not registered.");
}

void
EpcNetwork::SetSwitchDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);

  m_ofSwitchHelper->SetDeviceAttribute (n1, v1);
}

void
EpcNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel
  m_ofSwitchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices
  CsmaHelper helper;
  helper.EnablePcap (prefix + "web-sgi",    m_sgiDevices, promiscuous);
  helper.EnablePcap (prefix + "lte-epc-s5", m_s5Devices,  promiscuous);
  helper.EnablePcap (prefix + "lte-epc-x2", m_x2Devices,  promiscuous);
  helper.EnablePcap (prefix + "ofnetwork",  m_ofSwitches, promiscuous);
}

void
EpcNetwork::AddSdranCloud (Ptr<SdranCloud> sdranCloud)
{
  NS_LOG_FUNCTION (this << sdranCloud);

  // Save eNB nodes into SDRAN cloud map.
  NodeContainer enbs = sdranCloud->GetEnbNodes ();
  for (NodeContainer::Iterator it = enbs.Begin (); it != enbs.End (); ++it)
    {
      std::pair<NodeSdranMap_t::iterator, bool> ret;
      std::pair<Ptr<Node>, Ptr<SdranCloud> > entry (*it, sdranCloud);
      ret = m_enbSdranMap.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Can't register eNB at SDRAN cloud.");
        }
      NS_LOG_DEBUG ("eNB node " << *it << " registered SDRAN " << sdranCloud);
    }

  // Set the common LTE MME element.
  sdranCloud->SetMme (GetMme ());

  // Configure the S-GW node from the SDRAN cloud as an OpenFlow switch.
  Ptr<Node> sgwNode = sdranCloud->GetSgwNode ();
  Ptr<OFSwitch13Device> sgwSwitchDev =
    (m_ofSwitchHelper->InstallSwitch (sgwNode)).Get (0);

  // Get the switch datapath ID on the backhaul network to attatch the S-GW.
  uint64_t swDpId = TopologyGetSgwSwitch (sdranCloud);
  Ptr<Node> swNode = GetSwitchNode (swDpId);

  // Connect the S-GW to the backhaul network over S5 interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, sgwNode);
  m_s5Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS5Dev, sgwS5Dev;
  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  sgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "+" +
              Names::FindName (sgwNode), swS5Dev);
  Names::Add (Names::FindName (sgwNode) + "+" +
              Names::FindName (swNode), sgwS5Dev);

  // Add the swS5Dev device as OpenFlow switch port.
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
  uint32_t swS5PortNum = swS5Port->GetPortNo ();

  // Configure the sgwS5Dev as standard device on S-GW node (with IP address).
  Ipv4InterfaceContainer sgwS5IfContainer;
  sgwS5IfContainer = m_s5AddrHelper.Assign (NetDeviceContainer (sgwS5Dev));
  Ipv4Address sgwS5Addr = sgwS5IfContainer.GetAddress (0);
  NS_LOG_DEBUG ("S-GW S5 interface address: " << sgwS5Addr);

  // Create the virtual net device to work as the logical ports on the S-GW S5
  // interface. This logical ports will connect to the S-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the sgwS5Dev.
  Ptr<VirtualNetDevice> sgwS5PortDev = CreateObject<VirtualNetDevice> ();
  sgwS5PortDev->SetAttribute ("Mtu", UintegerValue (3000));
  sgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> sgwS5Port = sgwSwitchDev->AddSwitchPort (sgwS5PortDev);
  // uint32_t sgwS5PortNum = sgwS5Port->GetPortNo (); // FIXME

  // Create the P-GW S5 user-plane application.
  Ptr<PgwUserApp> sgwUserApp = CreateObject <PgwUserApp> (sgwS5PortDev);
  sgwNode->AddApplication (sgwUserApp);

  // Notify the controller of the new S-GW device attached to the OpenFlow
  // backhaul network.
  m_epcCtrlApp->NewS5Attach (swDev, swS5PortNum, sgwS5Dev, sgwS5Addr);
  // m_epcCtrlApp->NewSgwAttach (sgwSwitchDev, sgwS5PortNum); // FIXME
}

void
EpcNetwork::SdranCloudsDone ()
{
  NS_LOG_FUNCTION (this);

  // The OpenFlow backhaul network topology is done and all P/S-GW gateways are
  // already connected to the S5 interface. Let's connect the OpenFlow switches
  // to the EPC controller. From this point on it is not possible to change the
  // OpenFlow network configuration.
  m_ofSwitchHelper->CreateOpenFlowChannels ();

  // Enable OpenFlow switch statistics.
  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  m_ofSwitchHelper->EnableDatapathStats (prefix + "ofswitch-stats", true);
}

//
// Methods from EpcHelper are implemented at the end of this file.
//

void
EpcNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_ofSwitchHelper = 0;
  m_epcCtrlNode = 0;
  m_epcCtrlApp = 0;
  m_pgwNode = 0;
  m_epcStats = 0;
  m_mme = 0;

  m_enbSdranMap.clear ();

  Object::DoDispose ();
}

void
EpcNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Connect EPC stats calculator to trace sources *before* topology creation.
  TraceConnectWithoutContext (
    "TopologyBuilt", MakeCallback (
      &BackhaulStatsCalculator::NotifyTopologyBuilt, m_epcStats));
  TraceConnectWithoutContext (
    "NewSwitchConnection", MakeCallback (
      &BackhaulStatsCalculator::NotifyNewSwitchConnection, m_epcStats));

  // Configuring CSMA helper for connecting EPC nodes (P-GW and S-GWs) to the
  // backhaul topology. This same helper will be used to connect the P-GW to
  // the server node on the Internet.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Configuring IP addresses (don't change the masks!)
  // Using a /8 subnet for all UEs and the P-GW gateway address.
  m_ueAddrHelper.SetBase (m_ueNetworkAddr, "255.0.0.0");

  // Using a /30 subnet which can hold exactly two address for the Internet
  // connection (one used by the server and the other is logically associated
  // to the P-GW external interface.
  m_webAddrHelper.SetBase (m_webNetworkAddr, "255.255.255.252");

  // Using a /24 subnet which can hold up to 253 S-GWs and the P-GW addresses
  // on the same S5 backhaul network.
  m_s5AddrHelper.SetBase (m_s5NetworkAddr, "255.255.255.0");

  // Using a /30 subnet which can hold exactly two eNBs addresses for the point
  // to point X2 interface.
  m_x2AddrHelper.SetBase (m_x2NetworkAddr, "255.255.255.252");

  // Create the OpenFlow backhaul topology.
  TopologyCreate ();

  // Configuring the P-GW and the Internet topology.
  ConfigurePgwAndInternet ();

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
EpcNetwork::InstallController (Ptr<EpcController> controller)
{
  NS_LOG_FUNCTION (this << controller);

  // Installing the controller application into controller node.
  NS_ASSERT_MSG (!m_epcCtrlApp, "Controller application already set.");
  m_epcCtrlApp = controller;
  m_epcCtrlApp->SetMme (GetMme ());
  m_ofSwitchHelper->InstallController (m_epcCtrlNode, m_epcCtrlApp);
}

void
EpcNetwork::ConfigurePgwAndInternet ()
{
  NS_LOG_FUNCTION (this);

  // Configure the P-GW node as an OpenFlow switch.
  m_pgwSwitchDev = (m_ofSwitchHelper->InstallSwitch (m_pgwNode)).Get (0);

  //
  // The first part is to connect the P-GW node to the Interent Web server.
  //
  // Create the SGi interface.
  m_sgiDevices = m_csmaHelper.Install (m_pgwNode, m_webNode);

  Ptr<CsmaNetDevice> pgwSgiDev, webSgiDev;
  pgwSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (0));
  webSgiDev = DynamicCast<CsmaNetDevice> (m_sgiDevices.Get (1));

  Names::Add (Names::FindName (m_pgwNode) + "+" +
              Names::FindName (m_webNode), pgwSgiDev);
  Names::Add (Names::FindName (m_webNode) + "+" +
              Names::FindName (m_pgwNode), webSgiDev);

  // Add the pgwSgiDev as physical port on the P-GW OpenFlow switch.
  Ptr<OFSwitch13Port> pgwSgiPort = m_pgwSwitchDev->AddSwitchPort (pgwSgiDev);
  uint32_t pgwSgiPortNum = pgwSgiPort->GetPortNo ();

  // Note that the SGi device created on the P-GW node and configure as an
  // OpenFlow switch port will have no IP address assigned to it. However,
  // we need this address on the PgwUserApp, so we set it here.
  m_pgwSgiAddr = m_webAddrHelper.NewAddress ();
  NS_LOG_DEBUG ("P-GW SGi interface address: " << m_pgwSgiAddr);

  // Set the IP address on the Internet Web server.
  InternetStackHelper internet;
  internet.Install (m_webNode);
  Ipv4InterfaceContainer webSgiIfContainer;
  webSgiIfContainer = m_webAddrHelper.Assign (NetDeviceContainer (webSgiDev));
  m_webSgiIpAddr = webSgiIfContainer.GetAddress (0);
  NS_LOG_DEBUG ("Web SGi interface address: " << m_webSgiIpAddr);

  // Define static routes at the Internet Web server to the LTE network.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (m_webNode->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    m_ueNetworkAddr, Ipv4Mask ("255.0.0.0"), m_pgwSgiAddr, 1);

  //
  // The second part is to connect the P-GW node to the OpenFlow backhaul
  // infrasctructure.
  //
  // Get the switch datapath ID on the backhaul network to attatch the P-GW.
  uint64_t swDpId = TopologyGetPgwSwitch (m_pgwSwitchDev);
  Ptr<Node> swNode = GetSwitchNode (swDpId);

  // Connect the P-GW to the backhaul over S5 interface.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, m_pgwNode);
  m_s5Devices.Add (devices.Get (1));

  Ptr<CsmaNetDevice> swS5Dev, pgwS5Dev;
  swS5Dev  = DynamicCast<CsmaNetDevice> (devices.Get (0));
  pgwS5Dev = DynamicCast<CsmaNetDevice> (devices.Get (1));

  Names::Add (Names::FindName (swNode) + "+" +
              Names::FindName (m_pgwNode), swS5Dev);
  Names::Add (Names::FindName (m_pgwNode) + "+" +
              Names::FindName (swNode), pgwS5Dev);

  // Add the swS5Dev device as OpenFlow switch port.
  Ptr<OFSwitch13Device> swDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<OFSwitch13Port> swS5Port = swDev->AddSwitchPort (swS5Dev);
  uint32_t swS5PortNum = swS5Port->GetPortNo ();

  // Configure the pgwS5Dev as standard device on P-GW node (with IP address).
  Ipv4InterfaceContainer pgwS5IfContainer;
  pgwS5IfContainer = m_s5AddrHelper.Assign (NetDeviceContainer (pgwS5Dev));
  m_pgwS5Addr = pgwS5IfContainer.GetAddress (0);
  NS_LOG_DEBUG ("P-GW S5 interface address: " << m_pgwS5Addr);

  // Create the virtual net device to work as the logical ports on the P-GW S5
  // interface. This logical ports will connect to the P-GW user-plane
  // application, which will forward packets to/from this logical port and the
  // S5 UDP socket binded to the pgwS5Dev.
  Ptr<VirtualNetDevice> pgwS5PortDev = CreateObject<VirtualNetDevice> ();
  pgwS5PortDev->SetAttribute ("Mtu", UintegerValue (3000));
  pgwS5PortDev->SetAddress (Mac48Address::Allocate ());
  Ptr<OFSwitch13Port> pgwS5Port = m_pgwSwitchDev->AddSwitchPort (pgwS5PortDev);
  uint32_t pgwS5PortNum = pgwS5Port->GetPortNo ();

  // Create the P-GW S5 user-plane application.
  Ptr<PgwUserApp> pgwUserApp = CreateObject <PgwUserApp> (pgwS5PortDev);
  m_pgwNode->AddApplication (pgwUserApp);

  // Notify the controller of the new P-GW device attached to the Internet and
  // to the OpenFlow backhaul network.
  m_epcCtrlApp->NewS5Attach (swDev, swS5PortNum, pgwS5Dev, m_pgwS5Addr);
  m_epcCtrlApp->NewPgwAttach (m_pgwSwitchDev, pgwSgiDev, m_pgwSgiAddr,
                              pgwSgiPortNum, pgwS5PortNum, webSgiDev,
                              m_webSgiIpAddr);

  // Set the default P-GW gateway address, which will be used to set the static
  // route at UEs.
  m_pgwGwAddr = m_ueAddrHelper.NewAddress ();
  NS_LOG_DEBUG ("P-GW gateway address: " << GetUeDefaultGatewayAddress ());
}

Ptr<EpcMme>
EpcNetwork::GetMme ()
{
  NS_LOG_FUNCTION (this);

  return m_mme;
}

//
// Implementing methods inherited from EpcHelper.
//
uint8_t
EpcNetwork::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi,
                               Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);

  // Retrieve the IPv4 address of the UE and notify it to the S-GW.
  Ptr<Node> ueNode = ueDevice->GetNode ();
  Ptr<Ipv4> ueIpv4 = ueNode->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ueIpv4 != 0, "UEs need to have IPv4 installed.");

  int32_t interface = ueIpv4->GetInterfaceForDevice (ueDevice);
  NS_ASSERT (interface >= 0);
  NS_ASSERT (ueIpv4->GetNAddresses (interface) == 1);

  Ipv4Address ueAddr = ueIpv4->GetAddress (interface, 0).GetLocal ();
  m_epcCtrlApp->SetUeAddress (imsi, ueAddr);

  NS_LOG_DEBUG ("Activete EPS bearer UE IP address: " << ueAddr);
  uint8_t bearerId = GetMme ()->AddBearer (imsi, tft, bearer);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
    }
  return bearerId;
}

void
EpcNetwork::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice,
                    uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  GetSdranCloud (enb)->AddEnb (enb, lteEnbNetDevice, cellId);
}

void
EpcNetwork::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // TODO
  // Ptr<SdranCloud> sdran1 = GetSdranCloud (enb1);
  // Ptr<SdranCloud> sdran2 = GetSdranCloud (enb2);
  // if (sdran1 == sdran2)
  //   {
  //     sdran1->AddX2Interface (enb1, enb2);
  //   }
}

void
EpcNetwork::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice );

  GetMme ()->AddUe (imsi);
  m_epcCtrlApp->AddUe (imsi); // FIXME? pgw ou sgw?
}

Ptr<Node>
EpcNetwork::GetPgwNode ()
{
  NS_LOG_FUNCTION (this);

  return m_pgwNode;
}

Ipv4InterfaceContainer
EpcNetwork::AssignUeIpv4Address (NetDeviceContainer ueDevices)
{
  NS_LOG_FUNCTION (this);

  return m_ueAddrHelper.Assign (ueDevices);
}

Ipv4Address
EpcNetwork::GetUeDefaultGatewayAddress ()
{
  NS_LOG_FUNCTION (this);

  return m_pgwGwAddr;
}

};  // namespace ns3
