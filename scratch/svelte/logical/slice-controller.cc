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
    .AddAttribute ("SliceId",
                   "The LTE logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceId::UNKN),
                   MakeEnumAccessor (&SliceController::m_sliceId),
                   MakeEnumChecker (SliceId::HTC, SliceIdStr (SliceId::HTC),
                                    SliceId::MTC, SliceIdStr (SliceId::MTC),
                                    SliceId::TMP, SliceIdStr (SliceId::TMP)))

    // Infrastructure.
    .AddAttribute ("Aggregation",
                   "Enable bearer traffic aggregation.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::OFF),
                   MakeEnumAccessor (&SliceController::m_aggregation),
                   MakeEnumChecker (OpMode::OFF,  OpModeStr (OpMode::OFF),
                                    OpMode::ON,   OpModeStr (OpMode::ON),
                                    OpMode::AUTO, OpModeStr (OpMode::AUTO)))
    .AddAttribute ("BackhaulCtrl",
                   "The OpenFlow backhaul controller pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceController::m_backhaulCtrl),
                   MakePointerChecker<BackhaulController> ())
    .AddAttribute ("GbrBlockThs",
                   "The backhaul GBR bandwidth block threshold.",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&SliceController::m_gbrBlockThs),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("Priority",
                   "The priority for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   IntegerValue (1),
                   MakeIntegerAccessor (&SliceController::m_slicePrio),
                   MakeIntegerChecker<int> ())
    .AddAttribute ("Quota",
                   "The backhaul bandwidth quota for this slice.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   IntegerValue (0),
                   MakeIntegerAccessor (&SliceController::m_linkQuota),
                   MakeIntegerChecker<int> (0, 100))
    .AddAttribute ("Sharing",
                   "Enable backhaul bandwidth sharing.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&SliceController::m_linkSharing),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))

    // MME.
    .AddAttribute ("Mme", "The SVELTE MME pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&SliceController::m_mme),
                   MakePointerChecker<SvelteMme> ())

    // P-GW.
    .AddAttribute ("PgwBlockPolicy",
                   "P-GW overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&SliceController::m_pgwBlockPolicy),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("PgwBlockThs",
                   "The P-GW block threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&SliceController::m_pgwBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
    .AddAttribute ("PgwTftLoadBal",
                   "P-GW TFT load balancing operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::OFF),
                   MakeEnumAccessor (&SliceController::m_tftLoadBal),
                   MakeEnumChecker (OpMode::OFF,  OpModeStr (OpMode::OFF),
                                    OpMode::ON,   OpModeStr (OpMode::ON),
                                    OpMode::AUTO, OpModeStr (OpMode::AUTO)))
    .AddAttribute ("PgwTftJoinThs",
                   "The P-GW TFT join threshold.",
                   DoubleValue (0.30),
                   MakeDoubleAccessor (&SliceController::m_tftJoinThs),
                   MakeDoubleChecker<double> (0.0, 0.5))
    .AddAttribute ("PgwTftSplitThs",
                   "The P-GW TFT split threshold.",
                   DoubleValue (0.80),
                   MakeDoubleAccessor (&SliceController::m_tftSplitThs),
                   MakeDoubleChecker<double> (0.5, 1.0))
    .AddAttribute ("PgwTftTimeout",
                   "The interval between P-GW TFT load balancing operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&SliceController::m_tftTimeout),
                   MakeTimeChecker (Seconds (1)))

    // S-GW.
    .AddAttribute ("SgwBlockPolicy",
                   "S-GW overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&SliceController::m_sgwBlockPolicy),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SgwBlockThs",
                   "The S-GW block threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&SliceController::m_sgwBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))

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
    .AddTraceSource ("SessionModified", "The session modified trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_sessionModifiedTrace),
                     "ns3::SliceController::SessionModifiedTracedCallback")
    .AddTraceSource ("PgwTftLoadBal", "P-GW TFT load balancing trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_pgwTftLoadBalTrace),
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
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Reseting the blocked flag and the traffic aggregation flag, respecting
  // the operation mode.
  rInfo->ResetBlocked ();
  rInfo->SetAggregated (GetAggregation () == OpMode::ON);

  // Check for available resources on logical and infrastructure networks.
  bool success = true;
  success &= PgwBearerRequest (rInfo);
  success &= SgwBearerRequest (rInfo);
  success &= m_backhaulCtrl->BearerRequest (rInfo);
  if (success)
    {
      NS_ASSERT_MSG (!rInfo->IsBlocked (), "Bearer can't be blocked.");
      NS_LOG_INFO ("Bearer request accepted by controller.");
      if (!rInfo->IsAggregated ())
        {
          // Reserve infrastructure resources and install the bearer.
          success &= m_backhaulCtrl->BearerReserve (rInfo);
          success &= BearerInstall (rInfo);
          NS_ASSERT_MSG (success, "Error when installing the bearer.");
        }
      m_bearerRequestTrace (rInfo);
      return success;
    }

  // If we get here it's because the bearer request was blocked. When the
  // aggregation is in auto mode, check whether it can revert this.
  if (GetAggregation () == OpMode::AUTO && !rInfo->IsAggregated ())
    {
      // Reseting the blocked status and activating the traffic aggregation.
      rInfo->ResetBlocked ();
      rInfo->SetAggregated (true);

      // Check for available resources again.
      success = true;
      success &= PgwBearerRequest (rInfo);
      success &= SgwBearerRequest (rInfo);
      success &= m_backhaulCtrl->BearerRequest (rInfo);
      if (success)
        {
          NS_ASSERT_MSG (!rInfo->IsBlocked (), "Bearer can't be blocked.");
          NS_LOG_INFO ("Bearer request accepted by controller with "
                       "automatic traffic aggregation.");
          m_bearerRequestTrace (rInfo);
          return success;
        }
    }

  // If we get here it's because the bearer request was definitely blocked.
  NS_ASSERT_MSG (rInfo->IsBlocked (), "Bearer should be blocked.");
  NS_LOG_INFO ("Bearer request blocked by controller.");
  m_bearerRequestTrace (rInfo);
  return success;
}

bool
SliceController::DedicatedBearerRelease (
  EpsBearer bearer, uint64_t imsi, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << teid);

  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");

  // Release infrastructure resources and remove the bearer.
  bool success = true;
  if (!rInfo->IsAggregated ())
    {
      success &= m_backhaulCtrl->BearerRelease (rInfo);
      success &= BearerRemove (rInfo);
    }

  NS_LOG_INFO ("Bearer released by controller.");
  m_bearerReleaseTrace (rInfo);
  return success;
}

SliceId
SliceController::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

double
SliceController::GetGbrBlockThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_gbrBlockThs;
}

int
SliceController::GetPriority (void) const
{
  NS_LOG_FUNCTION (this);

  return m_slicePrio;
}

int
SliceController::GetQuota (void) const
{
  NS_LOG_FUNCTION (this);

  return m_linkQuota;
}

OpMode
SliceController::GetSharing (void) const
{
  NS_LOG_FUNCTION (this);

  return m_linkSharing;
}

OpMode
SliceController::GetAggregation (void) const
{
  NS_LOG_FUNCTION (this);

  return m_aggregation;
}

OpMode
SliceController::GetPgwBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwBlockPolicy;
}

