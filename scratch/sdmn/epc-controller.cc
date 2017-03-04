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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcController");
NS_OBJECT_ENSURE_REGISTERED (EpcController);

const uint16_t EpcController::m_dedicatedTmo = 15;

EpcController::QciDscpMap_t EpcController::m_qciDscpTable;
EpcController::QciDscpInitializer EpcController::initializer;

EpcController::EpcController ()
  : m_admissionStats (0),
    m_teidCount (0x0000000F),
    m_s11SapMme (0)
{
  NS_LOG_FUNCTION (this);

  // Creating the admission stats calculator for this OpenFlow controller
  m_admissionStats = CreateObject<AdmissionStatsCalculator> ();

  // The S-GW side of S11 AP
  m_s11SapSgw = new MemberEpcS11SapSgw<EpcController> (this);

  // TODO In the future, this will be moved to the RAN controller.
  // Connect the MME to the S-GW control plane via S11 interface.
  EpcMme::Get ()->SetS11SapSgw (GetS11SapSgw ());
  SetS11SapMme (EpcMme::Get ()->GetS11SapMme ());
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
EpcController::NewPgwAttach (
  Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwSgiDev,
  Ipv4Address pgwSgiIp, uint32_t sgiPortNo, uint32_t s5PortNo,
  Ptr<NetDevice> webSgiDev, Ipv4Address webIp)
{
  NS_LOG_FUNCTION (this << pgwSgiIp << sgiPortNo << webIp);

  m_pgwDpId = pgwSwDev->GetDatapathId ();
  m_pgwS5Port = s5PortNo;
  SaveArpEntry (pgwSgiIp, Mac48Address::ConvertFrom (pgwSgiDev->GetAddress ()));
  SaveArpEntry (webIp, Mac48Address::ConvertFrom (webSgiDev->GetAddress ()));

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
  Mac48Address webMac = GetArpEntry (webIp);
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
EpcController::NewS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
                            Ptr<NetDevice> gwDev, Ipv4Address gwIp)
{
  NS_LOG_FUNCTION (this << swtchDev << portNo << gwDev << gwIp);

  SaveArpEntry (gwIp, Mac48Address::ConvertFrom (gwDev->GetAddress ()));

  // Configure S5 port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // GTP packets entering the ring network from any EPC port. Send to the
  // Classification table.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64,flags=0x0007"
        << " eth_type=0x800,ip_proto=17,udp_src=2152,udp_dst=2152"
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
EpcController::NewSwitchConnection (Ptr<ConnectionInfo> cInfo)
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
  uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address pgwAddr,
  BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr << pgwAddr);

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
  rInfo->m_enbAddr = enbAddr;
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
      rInfo->m_enbAddr = enbAddr;
      rInfo->m_priority = 0x1FFF;           // Priority for dedicated bearer
      rInfo->m_timeout = m_dedicatedTmo;    // Timeout for dedicated bearer
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
  m_sessionCreatedTrace (imsi, cellId, enbAddr, pgwAddr, bearerList);
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

  m_admissionStats = 0;
  m_arpTable.clear ();
  m_enbInfoByCellId.clear ();

  delete (m_s11SapSgw);

  // Chain up.
  Object::DoDispose ();
}

void
EpcController::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Connect the admission stats calculator.
  TraceConnectWithoutContext (
    "BearerRequest", MakeCallback (
      &AdmissionStatsCalculator::NotifyBearerRequest, m_admissionStats));

  TraceConnectWithoutContext (
    "BearerRelease", MakeCallback (
      &AdmissionStatsCalculator::NotifyBearerRelease, m_admissionStats));

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
  uint64_t tunnelId = (uint64_t)rInfo->GetEnbAddr ().Get () << 32;
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
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=32 eth_type=0x800,"
                "ip_proto=17,udp_src=2152,udp_dst=2152"
                " goto:2");

  // ARP request packets. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=16 eth_type=0x0806"
                " apply:output=ctrl");

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

  ofl_match_tlv *tlv;
  enum ofp_packet_in_reason reason = msg->reason;
  if (reason == OFPR_NO_MATCH)
    {
      char *m = ofl_structs_match_to_string (msg->match, 0);
      NS_LOG_INFO ("Packet in match: " << m);
      free (m);

      NS_ABORT_MSG ("This packet in was not supposed to be sent to this "
                    "controller. Aborting...");
    }
  else if (reason == OFPR_ACTION)
    {
      // Get Ethernet frame type
      uint16_t ethType;
      tlv = oxm_match_lookup (OXM_OF_ETH_TYPE, (ofl_match*)msg->match);
      memcpy (&ethType, tlv->value, OXM_LENGTH (OXM_OF_ETH_TYPE));

      // Check for ARP packet
      if (ethType == ArpL3Protocol::PROT_NUMBER)
        {
          return HandleArpPacketIn (msg, swtch, xid);
        }
    }

  NS_LOG_WARN ("Ignoring packet sent to controller.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
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

void
EpcController::SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr)
{
  NS_LOG_FUNCTION (this << ipAddr << macAddr);

  std::pair<Ipv4Address, Mac48Address> entry (ipAddr, macAddr);
  std::pair<IpMacMap_t::iterator, bool> ret;
  ret = m_arpTable.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New ARP entry: " << ipAddr << " - " << macAddr);
      return;
    }
  NS_FATAL_ERROR ("This IP already exists in ARP table.");
}

