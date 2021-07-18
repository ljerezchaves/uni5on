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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include <ns3/csma-module.h>
#include "transport-network.h"
#include "transport-controller.h"
#include "switch-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TransportNetwork");
NS_OBJECT_ENSURE_REGISTERED (TransportNetwork);

// Initializing the static members.
const Ipv4Address TransportNetwork::m_s1Addr = Ipv4Address ("10.1.0.0");
const Ipv4Address TransportNetwork::m_s5Addr = Ipv4Address ("10.2.0.0");
const Ipv4Address TransportNetwork::m_x2Addr = Ipv4Address ("10.3.0.0");
const Ipv4Mask    TransportNetwork::m_s1Mask = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    TransportNetwork::m_s5Mask = Ipv4Mask ("255.255.255.0");
const Ipv4Mask    TransportNetwork::m_x2Mask = Ipv4Mask ("255.255.255.0");

TransportNetwork::TransportNetwork ()
  : m_controllerApp (0),
  m_controllerNode (0),
  m_switchHelper (0)
{
  NS_LOG_FUNCTION (this);
}

TransportNetwork::~TransportNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TransportNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TransportNetwork")
    .SetParent<Object> ()

    // Transport links.
    .AddAttribute ("LinkDataRate",
                   "The data rate for the transport CSMA links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&TransportNetwork::m_linkRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The delay for the transport CSMA links.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   // The default value is for 40km fiber cable latency.
                   TimeValue (MicroSeconds (200)),
                   MakeTimeAccessor (&TransportNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU for the transport CSMA links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&TransportNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())

    // Transport switches.
    .AddAttribute ("CpuCapacity",
                   "Processing capacity for the transport switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("2Gbps")),
                   MakeDataRateAccessor (&TransportNetwork::m_cpuCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("FlowTableSize",
                   "Flow table size for the transport switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (8192),
                   MakeUintegerAccessor (&TransportNetwork::m_flowTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("GroupTableSize",
                   "Group table size for the transport switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (4096),
                   MakeUintegerAccessor (&TransportNetwork::m_groupTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("MeterTableSize",
                   "Meter table size for the transport switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (4096),
                   MakeUintegerAccessor (&TransportNetwork::m_meterTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
  ;
  return tid;
}

void
TransportNetwork::EnablePcap (std::string prefix, bool promiscuous,
                              bool ofchannel, bool epcDevices, bool swtDevices)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous << ofchannel <<
                   epcDevices << swtDevices);

  if (ofchannel)
    {
      m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);
    }

  CsmaHelper helper;
  if (epcDevices)
    {
      helper.EnablePcap (prefix + "epc", m_epcDevices, promiscuous);
    }
  if (swtDevices)
    {
      helper.EnablePcap (prefix + "swt", m_switchNodes, promiscuous);
    }
}

std::pair<Ptr<CsmaNetDevice>, Ptr<OFSwitch13Port>>
TransportNetwork::AttachEpcNode (Ptr<Node> epcNode, uint16_t swIdx,
                                 EpsIface iface, std::string ifaceStr)
{
  NS_LOG_FUNCTION (this << epcNode << swIdx << iface);
  NS_LOG_INFO ("Attach EPC node " << epcNode << " to switch index " <<
               swIdx << " over " << EpsIfaceStr (iface) << " interface.");

  NS_ASSERT_MSG (swIdx < GetNSwitches (), "Invalid switch index.");

  // Get the switch on the transport network.
  uint64_t swDpId = m_switchDevices.Get (swIdx)->GetDatapathId ();
  Ptr<OFSwitch13Device> swOfDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<Node> swNode = swOfDev->GetObject<Node> ();

  // Connect the EPC node to the switch node.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, epcNode);
  Ptr<CsmaNetDevice> swDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  Ptr<CsmaNetDevice> epcDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
  m_epcDevices.Add (epcDev);

  // Set device names for PCAP files.
  if (ifaceStr.empty ())
    {
      ifaceStr = EpsIfaceStr (iface);
    }
  SetDeviceNames (swDev, epcDev, "~" + ifaceStr + "~");

  // Add the swDev device as OpenFlow switch port on the switch node.
  Ptr<OFSwitch13Port> swPort = swOfDev->AddSwitchPort (swDev);
  uint32_t swPortNo = swPort->GetPortNo ();

  // Configure the epcDev IP address according to the logical interface.
  switch (iface)
    {
    case EpsIface::S1:
      m_s1AddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    case EpsIface::S5:
      m_s5AddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    case EpsIface::X2:
      m_x2AddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    default:
      NS_ABORT_MSG ("Invalid interface.");
    }

  // Notify the controller of the new EPC device attached to the network.
  m_controllerApp->NotifyEpcAttach (swOfDev, swPortNo, epcDev);

  return std::make_pair (epcDev, swPort);
}

uint32_t
TransportNetwork::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchDevices.GetN ();
}

Ptr<TransportController>
TransportNetwork::GetControllerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_controllerApp;
}

void
TransportNetwork::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_controllerApp = 0;
  m_controllerNode = 0;
  m_switchHelper = 0;
  Object::DoDispose ();
}

void
TransportNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure IP address helpers.
  m_s1AddrHelper.SetBase (m_s1Addr, m_s1Mask);
  m_s5AddrHelper.SetBase (m_s5Addr, m_s5Mask);
  m_x2AddrHelper.SetBase (m_x2Addr, m_x2Mask);

  // Configuring the CSMA helper for the transport links.
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (m_linkRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Create the OFSwitch13 helper for the OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<SwitchHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Configuring the OFSwitch13 helper for the transport switches.
  // 4 fixed pipeline tables (input, classification, bandwidth, and output),
  // and one extra table for each logical network slice).
  m_switchHelper->SetDeviceAttribute (
    "CpuCapacity", DataRateValue (m_cpuCapacity));
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_flowTableSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (m_groupTableSize));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_meterTableSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (4 + static_cast<int> (SliceId::ALL)));

  // Create the OpenFlow transport network.
  CreateTopology ();

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

} // namespace ns3