double
SliceController::GetPgwBlockThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwBlockThs;
}

OpMode
SliceController::GetPgwTftLoadBal (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftLoadBal;
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

OpMode
SliceController::GetSgwBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwBlockPolicy;
}

double
SliceController::GetSgwBlockThs (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sgwBlockThs;
}

EpcS11SapSgw*
SliceController::GetS11SapSgw (void) const
{
  NS_LOG_FUNCTION (this);

  return m_s11SapSgw;
}

void
SliceController::NotifyPgwAttach (
  Ptr<PgwInfo> pgwInfo, Ptr<NetDevice> webSgiDev)
{
  NS_LOG_FUNCTION (this << pgwInfo << pgwInfo->GetPgwId () << webSgiDev);

  // Save the P-GW metadata.
  NS_ASSERT_MSG (!m_pgwInfo, "P-GW ID " << m_pgwInfo->GetPgwId () <<
                 " already configured with this controller.");
  m_pgwInfo = pgwInfo;

  // Set the P-GW TFT load balancing initial level.
  switch (GetPgwTftLoadBal ())
    {
    case OpMode::ON:
      {
        pgwInfo->SetCurLevel (pgwInfo->GetMaxLevel ());
        break;
      }
    case OpMode::OFF:
    case OpMode::AUTO:
      {
        pgwInfo->SetCurLevel (0);
        break;
      }
    }

  // Configuring the P-GW MAIN switch.
  // -------------------------------------------------------------------------
  // Table 0 -- P-GW MAIN default table -- [from higher to lower priority]
  //
  {
    // IP packets coming from the S-GW (P-GW S5 port) and addressed to the
    // Internet (Web IP address) have their destination MAC address rewritten
    // to the Web SGi MAC address (mandatory when using logical ports) and are
    // forward to the SGi interface port.
    Mac48Address webMac = Mac48Address::ConvertFrom (webSgiDev->GetAddress ());
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64"
        << ",table="    << PGW_MAIN_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << ",in_port="  << pgwInfo->GetMainS5PortNo ()
        << ",ip_dst="   << Ipv4AddressHelper::GetAddress (webSgiDev)
        << " write:set_field=eth_dst:" << webMac
        << ",output="   << pgwInfo->GetMainSgiPortNo ();
    DpctlExecute (pgwInfo->GetMainDpId (), cmd.str ());
  }
  {
    // IP packets coming from the Internet (P-GW SGi port) and addressed to the
    // UE network are sent to the table corresponding to the current P-GW TFT
    // load balancing level. This is the only rule that is updated when the
    // level changes, sending packets to a different pipeline table.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64"
        << ",table="    << PGW_MAIN_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << ",in_port="  << pgwInfo->GetMainSgiPortNo ()
        << ",ip_dst="   << m_ueAddr
        << "/"          << m_ueMask.GetPrefixLength ()
        << " goto:"     << pgwInfo->GetCurLevel () + 1;
    DpctlExecute (pgwInfo->GetMainDpId (), cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Table 1..N -- P-GW MAIN load balancing -- [from higher to lower priority]
  //
  for (uint16_t tftIdx = 1; tftIdx <= pgwInfo->GetMaxTfts (); tftIdx++)
    {
      // Configuring the P-GW main switch to forward traffic to different P-GW
      // TFT switches considering all possible load balancing levels.
      for (uint16_t tft = pgwInfo->GetMaxTfts (); tftIdx <= tft; tft /= 2)
        {
          uint16_t lbLevel = static_cast<uint16_t> (log2 (tft));
          uint16_t ipMask = (1 << lbLevel) - 1;
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,prio=64"
              << ",table="        << lbLevel + 1
              << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
              << " eth_type="     << IPV4_PROT_NUM
              << ",ip_dst=0.0.0." << tftIdx - 1
              << "/0.0.0."        << ipMask
              << " apply:output=" << pgwInfo->GetMainToTftPortNo (tftIdx);
          DpctlExecute (pgwInfo->GetMainDpId (), cmd.str ());
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
  NS_ASSERT_MSG (!m_sgwInfo, "S-GW ID " << m_sgwInfo->GetSgwId () <<
                 " already configured with this controller.");
  m_sgwInfo = sgwInfo;

  // -------------------------------------------------------------------------
  // Table 0 -- S-GW default table -- [from higher to lower priority]
  //
  {
    // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
    // network are sent to table 1, where rules will match the flow and set
    // both TEID and eNB address on tunnel metadata.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64"
        << ",table="    << SGW_MAIN_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << ",in_port="  << sgwInfo->GetS5PortNo ()
        << ",ip_dst="   << m_ueAddr
        << "/"          << m_ueMask.GetPrefixLength ()
        << " goto:1";
    DpctlExecute (sgwInfo->GetDpId (), cmd.str ());
  }
  {
    // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
    // Internet are sent to table 2, where rules will match the flow and set
    // both TEID and P-GW address on tunnel metadata.
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=64"
        << ",table="    << SGW_MAIN_TAB
        << ",flags="    << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type=" << IPV4_PROT_NUM
        << ",in_port="  << sgwInfo->GetS1uPortNo ()
        << ",ip_dst="   << m_webAddr
        << "/"          << m_webMask.GetPrefixLength ()
        << " goto:2";
    DpctlExecute (sgwInfo->GetDpId (), cmd.str ());
  }

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
  m_backhaulCtrl = 0;
  m_pgwInfo = 0;
  m_sgwInfo = 0;
  delete (m_s11SapSgw);
  Object::DoDispose ();
}

void
SliceController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_sliceId == SliceId::UNKN, "Unknown slice ID.");
  NS_ABORT_MSG_IF (!m_backhaulCtrl, "No backhaul controller application.");
  NS_ABORT_MSG_IF (!m_mme, "No SVELTE MME.");

  m_sliceIdStr = SliceIdStr (m_sliceId);

  // Connecting this controller to the MME.
  m_s11SapSgw = new MemberEpcS11SapSgw<SliceController> (this);
  m_s11SapMme = m_mme->GetS11SapMme ();

  // Schedule the first P-GW TFT load balancing operation.
  Simulator::Schedule (
    m_tftTimeout, &SliceController::PgwTftLoadBalancing, this);

  OFSwitch13Controller::NotifyConstructionCompleted ();
}

ofl_err
SliceController::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this error message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Slice " << SliceIdStr (GetSliceId ())
            << " controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with error message: " << msgStr
            << std::endl;
  return 0;
}

ofl_err
SliceController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = CookieGetTeid (msg->stats->cookie);
  uint16_t prio = msg->stats->priority;

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free_flow_removed (msg, true, 0);

  NS_LOG_DEBUG ("Flow removed: " << msgStr);

  // Check for existing routing information for this bearer.
  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
  NS_ASSERT_MSG (rInfo, "Routing metadata not found");

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for inactive bearer teid " << rInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the bearer
  // priority was increased and this removed flow rule is an old one.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for bearer teid " << rInfo->GetTeidHex () <<
                   " with old priority " << prio);
      return 0;
    }

  // 3) The application is running, the bearer is active, and the bearer
  // priority is the same of the removed rule. This is a critical situation!
  // For some reason, the flow rule was removed so we are going to abort the
  // program to avoid wrong results.
  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Rule removed for active bearer. " <<
                "OpenFlow flow removed message: " << msgStr);
  return 0;
}

