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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "slice-controller.h"
#include "../infrastructure/backhaul-controller.h"
#include "../infrastructure/backhaul-network.h"
#include "../metadata/enb-info.h"
#include "../metadata/pgw-info.h"
#include "../metadata/routing-info.h"
#include "../metadata/sgw-info.h"
#include "../metadata/ue-info.h"
#include "gtp-tunnel-app.h"
#include "slice-network.h"
#include "svelte-mme.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[Slice " << m_sliceIdStr << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceController");
NS_OBJECT_ENSURE_REGISTERED (SliceController);

// Initializing SliceController static members.
const uint16_t SliceController::m_flowTimeout = 0;

SliceController::SliceController ()
  : m_pgwInfo (0)
{
  NS_LOG_FUNCTION (this);
}

SliceController::~SliceController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SliceController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SliceController")
    .SetParent<OFSwitch13Controller> ()
    .AddConstructor<SliceController> ()

    // Slice.
    .AddAttribute ("SliceId", "The LTE logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceId::NONE),
                   MakeEnumAccessor (&SliceController::m_sliceId),
                   MakeEnumChecker (SliceId::MTC, "mtc",
                                    SliceId::HTC, "htc",
                                    SliceId::TMP, "tmp"))
    .AddAttribute ("Priority",
                   "Priority for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&SliceController::m_slicePrio),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Quota",
                   "Infrastructure quota for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (100),
                   MakeUintegerAccessor (&SliceController::m_sliceQuota),
                   MakeUintegerChecker<uint16_t> (0, 100))

    // Infrastructure.
    .AddAttribute ("BackhaulCtrl", "The OpenFlow backhaul network controller.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceController::m_backhaulCtrl),
                   MakePointerChecker<BackhaulController> ())

    // MME.
    .AddAttribute ("Mme", "The SVELTE MME pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceController::m_mme),
                   MakePointerChecker<SvelteMme> ())

    // P-GW.
    .AddAttribute ("PgwTftAdaptiveMode",
                   "P-GW TFT adaptive mechanism operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::AUTO),
                   MakeEnumAccessor (&SliceController::m_tftAdaptive),
                   MakeEnumChecker (OpMode::OFF,  "off",
                                    OpMode::ON,   "on",
                                    OpMode::AUTO, "auto"))
    .AddAttribute ("PgwTftBlockPolicy",
                   "P-GW TFT overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&SliceController::m_tftBlockPolicy),
                   MakeEnumChecker (OpMode::OFF,  "none",
                                    OpMode::ON,   "all",
                                    OpMode::AUTO, "gbr"))
    .AddAttribute ("PgwTftBlockThs",
                   "The P-GW TFT block threshold.",
                   DoubleValue (0.95),
                   MakeDoubleAccessor (&SliceController::m_tftBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
    .AddAttribute ("PgwTftJoinThs",
                   "The P-GW TFT join threshold.",
                   DoubleValue (0.30),
                   MakeDoubleAccessor (&SliceController::m_tftJoinThs),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("PgwTftSplitThs",
                   "The P-GW TFT split threshold.",
                   DoubleValue (0.90),
                   MakeDoubleAccessor (&SliceController::m_tftSplitThs),
                   MakeDoubleChecker<double> (0.5, 1.0))

    .AddAttribute ("TimeoutInterval",
                   "The interval between internal periodic operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&SliceController::m_timeout),
                   MakeTimeChecker ())

    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_bearerRequestTrace),
                     "ns3::RoutingInfo::TracedCallback")
    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_bearerReleaseTrace),
                     "ns3::RoutingInfo::TracedCallback")
    .AddTraceSource ("SessionCreated", "The session created trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_sessionCreatedTrace),
                     "ns3::SliceController::SessionCreatedTracedCallback")
    .AddTraceSource ("PgwTftAdaptive", "The P-GW TFT adaptive trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_pgwTftAdaptiveTrace),
                     "ns3::SliceController::PgwTftStatsTracedCallback")
  ;
  return tid;
}

