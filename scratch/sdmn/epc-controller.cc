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

#include "sdmn-mme.h"
#include "epc-controller.h"
#include "epc-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcController");
NS_OBJECT_ENSURE_REGISTERED (EpcController);

// Initializing EpcController static members.
const uint16_t EpcController::m_flowTimeout = 15;
EpcController::QciDscpMap_t EpcController::m_qciDscpTable;
EpcController::QciDscpInitializer EpcController::initializer;

EpcController::EpcController ()
  : m_teidCount (0x0000000F)
{
  NS_LOG_FUNCTION (this);
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
    .AddAttribute ("VoipQueue",
                   "Enable VoIP QoS through queuing traffic management.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&EpcController::m_voipQos),
                   MakeBooleanChecker ())
    .AddAttribute ("NonGbrCoexistence",
                   "Enable the coexistence of GBR and Non-GBR traffic, "
                   "installing meters to limit Non-GBR traffic bit rate.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&EpcController::m_nonGbrCoexistence),
                   MakeBooleanChecker ())

    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerRequestTrace),
                     "ns3::EpcController::BearerTracedCallback")
    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerReleaseTrace),
                     "ns3::EpcController::BearerTracedCallback")
    .AddTraceSource ("SessionCreated", "The session created trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_sessionCreatedTrace),
                     "ns3::EpcController::SessionCreatedTracedCallback")
  ;
  return tid;
}

bool
EpcController::RequestDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer " << teid);

  // Is it a default bearer?
  if (rInfo->IsDefault ())
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource reservation nor reinstall the switch rules, as
      // default rules were supposed to remain installed during entire
      // simulation and must be Non-GBR.
      NS_ASSERT_MSG (rInfo->IsActive () && rInfo->IsInstalled (),
                     "Default bearer should be intalled and activated.");
      return true;
    }

  // Is it an active (aready configured) bearer?
  if (rInfo->IsActive ())
    {
      NS_ASSERT_MSG (rInfo->IsInstalled (), "Bearer should be installed.");
      NS_LOG_WARN ("Routing path for " << teid << " is already installed.");
      return true;
    }

  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");
  // So, this bearer must be inactive and we are goind to reuse it's metadata.
  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will inscrease the bearer
  // priority. Doing this, we avoid problems with old 'expiring' rules, and we
  // can even use new routing paths when necessary.

  // Let's first check for available resources and fire trace source
  bool accepted = TopologyBearerRequest (rInfo);
  m_bearerRequestTrace (accepted, rInfo);
  if (!accepted)
    {
      NS_LOG_INFO ("Bearer request blocked by controller.");
      return false;
    }

  // Everything is ok! Let's activate and install this bearer.
  rInfo->SetActive (true);
  NS_LOG_INFO ("Bearer request accepted by controller.");
  return TopologyInstallRouting (rInfo);
}

bool
EpcController::ReleaseDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "No routing information for teid.");

  // Is it a default bearer?
  if (rInfo->IsDefault ())
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource release, as default rules were supposed to remain
      // installed during entire simulation and must be Non-GBR.
      NS_ASSERT_MSG (rInfo->IsActive () && rInfo->IsInstalled (),
                     "Default bearer should be installed and activated.");
      return true;
    }

  // Check for inactive bearer
  if (rInfo->IsActive () == false)
    {
      return true;
    }

  rInfo->SetActive (false);
  rInfo->SetInstalled (false);
  bool success = TopologyBearerRelease (rInfo);
  m_bearerReleaseTrace (success, rInfo);
  return TopologyRemoveRouting (rInfo);
}

