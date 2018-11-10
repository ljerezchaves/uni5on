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
    .AddAttribute ("EpcLinkDataRate",
                   "The data rate for the link connecting any EPC entity to "
                   "the OpenFlow backhaul network.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gbps")),
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
    .AddAttribute ("LinkMtu",
                   "The MTU for CSMA links. "
                   "Consider + 40 byter of GTP/UDP/IP tunnel overhead.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492), // Ethernet II - PPoE
                   MakeUintegerAccessor (&BackhaulNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())

    // Backhaul switches.
    .AddAttribute ("CpuCapacity",
                   "Processing capacity for the backhaul switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("100Gbps")),
                   MakeDataRateAccessor (&BackhaulNetwork::m_cpuCapacity),
                   MakeDataRateChecker ())
    .AddAttribute ("FlowTableSize",
                   "Flow table size for the backhaul switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&BackhaulNetwork::m_flowTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("GroupTableSize",
                   "Group table size for the backhaul switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&BackhaulNetwork::m_groupTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
    .AddAttribute ("MeterTableSize",
                   "Meter table size for the backhaul switches.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (65535),
                   MakeUintegerAccessor (&BackhaulNetwork::m_meterTableSize),
                   MakeUintegerChecker<uint16_t> (0, 65535))
  ;
  return tid;
}

void
BackhaulNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on OpenFlow channel.
  m_switchHelper->EnableOpenFlowPcap (prefix + "ofchannel", promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  helper.EnablePcap (prefix + "epc", m_epcDevices, promiscuous);
  helper.EnablePcap (prefix + "backhaul", m_switchNodes, promiscuous);
}

std::pair<Ptr<CsmaNetDevice>, Ptr<OFSwitch13Port> >
BackhaulNetwork::AttachEpcNode (Ptr<Node> epcNode, uint16_t swIdx,
                                LteIface iface, std::string ifaceStr)
{
  NS_LOG_FUNCTION (this << epcNode << swIdx << iface);
  NS_LOG_INFO ("Attach EPC node " << epcNode << " to backhaul switch index " <<
               swIdx << " over " << LteIfaceStr (iface) << " interface.");

  NS_ASSERT_MSG (swIdx < GetNSwitches (), "Invalid switch index.");

  // Get the switch on the backhaul network.
  uint64_t swDpId = m_switchDevices.Get (swIdx)->GetDatapathId ();
  Ptr<OFSwitch13Device> swOfDev = OFSwitch13Device::GetDevice (swDpId);
  Ptr<Node> swNode = swOfDev->GetObject<Node> ();

  // Connect the EPC node to the switch node.
  NetDeviceContainer devices = m_csmaHelper.Install (swNode, epcNode);
  Ptr<CsmaNetDevice> swDev = DynamicCast<CsmaNetDevice> (devices.Get (0));
  Ptr<CsmaNetDevice> epcDev = DynamicCast<CsmaNetDevice> (devices.Get (1));
  m_epcDevices.Add (epcDev);

  // Set device names for pcap files.
  if (ifaceStr.empty ())
    {
      ifaceStr = LteIfaceStr (iface);
    }
  SetDeviceNames (swDev, epcDev, "~" + ifaceStr + "~");

  // Add the swDev device as OpenFlow switch port on the switch node.
  Ptr<OFSwitch13Port> swPort = swOfDev->AddSwitchPort (swDev);
  uint32_t swPortNo = swPort->GetPortNo ();

  // Configure the epcDev IP address according to the LTE logical interface.
  switch (iface)
    {
    case LteIface::S1U:
      m_s1uAddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    case LteIface::S5:
      m_s5AddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    case LteIface::X2:
      m_x2AddrHelper.Assign (NetDeviceContainer (epcDev));
      break;
    default:
      NS_ABORT_MSG ("Invalid LTE interface.");
    }

  // Notify the controller of the new EPC device attached to the backhaul.
  m_controllerApp->NotifyEpcAttach (swOfDev, swPortNo, epcDev);

  return std::make_pair (epcDev, swPort);
}

uint32_t
BackhaulNetwork::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchDevices.GetN ();
}

Ptr<BackhaulController>
BackhaulNetwork::GetControllerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_controllerApp;
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

  // Configuring OpenFlow helper for backhaul switches.
  // 5 pipeline tables (input, classification, routing, bandwidth, and output)
  // plus one extra table for each logical network slice.
  m_switchHelper->SetDeviceAttribute (
    "CpuCapacity", DataRateValue (m_cpuCapacity));
  m_switchHelper->SetDeviceAttribute (
    "FlowTableSize", UintegerValue (m_flowTableSize));
  m_switchHelper->SetDeviceAttribute (
    "GroupTableSize", UintegerValue (m_groupTableSize));
  m_switchHelper->SetDeviceAttribute (
    "MeterTableSize", UintegerValue (m_meterTableSize));
  m_switchHelper->SetDeviceAttribute (
    "PipelineTables", UintegerValue (5 + static_cast<int> (SliceId::ALL)));

  // Create the OpenFlow backhaul network.
  CreateTopology ();

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

} // namespace ns3