bool
SliceController::DedicatedBearerRequest (
  EpsBearer bearer, uint64_t imsi, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);

  // This bearer must be inactive as we are going to reuse its metadata.
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Update the P-GW TFT index (the adaptive mechanism level may have changed
  // since the last time this bearer was active) and the blocked flag.
  rInfo->SetPgwTftIdx (GetTftIdx (rInfo));
  rInfo->SetBlocked (false);

  // Check for available resources on P-GW and backhaul network and then
  // reserve the requested bandwidth (don't change the order!).
  bool success = true;
  success &= PgwBearerRequest (rInfo);
  success &= m_backhaulCtrl->BearerRequest (rInfo);
  if (!success)
    {
      NS_LOG_INFO ("Bearer request blocked by controller.");
      m_bearerRequestTrace (rInfo);
      return false;
    }

  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will increase the bearer
  // priority. Doing this, we avoid problems with old 'expiring' rules, and
  // we can even use new routing paths when necessary.
  NS_LOG_INFO ("Bearer request accepted by controller.");

  // Activate and install the bearer.
  rInfo->SetActive (true);
  bool installed = BearerInstall (rInfo);
  m_bearerRequestTrace (rInfo);
  return installed;
}

bool
SliceController::DedicatedBearerRelease (
  EpsBearer bearer, uint64_t imsi, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);

  // This bearer must be active.
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");

  m_backhaulCtrl->BearerRelease (rInfo);
  m_bearerReleaseTrace (rInfo);
  NS_LOG_INFO ("Bearer released by controller.");

  // Deactivate and remove the bearer.
  rInfo->SetActive (false);
  return BearerRemove (rInfo);
}

OpMode
SliceController::GetPgwTftAdaptiveMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftAdaptive;
}

OpMode
SliceController::GetPgwTftBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftBlockPolicy;
}

double
SliceController::GetPgwTftBlockThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftBlockThs;
}

double
SliceController::GetPgwTftJoinThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftJoinThs;
}

double
SliceController::GetPgwTftSplitThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftSplitThs;
}

EpcS11SapSgw*
SliceController::GetS11SapSgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s11SapSgw;
}

SliceId
SliceController::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

void
SliceController::NotifyPgwAttach (
  Ptr<PgwInfo> pgwInfo, Ptr<NetDevice> webSgiDev)
{
  NS_LOG_FUNCTION (this << pgwInfo << pgwInfo->GetPgwId () << webSgiDev);

  // Save the P-GW metadata.
  NS_ASSERT_MSG (!m_pgwInfo, "P-GW already configured with this controller.");
  m_pgwInfo = pgwInfo;

  // Set the number of P-GW TFT active switches and the
  // adaptive mechanism initial level.
  switch (GetPgwTftAdaptiveMode ())
    {
    case OpMode::ON:
    case OpMode::AUTO:
      {
        pgwInfo->SetTftLevel (pgwInfo->GetMaxLevel ());
        break;
      }
    case OpMode::OFF:
      {
        pgwInfo->SetTftLevel (0);
        break;
      }
    }

  // Configuring the P-GW MAIN switch.
  // -------------------------------------------------------------------------
  // Table 0 -- P-GW MAIN default table -- [from higher to lower priority]
  //
  // IP packets coming from the S-GW (P-GW S5 port) and addressed to the
  // Internet (Web IP address) have their destination MAC address rewritten to
  // the Web SGi MAC address (mandatory when using logical ports) and are
  // forward to the SGi interface port.
  Mac48Address webMac = Mac48Address::ConvertFrom (webSgiDev->GetAddress ());
  std::ostringstream cmdUl;
  cmdUl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << pgwInfo->GetMainS5PortNo ()
        << ",ip_dst=" << Ipv4AddressHelper::GetAddress (webSgiDev)
        << " write:set_field=eth_dst:" << webMac
        << ",output=" << pgwInfo->GetMainSgiPortNo ();
  DpctlSchedule (pgwInfo->GetMainDpId (), cmdUl.str ());

  // IP packets coming from the Internet (P-GW SGi port) and addressed to the
  // UE network are sent to the table corresponding to the current P-GW
  // adaptive mechanism level. This is the only rule that is updated when the
  // level changes, sending packets to a different pipeline table.
  std::ostringstream cmdDl;
  cmdDl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << pgwInfo->GetMainSgiPortNo ()
        << ",ip_dst=" << m_ueAddr << "/" << m_ueMask.GetPrefixLength ()
        << " goto:" << pgwInfo->GetCurLevel () + 1;
  DpctlSchedule (pgwInfo->GetMainDpId (), cmdDl.str ());

  // -------------------------------------------------------------------------
  // Table 1..N -- P-GW MAIN adaptive level -- [from higher to lower priority]
  //
  for (uint16_t tftIdx = 1; tftIdx <= pgwInfo->GetMaxTfts (); tftIdx++)
    {
      // Configuring the P-GW main switch to forward traffic to different P-GW
      // TFT switches considering all possible adaptive mechanism levels.
      for (uint16_t tft = pgwInfo->GetMaxTfts (); tftIdx <= tft; tft /= 2)
        {
          uint16_t lbLevel = static_cast<uint16_t> (log2 (tft));
          uint16_t ipMask = (1 << lbLevel) - 1;
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,prio=64,table=" << lbLevel + 1
              << " eth_type=0x800,ip_dst=0.0.0." << tftIdx - 1
              << "/0.0.0." << ipMask
              << " apply:output=" << pgwInfo->GetMainToTftPortNo (tftIdx);
          DpctlSchedule (pgwInfo->GetMainDpId (), cmd.str ());
        }
    }

  // Configuring the P-GW TFT switches.
  // ---------------------------------------------------------------------
  // Table 0 -- P-GW TFT default table -- [from higher to lower priority]
  //
  // Entries will be installed here by PgwRulesInstall function.
}

