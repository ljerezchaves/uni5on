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

#include "epc-controller.h"
#include "epc-network.h"
#include "../sdran/sdran-controller.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcController");
NS_OBJECT_ENSURE_REGISTERED (EpcController);

// Initializing EpcController static members.
const uint16_t EpcController::m_flowTimeout = 0;
EpcController::QciDscpMap_t EpcController::m_qciDscpTable;
EpcController::DscpQueueMap_t EpcController::m_dscpQueueTable;

// TEID values for bearers ranges from 0x100 to 0xFEFFFFFF.
// Other values are reserved for controller usage.
const uint32_t EpcController::m_teidStart = 0x100;
const uint32_t EpcController::m_teidEnd = 0xFEFFFFFF;
uint32_t EpcController::m_teidCount = EpcController::m_teidStart;

EpcController::EpcController ()
  : m_tftMaxLoad (DataRate (std::numeric_limits<uint64_t>::max ())),
    m_tftTableSize (std::numeric_limits<uint32_t>::max ())
{
  NS_LOG_FUNCTION (this);

  StaticInitialize ();
  m_s5SapPgw = new MemberEpcS5SapPgw<EpcController> (this);
}

EpcController::~EpcController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcController")
    .SetParent<OFSwitch13Controller> ()
    .AddAttribute ("GbrSlicing",
                   "GBR slicing mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_gbrSlicing),
                   MakeEnumChecker (EpcController::OFF, "off",
                                    EpcController::ON,  "on"))
    .AddAttribute ("HtcAggregation",
                   "HTC traffic aggregation mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::OFF),
                   MakeEnumAccessor (&EpcController::m_htcAggregation),
                   MakeEnumChecker (EpcController::OFF,  "off",
                                    EpcController::ON,   "on",
                                    EpcController::AUTO, "auto"))
    .AddAttribute ("HtcAggGbrThs",
                   "HTC traffic aggregation GBR bandwidth threshold.",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&EpcController::m_htcAggGbrThs),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("HtcAggNonThs",
                   "HTC traffic aggregation Non-GBR bandwidth threshold.",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&EpcController::m_htcAggNonThs),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("MtcAggregation",
                   "MTC traffic aggregation mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::OFF),
                   MakeEnumAccessor (&EpcController::m_mtcAggregation),
                   MakeEnumChecker (EpcController::OFF,  "off",
                                    EpcController::ON,   "on"))
    .AddAttribute ("PgwTftAdaptiveMode",
                   "P-GW TFT adaptive mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_tftAdaptive),
                   MakeEnumChecker (EpcController::OFF,  "off",
                                    EpcController::ON,   "on",
                                    EpcController::AUTO, "auto"))
    .AddAttribute ("PgwTftBlockPolicy",
                   "P-GW TFT overloaded block policy.",
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_tftBlockPolicy),
                   MakeEnumChecker (EpcController::OFF,  "none",
                                    EpcController::ON,   "all",
                                    EpcController::AUTO, "gbr"))
    .AddAttribute ("PgwTftBlockThs",
                   "The P-GW TFT block threshold.",
                   DoubleValue (0.95),
                   MakeDoubleAccessor (&EpcController::m_tftBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
    .AddAttribute ("PgwTftJoinThs",
                   "The P-GW TFT join threshold.",
                   DoubleValue (0.30),
                   MakeDoubleAccessor (&EpcController::m_tftJoinThs),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("PgwTftSplitThs",
                   "The P-GW TFT split threshold.",
                   DoubleValue (0.90),
                   MakeDoubleAccessor (&EpcController::m_tftSplitThs),
                   MakeDoubleChecker<double> (0.5, 1.0))
    .AddAttribute ("PgwTftSwitches",
                   "The number of P-GW TFT switches available for use.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&EpcController::m_tftSwitches),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PriorityQueues",
                   "Priority output queues mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_priorityQueues),
                   MakeEnumChecker (EpcController::OFF, "off",
                                    EpcController::ON,  "on"))
    .AddAttribute ("TimeoutInterval",
                   "The interval between internal periodic operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&EpcController::m_timeout),
                   MakeTimeChecker ())

    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerReleaseTrace),
                     "ns3::RoutingInfo::TracedCallback")
    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerRequestTrace),
                     "ns3::RoutingInfo::TracedCallback")
    .AddTraceSource ("PgwTftStats", "The P-GW TFT stats trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_pgwTftStatsTrace),
                     "ns3::EpcController::PgwTftStatsTracedCallback")
    .AddTraceSource ("SessionCreated", "The session created trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_sessionCreatedTrace),
                     "ns3::EpcController::SessionCreatedTracedCallback")
  ;
  return tid;
}

bool
EpcController::DedicatedBearerRelease (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);

  // This bearer must be active.
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");

  TopologyBearerRelease (rInfo);
  m_bearerReleaseTrace (rInfo);
  NS_LOG_INFO ("Bearer released by controller.");

  // Everything is ok! Let's deactivate and remove this bearer.
  rInfo->SetActive (false);
  return BearerRemove (rInfo);
}