ofl_err
SliceController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_structs_match_to_string (msg->match, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this packet-in message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Slice " << SliceIdStr (GetSliceId ())
            << " controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with packet-in message: " << msgStr
            << std::endl;
  return 0;
}

void
SliceController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);
}

void
SliceController::DpctlSchedule (Time delay, uint64_t dpId,
                                const std::string textCmd)
{
  NS_LOG_FUNCTION (this << delay << dpId << textCmd);

  Simulator::Schedule (delay, &SliceController::DpctlExecute,
                       this, dpId, textCmd);
}

bool
SliceController::BearerInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");

  // Increasing the priority every time we (re)install routing rules. Doing
  // this, we avoid problems with old 'expiring' rules, and we can even use new
  // routing paths when necessary.
  rInfo->IncreasePriority ();

  // Install the rules.
  bool success = true;
  success &= PgwRulesInstall (rInfo);
  success &= SgwRulesInstall (rInfo);
  rInfo->SetGwInstalled (success);
  success &= m_backhaulCtrl->BearerInstall (rInfo);
  return success;
}

bool
SliceController::BearerRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");
  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should not be active.");

  // Remove the rules.
  bool success = true;
  success &= PgwRulesRemove (rInfo);
  success &= SgwRulesRemove (rInfo);
  rInfo->SetGwInstalled (!success);
  success &= m_backhaulCtrl->BearerRemove (rInfo);
  return success;
}