void
SliceController::NotifySgwAttach (Ptr<SgwInfo> sgwInfo)
{
  NS_LOG_FUNCTION (this << sgwInfo << sgwInfo->GetSgwId ());

  // Save the S-GW metadata.
  uint16_t swIdx = sgwInfo->GetInfraSwIdx ();
  std::pair<uint16_t, Ptr<SgwInfo> > entry (swIdx, sgwInfo);
  auto ret = m_sgwInfoBySwIdx.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing S-GW info for this index.");

  // -------------------------------------------------------------------------
  // Table 0 -- S-GW default table -- [from higher to lower priority]
  //
  // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
  // network are sent to table 1, where rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  std::ostringstream cmdDl;
  cmdDl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << sgwInfo->GetS5PortNo ()
        << ",ip_dst=" << m_ueAddr << "/" << m_ueMask.GetPrefixLength ()
        << " goto:1";
  DpctlSchedule (sgwInfo->GetDpId (), cmdDl.str ());

  // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
  // Internet are sent to table 2, where rules will match the flow and set both
  // TEID and P-GW address on tunnel metadata.
  std::ostringstream cmdUl;
  cmdUl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << sgwInfo->GetS1uPortNo ()
        << ",ip_dst=" << m_webAddr << "/" << m_webMask.GetPrefixLength ()
        << " goto:2";
  DpctlSchedule (sgwInfo->GetDpId (), cmdUl.str ());

  // -------------------------------------------------------------------------
  // Table 1 -- S-GW downlink table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.

  // -------------------------------------------------------------------------
  // Table 2 -- S-GW uplink table -- [from higher to lower priority]
  //
  // Entries will be installed here by SgwRulesInstall function.
}

void
SliceController::SetNetworkAttributes (
  Ipv4Address ueAddr, Ipv4Mask ueMask, Ipv4Address webAddr, Ipv4Mask webMask)
{
  NS_LOG_FUNCTION (this << ueAddr << ueMask << webAddr << webMask);

  m_ueAddr = ueAddr;
  m_ueMask = ueMask;
  m_webAddr = webAddr;
  m_webMask = webMask;
}

void
SliceController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_mme = 0;
  m_pgwInfo = 0;
  m_backhaulCtrl = 0;
  delete (m_s11SapSgw);
  Object::DoDispose ();
}

void
SliceController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_sliceId == SliceId::NONE, "Undefined slice ID.");
  NS_ABORT_MSG_IF (!m_backhaulCtrl, "No backhaul controller application.");
  NS_ABORT_MSG_IF (!m_mme, "No SVELTE MME.");

  m_sliceIdStr = SliceIdStr (m_sliceId);

  // Connecting this controller to the MME.
  m_s11SapSgw = new MemberEpcS11SapSgw<SliceController> (this);
  m_s11SapMme = m_mme->GetS11SapMme ();

  // Schedule the first timeout operation.
  Simulator::Schedule (m_timeout, &SliceController::ControllerTimeout, this);

  OFSwitch13Controller::NotifyConstructionCompleted ();
}

