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
#include "svelte-mme.h"
#include "../infrastructure/backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceController");
NS_OBJECT_ENSURE_REGISTERED (SliceController);

// Initializing SliceController static members.
// TODO Gerenciar TEID...
// FIXME Montar um esquema que o teid tenham bits identificando o slice e outros identificando o tunel.
const uint16_t SliceController::m_flowTimeout = 0;
const uint32_t SliceController::m_teidStart = 0x100;
const uint32_t SliceController::m_teidEnd = 0xFEFFFFFF;
uint32_t SliceController::m_teidCount = SliceController::m_teidStart;


SliceController::SliceController ()
  : m_tftMaxLoad (DataRate (std::numeric_limits<uint64_t>::max ())),
  m_tftTableSize (std::numeric_limits<uint32_t>::max ())
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

    // Controller
    .AddAttribute ("TimeoutInterval",
                   "The interval between internal periodic operations.",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&SliceController::m_timeout),
                   MakeTimeChecker ())

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
                   EnumValue (OperationMode::OFF),
                   MakeEnumAccessor (&SliceController::m_tftAdaptive),
                   MakeEnumChecker (OperationMode::OFF,  "off",
                                    OperationMode::ON,   "on",
                                    OperationMode::AUTO, "auto"))
    .AddAttribute ("PgwTftBlockPolicy",
                   "P-GW TFT overloaded block policy.",
                   EnumValue (OperationMode::ON),
                   MakeEnumAccessor (&SliceController::m_tftBlockPolicy),
                   MakeEnumChecker (OperationMode::OFF,  "none",
                                    OperationMode::ON,   "all",
                                    OperationMode::AUTO, "gbr"))
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

    // Aggregation. // FIXME Desabilitar isso???
//    .AddAttribute ("HtcAggregation",
//                   "HTC traffic aggregation mechanism operation mode.",
//                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
//                   EnumValue (OperationMode::OFF),
//                   MakeEnumAccessor (&SliceController::m_htcAggregation),
//                   MakeEnumChecker (OperationMode::OFF,  "off",
//                                    OperationMode::ON,   "on",
//                                    OperationMode::AUTO, "auto"))
//    .AddAttribute ("HtcAggGbrThs",
//                   "HTC traffic aggregation GBR bandwidth threshold.",
//                   DoubleValue (0.5),
//                   MakeDoubleAccessor (&SliceController::m_htcAggGbrThs),
//                   MakeDoubleChecker<double> (0.0, 1.0))
//    .AddAttribute ("HtcAggNonThs",
//                   "HTC traffic aggregation Non-GBR bandwidth threshold.",
//                   DoubleValue (0.5),
//                   MakeDoubleAccessor (&SliceController::m_htcAggNonThs),
//                   MakeDoubleChecker<double> (0.0, 1.0))
//    .AddAttribute ("MtcAggregation",
//                   "MTC traffic aggregation mechanism operation mode.",
//                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
//                   EnumValue (OperationMode::OFF),
//                   MakeEnumAccessor (&SliceController::m_mtcAggregation),
//                   MakeEnumChecker (OperationMode::OFF,  "off",
//                                    OperationMode::ON,   "on"))

// FIXME Comentado por causa da dependência com o rInfo.
//    .AddTraceSource ("BearerRelease", "The bearer release trace source.",
//                     MakeTraceSourceAccessor (
//                       &SliceController::m_bearerReleaseTrace),
//                     "ns3::RoutingInfo::TracedCallback")
//    .AddTraceSource ("BearerRequest", "The bearer request trace source.",
//                     MakeTraceSourceAccessor (
//                       &SliceController::m_bearerRequestTrace),
//                     "ns3::RoutingInfo::TracedCallback")
    .AddTraceSource ("PgwTftStats", "The P-GW TFT stats trace source.",
                     MakeTraceSourceAccessor (
                       &SliceController::m_pgwTftStatsTrace),
                     "ns3::SliceController::PgwTftStatsTracedCallback")
//    .AddTraceSource ("SessionCreated", "The session created trace source.",
//                     MakeTraceSourceAccessor (
//                       &SliceController::m_sessionCreatedTrace),
//                     "ns3::SliceController::SessionCreatedTracedCallback")
  ;
  return tid;
}

bool
SliceController::DedicatedBearerRelease (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

//   SgwRulesRemove (RoutingInfo::GetPointer (teid));
//   m_epcCtrlApp->DedicatedBearerRelease (bearer, teid);

//   FIXME Copiei do SliceController::DedicatedBearerRelease
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//
//  // This bearer must be active.
//  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't release the default bearer.");
//  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
//
//  TopologyBitRateRelease (rInfo);
//  m_bearerReleaseTrace (rInfo);
//  NS_LOG_INFO ("Bearer released by controller.");
//
//  // Deactivate and remove the bearer.
//  rInfo->SetActive (false);
//  return BearerRemove (rInfo);

  return true;
}