bool
EpcController::DedicatedBearerRequest (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  Ptr<S5AggregationInfo> aggInfo = rInfo->GetObject<S5AggregationInfo> ();

  // This bearer must be inactive as we are going to reuse it's metadata.
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Update the P-GW TFT index and the blocked flag.
  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
  rInfo->SetBlocked (false);

  // Update bandwidth usage and threshold values.
  TopologyBearerAggregate (rInfo);
  aggInfo->SetThreshold (rInfo->IsGbr () ? m_htcAggGbrThs : m_htcAggNonThs);

  // Check for S5 traffic agregation. The aggregation flag can only be changed
  // when the operation mode is set to AUTO (only supported by HTC UEs by now).
  if (!ueInfo->IsMtc () && GetHtcAggregMode () == OperationMode::AUTO)
    {
      if (aggInfo->GetMaxBandwidthUsage () <= aggInfo->GetThreshold ())
        {
          aggInfo->SetAggregated (true);
          NS_LOG_INFO ("Aggregating bearer teid " << rInfo->GetTeid ());
        }
      else
        {
          aggInfo->SetAggregated (false);
        }
    }

  // Let's first check for available resources on P-GW and backhaul switches.
  bool accepted = true;
  accepted &= PgwTftBearerRequest (rInfo);
  accepted &= TopologyBearerRequest (rInfo);
  m_bearerRequestTrace (rInfo);
  if (!accepted)
    {
      NS_LOG_INFO ("Bearer request blocked by controller.");
      return false;
    }

  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will increase the bearer
  // priority. Doing this, we avoid problems with old 'expiring' rules, and
  // we can even use new routing paths when necessary.
  NS_LOG_INFO ("Bearer request accepted by controller.");
  rInfo->SetActive (true);
  return BearerInstall (rInfo);
}

void
EpcController::NotifyPgwBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (devices.GetN () == m_pgwDpIds.size ()
                 && devices.GetN () == (m_tftSwitches + 1U),
                 "Inconsistent number of P-GW OpenFlow switches.");

  // When the P-GW adaptive mechanism is OFF, block the m_tftSwitches to 1.
  if (GetPgwAdaptiveMode () == OperationMode::OFF)
    {
      m_tftSwitches = 1;
    }
}

void
EpcController::NotifyPgwMainAttach (
  Ptr<OFSwitch13Device> pgwSwDev, uint32_t pgwS5PortNo, uint32_t pgwSgiPortNo,
  Ptr<NetDevice> pgwS5Dev, Ptr<NetDevice> webSgiDev)
{
  NS_LOG_FUNCTION (this << pgwSwDev << pgwS5PortNo << pgwSgiPortNo <<
                   pgwS5Dev << webSgiDev);

  // Saving information for P-GW main switch at first index.
  m_pgwDpIds.push_back (pgwSwDev->GetDatapathId ());
  m_pgwS5PortsNo.push_back (pgwS5PortNo);
  m_pgwS5Addr = EpcNetwork::GetIpv4Addr (pgwS5Dev);
  m_pgwSgiPortNo = pgwSgiPortNo;

  // -------------------------------------------------------------------------
  // Table 0 -- P-GW default table -- [from higher to lower priority]
  //
  // IP packets coming from the LTE network (S5 port) and addressed to the
  // Internet (Web IP address) have the destination MAC address rewritten to
  // the Web SGi MAC address (this is necessary when using logical ports) and
  // are forward to the SGi interface port.
  Mac48Address webMac = Mac48Address::ConvertFrom (webSgiDev->GetAddress ());
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
         << ",in_port=" << pgwS5PortNo
         << ",ip_dst=" << EpcNetwork::GetIpv4Addr (webSgiDev)
         << " write:set_field=eth_dst:" << webMac
         << ",output=" << pgwSgiPortNo;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdOut.str ());

  // IP packets coming from the Internet (SGi port) and addressed to the UE
  // network are sent to the table corresponding to the current P-GW adaptive
  // mechanism level.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << pgwSgiPortNo
        << ",ip_dst=" << EpcNetwork::m_ueAddr
        << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
        << " goto:" << m_tftLevel + 1;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdIn.str ());

  // Table miss entry. Send to controller.
  DpctlSchedule (pgwSwDev->GetDatapathId (), "flow-mod cmd=add,table=0,prio=0"
                 " apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 to N -- P-GW adaptive mechanism -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyPgwTftAttach function.
}

