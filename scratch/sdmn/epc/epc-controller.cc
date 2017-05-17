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
uint32_t EpcController::m_teidCount = 0x0000000F;
EpcController::QciDscpMap_t EpcController::m_qciDscpTable;

EpcController::EpcController ()
  : m_tftTableSize (std::numeric_limits<uint32_t>::max ()),
    m_tftPlCapacity (DataRate (std::numeric_limits<uint64_t>::max ()))
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
    .AddAttribute ("InternalTimeout",
                   "The interval between internal periodic operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&EpcController::m_timeout),
                   MakeTimeChecker ())
    .AddAttribute ("NonGbrCoexistence",
                   "Enable the coexistence of GBR and Non-GBR traffic, "
                   "installing meters to limit Non-GBR traffic bit rate.",
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_nonGbrCoex),
                   MakeEnumChecker (EpcController::OFF, "off",
                                    EpcController::ON,  "on"))
    .AddAttribute ("NumPgwTftSwitches",
                   "The number of P-GW TFT switches available for use.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&EpcController::m_tftSwitches),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PgwLoadBalancing",
                   "Configure the P-GW load balancing mechanism.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_pgwLoadBal),
                   MakeEnumChecker (EpcController::OFF,  "off",
                                    EpcController::ON,   "on",
                                    EpcController::AUTO, "auto"))
    .AddAttribute ("PgwThresholdFactor",
                   "The threshold factor used to trigger the load balancing.",
                   DoubleValue (0.8),
                   MakeDoubleAccessor (&EpcController::m_tftThrshFactor),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("S5TrafficAggregation",
                   "Configure the S5 traffic aggregation mechanism.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (EpcController::OFF),
                   MakeEnumAccessor (&EpcController::m_s5Aggreg),
                   MakeEnumChecker (EpcController::OFF,  "off",
                                    EpcController::ON,   "on",
                                    EpcController::AUTO, "auto"))
    .AddAttribute ("VoipQueue",
                   "Enable VoIP QoS through queuing traffic management.",
                   EnumValue (EpcController::ON),
                   MakeEnumAccessor (&EpcController::m_voipQos),
                   MakeEnumChecker (EpcController::OFF, "off",
                                    EpcController::ON,  "on"))

    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerReleaseTrace),
                     "ns3::EpcController::BearerTracedCallback")
    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerRequestTrace),
                     "ns3::EpcController::BearerTracedCallback")
    .AddTraceSource ("LoadBalancing", "The load balancing trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_loadBalancingTrace),
                     "ns3::EpcController::LoadBalancingTracedCallback")
    .AddTraceSource ("SessionCreated", "The session created trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_sessionCreatedTrace),
                     "ns3::EpcController::SessionCreatedTracedCallback")
  ;
  return tid;
}

bool
EpcController::RequestDedicatedBearer (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

  // This bearer must be inactive as we are going to reuse it's metadata.
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Update the P-GW TFT index for this bearer.
  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));

  // Check for the traffic aggregation mechanism enabled.
  if (GetS5TrafficAggregation () && !rInfo->IsDefault ())
    {
      NS_LOG_INFO ("Aggregating the traffic of bearer teid " << teid);
      rInfo->SetAggregated (true);
    }

  // Let's first check for available resources on P-GW and backhaul switches.
  bool accepted = false;
  if (PgwTftBearerRequest (rInfo))
    {
      accepted = TopologyBearerRequest (rInfo);
    }
  m_bearerRequestTrace (accepted, rInfo);
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
  return InstallBearer (rInfo);
}