void
EpcController::NotifyPgwAttach (
  Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwSgiDev,
  Ipv4Address pgwSgiIp, uint32_t sgiPortNo, uint32_t s5PortNo,
  Ptr<NetDevice> webSgiDev, Ipv4Address webIp)
{
  NS_LOG_FUNCTION (this << pgwSgiIp << sgiPortNo << webIp);

  m_pgwDpId = pgwSwDev->GetDatapathId ();
  m_pgwS5Port = s5PortNo;

  // Configure SGi port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- P-GW default table -- [from higher to lower priority]
  //
  // IP packets coming from the Internet (SGi) and addressed to the LTE network
  // (S5) are sent to table 1, where TFT rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << sgiPortNo
        << " goto:1";
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdIn.str ());

  // IP packets coming from the LTE network (S5) and addressed to the Internet
  // (SGi) have the destination MAC address rewritten (this is necessary when
  // using logical ports) and are forward to the SGi interface port.
  Mac48Address webMac = Mac48Address::ConvertFrom (webSgiDev->GetAddress ());
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
         << ",in_port=" << s5PortNo
         << ",ip_dst=" << webIp
         << " write:set_field=eth_dst:" << webMac
         << ",output=" << sgiPortNo;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdOut.str ());

  // ARP request packets. Send to controller.
  DpctlSchedule (pgwSwDev->GetDatapathId (),
                 "flow-mod cmd=add,table=0,prio=16 eth_type=0x0806 "
                 "apply:output=ctrl");

  // Table miss entry. Send to controller.
  DpctlSchedule (pgwSwDev->GetDatapathId (),
                 "flow-mod cmd=add,table=0,prio=0 "
                 "apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- P-GW TFT downlink table -- [from higher to lower priority]
  //
  // Entries will be installed here by InstallPgwTftRules function.
}

void
EpcController::NotifyS5Attach (
  Ptr<OFSwitch13Device> swtchDev, uint32_t portNo, Ptr<NetDevice> gwDev,
  Ipv4Address gwIp)
{
  NS_LOG_FUNCTION (this << swtchDev << portNo << gwDev << gwIp);

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
         << ",ip_dst=" << gwIp
         << " write:output=" << portNo
         << " goto:4";
  DpctlSchedule (swtchDev->GetDatapathId (), cmdOut.str ());
}

void
EpcController::NotifySwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);
}

void
EpcController::TopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);
}

void
EpcController::NotifySessionCreated (
  uint64_t imsi, uint16_t cellId, Ipv4Address sgwAddr, Ipv4Address pgwAddr,
  BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << sgwAddr << pgwAddr);

  // Create and save routing information for default bearer
  ContextBearer_t defaultBearer = bearerList.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");

  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo == 0, "Existing routing for default bearer " << teid);

  rInfo = CreateObject<RoutingInfo> (teid);
  rInfo->m_imsi = imsi;
  rInfo->m_cellId = cellId;
  rInfo->m_pgwAddr = pgwAddr;
  rInfo->m_sgwAddr = sgwAddr;
  rInfo->m_priority = 0x7F;               // Priority for default bearer
  rInfo->m_timeout = 0;                   // No timeout for default bearer
  rInfo->m_isInstalled = false;           // Bearer rules not installed yet
  rInfo->m_isActive = true;               // Default bearer is always active
  rInfo->m_isDefault = true;              // This is a default bearer
  rInfo->m_bearer = defaultBearer;

  // For default bearer, no Meter nor Reserver metadata.
  // For logic consistence, let's check for available resources.
  bool accepted = TopologyBearerRequest (rInfo);
  m_bearerRequestTrace (true, rInfo);
  NS_ASSERT_MSG (accepted, "Default bearer must be accepted.");

  // Install rules for default bearer
  if (!TopologyInstallRouting (rInfo))
    {
      NS_LOG_ERROR ("TEID rule installation failed!");
    }

  // For other dedicated bearers, let's create and save it's routing metadata
  // NOTE: starting at the second element
  BearerList_t::iterator it = bearerList.begin ();
  for (it++; it != bearerList.end (); it++)
    {
      ContextBearer_t dedicatedBearer = *it;
      teid = dedicatedBearer.sgwFteid.teid;

      rInfo = CreateObject<RoutingInfo> (teid);
      rInfo->m_imsi = imsi;
      rInfo->m_cellId = cellId;
      rInfo->m_pgwAddr = pgwAddr;
      rInfo->m_sgwAddr = sgwAddr;
      rInfo->m_priority = 0x1FFF;           // Priority for dedicated bearer
      rInfo->m_timeout = m_flowTimeout;     // Timeout for dedicated bearer
      rInfo->m_isInstalled = false;         // Switch rules not installed
      rInfo->m_isActive = false;            // Dedicated bearer not active
      rInfo->m_isDefault = false;           // This is a dedicated bearer
      rInfo->m_bearer = dedicatedBearer;

      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();

      // For all GBR beares, create the GBR metadata.
      if (rInfo->IsGbr ())
        {
          Ptr<GbrInfo> gbrInfo = CreateObject<GbrInfo> (rInfo);
          rInfo->AggregateObject (gbrInfo);

          // Set the appropriated DiffServ DSCP value for this bearer.
          gbrInfo->m_dscp = EpcController::GetDscpValue (rInfo->GetQciInfo ());
        }

      // If necessary, create the meter metadata for maximum bit rate.
      if (gbrQoS.mbrDl || gbrQoS.mbrUl)
        {
          Ptr<MeterInfo> meterInfo = CreateObject<MeterInfo> (rInfo);
          rInfo->AggregateObject (meterInfo);
        }
    }

  // Fire trace source notifying session created.
  m_sessionCreatedTrace (imsi, cellId, bearerList);
}