bool
SliceController::BearerUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsAggregated (), "Bearer should not be aggregated.");

  // Each slice has a single P-GW and S-GW, so handover only changes the eNB.
  // Thus, we only need to modify the S-GW downlink rules and backhaul rules.
  bool success = true;
  success &= SgwRulesUpdate (rInfo, dstEnbInfo);
  success &= m_backhaulCtrl->BearerUpdate (rInfo, dstEnbInfo);

  // Increase the routing priority (only after updating OpenFlow rules).
  rInfo->IncreasePriority ();

  return success;
}

void
SliceController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

  NS_ASSERT_MSG (m_pgwInfo, "P-GW not configure with this controller.");
  NS_ASSERT_MSG (m_sgwInfo, "S-GW not configure with this controller.");

  uint64_t imsi = msg.imsi;
  uint16_t cellId = msg.uli.gci;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  // This controller is responsible for configuring the eNB info in the UE.
  // In current implementation, each slice has a single P-GW and S-GW nodes.
  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
  ueInfo->SetEnbInfo (enbInfo);
  ueInfo->SetSgwInfo (m_sgwInfo);
  ueInfo->SetPgwInfo (m_pgwInfo);

  // Iterate over request message and create the response message.
  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = imsi;

  for (auto const &bit : msg.bearerContextsToBeCreated)
    {
      // Allocate an unique (system-wide) TEID for this EPS bearer.
      uint32_t teid = TeidCreate (m_sliceId, imsi, bit.epsBearerId);

      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.address = ueInfo->GetSgwInfo ()->GetS1uAddr ();
      bearerContext.sgwFteid.teid = teid;
      bearerContext.epsBearerId = bit.epsBearerId;
      bearerContext.bearerLevelQos = bit.bearerLevelQos;
      bearerContext.tft = bit.tft;
      res.bearerContextsCreated.push_back (bearerContext);

      // Creating bearer routing metadata.
      Ptr<RoutingInfo> rInfo = CreateObject<RoutingInfo> (
          teid, bearerContext, ueInfo, bit.tft->IsDefaultTft ());
      NS_LOG_DEBUG ("Saving bearer info for ue imsi " << imsi <<
                    " slice " << SliceIdStr (m_sliceId) <<
                    " bid " << static_cast<uint16_t> (bit.epsBearerId) <<
                    " teid " << rInfo->GetTeidHex ());

      rInfo->SetPgwTftIdx (GetTftIdx (rInfo));
      m_backhaulCtrl->NotifyBearerCreated (rInfo);

      if (rInfo->IsDefault ())
        {
          // Configure this default bearer.
          rInfo->SetPriority (0x7F);
          rInfo->SetTimeout (OFP_FLOW_PERMANENT);

          // For logic consistence, let's check for available resources.
          bool success = true;
          success &= PgwBearerRequest (rInfo);
          success &= SgwBearerRequest (rInfo);
          success &= m_backhaulCtrl->BearerRequest (rInfo);
          NS_ASSERT_MSG (success, "Default bearer must be accepted.");

          // Activate and install the default bearer.
          rInfo->SetActive (true);
          bool installed = BearerInstall (rInfo);
          m_bearerRequestTrace (rInfo);
          NS_ASSERT_MSG (installed, "Default bearer must be installed.");
        }
      else
        {
          // Configure this dedicated bearer.
          rInfo->SetPriority (0x1FFF);
          rInfo->SetTimeout (OFP_FLOW_PERMANENT);
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

  NS_ABORT_MSG ("Unsupported method.");
}

void
SliceController::DoDeleteBearerResponse (
  EpcS11SapSgw::DeleteBearerResponseMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  NS_ABORT_MSG ("Unsupported method.");
}

void
SliceController::DoModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

  uint64_t imsi = msg.teid;
  uint16_t cellId = msg.uli.gci;
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

  // The Modify Bearer Request procedure is triggered only by X2 handover, and
  // this controller is responsible for updating the UE's eNB info only after
  // updating the OpenFlow rules.
  Ptr<EnbInfo> dstEnbInfo = EnbInfo::GetPointer (cellId);

  // Check for consistent the number of modified bearers.
  NS_ASSERT_MSG (
    msg.bearerContextsToBeModified.size () == ueInfo->GetNBearers (),
    "Inconsistent number of modified EPS bearers.");

  // Iterate over request message and create the response message.
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = imsi;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  for (auto const &bit : msg.bearerContextsToBeModified)
    {
      // Check for consistent eNB S1-U address after handover procedure.
      NS_ASSERT_MSG (bit.enbFteid.address == dstEnbInfo->GetS1uAddr (),
                     "Inconsistent eNB S1-U IPv4 address.");

      EpcS11SapMme::BearerContextModified bearerContext;
      bearerContext.sgwFteid.address = ueInfo->GetSgwInfo ()->GetS1uAddr ();
      bearerContext.sgwFteid.teid = bit.enbFteid.teid;
      bearerContext.epsBearerId = bit.epsBearerId;
      res.bearerContextsModified.push_back (bearerContext);
    }

  // Iterate over routing infos and update bearers with installed rules.
  for (auto const &rit : ueInfo->GetRoutingInfoMap ())
    {
      Ptr<RoutingInfo> rInfo = rit.second;
      if (rInfo->IsGwInstalled ())
        {
          bool success = BearerUpdate (rInfo, dstEnbInfo);
          NS_ASSERT_MSG (success, "Error updating bearer after handover.");
        }
    }

  // Finally, update the UE's eNB info (only after updating OpenFlow rules).
  ueInfo->SetEnbInfo (dstEnbInfo);

  // Fire trace source notifying the modified session.
  m_sessionModifiedTrace (imsi, res.bearerContextsModified);

  // Forward the response message to the MME.
  m_s11SapMme->ModifyBearerResponse (res);
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
SliceController::PgwTftLoadBalancing (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_pgwInfo, "No P-GW attached to this slice.");

  // Check for valid P-GW TFT thresholds attributes.
  NS_ASSERT_MSG (m_tftSplitThs < m_pgwBlockThs
                 && m_tftSplitThs > 2 * m_tftJoinThs,
                 "The split threshold should be smaller than the block "
                 "threshold and two times larger than the join threshold.");

  uint16_t nextLevel = m_pgwInfo->GetCurLevel ();
  if (GetPgwTftLoadBal () == OpMode::AUTO)
    {
      double maxTabUse = m_pgwInfo->GetTftMaxFlowTableUse ();
      double maxCpuUse = m_pgwInfo->GetTftMaxEwmaCpuUse ();

      // We may increase the level when we hit the split threshold.
      if ((m_pgwInfo->GetCurLevel () < m_pgwInfo->GetMaxLevel ())
          && (maxTabUse >= m_tftSplitThs || maxCpuUse >= m_tftSplitThs))
        {
          NS_LOG_INFO ("Increasing the load balancing level.");
          nextLevel++;
        }

      // We may decrease the level when we hit the join threshold.
      else if ((m_pgwInfo->GetCurLevel () > 0)
               && (maxTabUse < m_tftJoinThs) && (maxCpuUse < m_tftJoinThs))
        {
          NS_LOG_INFO ("Decreasing the load balancing level.");
          nextLevel--;
        }
    }

  // Check if we need to update the load balancing level.
  uint32_t moved = 0;
  if (m_pgwInfo->GetCurLevel () != nextLevel)
    {
      uint16_t futureTfts = 1 << nextLevel;

      // Random variable to avoid simultaneously moving all bearers.
      Ptr<RandomVariableStream> rand = CreateObject<UniformRandomVariable> ();
      rand->SetAttribute ("Min", DoubleValue (0));
      rand->SetAttribute ("Max", DoubleValue (250));

      // Iterate over all bearers for this slice, updating the P-GW TFT switch
      // index and moving the bearer when necessary.
      RoutingInfoList_t bearerList;
      RoutingInfo::GetList (bearerList, m_sliceId);
      for (auto const &rInfo : bearerList)
        {
          uint16_t currIdx = rInfo->GetPgwTftIdx ();
          uint16_t destIdx = GetTftIdx (rInfo, futureTfts);
          if (destIdx != currIdx)
            {
              if (!rInfo->IsGwInstalled ())
                {
                  // Update the P-GW TFT switch index so new rules will be
                  // installed in the new switch.
                  rInfo->SetPgwTftIdx (destIdx);
                }
              else
                {
                  // Schedule the rules transfer from old to new switch.
                  moved++;
                  NS_LOG_INFO ("Move bearer teid " << rInfo->GetTeidHex () <<
                               " from TFT " << currIdx << " to " << destIdx);
                  Simulator::Schedule (MilliSeconds (rand->GetInteger ()),
                                       &SliceController::PgwRulesMove,
                                       this, rInfo, currIdx, destIdx);
                }
            }
        }

      // Schedule to update the P-GW main switch.
      std::ostringstream cmd;
      cmd << "flow-mod cmd=mods,prio=64"
          << ",table="    << PGW_MAIN_TAB
          << " eth_type=" << IPV4_PROT_NUM
          << ",in_port="  << m_pgwInfo->GetMainSgiPortNo ()
          << ",ip_dst="   << m_ueAddr
          << "/"          << m_ueMask.GetPrefixLength ()
          << " goto:"     << nextLevel + 1;
      DpctlSchedule (MilliSeconds (500), m_pgwInfo->GetMainDpId (), cmd.str ());
    }

  // Fire the load balancing trace source.
  m_pgwTftLoadBalTrace (m_pgwInfo, nextLevel, moved);

  // Update the load balancing level.
  m_pgwInfo->SetCurLevel (nextLevel);

  // Schedule the next load balancing operation.
  Simulator::Schedule (
    m_tftTimeout, &SliceController::PgwTftLoadBalancing, this);
}