void
EpcController::NotifyPgwTftAttach (
  uint16_t pgwTftCounter, Ptr<OFSwitch13Device> pgwSwDev, uint32_t pgwS5PortNo,
  uint32_t pgwMainPortNo)
{
  NS_LOG_FUNCTION (this << pgwTftCounter << pgwSwDev << pgwS5PortNo <<
                   pgwMainPortNo);

  // Saving information for P-GW TFT switches.
  NS_ASSERT_MSG (pgwTftCounter < m_tftSwitches, "No more TFTs allowed.");
  m_pgwDpIds.push_back (pgwSwDev->GetDatapathId ());
  m_pgwS5PortsNo.push_back (pgwS5PortNo);

  uint32_t tableSize = pgwSwDev->GetFlowTableSize ();
  DataRate plCapacity = pgwSwDev->GetPipelineCapacity ();
  m_tftTableSize = std::min (m_tftTableSize, tableSize);
  m_tftMaxLoad = std::min (m_tftMaxLoad, plCapacity);

  // Configuring the P-GW main switch to forward traffic to this TFT switch
  // considering all possible adaptive mechanism levels.
  for (uint16_t tft = m_tftSwitches; pgwTftCounter + 1 <= tft; tft /= 2)
    {
      uint16_t lbLevel = (uint16_t)log2 (tft);
      uint16_t ipMask = (1 << lbLevel) - 1;
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64,table=" << lbLevel + 1
          << " eth_type=0x800,ip_dst=0.0.0." << pgwTftCounter
          << "/0.0.0." << ipMask
          << " apply:output=" << pgwMainPortNo;
      DpctlSchedule (GetPgwMainDpId (), cmd.str ());
    }

  // -------------------------------------------------------------------------
  // Table 0 -- P-GW TFT default table -- [from higher to lower priority]
  //
  // Entries will be installed here by PgwRulesInstall function.
}

void
EpcController::NotifyS5Attach (
  Ptr<OFSwitch13Device> swtchDev, uint32_t portNo, Ptr<NetDevice> gwDev)
{
  NS_LOG_FUNCTION (this << swtchDev << portNo << gwDev);

  // Configure S5 port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // GTP packets entering the ring network from any EPC port. Send to the
  // Classification table.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64,flags=0x0007"
        << " eth_type=0x800,ip_proto=17"
        << ",udp_src=" << EpcNetwork::m_gtpuPort
        << ",udp_dst=" << EpcNetwork::m_gtpuPort
        << ",in_port=" << portNo
        << " goto:1";
  DpctlSchedule (swtchDev->GetDatapathId (), cmdIn.str ());

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // GTP packets addressed to EPC elements connected to this switch over EPC
  // ports. Write the output port into action set. Send the packet directly to
  // Output table.
  Mac48Address gwMac = Mac48Address::ConvertFrom (gwDev->GetAddress ());
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=2,prio=256 eth_type=0x800"
         << ",eth_dst=" << gwMac
         << ",ip_dst=" << EpcNetwork::GetIpv4Addr (gwDev)
         << " write:output=" << portNo
         << " goto:4";
  DpctlSchedule (swtchDev->GetDatapathId (), cmdOut.str ());
}

uint32_t
EpcController::NotifySgwAttach (Ptr<NetDevice> gwDev)
{
  NS_LOG_FUNCTION (this << gwDev);

  static uint32_t m_mtcTeidCount = EpcController::m_teidEnd + 1;
  uint32_t mtcTeid = 0;

  // When the MTC traffic aggregation is enable, let's create and install the
  // aggregation uplink GTP tunnel between this S-GW and the P-GW. We are using
  // a 'fake' rInfo for this aggregation bearer, in order to use existing
  // methods to install the OpenFlow rules.
  if (GetMtcAggregMode () == OperationMode::ON)
    {
      mtcTeid = m_mtcTeidCount++;

      // FIXME Should I use GBR to force DSCP?
      EpcTft::PacketFilter fakeUplinkfilter;
      fakeUplinkfilter.direction = EpcTft::UPLINK;
      Ptr<EpcTft> fakeTft = CreateObject<EpcTft> ();
      fakeTft->Add (fakeUplinkfilter);
      BearerContext_t fakeBearer;
      fakeBearer.tft = fakeTft;

      // Creating the 'fake' routing info.
      Ptr<RoutingInfo> rInfo = CreateObject<RoutingInfo> (mtcTeid);
      rInfo->SetActive (true);
      rInfo->SetBearerContext (fakeBearer);
      rInfo->SetBlocked (false);
      rInfo->SetDefault (false);
      rInfo->SetInstalled (false);
      rInfo->SetPgwS5Addr (m_pgwS5Addr);
      rInfo->SetPriority (0xFF00);
      rInfo->SetSgwS5Addr (EpcNetwork::GetIpv4Addr (gwDev));
      rInfo->SetTimeout (0);

      // Set the traffic aggregation flag.
      rInfo->GetObject<S5AggregationInfo> ()->SetAggregated (true);

      // Install the OpenFlow bearer rules after handshake procedures.
      TopologyBearerCreated (rInfo);
      Simulator::Schedule (Seconds (0.5),
                           &EpcController::MtcAggBearerInstall, this, rInfo);
    }
  return mtcTeid;
}

void
EpcController::NotifyTopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);
}

void
EpcController::NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);
}

EpcController::OperationMode
EpcController::GetGbrSlicingMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_gbrSlicing;
}