ofl_err
SliceController::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Chain up for logging and abort.
  OFSwitch13Controller::HandleError (msg, swtch, xid);
  NS_ABORT_MSG ("Should not get here.");
}

ofl_err
SliceController::HandleFlowRemoved (
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
  NS_ASSERT_MSG (rInfo, "Routing metadata not found");

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed for inactive bearer teid " <<
                   rInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Old rule removed for bearer teid " <<
                   rInfo->GetTeidHex ());
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we are going to abort the program to avoid wrong results.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Should not get here :/");
}

ofl_err
SliceController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *msgStr = ofl_structs_match_to_string (msg->match, 0);
  NS_LOG_DEBUG ("Packet in match: " << msgStr);
  free (msgStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  NS_ABORT_MSG ("Should not get here.");
  return 0;
}

void
SliceController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Table miss entry. Send to controller.
  DpctlExecute (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");
}

bool
SliceController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
  rInfo->SetTunnelInstalled (false);

  // Increasing the priority every time we (re)install routing rules.
  rInfo->IncreasePriority ();

  // Install the rules.
  bool success = true;
  success &= PgwRulesInstall (rInfo);
  success &= SgwRulesInstall (rInfo);
  success &= m_backhaulCtrl->TopologyRoutingInstall (rInfo);
  rInfo->SetTunnelInstalled (success);
  return success;
}

bool
SliceController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Remove the rules.
  bool success = true;
  success &= PgwRulesRemove (rInfo);
  success &= SgwRulesRemove (rInfo);
  success &= m_backhaulCtrl->TopologyRoutingRemove (rInfo);
  rInfo->SetTunnelInstalled (!success);
  return success;
}

void
SliceController::ControllerTimeout (void)
{
  NS_LOG_FUNCTION (this);

  PgwAdaptiveMechanism ();

  // Schedule the next timeout operation.
  Simulator::Schedule (m_timeout, &SliceController::ControllerTimeout, this);
}

void
SliceController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

  NS_ASSERT_MSG (m_pgwInfo, "P-GW not configure with this controller.");

  // This controller is responsible for assigning the S-GW and P-GW elements to
  // the UE. In current implementation, each slice has a single P-GW. We are
  // using the S-GW attached to the same OpenFlow backhaul switch where the
  // UE's serving eNB is also attached. The S-GW may change during handover.
  uint64_t imsi = msg.imsi;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  Ptr<SgwInfo> sgwInfo = GetSgwInfo (ueInfo->GetEnbInfo ()->GetInfraSwIdx ());

  ueInfo->SetPgwInfo (m_pgwInfo);
  ueInfo->SetSgwInfo (sgwInfo);

  // Iterate over request message and create the response message.
  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = imsi;

  for (auto const &bit : msg.bearerContextsToBeCreated)
    {
      uint32_t teid = GetSvelteTeid (m_sliceId, imsi, bit.epsBearerId);
      bool isDefault = res.bearerContextsCreated.empty ();

      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.teid = teid;
      bearerContext.sgwFteid.address = sgwInfo->GetS1uAddr ();
      bearerContext.epsBearerId = bit.epsBearerId;
      bearerContext.bearerLevelQos = bit.bearerLevelQos;
      bearerContext.tft = bit.tft;
      res.bearerContextsCreated.push_back (bearerContext);

      // Add the TFT entry to the UeInfo (don't move this command from here).
      ueInfo->AddTft (bit.tft, teid);

      // Saving bearer metadata.
      Ptr<RoutingInfo> rInfo = CreateObject<RoutingInfo> (
          teid, bearerContext, ueInfo, isDefault);
      NS_LOG_DEBUG ("Saving bearer info for UE IMSI " << imsi << ", slice " <<
                    SliceIdStr (m_sliceId) << ", internal bearer id " <<
                    static_cast<uint16_t> (bit.epsBearerId) << ", teid " <<
                    rInfo->GetTeidHex ());

      rInfo->SetPgwTftIdx (GetTftIdx (rInfo));
      m_backhaulCtrl->NotifyBearerCreated (rInfo);

      if (rInfo->IsDefault ())
        {
          // Configure this default bearer.
          rInfo->SetPriority (0x7F);
          rInfo->SetTimeout (0);

          // For logic consistence, let's check for available resources.
          bool success = true;
          success &= PgwBearerRequest (rInfo);
          success &= m_backhaulCtrl->BearerRequest (rInfo);
          NS_ASSERT_MSG (success, "Default bearer must be accepted.");

          // Activate and install the bearer.
          rInfo->SetActive (true);
          bool installed = BearerInstall (rInfo);
          m_bearerRequestTrace (rInfo);
          NS_ASSERT_MSG (installed, "Default bearer must be installed.");
        }
      else
        {
          // Configure this dedicated bearer.
          rInfo->SetPriority (0x1FFF);
          rInfo->SetTimeout (m_flowTimeout);
        }
    }

  // Fire trace source notifying the created session.
  m_sessionCreatedTrace (imsi, res.bearerContextsCreated);

  // Forward the response message to the MME.
  m_s11SapMme->CreateSessionResponse (res);
}