bool
SliceController::DedicatedBearerRequest (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

//   FIXME copiei do SliceController::DedicatedBearerRequest
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//
//  // This bearer must be inactive as we are going to reuse its metadata.
//  NS_ASSERT_MSG (!rInfo->IsDefault (), "Can't request the default bearer.");
//  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");
//
//  // Update the P-GW TFT index and the blocked flag.
//  rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
//  rInfo->SetBlocked (false);
//
//  // Update the aggregation threshold values.
//  Ptr<S5AggregationInfo> aggInfo = rInfo->GetObject<S5AggregationInfo> ();
//  aggInfo->SetThreshold (rInfo->IsMtc () ? 0.0 :
//                         rInfo->IsGbr () ? m_htcAggGbrThs : m_htcAggNonThs);
//
//  // Check for available resources on P-GW and backhaul network and then
//  // reserve the requested bandwidth (don't change the order!).
//  bool success = true;
//  success &= TopologyBearerRequest (rInfo);
//  success &= PgwBearerRequest (rInfo);
//  success &= TopologyBitRateReserve (rInfo);
//  m_bearerRequestTrace (rInfo);
//  if (!success)
//    {
//      NS_LOG_INFO ("Bearer request blocked by controller.");
//      return false;
//    }
//
//  // Every time the application starts using an (old) existing bearer, let's
//  // reinstall the rules on the switches, which will increase the bearer
//  // priority. Doing this, we avoid problems with old 'expiring' rules, and
//  // we can even use new routing paths when necessary.
//  NS_LOG_INFO ("Bearer request accepted by controller.");
//  if (rInfo->IsAggregated ())
//    {
//      NS_LOG_INFO ("Aggregating bearer teid " << rInfo->GetTeid ());
//    }
//
//  // Activate and install the bearer.
//  rInfo->SetActive (true);
//  return BearerInstall (rInfo);


//   if (m_epcCtrlApp->DedicatedBearerRequest (bearer, teid))
//     {
//       return SgwRulesInstall (RoutingInfo::GetPointer (teid));
//     }
  return false;
}

void
SliceController::NotifySgwAttach (
  Ptr<OFSwitch13Device> sgwSwDev, Ptr<NetDevice> sgwS1uDev,
  uint32_t sgwS1uPortNo, Ptr<NetDevice> sgwS5Dev, uint32_t sgwS5PortNo)
{
  NS_LOG_FUNCTION (this << sgwSwDev << sgwS1uDev << sgwS1uPortNo <<
                   sgwS5Dev << sgwS5PortNo);

  // -------------------------------------------------------------------------
  // Table 0 -- S-GW default table -- [from higher to lower priority]
  //
  // IP packets coming from the P-GW (S-GW S5 port) and addressed to the UE
  // network are sent to table 1, where rules will match the flow and set both
  // TEID and eNB address on tunnel metadata.
  std::ostringstream cmdDl;
  cmdDl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << sgwS5PortNo
        << ",ip_dst=" << m_ueAddr << "/" << m_ueMask.GetPrefixLength ()
        << " goto:1";
  DpctlSchedule (sgwSwDev->GetDatapathId (), cmdDl.str ());

  // IP packets coming from the eNB (S-GW S1-U port) and addressed to the
  // Internet are sent to table 2, where rules will match the flow and set both
  // TEID and P-GW address on tunnel metadata.
  std::ostringstream cmdUl;
  cmdUl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << sgwS1uPortNo
        << ",ip_dst=" << m_webAddr << "/" << m_webMask.GetPrefixLength ()
        << " goto:2";
  DpctlSchedule (sgwSwDev->GetDatapathId (), cmdUl.str ());

  // FIXME Aggregation
  // // The mtcGbrTeid != 0 or mtcNonTeid != 0 means that MTC traffic aggregation
  // // is enable. Install high-priority match rules on default table for
  // // aggregating traffic from all MTC UEs on the proper uplink S5 GTP tunnel.
  // if (mtcGbrTeid != 0)
  //   {
  //     // Print MTC aggregation TEID and P-GW IPv4 address into tunnel metadata.
  //     Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (mtcGbrTeid);
  //     uint64_t tunnelId;
  //     char tunnelIdStr [20];
  //     tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
  //     tunnelId <<= 32;
  //     tunnelId |= rInfo->GetTeid ();
  //     sprintf (tunnelIdStr, "0x%016lx", tunnelId);
  //
  //     // Instal OpenFlow MTC aggregation rule. We are using the DSCP field to
  //     // distinguish GBR/Non-GBR packets. Packets inside the S-GW are not
  //     // encapsulated, so the DSCP field is, in fact, the IP ToS set by the
  //     // application socket.
  //     uint8_t ipTos = SliceController::Dscp2Tos (rInfo->GetDscp ());
  //     std::ostringstream cmd;
  //     cmd << "flow-mod cmd=add,table=0,prio=65520 eth_type=0x800"
  //         << ",ip_src=" << LteNetwork::m_mtcAddr
  //         << "/" << LteNetwork::m_mtcMask.GetPrefixLength ()
  //         << ",ip_dscp=" << (ipTos >> 2)
  //         << " apply:set_field=tunn_id:" << tunnelIdStr
  //         << ",output=" << m_sgwS5PortNo;
  //     DpctlSchedule (m_sgwDpId, cmd.str ());
  //   }
  // if (mtcNonTeid != 0)
  //   {
  //     // Print MTC aggregation TEID and P-GW IPv4 address into tunnel metadata.
  //     Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (mtcNonTeid);
  //     uint64_t tunnelId;
  //     char tunnelIdStr [20];
  //     tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
  //     tunnelId <<= 32;
  //     tunnelId |= rInfo->GetTeid ();
  //     sprintf (tunnelIdStr, "0x%016lx", tunnelId);
  //
  //     // Instal OpenFlow MTC aggregation rule. We are using the DSCP field to
  //     // distinguish GBR/Non-GBR packets. Packets inside the S-GW are not
  //     // encapsulated, so the DSCP field is, in fact, the IP ToS set by the
  //     // application socket.
  //     uint8_t ipTos = SliceController::Dscp2Tos (rInfo->GetDscp ());
  //     std::ostringstream cmd;
  //     cmd << "flow-mod cmd=add,table=0,prio=65520 eth_type=0x800"
  //         << ",ip_src=" << LteNetwork::m_mtcAddr
  //         << "/" << LteNetwork::m_mtcMask.GetPrefixLength ()
  //         << ",ip_dscp=" << (ipTos >> 2)
  //         << " apply:set_field=tunn_id:" << tunnelIdStr
  //         << ",output=" << m_sgwS5PortNo;
  //     DpctlSchedule (m_sgwDpId, cmd.str ());
  //   }

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
SliceController::NotifyPgwMainAttach (
  Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwS5Dev,
  uint32_t pgwS5PortNo, Ptr<NetDevice> pgwSgiDev, uint32_t pgwSgiPortNo,
  Ptr<NetDevice> webSgiDev)
{
  NS_LOG_FUNCTION (this << pgwSwDev << pgwS5Dev << pgwS5PortNo <<
                   pgwSgiDev << pgwSgiPortNo << webSgiDev);

  // Saving information for P-GW main switch at first index.
  m_pgwDpIds.push_back (pgwSwDev->GetDatapathId ());
  m_pgwS5PortsNo.push_back (pgwS5PortNo);
  m_pgwS5Addr = Ipv4AddressHelper::GetAddress (pgwS5Dev);
  m_pgwSgiPortNo = pgwSgiPortNo;

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
        << ",in_port=" << pgwS5PortNo
        << ",ip_dst=" << Ipv4AddressHelper::GetAddress (webSgiDev)
        << " write:set_field=eth_dst:" << webMac
        << ",output=" << pgwSgiPortNo;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdUl.str ());

  // IP packets coming from the Internet (P-GW SGi port) and addressed to the
  // UE network are sent to the table corresponding to the current P-GW
  // adaptive mechanism level.
  std::ostringstream cmdDl;
  cmdDl << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
        << ",in_port=" << pgwSgiPortNo
        << ",ip_dst=" << m_ueAddr << "/" << m_ueMask.GetPrefixLength ()
        << " goto:" << m_tftLevel + 1;
  DpctlSchedule (pgwSwDev->GetDatapathId (), cmdDl.str ());

  // -------------------------------------------------------------------------
  // Table 1..N -- P-GW MAIN adaptive level -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyPgwTftAttach function.
}