EpcController::OperationMode
EpcController::GetHtcAggregMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_htcAggregation;
}

EpcController::OperationMode
EpcController::GetMtcAggregMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mtcAggregation;
}

EpcController::OperationMode
EpcController::GetPgwAdaptiveMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftAdaptive;
}


EpcController::OperationMode
EpcController::GetPriorityQueuesMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_priorityQueues;
}

EpcS5SapPgw*
EpcController::GetS5SapPgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5SapPgw;
}

uint16_t
EpcController::GetDscpValue (EpsBearer::Qci qci)
{
  NS_LOG_FUNCTION_NOARGS ();

  QciDscpMap_t::const_iterator it;
  it = EpcController::m_qciDscpTable.find (qci);
  if (it != EpcController::m_qciDscpTable.end ())
    {
      return it->second;
    }
  NS_FATAL_ERROR ("No DSCP mapped value for QCI " << qci);
}

void
EpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  delete (m_s5SapPgw);

  // Chain up.
  OFSwitch13Controller::DoDispose ();
}

void
EpcController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Check the number of P-GW TFT switches (must be a power of 2).
  NS_ASSERT_MSG ((m_tftSwitches & (m_tftSwitches - 1)) == 0,
                 "Invalid number of P-GW TFT switches.");

  // Set the initial number of P-GW TFT active switches.
  switch (GetPgwAdaptiveMode ())
    {
    case OperationMode::ON:
      {
        m_tftLevel = (uint8_t)log2 (m_tftSwitches);
        break;
      }
    case OperationMode::OFF:
    case OperationMode::AUTO:
      {
        m_tftLevel = 0;
        break;
      }
    }

  // Schedule the first timeout operation.
  Simulator::Schedule (m_timeout, &EpcController::ControllerTimeout, this);

  // Chain up.
  OFSwitch13Controller::NotifyConstructionCompleted ();
}

ofl_err
EpcController::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Chain up for logging and abort.
  OFSwitch13Controller::HandleError (msg, swtch, xid);
  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
EpcController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = msg->stats->cookie;
  uint16_t prio = msg->stats->priority;

  char *msgStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << msgStr);
  free (msgStr);

  // Since handlers must free the message when everything is ok,
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // Check for existing routing information for this bearer.
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed for inactive bearer teid " << teid);
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Old rule removed for bearer teid " << teid);
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we are going to abort the program to avoid wrong results.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
EpcController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *msgStr = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_DEBUG ("Packet in match: " << msgStr);
  free (msgStr);

  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);
  return 0;
}

void
EpcController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // This function is called after a successfully handshake between the EPC
  // controller and any switch on the EPC network (including the P-GW user
  // plane and the OpenFlow backhaul network). For the P-GW switches, all
  // entries will be installed by NotifyPgw*Attach and PgwRulesInstall
  // methods, so we scape here.
  std::vector<uint64_t>::iterator it;
  it = std::find (m_pgwDpIds.begin (), m_pgwDpIds.end (), swtch->GetDpId ());
  if (it != m_pgwDpIds.end ())
    {
      // Do nothing for P-GW user plane switches.
      return;
    }

  // For the switches on the backhaul network, install following rules:
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by NewS5Attach function.

  // GTP packets entering the switch from any port other then EPC ports.
  // Send to Routing table.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=32"
      << " eth_type=0x800,ip_proto=17"
      << ",udp_src=" << EpcNetwork::m_gtpuPort
      << ",udp_dst=" << EpcNetwork::m_gtpuPort
      << " goto:2";
  DpctlExecute (swtch, cmd.str ());

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- Classification table -- [from higher to lower priority]
  //
  // Entries will be installed here by TopologyRoutingInstall function.

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // Entries will be installed here by NewS5Attach function.
  // Entries will be installed here by NotifyTopologyBuilt function.

  // GTP packets classified at previous table. Write the output group into
  // action set based on metadata field. Send the packet to Slicing table.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x1"
                " write:group=1 goto:3");
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x2"
                " write:group=2 goto:3");

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 3 -- Slicing table -- [from higher to lower priority]
  //
  if (GetGbrSlicingMode () == OperationMode::ON)
    {
      // Non-GBR packets are indicated by DSCP field DSCP_AF11 nad DscpDefault.
      // Apply Non-GBR meter band. Send the packet to Output table.

      // DSCP_AF11 (DSCP decimal 10)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x1,ip_dscp=10"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=17"
                    " eth_type=0x800,meta=0x2,ip_dscp=10"
                    " meter:2 goto:4");

      // DscpDefault (DSCP decimal 0)
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x1,ip_dscp=0"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,meta=0x2,ip_dscp=0"
                    " meter:2 goto:4");
    }

  // Table miss entry. Send the packet to Output table
  DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=0 goto:4");

  // -------------------------------------------------------------------------
  // Table 4 -- Output table -- [from higher to lower priority]
  //
  if (GetPriorityQueuesMode () == OperationMode::ON)
    {
      // Priority output queues rules.
      DscpQueueMap_t::iterator it;
      for (it = EpcController::m_dscpQueueTable.begin ();
           it != EpcController::m_dscpQueueTable.end (); ++it)
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,table=4,prio=16 eth_type=0x800,"
              << "ip_dscp=" << static_cast<uint16_t> (it->first)
              << " write:queue=" << static_cast<uint32_t> (it->second);
          DpctlExecute (swtch, cmd.str ());
        }
    }

  // Table miss entry. No instructions. This will trigger action set execute.
  DpctlExecute (swtch, "flow-mod cmd=add,table=4,prio=0");
}