uint16_t
EpcController::GetDscpValue (EpsBearer::Qci qci)
{
  NS_LOG_FUNCTION_NOARGS ();

  QciDscpMap_t::iterator it;
  it = EpcController::m_qciDscpTable.find (qci);
  if (it != EpcController::m_qciDscpTable.end ())
    {
      NS_LOG_DEBUG ("Found DSCP value: " << qci << " - " << it->second);
      return it->second;
    }
  NS_FATAL_ERROR ("No DSCP mapped value for QCI " << qci);
}

void
EpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  Object::DoDispose ();
}

void
EpcController::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Chain up.
  ObjectBase::NotifyConstructionCompleted ();
}

void
EpcController::InstallPgwTftRules (Ptr<RoutingInfo> rInfo, uint32_t buffer)
{
  NS_LOG_FUNCTION (this << rInfo << buffer);

  // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS
  std::string flagsStr ("0x0007");

  // Printing the cookie and buffer values in dpctl string format
  char cookieStr [9], bufferStr [12];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
  sprintf (bufferStr, "%u",   buffer);

  // Building the dpctl command + arguments string (withoug p
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=1"
      << ",buffer=" << bufferStr
      << ",flags=" << flagsStr
      << ",cookie=" << cookieStr
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Printing TEID and destination IPv4 address into tunnel metadata
  uint64_t tunnelId = (uint64_t)rInfo->GetSgwAddress ().Get () << 32;
  tunnelId |= rInfo->GetTeid ();
  char tunnelIdStr [12];
  sprintf (tunnelIdStr, "0x%016lX", tunnelId);

  // Build instruction string
  std::ostringstream act;
  act << " apply:set_field=tunn_id:" << tunnelIdStr
      << ",output=" << m_pgwS5Port;

  if (rInfo->IsDefault ())
    {
      // TODO Install TFT for default bearer
      // std::ostringstream match;
      // match << " eth_type=0x800"
      //       << ",ip_src=" << filter.remoteAddress
      //       << ",ip_dst=" << filter.localAddress
      //       << ",ip_proto=6"
      //       << ",tcp_src=" << filter.remotePortStart
      //       << ",tcp_dst=" << filter.localPortStart;
      // std::string cmdStr = cmd.str () + match.str () + act.str ();
      // DpctlExecute (m_pgwDpId, cmdStr);
      return;
    }

  // Install one downlink dedicated bearer rule for each packet filter
  Ptr<EpcTft> tft = rInfo->GetTft ();
  for (uint8_t i = 0; i < tft->GetNumFilters (); i++)
    {
      EpcTft::PacketFilter filter = tft->GetFilter (i);
      if (filter.direction == EpcTft::UPLINK)
        {
          continue;
        }

      // Install rules for TCP traffic
      if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
        {
          std::ostringstream match;
          match << " eth_type=0x800"
                << ",ip_src=" << filter.remoteAddress
                << ",ip_dst=" << filter.localAddress
                << ",ip_proto=6"
                << ",tcp_src=" << filter.remotePortStart
                << ",tcp_dst=" << filter.localPortStart;
          std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
          DpctlExecute (m_pgwDpId, cmdTcpStr); // FIXME remover m_pgwDpId
        }

      // Install rules for UDP traffic
      else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
        {
          std::ostringstream match;
          match << " eth_type=0x800"
                << ",ip_src=" << filter.remoteAddress
                << ",ip_dst=" << filter.localAddress
                << ",ip_proto=17"
                << ",udp_src=" << filter.remotePortStart
                << ",udp_dst=" << filter.localPortStart;
          std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
          DpctlExecute (m_pgwDpId, cmdUdpStr);
        }
    }
}