void
SliceController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  EpcS11SapMme::DeleteBearerRequestMessage res;
  res.teid = msg.teid;

  for (auto const &bit : msg.bearerContextsToBeRemoved)
    {
      EpcS11SapMme::BearerContextRemoved bearerContext;
      bearerContext.epsBearerId = bit.epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
    }

  // Forward the response message to the MME.
  m_s11SapMme->DeleteBearerRequest (res);
}

void
SliceController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // Nothing to do here.
}

void
SliceController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  // In current implementation, this Modify Bearer Request is triggered only by
  // X2 handover procedures. There is no actual bearer modification, for now we
  // just support the minimum needed for path switch request (handover).

  // FIXME: We need to identify which is the best S-GW for this UE after the
  // handover procedure. We also need to move the S-GW rules from the old S-GW
  // switch to the new one. Update the bearer S-GW address.
  // ueInfo->SetSgwInfo (?);

  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = msg.teid;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  m_s11SapMme->ModifyBearerResponse (res);
}

Ptr<SgwInfo>
SliceController::GetSgwInfo (uint16_t infraSwIdx)
{
  NS_LOG_FUNCTION (this << infraSwIdx);

  Ptr<SgwInfo> sgwInfo = 0;
  auto ret = m_sgwInfoBySwIdx.find (infraSwIdx);
  if (ret != m_sgwInfoBySwIdx.end ())
    {
      sgwInfo = ret->second;
    }
  return sgwInfo;
}

uint16_t
SliceController::GetTftIdx (
  Ptr<const RoutingInfo> rInfo, uint16_t activeTfts) const
{
  NS_LOG_FUNCTION (this << rInfo << activeTfts);

  if (activeTfts == 0)
    {
      activeTfts = m_pgwInfo->GetCurTfts ();
    }
  return 1 + (rInfo->GetUeAddr ().Get () % activeTfts);
}

