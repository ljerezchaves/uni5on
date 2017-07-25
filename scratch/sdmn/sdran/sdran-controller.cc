/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#include "sdran-controller.h"
#include "../epc/epc-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdranController");
NS_OBJECT_ENSURE_REGISTERED (SdranController);

// Initializing SdranController static members.
SdranController::CellIdCtrlMap_t SdranController::m_cellIdCtrlMap;

SdranController::SdranController ()
  : m_epcCtrlApp (0)
{
  NS_LOG_FUNCTION (this);

  // The S-GW side of S11 and S5 APs
  m_s11SapSgw = new MemberEpcS11SapSgw<SdranController> (this);
  m_s5SapSgw  = new MemberEpcS5SapSgw<SdranController> (this);

  m_mme = CreateObject<SdranMme> ();
  m_mme->SetS11SapSgw (m_s11SapSgw);
  m_s11SapMme = m_mme->GetS11SapMme ();
}

SdranController::~SdranController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdranController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdranController")
    .SetParent<OFSwitch13Controller> ()
    .AddConstructor<SdranController> ()
  ;
  return tid;
}

bool
SdranController::DedicatedBearerRelease (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  SgwRulesRemove (RoutingInfo::GetPointer (teid));
  m_epcCtrlApp->DedicatedBearerRelease (bearer, teid);
  return true;
}

bool
SdranController::DedicatedBearerRequest (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  if (m_epcCtrlApp->DedicatedBearerRequest (bearer, teid))
    {
      return SgwRulesInstall (RoutingInfo::GetPointer (teid));
    }
  return false;
}

void
SdranController::NotifyEnbAttach (uint16_t cellId, uint32_t sgwS1uPortNo)
{
  NS_LOG_FUNCTION (this << cellId << sgwS1uPortNo);

  // Register this controller by cell ID for further usage.
  RegisterController (Ptr<SdranController> (this), cellId);

  // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
  // Internet are sent to table 2, where rules will match the flow and set both
  // TEID and P-GW address on tunnel metadata.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
      << ",in_port=" << sgwS1uPortNo
      << ",ip_dst=" << EpcNetwork::m_sgiAddr
      << "/" << EpcNetwork::m_sgiMask.GetPrefixLength ()
      << " goto:2";
  DpctlSchedule (m_sgwDpId, cmd.str ());
}

void
SdranController::NotifySgwAttach (
  uint32_t sgwS5PortNo, Ptr<NetDevice> sgwS5Dev, uint32_t mtcTeid)
{
  NS_LOG_FUNCTION (this << sgwS5PortNo << sgwS5Dev << mtcTeid);

  m_sgwS5Addr = EpcNetwork::GetIpv4Addr (sgwS5Dev);
  m_sgwS5PortNo = sgwS5PortNo;

  // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
  // network are sent to table 1, where rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
      << ",in_port=" << sgwS5PortNo
      << ",ip_dst=" << EpcNetwork::m_ueAddr
      << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
      << " goto:1";
  DpctlSchedule (m_sgwDpId, cmd.str ());

  // The mtcTeid != 0 means that MTC traffic aggregation is enable.
  // Install a high-priority match rule on default table for aggregating
  // traffic from all MTC UEs on the same uplink S5 GTP tunnel.
  if (mtcTeid != 0)
    {
      // Print MTC aggregation TEID and P-GW IPv4 address into tunnel metadata.
      Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (mtcTeid);
      uint64_t tunnelId = (uint64_t)rInfo->GetPgwS5Addr ().Get () << 32;
      tunnelId |= rInfo->GetTeid ();
      char tunnelIdStr [20];
      sprintf (tunnelIdStr, "0x%016lx", tunnelId);

      // Instal OpenFlow MTC aggregation rule.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,table=0,prio=65520 eth_type=0x800"
          << ",ip_src=" << EpcNetwork::m_mtcAddr
          << "/" << EpcNetwork::m_mtcMask.GetPrefixLength ()
          << " apply:set_field=tunn_id:" << tunnelIdStr
          << ",output=" << m_sgwS5PortNo;
      DpctlSchedule (m_sgwDpId, cmd.str ());
    }
}