bool
EpcController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
  rInfo->SetInstalled (false);

  if (rInfo->IsAggregated ())
    {
      // Don't install rules for aggregated traffic. This will automatically
      // force the traffic over the S5 default bearer.
      return true;
    }

  // Increasing the priority every time we (re)install routing rules.
  rInfo->IncreasePriority ();

  // Install the rules.
  bool success = true;
  success &= PgwRulesInstall (rInfo);
  success &= TopologyRoutingInstall (rInfo);

  rInfo->SetInstalled (success);
  return success;
}

bool
EpcController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  if (rInfo->IsAggregated ())
    {
      // No rules to remove for aggregated traffic.
      return true;
    }

  // Remove the rules.
  bool success = true;
  success &= PgwRulesRemove (rInfo);
  success &= TopologyRoutingRemove (rInfo);

  rInfo->SetInstalled (!success);
  return success;
}

void
EpcController::ControllerTimeout (void)
{
  NS_LOG_FUNCTION (this);

  PgwTftCheckUsage ();

  // Schedule the next timeout operation.
  Simulator::Schedule (m_timeout, &EpcController::ControllerTimeout, this);
}

void
EpcController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

  uint16_t cellId = msg.uli.gci;
  uint64_t imsi = msg.imsi;

  Ptr<SdranController> sdranCtrl = SdranController::GetPointer (cellId);
  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  // Create the response message.
  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = imsi;
  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
  for (bit = msg.bearerContextsToBeCreated.begin ();
       bit != msg.bearerContextsToBeCreated.end ();
       ++bit)
    {
      NS_ABORT_IF (EpcController::m_teidCount > EpcController::m_teidEnd);
      uint32_t teid = EpcController::m_teidCount++;
      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.teid = teid;
      bearerContext.sgwFteid.address = enbInfo->GetSgwS1uAddr ();
      bearerContext.epsBearerId = bit->epsBearerId;
      bearerContext.bearerLevelQos = bit->bearerLevelQos;
      bearerContext.tft = bit->tft;
      res.bearerContextsCreated.push_back (bearerContext);

      // Add the TFT entry to the UeInfo (don't move this command from here).
      ueInfo->AddTft (bit->tft, teid);
    }

  // Create and save routing information for default bearer.
  // (first element on the res.bearerContextsCreated)
  BearerContext_t defaultBearer = res.bearerContextsCreated.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");

  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo == 0, "Existing routing for bearer teid " << teid);

  // Create the routing information for this default bearer.
  rInfo = CreateObject<RoutingInfo> (teid);
  rInfo->SetActive (true);
  rInfo->SetBearerContext (defaultBearer);
  rInfo->SetBlocked (false);
  rInfo->SetDefault (true);
  rInfo->SetImsi (imsi);
  rInfo->SetInstalled (false);
  rInfo->SetPgwS5Addr (m_pgwS5Addr);
  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
  rInfo->SetPriority (0x7F);
  rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
  rInfo->SetTimeout (0);
  TopologyBearerCreated (rInfo);

  // Set the aggregation flag for the default bearer of MTC UEs when MTC
  // traffic aggregation is ON. This will prevent OpenFlow rules from being
  // installed even for the default MTC bearer.
  if (ueInfo->IsMtc () && GetMtcAggregMode () == OperationMode::ON)
    {
      rInfo->GetObject<S5AggregationInfo> ()->SetAggregated (true);
    }

  // For default bearer, no meter nor GBR metadata.
  // For logic consistence, let's check for available resources.
  bool accepted = true;
  accepted &= PgwTftBearerRequest (rInfo);
  accepted &= TopologyBearerRequest (rInfo);
  NS_ASSERT_MSG (accepted, "Default bearer must be accepted.");
  m_bearerRequestTrace (rInfo);

  // Install rules for default bearer.
  bool installed = BearerInstall (rInfo);
  NS_ASSERT_MSG (installed, "Default bearer must be installed.");

  // For other dedicated bearers, let's create and save it's routing metadata.
  // (starting at the second element of res.bearerContextsCreated).
  BearerContextList_t::iterator it = res.bearerContextsCreated.begin ();
  for (it++; it != res.bearerContextsCreated.end (); it++)
    {
      BearerContext_t dedicatedBearer = *it;
      teid = dedicatedBearer.sgwFteid.teid;

      // Create the routing information for this dedicated bearer.
      rInfo = CreateObject<RoutingInfo> (teid);
      rInfo->SetActive (false);
      rInfo->SetBearerContext (dedicatedBearer);
      rInfo->SetBlocked (false);
      rInfo->SetDefault (false);
      rInfo->SetImsi (imsi);
      rInfo->SetInstalled (false);
      rInfo->SetPgwS5Addr (m_pgwS5Addr);
      rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
      rInfo->SetPriority (0x1FFF);
      rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
      rInfo->SetTimeout (m_flowTimeout);
      TopologyBearerCreated (rInfo);

      // Set the aggregation flag for dedicated beareres of UEs when
      // traffic aggregation is ON. This will prevent OpenFlow rules from
      // being installed for dedicated bearers.
      if ((ueInfo->IsMtc () && GetMtcAggregMode () == OperationMode::ON)
          || (!ueInfo->IsMtc () && GetHtcAggregMode () == OperationMode::ON))
        {
          rInfo->GetObject<S5AggregationInfo> ()->SetAggregated (true);
          NS_LOG_INFO ("Aggregating bearer teid " << rInfo->GetTeid ());
        }

      // For all GBR bearers, create the GBR metadata.
      if (rInfo->IsGbr ())
        {
          Ptr<GbrInfo> gbrInfo = CreateObject<GbrInfo> (rInfo);

          // Set the appropriated DiffServ DSCP value for this bearer.
          gbrInfo->SetDscp (
            EpcController::GetDscpValue (rInfo->GetQciInfo ()));
        }

      // If necessary, create the meter metadata for maximum bit rate.
      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
      if (gbrQoS.mbrDl || gbrQoS.mbrUl)
        {
          CreateObject<MeterInfo> (rInfo);
        }
    }

  // Fire trace source notifying the created session.
  m_sessionCreatedTrace (imsi, cellId, res.bearerContextsCreated);

  // Send the response message back to the S-GW.
  sdranCtrl->GetS5SapSgw ()->CreateSessionResponse (res);
}