bool
SliceController::PgwBearerRequest (Ptr<RoutingInfo> rInfo) const
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  bool success = true;

  // First check: OpenFlow switch table usage (only non-aggregated bearers).
  // Block the bearer if the P-GW TFT switch table (#1) usage is exceeding the
  // block threshold.
  if (!rInfo->IsAggregated ())
    {
      double tabUse = m_pgwInfo->GetFlowTableUse (rInfo->GetPgwTftIdx (), 0);
      if (tabUse >= GetPgwBlockThs ())
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::PGWTABLE);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the P-GW table is full.");
        }
    }

  // Second check: OpenFlow switch CPU load (only when block policy is ON).
  // Block the bearer if the P-GW TFT switch CPU load is exceeding
  // the block threshold.
  if (GetPgwBlockPolicy () == OpMode::ON)
    {
      double cpuUse = m_pgwInfo->GetEwmaCpuUse (rInfo->GetPgwTftIdx ());
      if (cpuUse >= GetPgwBlockThs ())
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::PGWLOAD);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the P-GW is overloaded.");
        }
    }

  return success;
}

bool
SliceController::PgwRulesInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsGwInstalled (), "Gateway rules installed.");
  NS_LOG_INFO ("Installing P-GW rules for teid " << rInfo->GetTeidHex ());
  bool success = true;

  uint64_t pgwTftDpId = rInfo->GetPgwTftDpId ();
  NS_LOG_DEBUG ("Installing into P-GW TFT idx " << rInfo->GetPgwTftIdx ());

  // Configure downlink.
  if (rInfo->HasDlTraffic ())
    {
      // Cookie for new downlink rules.
      uint64_t cookie = CookieCreate (
          LteIface::S5, rInfo->GetPriority (), rInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add"
          << ",table="  << PGW_TFT_TAB
          << ",flags="  << FLAGS_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (cookie)
          << ",prio="   << rInfo->GetPriority ()
          << ",idle="   << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetSgwS5Addr ())
          << ",output=" << rInfo->GetPgwTftS5PortNo ();

      // Install downlink OpenFlow TFT rules.
      success &= TftRulesInstall (rInfo->GetTft (), Direction::DLINK,
                                  pgwTftDpId, cmd.str (), act.str ());
    }

  return success;
}