bool
EpcController::ReleaseDedicatedBearer (EpsBearer bearer, uint32_t teid)
{
  NS_LOG_FUNCTION (this << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");

  bool released = TopologyBearerRelease (rInfo);
  m_bearerReleaseTrace (released, rInfo);
  NS_LOG_INFO ("Bearer released by controller.");

  // Everything is ok! Let's deactivate and remove this bearer.
  rInfo->SetActive (false);
  return RemoveBearer (rInfo);
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
  // network are sent to the table corresponding to the current P-GW load
  // balancing level.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << pgwSgiPortNo
        << ",ip_dst=" << EpcNetwork::m_ueAddr
        << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
        << " goto:" << m_tftLbLevel + 1;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdIn.str ());

  // Table miss entry. Send to controller.
  DpctlSchedule (pgwSwDev->GetDatapathId (), "flow-mod cmd=add,table=0,prio=0"
                 " apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 to N -- P-GW LB tables -- [from higher to lower priority]
  //
  // Tables used when balancing the load among TFT switches.
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
  NS_ASSERT_MSG (pgwTftCounter < m_tftSwitches, "No more P-GW TFTs allowed.");
  m_pgwDpIds.push_back (pgwSwDev->GetDatapathId ());
  m_pgwS5PortsNo.push_back (pgwS5PortNo);

  uint32_t tableSize = pgwSwDev->GetFlowTableSize ();
  DataRate plCapacity = pgwSwDev->GetPipelineCapacity ();
  m_tftTableSize = std::min (m_tftTableSize, tableSize);
  m_tftPlCapacity = std::min (m_tftPlCapacity, plCapacity);

  // Configuring the P-GW main switch to forward traffic to this TFT switch
  // considering all possible load balancing levels.
  for (uint16_t tft = m_tftSwitches; pgwTftCounter + 1 <= tft; tft /= 2)
    {
      uint16_t lbLevel = (uint16_t)log2 (tft);
      uint16_t ipMask = (1 << lbLevel) - 1;
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64,table=" << lbLevel + 1
          << " eth_type=0x800,ip_dst=0.0.0." << pgwTftCounter
          << "/0.0.0." << ipMask
          << " apply:output=" << pgwMainPortNo;
      DpctlSchedule (m_pgwDpIds.at (0), cmd.str ());
    }

  // -------------------------------------------------------------------------
  // Table 0 -- P-GW TFT default table -- [from higher to lower priority]
  //
  // Entries will be installed here by InstallPgwSwitchRules function.
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

void
EpcController::NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);
}

void
EpcController::NotifyTopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);
}

void
EpcController::NotifyPgwBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (devices.GetN () == m_pgwDpIds.size ()
                 && devices.GetN () == (m_tftSwitches + 1U),
                 "Inconsistent number of P-GW OpenFlow switches.");

  // Update table size and pipeline capacity using the threshold factor.
  uint64_t bitrate = m_tftPlCapacity.GetBitRate () * m_tftThrshFactor;
  m_tftPlCapacity = DataRate (bitrate);
  m_tftTableSize = std::round (m_tftTableSize * m_tftThrshFactor);
}

EpcS5SapPgw*
EpcController::GetS5SapPgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5SapPgw;
}

EpcController::FeatureStatus
EpcController::GetNonGbrCoexistence (void) const
{
  NS_LOG_FUNCTION (this);

  return m_nonGbrCoex;
}

EpcController::FeatureStatus
EpcController::GetPgwLoadBalancing (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwLoadBal;
}

EpcController::FeatureStatus
EpcController::GetS5TrafficAggregation (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5Aggreg;
}

EpcController::FeatureStatus
EpcController::GetVoipQos (void) const
{
  NS_LOG_FUNCTION (this);

  return m_voipQos;
}

uint16_t
EpcController::GetDscpValue (EpsBearer::Qci qci)
{
  NS_LOG_FUNCTION_NOARGS ();

  QciDscpMap_t::iterator it;
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
  switch (GetPgwLoadBalancing ())
    {
    case FeatureStatus::OFF:
      {
        m_tftLbLevel = 0;
        break;
      }
    case FeatureStatus::ON:
      {
        m_tftLbLevel = (uint8_t)log2 (m_tftSwitches);
        break;
      }
    case FeatureStatus::AUTO:
      {
        m_tftLbLevel = 0;

        // Schedule the first P-GW load check.
        Simulator::Schedule (m_timeout, &EpcController::CheckPgwTftLoad, this);
        break;
      }
    }

  // Chain up.
  OFSwitch13Controller::NotifyConstructionCompleted ();
}