void
SliceController::PgwAdaptiveMechanism (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_pgwInfo, "No P-GW attached to this slice.");

  uint16_t nextLevel = m_pgwInfo->GetCurLevel ();
  if (GetPgwTftAdaptiveMode () == OpMode::AUTO)
    {
      double tableUsage = m_pgwInfo->GetTftMaxFlowTableUsage ();
      double pipeUsage = m_pgwInfo->GetTftMaxPipeCapacityUsage ();

      // We may increase the level when we hit the split threshold.
      if ((m_pgwInfo->GetCurLevel () < m_pgwInfo->GetMaxLevel ())
          && (tableUsage >= m_tftSplitThs || pipeUsage >= m_tftSplitThs))
        {
          NS_LOG_INFO ("Increasing the adaptive mechanism level.");
          nextLevel++;
        }

      // We may decrease the level when we hit the join threshold.
      else if ((m_pgwInfo->GetCurLevel () > 0)
               && (tableUsage < m_tftJoinThs) && (pipeUsage < m_tftJoinThs))
        {
          NS_LOG_INFO ("Decreasing the adaptive mechanism level.");
          nextLevel--;
        }
    }

  // Check if we need to update the adaptive mechanism level.
  uint32_t moved = 0;
  if (m_pgwInfo->GetCurLevel () != nextLevel)
    {
      // Identify and move bearers to the correct P-GW TFT switches.
      uint16_t futureTfts = 1 << nextLevel;
      for (uint16_t currIdx = 1; currIdx <= m_pgwInfo->GetCurTfts (); currIdx++)
        {
          RoutingInfoList_t bearerList;
          RoutingInfo::GetInstalledList (bearerList, m_sliceId, currIdx);
          for (auto const &rInfo : bearerList)
            {
              uint16_t destIdx = GetTftIdx (rInfo, futureTfts);
              if (destIdx != currIdx)
                {
                  NS_LOG_INFO ("Move bearer teid " << (rInfo)->GetTeidHex ());
                  PgwRulesRemove (rInfo, currIdx, true);
                  PgwRulesInstall (rInfo, destIdx, true);
                  rInfo->SetPgwTftIdx (destIdx);
                  moved++;
                }
            }
        }

      // Update the P-GW main switch.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=mods,table=0,prio=64 eth_type=0x800"
          << ",in_port=" << m_pgwInfo->GetMainSgiPortNo ()
          << ",ip_dst=" << m_ueAddr << "/" << m_ueMask.GetPrefixLength ()
          << " goto:" << nextLevel + 1;
      DpctlExecute (m_pgwInfo->GetMainDpId (), cmd.str ());
    }

  // Fire the P-GW TFT adaptation trace source.
  m_pgwTftAdaptiveTrace (m_pgwInfo, nextLevel, moved);

  // Update the adaptive mechanism level.
  m_pgwInfo->SetTftLevel (nextLevel);
}

bool
SliceController::PgwBearerRequest (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  // If the bearer is already blocked, there's nothing more to do.
  if (rInfo->IsBlocked ())
    {
      return false;
    }

  // Check for valid P-GW TFT thresholds attributes.
  NS_ASSERT_MSG (m_tftSplitThs < m_tftBlockThs
                 && m_tftSplitThs > 2 * m_tftJoinThs,
                 "The split threshold should be smaller than the block "
                 "threshold and two times larger than the join threshold.");

  // First check: OpenFlow switch table usage.
  // Blocks the bearer if the table usage is exceeding the block threshold.
  double tableUsage = m_pgwInfo->GetFlowTableUsage (rInfo->GetPgwTftIdx ());
  if (tableUsage >= m_tftBlockThs)
    {
      rInfo->SetBlocked (true, RoutingInfo::TFTTABLE);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because the TFT flow table is full.");
    }

  // Second check: OpenFlow switch pipeline load.
  // If the current pipeline load is exceeding the block threshold, block the
  // bearer accordingly to the PgwTftBlockPolicy attribute:
  // - If OFF (none): don't block the request.
  // - If ON (all)  : block the request.
  // - If AUTO (gbr): block only if GBR request.
  double pipeUsage = m_pgwInfo->GetPipeCapacityUsage (rInfo->GetPgwTftIdx ());
  if (pipeUsage >= m_tftBlockThs
      && (m_tftBlockPolicy == OpMode::ON
          || (m_tftBlockPolicy == OpMode::AUTO && rInfo->IsGbr ())))
    {
      rInfo->SetBlocked (true, RoutingInfo::TFTLOAD);
      NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                   " because the TFT processing capacity is overloaded.");
    }

  // Return false if blocked.
  return !rInfo->IsBlocked ();
}