bool
SliceController::PgwRulesMove (
  Ptr<RoutingInfo> rInfo, uint16_t srcTftIdx, uint16_t dstTftIdx)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex () << srcTftIdx << dstTftIdx);

  NS_LOG_INFO ("Moving P-GW rules for teid " << rInfo->GetTeidHex ());
  bool success = true;

  // Update the P-GW TFT switch index.
  rInfo->SetPgwTftIdx (dstTftIdx);

  if (rInfo->HasDlTraffic () && rInfo->IsGwInstalled ())
    {
      uint64_t srcTftDpId = m_pgwInfo->GetTftDpId (srcTftIdx);
      uint64_t dstTftDpId = m_pgwInfo->GetTftDpId (dstTftIdx);
      NS_LOG_DEBUG ("Moving from P-GW TFT switch index " <<
                    srcTftIdx << " to " << dstTftIdx);

      // Schedule the removal of rules from source switch.
      // Building the dpctl command. Matching cookie just for TEID.
      std::ostringstream del;
      del << "flow-mod cmd=del"
          << ",table="        << PGW_TFT_TAB
          << ",cookie="       << GetUint64Hex (rInfo->GetTeid ())
          << ",cookie_mask="  << GetUint64Hex (COOKIE_TEID_MASK);
      DpctlSchedule (MilliSeconds (750), srcTftDpId, del.str ());

      // Install rules into target switch now.
      // Cookie for new downlink rules.
      uint64_t cookie = CookieCreate (
          LteIface::S5, rInfo->GetPriority (), rInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add"
          << ",table="  << PGW_TFT_TAB
          << ",flags="  << FLAGS_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (cookie)
          << ",prio="   << rInfo->GetPriority ()
          << ",idle="   << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetSgwS5Addr ())
          << ",output=" << rInfo->GetPgwTftS5PortNo ();

      // Install downlink OpenFlow TFT rules.
      success &= TftRulesInstall (rInfo->GetTft (), Direction::DLINK,
                                  dstTftDpId, cmd.str (), act.str ());
    }

  return success;
}