void
EpcController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // This function is called after a successfully handshake between the EPC
  // controller and any switch on the EPC network (including the P-GW user
  // plane and the OpenFlow backhaul network). For the P-GW switches, all
  // entries will be installed by NotifyPgw*Attach and InstallPgwSwitchRules
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
  // Entries will be installed here by TopologyInstallRouting function.

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // Entries will be installed here by NewS5Attach function.
  // Entries will be installed here by NotifyTopologyBuilt function.

  // GTP packets classified at previous table. Write the output group into
  // action set based on metadata field. Send the packet to Coexistence QoS
  // table.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x1"
                " write:group=1 goto:3");
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=64"
                " meta=0x2"
                " write:group=2 goto:3");

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=2,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 3 -- Coexistence QoS table -- [from higher to lower priority]
  //
  if (GetNonGbrCoexistence () == FeatureStatus::ON)
    {
      // Non-GBR packets indicated by DSCP field. Apply corresponding Non-GBR
      // meter band. Send the packet to Output table.
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,ip_dscp=0,meta=0x1"
                    " meter:1 goto:4");
      DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=16"
                    " eth_type=0x800,ip_dscp=0,meta=0x2"
                    " meter:2 goto:4");
    }

  // Table miss entry. Send the packet to Output table
  DpctlExecute (swtch, "flow-mod cmd=add,table=3,prio=0 goto:4");

  // -------------------------------------------------------------------------
  // Table 4 -- Output table -- [from higher to lower priority]
  //
  if (GetVoipQos () == FeatureStatus::ON)
    {
      int dscpVoip = EpcController::GetDscpValue (EpsBearer::GBR_CONV_VOICE);

      // VoIP packets. Write the high-priority output queue #1.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,table=4,prio=16"
          << " eth_type=0x800,ip_dscp=" << dscpVoip
          << " write:queue=1";
      DpctlExecute (swtch, cmd.str ());
    }

  // Table miss entry. No instructions. This will trigger action set execute.
  DpctlExecute (swtch, "flow-mod cmd=add,table=4,prio=0");
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
  // expiration, and we need to reinstall the rules with higher priority to
  // avoid problems.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  if (rInfo->IsActive ())
    {
      NS_LOG_WARN ("Rule removed for active bearer teid " << teid << ". " <<
                   "Reinstall rules...");
      bool installed = InstallBearer (rInfo);
      NS_ASSERT_MSG (installed, "Rules reinstallation failed!");
      return 0;
    }
  NS_ABORT_MSG ("Should not get here :/");
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

uint16_t
EpcController::GetPgwTftIdx (
  Ptr<const RoutingInfo> rInfo, uint16_t activeTfts) const
{
  NS_LOG_FUNCTION (this << rInfo << activeTfts);

  if (activeTfts == 0)
    {
      activeTfts = 1 << m_tftLbLevel;
    }
  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  return 1 + (ueInfo->GetUeAddr ().Get () % activeTfts);
}

bool
EpcController::PgwTftBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid ());

  // Get the P-GW TFT stats calculator responsible for this bearer.
  uint16_t tftIdx = rInfo->GetPgwTftIdx ();
  Ptr<OFSwitch13StatsCalculator> stats = OFSwitch13Device::GetDevice (
      m_pgwDpIds.at (tftIdx))->GetObject<OFSwitch13StatsCalculator> ();
  NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");

  // Block if table is full or if current load is exceeding pipeline capacity.
  bool accept = true;
  if (stats->GetEwmaFlowEntries () >= m_tftTableSize)
    {
      accept = false;
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
                   " because the flow tables is full.");
    }
  else if (stats->GetEwmaPipelineLoad () >= m_tftPlCapacity)
    {
      accept = false;
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
                   " because the load is exceeding pipeline capacity.");
    }
  return accept;
}

bool
EpcController::InstallPgwSwitchRules (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool forceMeterInstall)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << forceMeterInstall);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = m_pgwDpIds.at (pgwTftIdx);
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
EpcController::RemovePgwSwitchRules (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool keepMeterFlag)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << keepMeterFlag);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = m_pgwDpIds.at (pgwTftIdx);
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
EpcController::InstallBearer (Ptr<RoutingInfo> rInfo)
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
  success &= InstallPgwSwitchRules (rInfo);
  success &= TopologyInstallRouting (rInfo);

  rInfo->SetInstalled (success);
  return success;
}

bool
EpcController::RemoveBearer (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  if (rInfo->IsAggregated ())
    {
      // No rules to remove for aggregated traffic.
      rInfo->SetAggregated (false);
      return true;
    }

  // Remove the rules.
  bool success = true;
  success &= RemovePgwSwitchRules (rInfo);
  success &= TopologyRemoveRouting (rInfo);

  rInfo->SetInstalled (!success);
  return success;
}