bool
SliceController::PgwRulesInstall (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool moveFlag)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex () << pgwTftIdx << moveFlag);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = m_pgwInfo->GetTftDpId (pgwTftIdx);
  NS_LOG_INFO ("Installing P-GW rules for teid " << rInfo->GetTeidHex () <<
               " into P-GW TFT switch index " << pgwTftIdx);

  // Build the dpctl command string
  std::ostringstream cmd, act;
  cmd << "flow-mod cmd=add,table=0"
      << ",flags=" << (OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",prio=" << rInfo->GetPriority ()
      << ",idle=" << rInfo->GetTimeout ();

  // Check for meter entry.
  if (rInfo->HasMbrDl ())
    {
      if (moveFlag || !rInfo->IsMbrDlInstalled ())
        {
          // Install the per-flow meter entry.
          DpctlExecute (pgwTftDpId, rInfo->GetMbrDlAddCmd ());
          rInfo->SetMbrDlInstalled (true);
        }

      // Instruction: meter.
      act << " meter:" << rInfo->GetTeid ();
    }

  // Instruction: apply action: set tunnel ID, output port.
  act << " apply:set_field=tunn_id:"
      << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetSgwS5Addr ())
      << ",output=" << m_pgwInfo->GetTftS5PortNo (pgwTftIdx);

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
SliceController::PgwRulesRemove (
  Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool moveFlag)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex () << pgwTftIdx << moveFlag);

  // Use the rInfo P-GW TFT index when the parameter is not set.
  if (pgwTftIdx == 0)
    {
      pgwTftIdx = rInfo->GetPgwTftIdx ();
    }
  uint64_t pgwTftDpId = m_pgwInfo->GetTftDpId (pgwTftIdx);
  NS_LOG_INFO ("Removing P-GW rules for teid " << rInfo->GetTeidHex () <<
               " from P-GW TFT switch index " << pgwTftIdx);

  // Remove P-GW TFT flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,table=0"
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",cookie_mask=" << COOKIE_STRICT_MASK_STR;
  DpctlExecute (pgwTftDpId, cmd.str ());

  // Remove meter entry for this TEID.
  if (rInfo->IsMbrDlInstalled ())
    {
      DpctlExecute (pgwTftDpId, rInfo->GetMbrDelCmd ());
      if (!moveFlag)
        {
          rInfo->SetMbrDlInstalled (false);
        }
    }
  return true;
}

bool
SliceController::SgwRulesInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Installing S-GW rules for teid " << rInfo->GetTeidHex ());

  // Configure downlink.
  if (rInfo->HasDlTraffic ())
    {
      // Build the dpctl command string.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add,table=1,flags="
          << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
          << ",cookie=" << rInfo->GetTeidHex ()
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetEnbS1uAddr ())
          << ",output=" << rInfo->GetSgwS1uPortNo ();

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
              DpctlExecute (rInfo->GetSgwDpId (), cmdTcpStr);
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
              DpctlExecute (rInfo->GetSgwDpId (), cmdUdpStr);
            }
        }
    }

  // Configure uplink.
  if (rInfo->HasUlTraffic ())
    {
      // Build the dpctl command string.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add,table=2,flags="
          << (OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS)
          << ",cookie=" << rInfo->GetTeidHex ()
          << ",prio=" << rInfo->GetPriority ()
          << ",idle=" << rInfo->GetTimeout ();

      // Check for meter entry.
      if (rInfo->HasMbrUl ())
        {
          if (!rInfo->IsMbrUlInstalled ())
            {
              // Install the per-flow meter entry.
              DpctlExecute (rInfo->GetSgwDpId (), rInfo->GetMbrUlAddCmd ());
              rInfo->SetMbrUlInstalled (true);
            }

          // Instruction: meter.
          act << " meter:" << rInfo->GetTeid ();
        }

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetPgwS5Addr ())
          << ",output=" << rInfo->GetSgwS5PortNo ();

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
              DpctlExecute (rInfo->GetSgwDpId (), cmdTcpStr);
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
              DpctlExecute (rInfo->GetSgwDpId (), cmdUdpStr);
            }
        }
    }
  return true;
}

bool
SliceController::SgwRulesRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_LOG_INFO ("Removing S-GW rules for bearer teid " << rInfo->GetTeidHex ());

  // Remove flow entries for this TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del,"
      << ",cookie=" << rInfo->GetTeidHex ()
      << ",cookie_mask=" << COOKIE_STRICT_MASK_STR;
  DpctlExecute (rInfo->GetSgwDpId (), cmd.str ());

  // Remove meter entry for this TEID.
  if (rInfo->IsMbrUlInstalled ())
    {
      DpctlExecute (rInfo->GetSgwDpId (), rInfo->GetMbrDelCmd ());
      rInfo->SetMbrUlInstalled (false);
    }
  return true;
}

} // namespace ns3