void
SliceController::NotifyPgwTftAttach (
  Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwS5Dev,
  uint32_t pgwS5PortNo, uint32_t pgwMainPortNo, uint16_t pgwTftCounter)
{
  NS_LOG_FUNCTION (this << pgwSwDev << pgwS5Dev << pgwS5PortNo <<
                   pgwS5PortNo << pgwTftCounter);

  // Saving information for P-GW TFT switches.
  NS_ASSERT_MSG (pgwTftCounter < m_tftSwitches, "No more TFTs allowed.");
  m_pgwDpIds.push_back (pgwSwDev->GetDatapathId ());
  m_pgwS5PortsNo.push_back (pgwS5PortNo);

  uint32_t tableSize = pgwSwDev->GetFlowTableSize ();
  DataRate plCapacity = pgwSwDev->GetPipelineCapacity ();
  m_tftTableSize = std::min (m_tftTableSize, tableSize);
  m_tftMaxLoad = std::min (m_tftMaxLoad, plCapacity);

  // -------------------------------------------------------------------------
  // Table 0 -- P-GW TFT default table -- [from higher to lower priority]
  //
  // Entries will be installed here by PgwRulesInstall function.

  // -------------------------------------------------------------------------
  // Table 1..N -- P-GW MAIN adaptive level -- [from higher to lower priority]
  //
  // Configuring the P-GW main switch to forward traffic to this P-GW TFT
  // switch considering all possible adaptive mechanism levels.
  for (uint16_t tft = m_tftSwitches; pgwTftCounter + 1 <= tft; tft /= 2)
    {
      uint16_t lbLevel = static_cast<uint16_t> (log2 (tft));
      uint16_t ipMask = (1 << lbLevel) - 1;
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64,table=" << lbLevel + 1
          << " eth_type=0x800,ip_dst=0.0.0." << pgwTftCounter
          << "/0.0.0." << ipMask
          << " apply:output=" << pgwMainPortNo;
      DpctlSchedule (GetPgwMainDpId (), cmd.str ());
    }
}

void
SliceController::NotifyPgwBuilt (OFSwitch13DeviceContainer devices)
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

OperationMode
SliceController::GetAggregationMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_aggregation;
}

OperationMode
SliceController::GetPgwAdaptiveMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_tftAdaptive;
}

void
SliceController::SetNetworkAttributes (
  uint16_t nPgwTfts, Ipv4Address ueAddr, Ipv4Mask ueMask,
  Ipv4Address webAddr, Ipv4Mask webMask)
{
  NS_LOG_FUNCTION (this << nPgwTfts << ueAddr << ueMask << webAddr << webMask);

  m_tftSwitches = nPgwTfts;
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
  delete (m_s11SapSgw);

  Object::DoDispose ();
}

void
SliceController::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (!m_mme, "No SVELTE MME.");

  // Connecting this controller to the MME.
  m_s11SapSgw = new MemberEpcS11SapSgw<SliceController> (this);
  // m_mme->SetS11SapSgw (m_s11SapSgw); FIXME Should be AddS11SapSgw?
  m_s11SapMme = m_mme->GetS11SapMme ();

  // Set the initial number of P-GW TFT active switches.
  switch (GetPgwAdaptiveMode ())
    {
    case OperationMode::ON:
    case OperationMode::AUTO:
      {
        m_tftLevel = static_cast<uint8_t> (log2 (m_tftSwitches));
        break;
      }
    case OperationMode::OFF:
      {
        m_tftLevel = 0;
        break;
      }
    }

  // Schedule the first timeout operation.
  Simulator::Schedule (m_timeout, &SliceController::ControllerTimeout, this);

  // Chain up.
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

  return 0; // FIXME remover