EpcS1apSapMme*
SdranController::GetS1apSapMme (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mme->GetS1apSapMme ();
}

EpcS5SapSgw*
SdranController::GetS5SapSgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s5SapSgw;
}

Ipv4Address
SdranController::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwS5Addr;
}

void
SdranController::SetEpcCtlrApp (Ptr<EpcController> value)
{
  NS_LOG_FUNCTION (this << value);

  m_epcCtrlApp = value;
  m_s5SapPgw = m_epcCtrlApp->GetS5SapPgw ();
}

void
SdranController::SetSgwDpId (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sgwDpId = value;
}

Ptr<SdranController>
SdranController::GetPointer (uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<SdranController> ctrl = 0;
  CellIdCtrlMap_t::iterator ret;
  ret = SdranController::m_cellIdCtrlMap.find (cellId);
  if (ret != SdranController::m_cellIdCtrlMap.end ())
    {
      ctrl = ret->second;
    }
  return ctrl;
}

void
SdranController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_epcCtrlApp = 0;
  m_mme = 0;
  delete (m_s11SapSgw);
  delete (m_s5SapSgw);

  // Chain up.
  Object::DoDispose ();
}

ofl_err
SdranController::HandleFlowRemoved (
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
SdranController::HandlePacketIn (
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
SdranController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Configure S-GW port rules.
  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
  // network are sent to table 1, where rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  //
  // Entries will be installed here by NotifySgwAttach function.

  // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
  // Internet are sent to table 2, where rules will match the flow and set both
  // TEID and P-GW address on tunnel metadata.
  //
  // Entries will be installed here by NotifyEnbAttach function.

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- S-GW downlink forward table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.

  // -------------------------------------------------------------------------
  // Table 2 -- S-GW uplink forward table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
SdranController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

  // Send the request message to the P-GW.
  m_s5SapPgw->CreateSessionRequest (msg);
}

void
SdranController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  uint64_t imsi = msg.teid;

  EpcS11SapMme::DeleteBearerRequestMessage res;
  res.teid = imsi;

  std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit;
  for (bit = msg.bearerContextsToBeRemoved.begin ();
       bit != msg.bearerContextsToBeRemoved.end ();
       ++bit)
    {
      EpcS11SapMme::BearerContextRemoved bearerContext;
      bearerContext.epsBearerId = bit->epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
    }

  m_s11SapMme->DeleteBearerRequest (res);
}

void
SdranController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // Nothing to do here.
}

void
SdranController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // In current implementation, this Modify Bearer Request is triggered only by
  // X2 handover procedures. There is no actual bearer modification, for now we
  // just support the minimum needed for path switch request (handover). There
  // is no need to forward the request message to the P-GW.
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = msg.teid;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  m_s11SapMme->ModifyBearerResponse (res);
}

void
SdranController::DoCreateSessionResponse (
  EpcS11SapMme::CreateSessionResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // Install S-GW rules for default bearer.
  BearerContext_t defaultBearer = msg.bearerContextsCreated.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
  uint32_t teid = defaultBearer.sgwFteid.teid;
  SgwRulesInstall (RoutingInfo::GetPointer (teid));

  // Forward the response message to the MME.
  m_s11SapMme->CreateSessionResponse (msg);
}