void
EpcController::CheckPgwTftLoad (void)
{
  NS_LOG_FUNCTION (this);

  uint32_t maxEntries = 0, sumEntries = 0;
  uint64_t maxLoad    = 0, sumLoad    = 0;
  uint32_t maxLbLevel = (uint8_t)log2 (m_tftSwitches);
  uint16_t activeTfts = 1 << m_tftLbLevel;
  uint8_t  nextLbLevel = m_tftLbLevel;

  Ptr<OFSwitch13Device> device;
  Ptr<OFSwitch13StatsCalculator> stats;
  for (uint16_t tftIdx = 1; tftIdx <= activeTfts; tftIdx++)
    {
      device = OFSwitch13Device::GetDevice (m_pgwDpIds.at (tftIdx));
      stats = device->GetObject<OFSwitch13StatsCalculator> ();
      NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");

      uint32_t entries = stats->GetEwmaFlowEntries ();
      maxEntries = std::max (maxEntries, entries);
      sumEntries += entries;

      uint64_t load = stats->GetEwmaPipelineLoad ().GetBitRate ();
      maxLoad = std::max (maxLoad, load);
      sumLoad += load;
    }

  // We may increase the level when we hit the maximum number of flow entries
  // or pipeline capacity on any P-GW TFT switch.
  uint64_t tftPlBitrate = m_tftPlCapacity.GetBitRate ();
  if ((m_tftLbLevel < maxLbLevel)
      && (maxEntries >= m_tftTableSize || maxLoad >= tftPlBitrate))
    {
      NS_LOG_INFO ("Increasing the load balancing level.");
      nextLbLevel++;
    }
  // We may decrease the level when we can accommodate the current load and
  // flow entries on the lower level using up to 60% of resources. We expect
  // the the entries and load will be equally distributed among switches
  // because the traffic pattern is the same for all UEs.
  else if ((m_tftLbLevel > 0)
           && (sumLoad < (0.6 * (activeTfts >> 1) * tftPlBitrate))
           && (sumEntries < (0.6 * (activeTfts >> 1) * m_tftTableSize)))
    {
      NS_LOG_INFO ("Decreasing the load balancing level.");
      nextLbLevel--;
    }

  // Check if we need to update the load balancing level.
  uint32_t moved = 0;
  if (m_tftLbLevel != nextLbLevel)
    {
      // Identify and move bearers to the correct P-GW TFT switches.
      uint16_t futureTfts = 1 << nextLbLevel;
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
                  RemovePgwSwitchRules  (*it, currIdx, true);
                  InstallPgwSwitchRules (*it, destIdx, true);
                  (*it)->SetPgwTftIdx (destIdx);
                  moved++;
                }
            }
        }

      // Update the load balancing level and the P-GW main switch.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=mods,table=0,prio=64 eth_type=0x800"
          << ",in_port=" << m_pgwSgiPortNo
          << ",ip_dst=" << EpcNetwork::m_ueAddr
          << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
          << " goto:" << nextLbLevel + 1;
      DpctlExecute (m_pgwDpIds.at (0), cmd.str ());
    }

  // Fire the load balancing trace source.
  struct LoadBalancingStats lbStats;
  lbStats.currentLevel = m_tftLbLevel;
  lbStats.nextLevel    = nextLbLevel;
  lbStats.maxLevel     = maxLbLevel;
  lbStats.tableSize    = m_tftTableSize;
  lbStats.maxEntries   = maxEntries;
  lbStats.avgEntries   = sumEntries / activeTfts;
  lbStats.pipeCapacity = m_tftPlCapacity;
  lbStats.maxLoad      = DataRate (maxLoad);
  lbStats.avgLoad      = DataRate (sumLoad / activeTfts);
  lbStats.bearersMoved = moved;
  m_loadBalancingTrace (lbStats);

  // Finally, update the level and schedule the next P-GW load check.
  m_tftLbLevel = nextLbLevel;
  Simulator::Schedule (m_timeout, &EpcController::CheckPgwTftLoad, this);
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
      // Check for available TEID.
      NS_ABORT_IF (EpcController::m_teidCount == 0xFFFFFFFF);
      uint32_t teid = ++EpcController::m_teidCount;
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

  rInfo = CreateObject<RoutingInfo> (teid);
  rInfo->SetImsi (imsi);
  rInfo->SetPgwS5Addr (m_pgwS5Addr);
  rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
  rInfo->SetPriority (0x7F);             // Priority for default bearer
  rInfo->SetTimeout (0);                 // No timeout for default bearer
  rInfo->SetDefault (true);              // This is a default bearer
  rInfo->SetInstalled (false);           // Bearer rules not installed yet
  rInfo->SetActive (true);               // Default bearer is always active
  rInfo->SetAggregated (false);          // Default bearer never aggregates
  rInfo->SetBearerContext (defaultBearer);
  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
  TopologyBearerCreated (rInfo);

  // For default bearer, no meter nor GBR metadata.
  // For logic consistence, let's check for available resources.
  bool accepted = true;
  accepted &= PgwTftBearerRequest (rInfo);
  accepted &= TopologyBearerRequest (rInfo);
  NS_ASSERT_MSG (accepted, "Default bearer must be accepted.");
  m_bearerRequestTrace (accepted, rInfo);

  // Install rules for default bearer.
  bool installed = InstallBearer (rInfo);
  NS_ASSERT_MSG (installed, "Default bearer must be installed.");

  // For other dedicated bearers, let's create and save it's routing metadata.
  // (starting at the second element of res.bearerContextsCreated).
  BearerContextList_t::iterator it = res.bearerContextsCreated.begin ();
  for (it++; it != res.bearerContextsCreated.end (); it++)
    {
      BearerContext_t dedicatedBearer = *it;
      teid = dedicatedBearer.sgwFteid.teid;

      rInfo = CreateObject<RoutingInfo> (teid);
      rInfo->SetImsi (imsi);
      rInfo->SetPgwS5Addr (m_pgwS5Addr);
      rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
      rInfo->SetPriority (0x1FFF);          // Priority for dedicated bearer
      rInfo->SetTimeout (m_flowTimeout);    // Timeout for dedicated bearer
      rInfo->SetDefault (false);            // This is a dedicated bearer
      rInfo->SetInstalled (false);          // Bearer rules not installed yet
      rInfo->SetActive (false);             // Dedicated bearer not active
      rInfo->SetAggregated (false);         // Dedicated bearer not aggregated
      rInfo->SetBearerContext (dedicatedBearer);
      rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
      TopologyBearerCreated (rInfo);

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
EpcController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
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
EpcController::StaticInitialize ()
{
  NS_LOG_FUNCTION_NOARGS ();

  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;

      std::pair<EpsBearer::Qci, uint16_t> entries [9];
      entries [0] = std::make_pair (
          EpsBearer::GBR_CONV_VOICE, Ipv4Header::DSCP_EF);
      entries [1] = std::make_pair (
          EpsBearer::GBR_CONV_VIDEO, Ipv4Header::DSCP_AF12);
      entries [2] = std::make_pair (
          EpsBearer::GBR_GAMING, Ipv4Header::DSCP_AF21);
      entries [3] = std::make_pair (
          EpsBearer::GBR_NON_CONV_VIDEO, Ipv4Header::DSCP_AF11);

      // Mapping all Non-GBR bearers to best effort DSCP traffic.
      entries [4] = std::make_pair (
          EpsBearer::NGBR_IMS, Ipv4Header::DscpDefault);
      entries [5] = std::make_pair (
          EpsBearer::NGBR_VIDEO_TCP_OPERATOR, Ipv4Header::DscpDefault);
      entries [6] = std::make_pair (
          EpsBearer::NGBR_VOICE_VIDEO_GAMING, Ipv4Header::DscpDefault);
      entries [7] = std::make_pair (
          EpsBearer::NGBR_VIDEO_TCP_PREMIUM, Ipv4Header::DscpDefault);
      entries [8] = std::make_pair (
          EpsBearer::NGBR_VIDEO_TCP_DEFAULT, Ipv4Header::DscpDefault);

      std::pair<QciDscpMap_t::iterator, bool> ret;
      for (int i = 0; i < 9; i++)
        {
          ret = EpcController::m_qciDscpTable.insert (entries [i]);
          NS_ASSERT_MSG (ret.second, "Can't insert DSCP map value.");
        }
    }
}

};  // namespace ns3