void
EpcController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

void
EpcController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

void
EpcController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

uint64_t
EpcController::GetPgwMainDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwDpIds.at (0);
}

uint64_t
EpcController::GetPgwTftDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  return m_pgwDpIds.at (idx);
}

uint16_t
EpcController::GetPgwTftIdx (
  Ptr<const RoutingInfo> rInfo, uint16_t activeTfts) const
{
  NS_LOG_FUNCTION (this << rInfo << activeTfts);

  if (activeTfts == 0)
    {
      activeTfts = 1 << m_tftLevel;
    }
  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  return 1 + (ueInfo->GetUeAddr ().Get () % activeTfts);
}

bool
EpcController::MtcAggBearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  bool success = TopologyRoutingInstall (rInfo);
  NS_ASSERT_MSG (success, "Error when installing the MTC aggregation bearer.");
  NS_LOG_INFO ("MTC aggregation bearer teid " << rInfo->GetTeid () <<
               " installed for S-GW " << rInfo->GetSgwS5Addr ());

  rInfo->SetInstalled (success);
  return success;
}

bool
EpcController::PgwRulesInstall (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool forceMeterInstall)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << forceMeterInstall);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = GetPgwTftDpId (pgwTftIdx);
  uint32_t pgwTftS5PortNo = m_pgwS5PortsNo.at (pgwTftIdx);
  NS_LOG_INFO ("Installing P-GW rules for bearer teid " << rInfo->GetTeid () <<
               " into P-GW TFT switch index " << pgwTftIdx);

  // Flags OFPFF_CHECK_OVERLAP and OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0006");

  // Print the cookie and buffer values in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Print downlink TEID and destination IPv4 address into tunnel metadata.
  uint64_t tunnelId = (uint64_t)rInfo->GetSgwS5Addr ().Get () << 32;
  tunnelId |= rInfo->GetTeid ();
  char tunnelIdStr [20];
  sprintf (tunnelIdStr, "0x%016lx", tunnelId);

  // Build the dpctl command string
  std::ostringstream cmd, act;
  cmd << "flow-mod cmd=add,table=0"
      << ",flags=" << flagsStr
      << ",cookie=" << cookieStr
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Check for meter entry.
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->HasDown ())
    {
      if (forceMeterInstall || !meterInfo->IsDownInstalled ())
        {
          // Install the per-flow meter entry.
          DpctlExecute (pgwTftDpId, meterInfo->GetDownAddCmd ());
          meterInfo->SetDownInstalled (true);
        }

      // Instruction: meter.
      act << " meter:" << rInfo->GetTeid ();
    }

  // Instruction: apply action: set tunnel ID, output port.
  act << " apply:set_field=tunn_id:" << tunnelIdStr
      << ",output=" << pgwTftS5PortNo;

  // Install one downlink dedicated bearer rule for each packet filter.
  Ptr<EpcTft> tft = rInfo->GetTft ();
  for (uint8_t i = 0; i < tft->GetNFilters (); i++)
    {
      EpcTft::PacketFilter filter = tft->GetFilter (i);
      if (filter.direction == EpcTft::UPLINK)
        {
          continue;
        }

      // Install rules for TCP traffic.
      if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
        {
          std::ostringstream match;
          match << " eth_type=0x800"
                << ",ip_proto=6"
                << ",ip_dst=" << filter.localAddress;

          if (tft->IsDefaultTft () == false)
            {
              match << ",ip_src=" << filter.remoteAddress
                    << ",tcp_src=" << filter.remotePortStart;
            }

          std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
          DpctlExecute (pgwTftDpId, cmdTcpStr);
        }

      // Install rules for UDP traffic.
      else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
        {
          std::ostringstream match;
          match << " eth_type=0x800"
                << ",ip_proto=17"
                << ",ip_dst=" << filter.localAddress;

          if (tft->IsDefaultTft () == false)
            {
              match << ",ip_src=" << filter.remoteAddress
                    << ",udp_src=" << filter.remotePortStart;
            }

          std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
          DpctlExecute (pgwTftDpId, cmdUdpStr);
        }
    }
  return true;
}

