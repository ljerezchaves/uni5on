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
#include "epc-network.h"

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

  m_mme = CreateObject<SdmnMme> ();
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
SdranController::RequestDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  bool accepted = m_epcCtrlApp->RequestDedicatedBearer (bearer, teid);
  if (accepted)
    {
      InstallSgwSwitchRules (RoutingInfo::GetPointer (teid));
    }
  return accepted;
}

bool
SdranController::ReleaseDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);
  // TODO
  return m_epcCtrlApp->ReleaseDedicatedBearer (bearer, teid);
}

void
SdranController::NotifySgwAttach (
  uint32_t sgwS5PortNo, Ptr<NetDevice> sgwS5Dev)
{
  NS_LOG_FUNCTION (this << sgwS5PortNo << sgwS5Dev);

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

Ipv4Address
SdranController::GetSgwS5Addr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwS5Addr;
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

bool
SdranController::InstallSgwSwitchRules (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());

  Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
  Ptr<const EnbInfo> enbInfo = EnbInfo::GetPointer (ueInfo->GetCellId ());

  // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
  std::string flagsStr ("0x0007");

  // Print the cookie and buffer values in dpctl string format.
  char cookieStr [9], bufferStr [12];
  sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
  sprintf (bufferStr, "%u", OFP_NO_BUFFER);

  // Configure downlink.
  if (rInfo->HasDownlinkTraffic ())
    {
      // Print downlink TEID and destination IPv4 address into tunnel metadata.
      uint64_t tunnelId = (uint64_t)enbInfo->GetEnbS1uAddr ().Get () << 32;
      tunnelId |= rInfo->GetTeid ();
      char tunnelIdStr [17];
      sprintf (tunnelIdStr, "0x%016lX", tunnelId);

      // Build the dpctl command string.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,table=1"
          << ",buffer=" << bufferStr
          << ",flags=" << flagsStr
          << ",cookie=" << cookieStr
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ()
          << " tunn_id=" << cookieStr // TEID value
          << " apply:set_field=tunn_id:" << tunnelIdStr
          << ",output=" << enbInfo->GetSgwS1uPortNo ();

      DpctlExecute (m_sgwDpId, cmd.str ());
    }

  // Configure uplink.
  if (rInfo->HasUplinkTraffic ())
    {
      // Print uplink TEID and destination IPv4 address into tunnel metadata.
      uint64_t tunnelId = (uint64_t)rInfo->GetPgwS5Addr ().Get () << 32;
      tunnelId |= rInfo->GetTeid ();
      char tunnelIdStr [17];
      sprintf (tunnelIdStr, "0x%016lX", tunnelId);

      // Build the dpctl command string.
      std::ostringstream cmd, match, act;
      cmd << "flow-mod cmd=add,table=2"
          << ",buffer=" << bufferStr
          << ",flags=" << flagsStr
          << ",cookie=" << cookieStr
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ();

      // Match IP and the TEID value.
      match << " eth_type=0x800,tunn_id=" << cookieStr;

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

      std::string commandStr = cmd.str () + match.str () + act.str ();
      DpctlExecute (m_sgwDpId, commandStr);
    }
  return true;
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
  // Entries will be installed here by InstallSgwSwitchRules function.

  // -------------------------------------------------------------------------
  // Table 2 -- S-GW uplink forward table -- [from higher to lower priority]
  //
  // Entries will be installed here by InstallSgwSwitchRules function.
}

ofl_err
SdranController::HandlePacketIn (
  ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *m = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_INFO ("Packet in match: " << m);
  free (m);

  NS_ABORT_MSG ("Packet not supposed to be sent to this controller. Abort.");

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

ofl_err
SdranController::HandleFlowRemoved (
  ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
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

  NS_LOG_DEBUG ("Nothing to do here. Done.");
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

  InstallSgwSwitchRules (RoutingInfo::GetPointer (teid));

  // Forward the response message to the MME.
  m_s11SapMme->CreateSessionResponse (msg);
}

void
SdranController::DoModifyBearerResponse (
  EpcS11SapMme::ModifyBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
}

void
SdranController::DoDeleteBearerRequest (
  EpcS11SapMme::DeleteBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_FATAL_ERROR ("Unimplemented method.");
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