void
EpcController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // This function is called after a successfully handshake between controller
  // and switch. So, let's start configure the switch tables in accordance to
  // our methodology.

  // Configure the switch to buffer packets and send only the first 128 bytes
  // to the controller.
  DpctlExecute (swtch, "set-config miss=128");

  // TODO: Por enquanto o S-GW está registrado aqui neste controlador, mas
  // deveria ser no controlador do SDRAN. Corrigir isso assim que o SDRAN
  // controller for implantado.

  // FIXME Find a better way to identify which nodes should or not scape here.
  if (swtch->GetDpId () == m_pgwDpId)
    {
      // Don't install the following rules on the P-GW switch.
      return;
    }

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
  if (m_nonGbrCoexistence)
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
  if (m_voipQos)
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
  ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *m = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_INFO ("Packet in match: " << m);
  free (m);

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);

  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");
  return 0;
}

ofl_err
EpcController::HandleFlowRemoved (
  ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint8_t table = msg->stats->table_id;
  uint32_t teid = msg->stats->cookie;
  uint16_t prio = msg->stats->priority;

  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // Since handlers must free the message when everything is ok,
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // Only entries at Classification table (#1) can expire due idle timeout or
  // can be removed by TopologyRemoveRouting (if they reference any per-flow
  // meter entry). So, other flows cannot be removed.
  if (table != 1)
    {
      NS_FATAL_ERROR ("Flow not supposed to be removed from table " << table);
      return 0;
    }

  // Check for existing routing information for this bearer
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Flow " << teid << " removed for stopped application.");
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Flow " << teid << " removed for old rule.");
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we need to reinstall the rules with higher priority to
  // avoid problems.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  if (rInfo->IsActive ())
    {
      NS_LOG_WARN ("Flow " << teid << " is still active. Reinstall rules...");
      if (!TopologyInstallRouting (rInfo))
        {
          NS_LOG_ERROR ("TEID rule installation failed!");
        }
      return 0;
    }
  NS_ABORT_MSG ("Should not get here :/");
}

EpcController::QciDscpInitializer::QciDscpInitializer ()
{
  NS_LOG_FUNCTION_NOARGS ();

  std::pair<EpsBearer::Qci, uint16_t> entries [9];
  entries [0] = std::make_pair (
      EpsBearer::GBR_CONV_VOICE, Ipv4Header::DSCP_EF);
  entries [1] = std::make_pair (
      EpsBearer::GBR_CONV_VIDEO, Ipv4Header::DSCP_AF12);
  entries [2] = std::make_pair (
      EpsBearer::GBR_GAMING, Ipv4Header::DSCP_AF21);
  entries [3] = std::make_pair (
      EpsBearer::GBR_NON_CONV_VIDEO, Ipv4Header::DSCP_AF11);

  // Currently we are mapping all Non-GBR bearers to best effort DSCP traffic.
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

};  // namespace ns3