// FIXME Comentado apenas para remover a dependencia do rinfo
//  uint32_t teid = msg->stats->cookie;
//  uint16_t prio = msg->stats->priority;
//
//  char *msgStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
//  NS_LOG_DEBUG ("Flow removed: " << msgStr);
//  free (msgStr);
//
//  // Since handlers must free the message when everything is ok,
//  // let's remove it now, as we already got the necessary information.
//  ofl_msg_free_flow_removed (msg, true, 0);
//
//  // Check for existing routing information for this bearer.
//  Ptr<RoutingInfo> rInfo = RoutingInfo::GetPointer (teid);
//  NS_ASSERT_MSG (rInfo, "No routing for dedicated bearer teid " << teid);
//
//  // When a flow is removed, check the following situations:
//  // 1) The application is stopped and the bearer must be inactive.
//  if (!rInfo->IsActive ())
//    {
//      NS_LOG_INFO ("Rule removed for inactive bearer teid " << teid);
//      return 0;
//    }
//
//  // 2) The application is running and the bearer is active, but the
//  // application has already been stopped since last rule installation. In this
//  // case, the bearer priority should have been increased to avoid conflicts.
//  if (rInfo->GetPriority () > prio)
//    {
//      NS_LOG_INFO ("Old rule removed for bearer teid " << teid);
//      return 0;
//    }
//
//  // 3) The application is running and the bearer is active. This is the
//  // critical situation. For some reason, the traffic absence lead to flow
//  // expiration, and we are going to abort the program to avoid wrong results.
//  NS_ASSERT_MSG (rInfo->GetPriority () == prio, "Invalid flow priority.");
//  NS_ABORT_MSG ("Should not get here :/");
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

//bool
//SliceController::BearerInstall (Ptr<RoutingInfo> rInfo)
//{
//  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//  // FIXME Aqui tem que fazer a interface direta lá com o controlador do backaul
//  NS_ASSERT_MSG (rInfo->IsActive (), "Bearer should be active.");
//  rInfo->SetInstalled (false);
//
//  if (rInfo->IsAggregated ())
//    {
//      // Don't install rules for aggregated traffic. This will automatically
//      // force the traffic over the S5 default bearer.
//      return true;
//    }
//
//  // Increasing the priority every time we (re)install routing rules.
//  rInfo->IncreasePriority ();
//
//  // Install the rules.
//  bool success = true;
//  success &= PgwRulesInstall (rInfo);
//  success &= TopologyRoutingInstall (rInfo);
//
//  rInfo->SetInstalled (success);
//  return success;
//}
//
//bool
//SliceController::BearerRemove (Ptr<RoutingInfo> rInfo)
//{
//  NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//  // FIXME Aqui tem que fazer a interface direta lá com o controlador do backaul
//  NS_ASSERT_MSG (!rInfo->IsActive (), "Bearer should be inactive.");
//
//  if (rInfo->IsAggregated ())
//    {
//      // No rules to remove for aggregated traffic.
//      return true;
//    }
//
//  // Remove the rules.
//  bool success = true;
//  success &= PgwRulesRemove (rInfo);
//  success &= TopologyRoutingRemove (rInfo);
//
//  rInfo->SetInstalled (!success);
//  return success;
//}


void
SliceController::ControllerTimeout (void)
{
  NS_LOG_FUNCTION (this);

  PgwTftCheckUsage ();

  // Schedule the next timeout operation.
  Simulator::Schedule (m_timeout, &SliceController::ControllerTimeout, this);
}

//
// On the following Do* methods, note the trick to avoid the need for
// allocating TEID on the S11 interface using the IMSI as identifier.
//
void
SliceController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  NS_LOG_FUNCTION (this << msg.imsi);

//  FIXME O código abaixo veio do antigo SliceController. Antes o mesmo controlador configurava o backhaul e o P-GW. Agora eu vou ter que chamar manualmente alguma função no controlador do backhaul pra configurar a parte de lá serparado dos gateways aqui.
//  uint16_t cellId = msg.uli.gci;
//  uint64_t imsi = msg.imsi;
//
//  Ptr<SdranController> sdranCtrl = SdranController::GetPointer (cellId);
//  Ptr<EnbInfo> enbInfo = EnbInfo::GetPointer (cellId);
//  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
//
//  // Iterate over request message and create the response message.
//  EpcS11SapMme::CreateSessionResponseMessage res;
//  res.teid = imsi;
//
//  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
//  for (bit = msg.bearerContextsToBeCreated.begin ();
//       bit != msg.bearerContextsToBeCreated.end ();
//       ++bit)
//    {
//      NS_ABORT_IF (SliceController::m_teidCount > SliceController::m_teidEnd);
//
//      uint32_t teid = SliceController::m_teidCount++;
//      bool isDefault = res.bearerContextsCreated.empty ();
//
//      EpcS11SapMme::BearerContextCreated bearerContext;
//      bearerContext.sgwFteid.teid = teid;
//      bearerContext.sgwFteid.address = enbInfo->GetSgwS1uAddr ();
//      bearerContext.epsBearerId = bit->epsBearerId;
//      bearerContext.bearerLevelQos = bit->bearerLevelQos;
//      bearerContext.tft = bit->tft;
//      res.bearerContextsCreated.push_back (bearerContext);
//
//      // Add the TFT entry to the UeInfo (don't move this command from here).
//      ueInfo->AddTft (bit->tft, teid);
//
//      // Create the routing metadata for this bearer.
//      Ptr<RoutingInfo> rInfo = CreateObject<RoutingInfo> (
//          teid, bearerContext, imsi, isDefault, ueInfo->IsMtc ());
//      rInfo->SetPgwS5Addr (m_pgwS5Addr);
//      rInfo->SetPgwTftIdx (GetPgwTftIdx (rInfo));
//      rInfo->SetSgwS5Addr (sdranCtrl->GetSgwS5Addr ());
//      TopologyBearerCreated (rInfo);
//
//      // Set the network slice for this bearer.
//      if (GetSlicingMode () != OperationMode::OFF)
//        {
//          if (rInfo->IsMtc ())
//            {
//              rInfo->SetSlice (Slice::MTC);
//            }
//          else if (rInfo->IsGbr ())
//            {
//              rInfo->SetSlice (Slice::GBR);
//            }
//        }
//
//      // Check for the proper traffic aggregation mode for this bearer.
//      OperationMode mode;
//      mode = rInfo->IsMtc () ? GetMtcAggregMode () : GetHtcAggregMode ();
//      if (rInfo->IsHtc () && rInfo->IsDefault ())
//        {
//          // Never aggregates the default HTC bearer.
//          mode = OperationMode::OFF;
//        }
//
//      // Set the traffic aggregation operation mode.
//      Ptr<S5AggregationInfo> aggInfo = rInfo->GetObject<S5AggregationInfo> ();
//      aggInfo->SetOperationMode (mode);
//
//      if (rInfo->IsDefault ())
//        {
//          // Configure this default bearer.
//          rInfo->SetPriority (0x7F);
//          rInfo->SetTimeout (0);
//
//          // For logic consistence, let's check for available resources.
//          bool success = true;
//          success &= TopologyBearerRequest (rInfo);
//          success &= PgwBearerRequest (rInfo);
//          success &= TopologyBitRateReserve (rInfo);
//          NS_ASSERT_MSG (success, "Default bearer must be accepted.");
//          m_bearerRequestTrace (rInfo);
//
//          // Activate and install the bearer.
//          rInfo->SetActive (true);
//          bool installed = BearerInstall (rInfo);
//          NS_ASSERT_MSG (installed, "Default bearer must be installed.");
//        }
//      else
//        {
//          // Configure this dedicated bearer.
//          rInfo->SetPriority (0x1FFF);
//          rInfo->SetTimeout (m_flowTimeout);
//        }
//    }
//
//  // Fire trace source notifying the created session.
//  m_sessionCreatedTrace (imsi, cellId, res.bearerContextsCreated);
//
//   /// FIXME Esse pedaço abaixo veio da mensagem de volta, DoCreateSessionResponse no sdran.
//   // Install S-GW rules for default bearer.
//   BearerContext_t defaultBearer = msg.bearerContextsCreated.front ();
//   NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
//   uint32_t teid = defaultBearer.sgwFteid.teid;
//   SgwRulesInstall (RoutingInfo::GetPointer (teid));
//
//   // Forward the response message to the MME.
//   m_s11SapMme->CreateSessionResponse (msg);
}