bool
EpcController::PgwRulesRemove (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool keepMeterFlag)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << keepMeterFlag);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = GetPgwTftDpId (pgwTftIdx);
  NS_LOG_INFO ("Removing P-GW rules for bearer teid " << rInfo->GetTeid () <<
               " from P-GW TFT switch index " << pgwTftIdx);

  // Print the cookie value in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Remove P-GW TFT flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,table=0"
      << ",cookie=" << cookieStr
      << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
  DpctlExecute (pgwTftDpId, cmd.str ());

  // Remove meter entry for this TEID.
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->IsDownInstalled ())
    {
      DpctlExecute (pgwTftDpId, meterInfo->GetDelCmd ());
      if (!keepMeterFlag)
        {
          meterInfo->SetDownInstalled (false);
        }
    }
  return true;
}

bool
EpcController::PgwTftBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid ());

  // Check for valid thresholds attributes.
  NS_ASSERT_MSG (m_tftSplitThs < m_tftBlockThs
                 && m_tftSplitThs > 2 * m_tftJoinThs,
                 "The split threshold should be smaller than the block "
                 "threshold and two times larger than the join threshold.");

  // For default bearers and for bearers with aggregated traffic:
  // let's accept it without guarantees.
  if (rInfo->IsDefault () || rInfo->IsAggregated ())
    {
      return true;
    }

  // Get the P-GW TFT stats calculator for this bearer.
  Ptr<OFSwitch13Device> device;
  Ptr<OFSwitch13StatsCalculator> stats;
  uint16_t tftIdx = rInfo->GetPgwTftIdx ();
  device = OFSwitch13Device::GetDevice (GetPgwTftDpId (tftIdx));
  stats = device->GetObject<OFSwitch13StatsCalculator> ();
  NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");

  // Non-aggregated bearers always install rules on P-GW TFT flow table.
  // Block the bearer if the table usage is exceeding the block threshold.
  double tableUsage =
    static_cast<double> (stats->GetEwmaFlowEntries ()) / m_tftTableSize;
  if (tableUsage >= m_tftBlockThs)
    {
      rInfo->SetBlocked (true, RoutingInfo::TFTTABLEFULL);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
                   " because the TFT flow tables is full.");
    }

  // If the load usage is exceeding the block threshold, handle the bearer
  // request based on the block policy as follow:
  // - If OFF (none): don't block the request.
  // - If ON (all)  : block the request.
  // - If AUTO (gbr): block only if GBR request.
  double loadUsage =
    stats->GetEwmaPipelineLoad ().GetBitRate () / m_tftMaxLoad.GetBitRate ();
  if (loadUsage >= m_tftBlockThs
      && (m_tftBlockPolicy == OperationMode::ON
          || (m_tftBlockPolicy == OperationMode::AUTO && rInfo->IsGbr ())))
    {
      rInfo->SetBlocked (true, RoutingInfo::TFTMAXLOAD);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
                   " because the TFT processing capacity is overloaded.");
    }
  return !rInfo->IsBlocked ();
}