Mac48Address
EpcController::GetArpEntry (Ipv4Address ip) const
{
  NS_LOG_FUNCTION (this << ip);

  IpMacMap_t::const_iterator ret;
  ret = m_arpTable.find (ip);
  if (ret != m_arpTable.end ())
    {
      NS_LOG_DEBUG ("Found ARP entry: " << ip << " - " << ret->second);
      return ret->second;
    }
  NS_FATAL_ERROR ("No ARP information for this IP.");
}

ofl_err
EpcController::HandleArpPacketIn (
  ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  ofl_match_tlv *tlv;

  // Get ARP operation
  uint16_t arpOp;
  tlv = oxm_match_lookup (OXM_OF_ARP_OP, (ofl_match*)msg->match);
  memcpy (&arpOp, tlv->value, OXM_LENGTH (OXM_OF_ARP_OP));

  // Get input port
  uint32_t inPort;
  tlv = oxm_match_lookup (OXM_OF_IN_PORT, (ofl_match*)msg->match);
  memcpy (&inPort, tlv->value, OXM_LENGTH (OXM_OF_IN_PORT));

  // Check for ARP request
  if (arpOp == ArpHeader::ARP_TYPE_REQUEST)
    {
      // Get target IP address
      Ipv4Address dstIp;
      dstIp = ExtractIpv4Address (OXM_OF_ARP_TPA, (ofl_match*)msg->match);

      // Get target MAC address from ARP table
      Mac48Address dstMac = GetArpEntry (dstIp);
      NS_LOG_DEBUG ("Got ARP request for IP " << dstIp <<
                    ", resolved to " << dstMac);

      // Get source IP address
      Ipv4Address srcIp;
      srcIp = ExtractIpv4Address (OXM_OF_ARP_SPA, (ofl_match*)msg->match);

      // Get Source MAC address
      Mac48Address srcMac;
      tlv = oxm_match_lookup (OXM_OF_ARP_SHA, (ofl_match*)msg->match);
      srcMac.CopyFrom (tlv->value);

      // Create the ARP reply packet
      Ptr<Packet> pkt = CreateArpReply (dstMac, dstIp, srcMac, srcIp);
      uint8_t pktData[pkt->GetSize ()];
      pkt->CopyData (pktData, pkt->GetSize ());

      // Send the ARP reply within an OpenFlow PacketOut message
      ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = OFP_NO_BUFFER;
      reply.in_port = inPort;
      reply.data_length = pkt->GetSize ();
      reply.data = &pktData[0];

      // Send the ARP replay back to the input port
      ofl_action_output *action;
      action = (ofl_action_output*)xmalloc (sizeof (ofl_action_output));
      action->header.type = OFPAT_OUTPUT;
      action->port = OFPP_IN_PORT;
      action->max_len = 0;

      reply.actions_num = 1;
      reply.actions = (ofl_action_header**)&action;

      int error = SendToSwitch (swtch, (ofl_msg_header*)&reply, xid);
      free (action);
      if (error)
        {
          NS_LOG_ERROR ("Error sending packet out with arp request");
        }
    }
  else
    {
      NS_LOG_WARN ("Not supposed to get ARP reply. Ignoring...");
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

Ipv4Address
EpcController::ExtractIpv4Address (uint32_t oxm_of, ofl_match* match)
{
  switch (oxm_of)
    {
    case OXM_OF_ARP_SPA:
    case OXM_OF_ARP_TPA:
    case OXM_OF_IPV4_DST:
    case OXM_OF_IPV4_SRC:
      {
        uint32_t ip;
        int size = OXM_LENGTH (oxm_of);
        ofl_match_tlv *tlv = oxm_match_lookup (oxm_of, match);
        memcpy (&ip, tlv->value, size);
        return Ipv4Address (ntohl (ip));
      }
    default:
      NS_FATAL_ERROR ("Invalid IP field.");
    }
}

Ptr<Packet>
EpcController::CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp,
                               Mac48Address dstMac, Ipv4Address dstIp)
{
  NS_LOG_FUNCTION (this << srcMac << srcIp << dstMac << dstIp);

  Ptr<Packet> packet = Create<Packet> ();

  // Add ARP header.
  ArpHeader arp;
  arp.SetReply (srcMac, srcIp, dstMac, dstIp);
  packet->AddHeader (arp);

  // All Ethernet frames must carry a minimum payload of 46 bytes. We need to
  // pad out if we don't have enough bytes. These must be real bytes since they
  // will be written to pcap files and compared in regression trace files.
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }

  // Add Ethernet header and trailer.
  EthernetHeader eth (false);
  eth.SetSource (srcMac);
  eth.SetDestination (dstMac);
  eth.SetLengthType (ArpL3Protocol::PROT_NUMBER);
  packet->AddHeader (eth);

  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);

  return packet;
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