void
SliceController::DoDeleteBearerCommand (
  EpcS11SapSgw::DeleteBearerCommandMessage msg)
{
  NS_LOG_FUNCTION (this << msg.teid);

//   uint64_t imsi = msg.teid;
//
//   EpcS11SapMme::DeleteBearerRequestMessage res;
//   res.teid = imsi;
//
//   std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit;
//   for (bit = msg.bearerContextsToBeRemoved.begin ();
//        bit != msg.bearerContextsToBeRemoved.end ();
//        ++bit)
//     {
//       EpcS11SapMme::BearerContextRemoved bearerContext;
//       bearerContext.epsBearerId = bit->epsBearerId;
//       res.bearerContextsRemoved.push_back (bearerContext);
//     }
//
//   m_s11SapMme->DeleteBearerRequest (res);
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
  // just support the minimum needed for path switch request (handover). There
  // is no need to forward the request message to the P-GW.
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = msg.teid;
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;

  m_s11SapMme->ModifyBearerResponse (res);
}

uint64_t
SliceController::GetPgwMainDpId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_pgwDpIds.at (0);
}

uint64_t
SliceController::GetPgwTftDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  return m_pgwDpIds.at (idx);
}

// uint16_t
// SliceController::GetPgwTftIdx (
//   Ptr<const RoutingInfo> rInfo, uint16_t activeTfts) const
// {
//   NS_LOG_FUNCTION (this << rInfo << activeTfts);
//
//   if (activeTfts == 0)
//     {
//       activeTfts = 1 << m_tftLevel;
//     }
//   Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
//   return 1 + (ueInfo->GetUeAddr ().Get () % activeTfts);
// }