void
SdranController::DoDeleteBearerRequest (
  EpcS11SapMme::DeleteBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

void
SdranController::DoModifyBearerResponse (
  EpcS11SapMme::ModifyBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

bool
SdranController::SgwRulesInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Installing S-GW rules for bearer teid " << rInfo->GetTeid ());
  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  Ptr<const EnbInfo> enbInfo = EnbInfo::GetPointer (ueInfo->GetCellId ());

  // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Print the cookie and buffer values in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Configure downlink.
  if (rInfo->HasDownlinkTraffic ())
    {
      // Print downlink TEID and destination IPv4 address into tunnel metadata.
      uint64_t tunnelId = (uint64_t)enbInfo->GetEnbS1uAddr ().Get () << 32;
      tunnelId |= rInfo->GetTeid ();
      char tunnelIdStr [20];
      sprintf (tunnelIdStr, "0x%016lx", tunnelId);

      // Build the dpctl command string.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add,table=1"
          << ",flags=" << flagsStr
          << ",cookie=" << cookieStr
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:" << tunnelIdStr
          << ",output=" << enbInfo->GetSgwS1uPortNo ();

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
              DpctlExecute (m_sgwDpId, cmdTcpStr);
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
              DpctlExecute (m_sgwDpId, cmdUdpStr);
            }
        }
    }

  // Configure uplink.
  if (rInfo->HasUplinkTraffic () && !rInfo->IsAggregated ())
    {
      // Print uplink TEID and destination IPv4 address into tunnel metadata.
      uint64_t tunnelId = (uint64_t)rInfo->GetPgwS5Addr ().Get () << 32;
      tunnelId |= rInfo->GetTeid ();
      char tunnelIdStr [20];
      sprintf (tunnelIdStr, "0x%016lx", tunnelId);

      // Build the dpctl command string.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add,table=2"
          << ",flags=" << flagsStr
          << ",cookie=" << cookieStr
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ();

      // Check for meter entry.
      Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
      if (meterInfo && meterInfo->HasUp ())
        {
          if (!meterInfo->IsUpInstalled ())
            {
              // Install the per-flow meter entry.
              DpctlExecute (m_sgwDpId, meterInfo->GetUpAddCmd ());
              meterInfo->SetUpInstalled (true);
            }

          // Instruction: meter.
          act << " meter:" << rInfo->GetTeid ();
        }

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:" << tunnelIdStr
          << ",output=" << m_sgwS5PortNo;

      // Install one uplink dedicated bearer rule for each packet filter.
      Ptr<EpcTft> tft = rInfo->GetTft ();
      for (uint8_t i = 0; i < tft->GetNFilters (); i++)
        {
          EpcTft::PacketFilter filter = tft->GetFilter (i);
          if (filter.direction == EpcTft::DOWNLINK)
            {
              continue;
            }

          // Install rules for TCP traffic.
          if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
            {
              std::ostringstream match;
              match << " eth_type=0x800"
                    << ",ip_proto=6"
                    << ",ip_src=" << filter.localAddress;

              if (tft->IsDefaultTft () == false)
                {
                  match << ",ip_dst=" << filter.remoteAddress
                        << ",tcp_dst=" << filter.remotePortStart;
                }

              std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
              DpctlExecute (m_sgwDpId, cmdTcpStr);
            }

          // Install rules for UDP traffic.
          else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
            {
              std::ostringstream match;
              match << " eth_type=0x800"
                    << ",ip_proto=17"
                    << ",ip_src=" << filter.localAddress;

              if (tft->IsDefaultTft () == false)
                {
                  match << ",ip_dst=" << filter.remoteAddress
                        << ",udp_dst=" << filter.remotePortStart;
                }

              std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
              DpctlExecute (m_sgwDpId, cmdUdpStr);
            }
        }
    }
  return true;
}

bool
SdranController::SgwRulesRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  NS_LOG_INFO ("Removing S-GW rules for bearer teid " << rInfo->GetTeid ());

  // Print the cookie value in dpctl string format.
  char cookieStr [20];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,"
      << ",cookie=" << cookieStr
      << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
  DpctlExecute (m_sgwDpId, cmd.str ());

  // Remove meter entry for this TEID.
  Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
  if (meterInfo && meterInfo->IsUpInstalled ())
    {
      DpctlExecute (m_sgwDpId, meterInfo->GetDelCmd ());
      meterInfo->SetUpInstalled (false);
    }
  return true;
}

void
SdranController::RegisterController (Ptr<SdranController> ctrl,
                                     uint16_t cellId)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Saving map by cell ID.
  std::pair<CellIdCtrlMap_t::iterator, bool> ret;
  std::pair<uint16_t, Ptr<SdranController> > entry (cellId, ctrl);
  ret = SdranController::m_cellIdCtrlMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Can't register SDRAN controller by cell ID.");
    }
}

};  // namespace ns3