bool
SliceController::PgwRulesRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsGwInstalled (), "Gateway rules not installed.");
  NS_LOG_INFO ("Removing P-GW rules for teid " << rInfo->GetTeidHex ());

  uint64_t pgwTftDpId = rInfo->GetPgwTftDpId ();
  NS_LOG_DEBUG ("Removing from P-GW TFT idx " << rInfo->GetPgwTftIdx ());

  // Building the dpctl command. Matching cookie just for TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",table="        << PGW_TFT_TAB
      << ",cookie="       << GetUint64Hex (rInfo->GetTeid ())
      << ",cookie_mask="  << GetUint64Hex (COOKIE_TEID_MASK);
  DpctlExecute (pgwTftDpId, cmd.str ());

  return true;
}

bool
SliceController::SgwBearerRequest (Ptr<RoutingInfo> rInfo) const
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  Ptr<SgwInfo> sgwInfo = rInfo->GetUeInfo ()->GetSgwInfo ();
  bool success = true;

  // First check: OpenFlow switch table usage (only non-aggregated bearers).
  // Block the bearer if any of the S-GW switch tables (#1 or #2) usage is
  // exceeding the block threshold.
  if (!rInfo->IsAggregated ())
    {
      double dlTabUse = sgwInfo->GetFlowTableUse (1);
      double ulTabUse = sgwInfo->GetFlowTableUse (2);
      if (dlTabUse >= GetSgwBlockThs () || ulTabUse >= GetSgwBlockThs ())
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::SGWTABLE);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the S-GW table is full.");
        }
    }

  // Second check: OpenFlow switch CPU load (only when block policy is ON).
  // Block the bearer if the S-GW switch CPU load is exceeding
  // the block threshold.
  if (GetSgwBlockPolicy () == OpMode::ON)
    {
      double cpuUse = sgwInfo->GetEwmaCpuUse ();
      if (cpuUse >= GetSgwBlockThs ())
        {
          success = false;
          rInfo->SetBlocked (RoutingInfo::SGWLOAD);
          NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeidHex () <<
                       " because the S-GW is overloaded.");
        }
    }

  return success;
}

bool
SliceController::SgwRulesInstall (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (!rInfo->IsGwInstalled (), "Gateway rules installed.");
  NS_LOG_INFO ("Installing S-GW rules for teid " << rInfo->GetTeidHex ());
  bool success = true;

  // Configure downlink.
  if (rInfo->HasDlTraffic ())
    {
      // Cookie for new downlink rules.
      uint64_t cookie = CookieCreate (
          LteIface::S1, rInfo->GetPriority (), rInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add"
          << ",table="  << SGW_DL_TAB
          << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (cookie)
          << ",prio="   << rInfo->GetPriority ()
          << ",idle="   << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetEnbS1uAddr ())
          << ",output=" << rInfo->GetSgwS1uPortNo ();

      // Install downlink OpenFlow TFT rules.
      success &= TftRulesInstall (rInfo->GetTft (), Direction::DLINK,
                                  rInfo->GetSgwDpId (), cmd.str (), act.str ());
    }

  // Configure uplink.
  if (rInfo->HasUlTraffic ())
    {
      // Cookie for new uplink rules.
      uint64_t cookie = CookieCreate (
          LteIface::S5, rInfo->GetPriority (), rInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add"
          << ",table="  << SGW_UL_TAB
          << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (cookie)
          << ",prio="   << rInfo->GetPriority ()
          << ",idle="   << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"
          << GetTunnelIdStr (rInfo->GetTeid (), rInfo->GetPgwS5Addr ())
          << ",output=" << rInfo->GetSgwS5PortNo ();

      // Install uplink OpenFlow TFT rules.
      success &= TftRulesInstall (rInfo->GetTft (), Direction::ULINK,
                                  rInfo->GetSgwDpId (), cmd.str (), act.str ());
    }

  return success;
}

bool
SliceController::SgwRulesRemove (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsGwInstalled (), "Gateway rules not installed.");
  NS_LOG_INFO ("Removing S-GW rules for bearer teid " << rInfo->GetTeidHex ());

  // Building the dpctl command. Matching cookie just for TEID.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=del"
      << ",cookie="       << GetUint64Hex (rInfo->GetTeid ())
      << ",cookie_mask="  << GetUint64Hex (COOKIE_TEID_MASK);
  DpctlExecute (rInfo->GetSgwDpId (), cmd.str ());

  return true;
}