void
EpcController::SetS11SapMme (EpcS11SapMme * s)
{
  m_s11SapMme = s;
}

EpcS11SapSgw*
EpcController::GetS11SapSgw ()
{
  return m_s11SapSgw;
}

void
EpcController::AddEnb (uint16_t cellId, Ipv4Address enbAddr,
                       Ipv4Address sgwAddr)
{
  NS_LOG_FUNCTION (this << cellId << enbAddr << sgwAddr);

  // Create and save the eNB info for this cell ID.
  EnbInfo enbInfo;
  enbInfo.enbAddr = enbAddr;
  enbInfo.sgwAddr = sgwAddr;

  std::pair<uint16_t, EnbInfo> entry (cellId, enbInfo);
  std::pair<CellIdEnbInfo_t::iterator, bool> ret;
  ret = m_enbInfoByCellId.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing information for cell ID " << cellId);
    }
}

EpcController::EnbInfo
EpcController::GetEnbInfo (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);

  CellIdEnbInfo_t::iterator it = m_enbInfoByCellId.find (cellId);
  if (it == m_enbInfoByCellId.end ())
    {
      NS_FATAL_ERROR ("No info found for eNB cell ID " << cellId);
    }
  return it->second;
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
EpcController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.imsi);

  uint16_t cellId = req.uli.gci;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (req.imsi);
  EnbInfo enbInfo = GetEnbInfo (cellId);
  
  ueInfo->SetEnbAddress (enbInfo.enbAddr);

  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = req.imsi;

  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
  for (bit = req.bearerContextsToBeCreated.begin ();
       bit != req.bearerContextsToBeCreated.end ();
       ++bit)
    {
      // Check for available TEID.
      NS_ABORT_IF (m_teidCount == 0xFFFFFFFF);
      uint32_t teid = ++m_teidCount;
      ueInfo->AddBearer (bit->epsBearerId, teid);

      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.teid = teid;
      bearerContext.sgwFteid.address = enbInfo.sgwAddr;
      bearerContext.epsBearerId = bit->epsBearerId;
      bearerContext.bearerLevelQos = bit->bearerLevelQos;
      bearerContext.tft = bit->tft;
      res.bearerContextsCreated.push_back (bearerContext);
    }

  // Notify the controller of the new create session request accepted.
  NotifySessionCreated (req.imsi, cellId, enbInfo.enbAddr, enbInfo.sgwAddr,
                        res.bearerContextsCreated);

  m_s11SapMme->CreateSessionResponse (res);
}

void
EpcController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;
  uint16_t cellId = req.uli.gci;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  EnbInfo enbInfo = GetEnbInfo (cellId);
  
  ueInfo->SetEnbAddress (enbInfo.enbAddr);

  // No actual bearer modification: for now we just support the minimum needed
  // for path switch request (handover).
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = imsi;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  m_s11SapMme->ModifyBearerResponse (res);
}

void
EpcController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;

  EpcS11SapMme::DeleteBearerRequestMessage res;
  res.teid = imsi;

  std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit;
  for (bit = req.bearerContextsToBeRemoved.begin ();
       bit != req.bearerContextsToBeRemoved.end ();
       ++bit)
    {
      EpcS11SapMme::BearerContextRemoved bearerContext;
      bearerContext.epsBearerId = bit->epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
    }

  m_s11SapMme->DeleteBearerRequest (res);
}

void
EpcController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);

  uint64_t imsi = req.teid;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  std::list<EpcS11SapSgw::BearerContextRemovedSgwPgw>::iterator bit;
  for (bit = req.bearerContextsRemoved.begin ();
       bit != req.bearerContextsRemoved.end ();
       ++bit)
    {
      ueInfo->RemoveBearer (bit->epsBearerId);
    }
}

};  // namespace ns3