void
EpcController::PgwTftCheckUsage (void)
{
  NS_LOG_FUNCTION (this);

  double maxEntries = 0.0, sumEntries = 0.0;
  double maxLoad = 0.0, sumLoad = 0.0;
  uint32_t maxLbLevel = (uint8_t)log2 (m_tftSwitches);
  uint16_t activeTfts = 1 << m_tftLevel;
  uint8_t nextLevel = m_tftLevel;

  Ptr<OFSwitch13Device> device;
  Ptr<OFSwitch13StatsCalculator> stats;
  for (uint16_t tftIdx = 1; tftIdx <= activeTfts; tftIdx++)
    {
      device = OFSwitch13Device::GetDevice (GetPgwTftDpId (tftIdx));
      stats = device->GetObject<OFSwitch13StatsCalculator> ();
      NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");

      double entries = stats->GetEwmaFlowEntries ();
      maxEntries = std::max (maxEntries, entries);
      sumEntries += entries;

      double load = stats->GetEwmaPipelineLoad ().GetBitRate ();
      maxLoad = std::max (maxLoad, load);
      sumLoad += load;
    }

  if (GetPgwAdaptiveMode () == OperationMode::AUTO)
    {
      double maxTableUsage = maxEntries / m_tftTableSize;
      double maxLoadUsage = maxLoad / m_tftMaxLoad.GetBitRate ();

      // We may increase the level when we hit the split threshold.
      if ((m_tftLevel < maxLbLevel)
          && (maxTableUsage >= m_tftSplitThs
              || maxLoadUsage >= m_tftSplitThs))
        {
          NS_LOG_INFO ("Increasing the adaptive mechanism level.");
          nextLevel++;
        }

      // We may decrease the level when we hit the joing threshold.
      else if ((m_tftLevel > 0)
               && (maxTableUsage < m_tftJoinThs)
               && (maxLoadUsage < m_tftJoinThs))
        {
          NS_LOG_INFO ("Decreasing the adaptive mechanism level.");
          nextLevel--;
        }
    }

  // Check if we need to update the adaptive mechanism level.
  uint32_t moved = 0;
  if (m_tftLevel != nextLevel)
    {
      // Identify and move bearers to the correct P-GW TFT switches.
      uint16_t futureTfts = 1 << nextLevel;
      for (uint16_t currIdx = 1; currIdx <= activeTfts; currIdx++)
        {
          RoutingInfoList_t bearers = RoutingInfo::GetInstalledList (currIdx);
          RoutingInfoList_t::iterator it;
          for (it = bearers.begin (); it != bearers.end (); ++it)
            {
              uint16_t destIdx = GetPgwTftIdx (*it, futureTfts);
              if (destIdx != currIdx)
                {
                  NS_LOG_INFO ("Moving bearer teid " << (*it)->GetTeid ());
                  PgwRulesRemove  (*it, currIdx, true);
                  PgwRulesInstall (*it, destIdx, true);
                  (*it)->SetPgwTftIdx (destIdx);
                  moved++;
                }
            }
        }

      // Update the adaptive mechanism level and the P-GW main switch.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=mods,table=0,prio=64 eth_type=0x800"
          << ",in_port=" << m_pgwSgiPortNo
          << ",ip_dst=" << EpcNetwork::m_ueAddr
          << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
          << " goto:" << nextLevel + 1;
      DpctlExecute (GetPgwMainDpId (), cmd.str ());
    }

  // Fire the P-GW TFT adaptation trace source.
  struct PgwTftStats tftStats;
  tftStats.tableSize = m_tftTableSize;
  tftStats.maxEntries = maxEntries;
  tftStats.sumEntries = sumEntries;
  tftStats.pipeCapacity = m_tftMaxLoad.GetBitRate ();
  tftStats.maxLoad = maxLoad;
  tftStats.sumLoad = sumLoad;
  tftStats.currentLevel = m_tftLevel;
  tftStats.nextLevel = nextLevel;
  tftStats.maxLevel = maxLbLevel;
  tftStats.bearersMoved = moved;
  tftStats.blockThrs = m_tftBlockThs;
  tftStats.joinThrs = m_tftJoinThs;
  tftStats.splitThrs = m_tftSplitThs;
  m_pgwTftStatsTrace (tftStats);

  m_tftLevel = nextLevel;
}

void
EpcController::StaticInitialize ()
{
  NS_LOG_FUNCTION_NOARGS ();

  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;

      // Populating the EPS QCI --> IP DSCP mapping table.
      // We are using EF (QCIs 1, 2, and 3) and AF41 (QCI 4) for GBR traffic,
      // AF11 (QCIs 5, 6, 7, and 8) and BE (QCI 9) for Non-GBR traffic.
      // See https://ericlajoie.com/epcqos.html for details.
      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_CONV_VOICE,
                        Ipv4Header::DSCP_EF));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_CONV_VIDEO,
                        Ipv4Header::DSCP_EF));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_GAMING,
                        Ipv4Header::DSCP_EF));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::GBR_NON_CONV_VIDEO,
                        Ipv4Header::DSCP_AF41));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_IMS,
                        Ipv4Header::DSCP_AF11));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_OPERATOR,
                        Ipv4Header::DSCP_AF11));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VOICE_VIDEO_GAMING,
                        Ipv4Header::DSCP_AF11));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_PREMIUM,
                        Ipv4Header::DSCP_AF11));

      EpcController::m_qciDscpTable.insert (
        std::make_pair (EpsBearer::NGBR_VIDEO_TCP_DEFAULT,
                        Ipv4Header::DscpDefault));

      // Populating the IP DSCP --> Output queue id mapping table.
      EpcController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_EF, 2));

      EpcController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF41, 1));

      EpcController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DSCP_AF11, 1));

      EpcController::m_dscpQueueTable.insert (
        std::make_pair (Ipv4Header::DscpDefault, 0));
    }
}

};  // namespace ns3