bool
SliceController::SgwRulesUpdate (Ptr<RoutingInfo> rInfo,
                                 Ptr<EnbInfo> dstEnbInfo)
{
  NS_LOG_FUNCTION (this << rInfo->GetTeidHex ());

  NS_ASSERT_MSG (rInfo->IsGwInstalled (), "Gateway rules not installed.");
  NS_LOG_INFO ("Updating S-GW S1-U rules for teid " << rInfo->GetTeidHex ());
  bool success = true;

  if (rInfo->HasDlTraffic ())
    {
      // Schedule the removal of old low-priority OpenFlow rules.
      // Cookie for old rules.
      uint64_t oldCookie = CookieCreate (
          LteIface::S1, rInfo->GetPriority (), rInfo->GetTeid ());

      // Building the dpctl command. Strict matching cookie.
      std::ostringstream del;
      del << "flow-mod cmd=del"
          << ",table="        << SGW_DL_TAB
          << ",cookie="       << GetUint64Hex (oldCookie)
          << ",cookie_mask="  << GetUint64Hex (COOKIE_STRICT_MASK);
      DpctlSchedule (MilliSeconds (250), rInfo->GetSgwDpId (), del.str ());

      // Install updated rules now.
      // Cookie for new downlink rules.
      uint16_t newPriority = rInfo->GetPriority () + 1;
      uint64_t newCookie = CookieCreate (
          LteIface::S1, newPriority, rInfo->GetTeid ());

      // Building the dpctl command.
      std::ostringstream cmd, act;
      cmd << "flow-mod cmd=add"
          << ",table="  << SGW_DL_TAB
          << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
          << ",cookie=" << GetUint64Hex (newCookie)
          << ",prio="   << newPriority
          << ",idle="   << rInfo->GetTimeout ();

      // Instruction: apply action: set tunnel ID, output port.
      act << " apply:set_field=tunn_id:"          // Target eNB
          << GetTunnelIdStr (rInfo->GetTeid (), dstEnbInfo->GetS1uAddr ())
          << ",output=" << rInfo->GetSgwS1uPortNo ();

      // Install new high-priority downlink OpenFlow TFT rules.
      success &= TftRulesInstall (rInfo->GetTft (), Direction::DLINK,
                                  rInfo->GetSgwDpId (), cmd.str (), act.str ());
    }

  return success;
}

bool
SliceController::TftRulesInstall (
  Ptr<EpcTft> tft, Direction dir, uint64_t dpId,
  std::string cmdStr, std::string actStr)
{
  NS_LOG_FUNCTION (this << tft << dir << dpId << cmdStr << actStr);

  // Configure variables for the given traffic direction.
  std::string local = "dst=";
  std::string remot = "src=";
  EpcTft::Direction skipDir = EpcTft::UPLINK;
  if (dir == Direction::ULINK)
    {
      local = "src=";
      remot = "dst=";
      skipDir = EpcTft::DOWNLINK;
    }

  // Install one dedicated rule for each packet filter.
  for (uint8_t i = 0; i < tft->GetNFilters (); i++)
    {
      EpcTft::PacketFilter filter = tft->GetFilter (i);
      if (filter.direction != skipDir)
        {
          if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
            {
              // Install rules for TCP traffic.
              std::ostringstream mat;
              mat << " eth_type="    << IPV4_PROT_NUM
                  << ",ip_proto="    << TCP_PROT_NUM
                  << ",ip_" << local << filter.localAddress;
              if (tft->IsDefaultTft () == false)
                {
                  mat << ",ip_"  << remot << filter.remoteAddress
                      << ",tcp_" << remot << filter.remotePortStart;
                }
              DpctlExecute (dpId, cmdStr + mat.str () + actStr);
            }
          else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
            {
              // Install rules for UDP traffic.
              std::ostringstream mat;
              mat << " eth_type="    << IPV4_PROT_NUM
                  << ",ip_proto="    << UDP_PROT_NUM
                  << ",ip_" << local << filter.localAddress;
              if (tft->IsDefaultTft () == false)
                {
                  mat << ",ip_"  << remot << filter.remoteAddress
                      << ",udp_" << remot << filter.remotePortStart;
                }
              DpctlExecute (dpId, cmdStr + mat.str () + actStr);
            }
        }
    }

  return true;
}

} // namespace ns3