void
SliceController::PgwTftCheckUsage (void)
{
  NS_LOG_FUNCTION (this);
  // TODO

// double maxEntries = 0.0, sumEntries = 0.0;
//  double maxLoad = 0.0, sumLoad = 0.0;
//  uint32_t maxLbLevel = static_cast<uint32_t> (log2 (m_tftSwitches));
//  uint16_t activeTfts = 1 << m_tftLevel;
//  uint8_t nextLevel = m_tftLevel;
//
//  Ptr<OFSwitch13Device> device;
//  Ptr<OFSwitch13StatsCalculator> stats;
//  for (uint16_t tftIdx = 1; tftIdx <= activeTfts; tftIdx++)
//    {
//      device = OFSwitch13Device::GetDevice (GetPgwTftDpId (tftIdx));
//      stats = device->GetObject<OFSwitch13StatsCalculator> ();
//      NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");
//
//      double entries = stats->GetEwmaFlowEntries ();
//      maxEntries = std::max (maxEntries, entries);
//      sumEntries += entries;
//
//      double load = stats->GetEwmaPipelineLoad ().GetBitRate ();
//      maxLoad = std::max (maxLoad, load);
//      sumLoad += load;
//    }
//
//  if (GetPgwAdaptiveMode () == OperationMode::AUTO)
//    {
//      double maxTableUsage = maxEntries / m_tftTableSize;
//      double maxLoadUsage = maxLoad / m_tftMaxLoad.GetBitRate ();
//
//      // We may increase the level when we hit the split threshold.
//      if ((m_tftLevel < maxLbLevel)
//          && (maxTableUsage >= m_tftSplitThs
//              || maxLoadUsage >= m_tftSplitThs))
//        {
//          NS_LOG_INFO ("Increasing the adaptive mechanism level.");
//          nextLevel++;
//        }
//
//      // We may decrease the level when we hit the joing threshold.
//      else if ((m_tftLevel > 0)
//               && (maxTableUsage < m_tftJoinThs)
//               && (maxLoadUsage < m_tftJoinThs))
//        {
//          NS_LOG_INFO ("Decreasing the adaptive mechanism level.");
//          nextLevel--;
//        }
//    }
//
//  // Check if we need to update the adaptive mechanism level.
//  uint32_t moved = 0;
//  if (m_tftLevel != nextLevel)
//    {
//      // Identify and move bearers to the correct P-GW TFT switches.
//      uint16_t futureTfts = 1 << nextLevel;
//      for (uint16_t currIdx = 1; currIdx <= activeTfts; currIdx++)
//        {
//          RoutingInfoList_t bearers = RoutingInfo::GetInstalledList (currIdx);
//          RoutingInfoList_t::iterator it;
//          for (it = bearers.begin (); it != bearers.end (); ++it)
//            {
//              uint16_t destIdx = GetPgwTftIdx (*it, futureTfts);
//              if (destIdx != currIdx)
//                {
//                  NS_LOG_INFO ("Moving bearer teid " << (*it)->GetTeid ());
//                  PgwRulesRemove  (*it, currIdx, true);
//                  PgwRulesInstall (*it, destIdx, true);
//                  (*it)->SetPgwTftIdx (destIdx);
//                  moved++;
//                }
//            }
//        }
//
//      // Update the adaptive mechanism level and the P-GW main switch.
//      std::ostringstream cmd;
//      cmd << "flow-mod cmd=mods,table=0,prio=64 eth_type=0x800"
//          << ",in_port=" << m_pgwSgiPortNo
//          << ",ip_dst=" << EpcNetwork::m_ueAddr
//          << "/" << EpcNetwork::m_ueMask.GetPrefixLength ()
//          << " goto:" << nextLevel + 1;
//      DpctlExecute (GetPgwMainDpId (), cmd.str ());
//    }
//
//  // Fire the P-GW TFT adaptation trace source.
//  struct PgwTftStats tftStats;
//  tftStats.tableSize = m_tftTableSize;
//  tftStats.maxEntries = maxEntries;
//  tftStats.sumEntries = sumEntries;
//  tftStats.pipeCapacity = m_tftMaxLoad.GetBitRate ();
//  tftStats.maxLoad = maxLoad;
//  tftStats.sumLoad = sumLoad;
//  tftStats.currentLevel = m_tftLevel;
//  tftStats.nextLevel = nextLevel;
//  tftStats.maxLevel = maxLbLevel;
//  tftStats.bearersMoved = moved;
//  tftStats.blockThrs = m_tftBlockThs;
//  tftStats.joinThrs = m_tftJoinThs;
//  tftStats.splitThrs = m_tftSplitThs;
//  m_pgwTftStatsTrace (tftStats);
//
//  m_tftLevel = nextLevel;
}

// FIXME os dois métodos vieram do SdranController
// bool
// SliceController::SgwRulesInstall (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//   NS_LOG_INFO ("Installing S-GW rules for bearer teid " << rInfo->GetTeid ());
//   Ptr<const UeInfo> ueInfo = UeInfo::GetPointer (rInfo->GetImsi ());
//   Ptr<const EnbInfo> enbInfo = EnbInfo::GetPointer (ueInfo->GetCellId ());
//
//   // Flags OFPFF_SEND_FLOW_REM, OFPFF_CHECK_OVERLAP, and OFPFF_RESET_COUNTS.
//   std::string flagsStr ("0x0007");
//
//   // Print the cookie and buffer values in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Configure downlink.
//   if (rInfo->HasDownlinkTraffic ())
//     {
//       // Print downlink TEID and destination IPv4 address into tunnel metadata.
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (enbInfo->GetEnbS1uAddr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Build the dpctl command string.
//       std::ostringstream cmd, act;
//       cmd << "flow-mod cmd=add,table=1"
//           << ",flags=" << flagsStr
//           << ",cookie=" << cookieStr
//           << ",prio=" << rInfo->GetPriority ()
//           << ",idle=" << rInfo->GetTimeout ();
//
//       // Instruction: apply action: set tunnel ID, output port.
//       act << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << enbInfo->GetSgwS1uPortNo ();
//
//       // Install one downlink dedicated bearer rule for each packet filter.
//       Ptr<EpcTft> tft = rInfo->GetTft ();
//       for (uint8_t i = 0; i < tft->GetNFilters (); i++)
//         {
//           EpcTft::PacketFilter filter = tft->GetFilter (i);
//           if (filter.direction == EpcTft::UPLINK)
//             {
//               continue;
//             }
//
//           // Install rules for TCP traffic.
//           if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=6"
//                     << ",ip_dst=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_src=" << filter.remoteAddress
//                         << ",tcp_src=" << filter.remotePortStart;
//                 }
//
//               std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdTcpStr);
//             }
//
//           // Install rules for UDP traffic.
//           else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=17"
//                     << ",ip_dst=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_src=" << filter.remoteAddress
//                         << ",udp_src=" << filter.remotePortStart;
//                 }
//
//               std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdUdpStr);
//             }
//         }
//     }
//
//   // Configure uplink.
//   if (rInfo->HasUplinkTraffic () && !rInfo->IsAggregated ())
//     {
//       // Print uplink TEID and destination IPv4 address into tunnel metadata.
//       uint64_t tunnelId;
//       char tunnelIdStr [20];
//       tunnelId = static_cast<uint64_t> (rInfo->GetPgwS5Addr ().Get ());
//       tunnelId <<= 32;
//       tunnelId |= rInfo->GetTeid ();
//       sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//       // Build the dpctl command string.
//       std::ostringstream cmd, act;
//       cmd << "flow-mod cmd=add,table=2"
//           << ",flags=" << flagsStr
//           << ",cookie=" << cookieStr
//           << ",prio=" << rInfo->GetPriority ()
//           << ",idle=" << rInfo->GetTimeout ();
//
//       // Check for meter entry.
//       Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//       if (meterInfo && meterInfo->HasUp ())
//         {
//           if (!meterInfo->IsUpInstalled ())
//             {
//               // Install the per-flow meter entry.
//               DpctlExecute (m_sgwDpId, meterInfo->GetUpAddCmd ());
//               meterInfo->SetUpInstalled (true);
//             }
//
//           // Instruction: meter.
//           act << " meter:" << rInfo->GetTeid ();
//         }
//
//       // Instruction: apply action: set tunnel ID, output port.
//       act << " apply:set_field=tunn_id:" << tunnelIdStr
//           << ",output=" << m_sgwS5PortNo;
//
//       // Install one uplink dedicated bearer rule for each packet filter.
//       Ptr<EpcTft> tft = rInfo->GetTft ();
//       for (uint8_t i = 0; i < tft->GetNFilters (); i++)
//         {
//           EpcTft::PacketFilter filter = tft->GetFilter (i);
//           if (filter.direction == EpcTft::DOWNLINK)
//             {
//               continue;
//             }
//
//           // Install rules for TCP traffic.
//           if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=6"
//                     << ",ip_src=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_dst=" << filter.remoteAddress
//                         << ",tcp_dst=" << filter.remotePortStart;
//                 }
//
//               std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdTcpStr);
//             }
//
//           // Install rules for UDP traffic.
//           else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
//             {
//               std::ostringstream match;
//               match << " eth_type=0x800"
//                     << ",ip_proto=17"
//                     << ",ip_src=" << filter.localAddress;
//
//               if (tft->IsDefaultTft () == false)
//                 {
//                   match << ",ip_dst=" << filter.remoteAddress
//                         << ",udp_dst=" << filter.remotePortStart;
//                 }
//
//               std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
//               DpctlExecute (m_sgwDpId, cmdUdpStr);
//             }
//         }
//     }
//   return true;
// }
//
// bool
// SliceController::SgwRulesRemove (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//   NS_LOG_INFO ("Removing S-GW rules for bearer teid " << rInfo->GetTeid ());
//
//   // Print the cookie value in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Remove flow entries for this TEID.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=del,"
//       << ",cookie=" << cookieStr
//       << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
//   DpctlExecute (m_sgwDpId, cmd.str ());
//
//   // Remove meter entry for this TEID.
//   Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//   if (meterInfo && meterInfo->IsUpInstalled ())
//     {
//       DpctlExecute (m_sgwDpId, meterInfo->GetDelCmd ());
//       meterInfo->SetUpInstalled (false);
//     }
//   return true;
// }



// bool
// SliceController::MtcAggBearerInstall (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo << rInfo->GetTeid ());
//
//   bool success = TopologyRoutingInstall (rInfo);
//   NS_ASSERT_MSG (success, "Error when installing the MTC aggregation bearer.");
//   NS_LOG_INFO ("MTC aggregation bearer teid " << rInfo->GetTeid () <<
//                " installed for S-GW " << rInfo->GetSgwS5Addr ());
//
//   rInfo->SetInstalled (success);
//   return success;
// }
//
// bool
// SliceController::PgwBearerRequest (Ptr<RoutingInfo> rInfo)
// {
//   NS_LOG_FUNCTION (this << rInfo->GetTeid ());
//
//   // If the bearer is already blocked, there's nothing more to do.
//   if (rInfo->IsBlocked ())
//     {
//       return false;
//     }
//
//   // Check for valid P-GW TFT thresholds attributes.
//   NS_ASSERT_MSG (m_tftSplitThs < m_tftBlockThs
//                  && m_tftSplitThs > 2 * m_tftJoinThs,
//                  "The split threshold should be smaller than the block "
//                  "threshold and two times larger than the join threshold.");
//
//   // Get the P-GW TFT stats calculator for this bearer.
//   Ptr<OFSwitch13Device> device;
//   Ptr<OFSwitch13StatsCalculator> stats;
//   uint16_t tftIdx = rInfo->GetPgwTftIdx ();
//   device = OFSwitch13Device::GetDevice (GetPgwTftDpId (tftIdx));
//   stats = device->GetObject<OFSwitch13StatsCalculator> ();
//   NS_ASSERT_MSG (stats, "Enable OFSwitch13 datapath stats.");
//
//   // First check: OpenFlow switch table usage.
//   // Only non-aggregated bearers will install rules on P-GW TFT flow table.
//   // Blocks the bearer if the table usage is exceeding the block threshold.
//   if (!rInfo->IsAggregated ())
//     {
//       uint32_t entries = stats->GetEwmaFlowEntries ();
//       double tableUsage = static_cast<double> (entries) / m_tftTableSize;
//       if (tableUsage >= m_tftBlockThs)
//         {
//           rInfo->SetBlocked (true, RoutingInfo::TFTTABLEFULL);
//           NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
//                        " because the TFT flow tables is full.");
//         }
//     }
//
//   // Second check: OpenFlow switch pipeline load.
//   // Is the current pipeline load is exceeding the block threshold, blocks the
//   // bearer accordingly to the PgwTftBlockPolicy attribute:
//   // - If OFF (none): don't block the request.
//   // - If ON (all)  : block the request.
//   // - If AUTO (gbr): block only if GBR request.
//   uint64_t rate = stats->GetEwmaPipelineLoad ().GetBitRate ();
//   double loadUsage = static_cast<double> (rate) / m_tftMaxLoad.GetBitRate ();
//   if (loadUsage >= m_tftBlockThs
//       && (m_tftBlockPolicy == OperationMode::ON
//           || (m_tftBlockPolicy == OperationMode::AUTO && rInfo->IsGbr ())))
//     {
//       rInfo->SetBlocked (true, RoutingInfo::TFTMAXLOAD);
//       NS_LOG_WARN ("Blocking bearer teid " << rInfo->GetTeid () <<
//                    " because the TFT processing capacity is overloaded.");
//     }
//
//   // Return false if blocked.
//   return !rInfo->IsBlocked ();
// }
//
// bool
// SliceController::PgwRulesInstall (
//   Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool forceMeterInstall)
// {
//   NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << forceMeterInstall);
//
//   // Use the rInfo P-GW TFT index when the parameter is not set.
//   if (pgwTftIdx == 0)
//     {
//       pgwTftIdx = rInfo->GetPgwTftIdx ();
//     }
//   uint64_t pgwTftDpId = GetPgwTftDpId (pgwTftIdx);
//   uint32_t pgwTftS5PortNo = m_pgwS5PortsNo.at (pgwTftIdx);
//   NS_LOG_INFO ("Installing P-GW rules for bearer teid " << rInfo->GetTeid () <<
//                " into P-GW TFT switch index " << pgwTftIdx);
//
//   // Flags OFPFF_CHECK_OVERLAP and OFPFF_RESET_COUNTS.
//   std::string flagsStr ("0x0006");
//
//   // Print the cookie and buffer values in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Print downlink TEID and destination IPv4 address into tunnel metadata.
//   uint64_t tunnelId;
//   char tunnelIdStr [20];
//   tunnelId = static_cast<uint64_t> (rInfo->GetSgwS5Addr ().Get ());
//   tunnelId <<= 32;
//   tunnelId |= rInfo->GetTeid ();
//   sprintf (tunnelIdStr, "0x%016lx", tunnelId);
//
//   // Build the dpctl command string
//   std::ostringstream cmd, act;
//   cmd << "flow-mod cmd=add,table=0"
//       << ",flags=" << flagsStr
//       << ",cookie=" << cookieStr
//       << ",prio=" << rInfo->GetPriority ()
//       << ",idle=" << rInfo->GetTimeout ();
//
//   // Check for meter entry.
//   Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//   if (meterInfo && meterInfo->HasDown ())
//     {
//       if (forceMeterInstall || !meterInfo->IsDownInstalled ())
//         {
//           // Install the per-flow meter entry.
//           DpctlExecute (pgwTftDpId, meterInfo->GetDownAddCmd ());
//           meterInfo->SetDownInstalled (true);
//         }
//
//       // Instruction: meter.
//       act << " meter:" << rInfo->GetTeid ();
//     }
//
//   // Instruction: apply action: set tunnel ID, output port.
//   act << " apply:set_field=tunn_id:" << tunnelIdStr
//       << ",output=" << pgwTftS5PortNo;
//
//   // Install one downlink dedicated bearer rule for each packet filter.
//   Ptr<EpcTft> tft = rInfo->GetTft ();
//   for (uint8_t i = 0; i < tft->GetNFilters (); i++)
//     {
//       EpcTft::PacketFilter filter = tft->GetFilter (i);
//       if (filter.direction == EpcTft::UPLINK)
//         {
//           continue;
//         }
//
//       // Install rules for TCP traffic.
//       if (filter.protocol == TcpL4Protocol::PROT_NUMBER)
//         {
//           std::ostringstream match;
//           match << " eth_type=0x800"
//                 << ",ip_proto=6"
//                 << ",ip_dst=" << filter.localAddress;
//
//           if (tft->IsDefaultTft () == false)
//             {
//               match << ",ip_src=" << filter.remoteAddress
//                     << ",tcp_src=" << filter.remotePortStart;
//             }
//
//           std::string cmdTcpStr = cmd.str () + match.str () + act.str ();
//           DpctlExecute (pgwTftDpId, cmdTcpStr);
//         }
//
//       // Install rules for UDP traffic.
//       else if (filter.protocol == UdpL4Protocol::PROT_NUMBER)
//         {
//           std::ostringstream match;
//           match << " eth_type=0x800"
//                 << ",ip_proto=17"
//                 << ",ip_dst=" << filter.localAddress;
//
//           if (tft->IsDefaultTft () == false)
//             {
//               match << ",ip_src=" << filter.remoteAddress
//                     << ",udp_src=" << filter.remotePortStart;
//             }
//
//           std::string cmdUdpStr = cmd.str () + match.str () + act.str ();
//           DpctlExecute (pgwTftDpId, cmdUdpStr);
//         }
//     }
//   return true;
// }
//
// bool
// SliceController::PgwRulesRemove (
//   Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx, bool keepMeterFlag)
// {
//   NS_LOG_FUNCTION (this << rInfo->GetTeid () << pgwTftIdx << keepMeterFlag);
//
//   // Use the rInfo P-GW TFT index when the parameter is not set.
//   if (pgwTftIdx == 0)
//     {
//       pgwTftIdx = rInfo->GetPgwTftIdx ();
//     }
//   uint64_t pgwTftDpId = GetPgwTftDpId (pgwTftIdx);
//   NS_LOG_INFO ("Removing P-GW rules for bearer teid " << rInfo->GetTeid () <<
//                " from P-GW TFT switch index " << pgwTftIdx);
//
//   // Print the cookie value in dpctl string format.
//   char cookieStr [20];
//   sprintf (cookieStr, "0x%x", rInfo->GetTeid ());
//
//   // Remove P-GW TFT flow entries for this TEID.
//   std::ostringstream cmd;
//   cmd << "flow-mod cmd=del,table=0"
//       << ",cookie=" << cookieStr
//       << ",cookie_mask=0xffffffffffffffff"; // Strict cookie match.
//   DpctlExecute (pgwTftDpId, cmd.str ());
//
//   // Remove meter entry for this TEID.
//   Ptr<MeterInfo> meterInfo = rInfo->GetObject<MeterInfo> ();
//   if (meterInfo && meterInfo->IsDownInstalled ())
//     {
//       DpctlExecute (pgwTftDpId, meterInfo->GetDelCmd ());
//       if (!keepMeterFlag)
//         {
//           meterInfo->SetDownInstalled (false);
//         }
//     }
//   return true;
// }



} // namespace ns3
