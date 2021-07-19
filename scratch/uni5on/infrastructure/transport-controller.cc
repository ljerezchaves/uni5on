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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "transport-controller.h"
#include "transport-network.h"
#include "../metadata/bearer-info.h"
#include "../metadata/link-info.h"
#include "../mano-apps/link-sharing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TransportController");
NS_OBJECT_ENSURE_REGISTERED (TransportController);

// Initializing TransportController static members.
const TransportController::QciList_t TransportController::m_nonQciList = {
  EpsBearer::NGBR_IMS,
  EpsBearer::NGBR_VIDEO_TCP_OPERATOR,
  EpsBearer::NGBR_VOICE_VIDEO_GAMING,
  EpsBearer::NGBR_VIDEO_TCP_PREMIUM,
  EpsBearer::NGBR_VIDEO_TCP_DEFAULT
};

TransportController::TransportController ()
  : m_sharingApp (0)
{
  NS_LOG_FUNCTION (this);
}

TransportController::~TransportController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TransportController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TransportController")
    .SetParent<OFSwitch13Controller> ()

    .AddAttribute ("AggBitRateCheck",
                   "Check for the available bit rate for aggregated bearers.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::OFF),
                   MakeEnumAccessor (&TransportController::m_aggCheck),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("MeterStep",
                   "Meter bit rate adjustment step.",
                   DataRateValue (DataRate ("2Mbps")),
                   MakeDataRateAccessor (&TransportController::m_meterStep),
                   MakeDataRateChecker ())
    .AddAttribute ("QosQueues",
                   "QoS output queues operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&TransportController::m_qosQueues),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SwBlockPolicy",
                   "Switch overloaded block policy.",
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&TransportController::m_swBlockPolicy),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("SwBlockThs",
                   "Switch overloaded block threshold.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&TransportController::m_swBlockThs),
                   MakeDoubleChecker<double> (0.8, 1.0))
  ;
  return tid;
}

uint64_t
TransportController::GetDpId (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid switch index.");
  return m_switchDevices.Get (idx)->GetDatapathId ();
}

uint16_t
TransportController::GetNSwitches (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchDevices.GetN ();
}

OpMode
TransportController::GetAggBitRateCheck (void) const
{
  NS_LOG_FUNCTION (this);

  return m_aggCheck;
}

OpMode
TransportController::GetSwBlockPolicy (void) const
{
  NS_LOG_FUNCTION (this);

  return m_swBlockPolicy;
}

double
TransportController::GetSwBlockThreshold (void) const
{
  NS_LOG_FUNCTION (this);

  return m_swBlockThs;
}

OpMode
TransportController::GetQosQueuesMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_qosQueues;
}

void
TransportController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_sharingApp = 0;
  m_sliceCtrlById.clear ();
  OFSwitch13Controller::DoDispose ();
}

void
TransportController::DpctlSchedule (Time delay, uint64_t dpId,
                                    const std::string textCmd)
{
  NS_LOG_FUNCTION (this << delay << dpId << textCmd);

  Simulator::Schedule (delay, &TransportController::DpctlExecute,
                       this, dpId, textCmd);
}

double
TransportController::GetFlowTableUse (uint16_t idx, uint8_t tableId) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid index.");
  return m_switchDevices.Get (idx)->GetFlowTableUsage (tableId);
}

std::tuple<Ptr<LinkInfo>, LinkInfo::LinkDir, LinkInfo::LinkDir>
TransportController::GetLinkInfo (uint16_t idx1, uint16_t idx2) const
{
  NS_LOG_FUNCTION (this << idx1 << idx2);

  uint64_t dpId1 = GetDpId (idx1);
  uint64_t dpId2 = GetDpId (idx2);
  Ptr<LinkInfo> lInfo = LinkInfo::GetPointer (dpId1, dpId2);
  LinkInfo::LinkDir dir = lInfo->GetLinkDir (dpId1, dpId2);
  return std::make_tuple (lInfo, dir, LinkInfo::InvertDir (dir));
}

double
TransportController::GetEwmaCpuUse (uint16_t idx) const
{
  NS_LOG_FUNCTION (this << idx);

  NS_ASSERT_MSG (idx < m_switchDevices.GetN (), "Invalid index.");
  Ptr<OFSwitch13Device> device;
  Ptr<OFSwitch13StatsCalculator> stats;
  device = m_switchDevices.Get (idx);
  stats = device->GetObject<OFSwitch13StatsCalculator> ();
  NS_ABORT_MSG_IF (!stats, "Enable OFSwitch13 datapath stats.");

  return static_cast<double> (stats->GetEwmaCpuLoad ().GetBitRate ()) /
         static_cast<double> (device->GetCpuCapacity ().GetBitRate ());
}

const TransportController::QciList_t&
TransportController::GetNonGbrQcis (void)
{
  return TransportController::m_nonQciList;
}

Ptr<SliceController>
TransportController::GetSliceController (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  auto it = m_sliceCtrlById.find (slice);
  NS_ASSERT_MSG (it != m_sliceCtrlById.end (), "Slice controller not found.");
  return it->second;
}

uint16_t
TransportController::GetSliceTable (SliceId slice) const
{
  NS_LOG_FUNCTION (this << slice);

  return static_cast<int> (slice) + SLICE_TAB_START;
}

Ptr<LinkSharing>
TransportController::GetSharingApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sharingApp;
}

void
TransportController::NotifyBearerCreated (Ptr<BearerInfo> bInfo)
{
  NS_LOG_FUNCTION (this << bInfo->GetTeidHex ());
}

void
TransportController::NotifyEpcAttach (
  Ptr<OFSwitch13Device> swDev, uint32_t portNo, Ptr<NetDevice> epcDev)
{
  NS_LOG_FUNCTION (this << swDev << portNo << epcDev);

  // -------------------------------------------------------------------------
  // Input table -- [from higher to lower priority]
  //
  // IP packets addressed to EPC elements connected to this EPC port.
  // Write the output port into action set.
  // Send the packet directly to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=256"
        << ",table="        << INPUT_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_dst="       << Ipv4AddressHelper::GetAddress (epcDev)
        << " write:output=" << portNo
        << " goto:"         << OUTPT_TAB;
    DpctlExecute (swDev->GetDatapathId (), cmd.str ());
  }
  //
  // X2-C packets entering the transport network from this EPC port.
  // Set the DSCP field for Expedited Forwarding.
  // Send the packet to the classification table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=32"
        << ",table="        << INPUT_TAB
        << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
        << " eth_type="     << IPV4_PROT_NUM
        << ",ip_proto="     << UDP_PROT_NUM
        << ",ip_dst="       << TransportNetwork::m_x2Addr
        << "/"              << TransportNetwork::m_x2Mask.GetPrefixLength ()
        << ",in_port="      << portNo
        << " apply:set_field=ip_dscp:" << Ipv4Header::DSCP_EF
        << " goto:"         << CLASS_TAB;
    DpctlExecute (swDev->GetDatapathId (), cmd.str ());
  }
}

void
TransportController::NotifySlicesBuilt (ApplicationContainer &controllers)
{
  NS_LOG_FUNCTION (this);

  ApplicationContainer::Iterator it;
  for (it = controllers.Begin (); it != controllers.End (); ++it)
    {
      Ptr<SliceController> controller = DynamicCast<SliceController> (*it);
      SliceId slice = controller->GetSliceId ();
      int quota = controller->GetQuota ();

      // Saving controller application pointers.
      std::pair<SliceId, Ptr<SliceController>> entry (slice, controller);
      auto ret = m_sliceCtrlById.insert (entry);
      NS_ABORT_MSG_IF (ret.second == false, "Existing slice controller.");

      // Iterate over links configuring the initial quotas.
      for (auto const &lInfo : LinkInfo::GetList ())
        {
          bool success = true;
          success &= lInfo->UpdateQuota (LinkInfo::FWD, slice, quota);
          success &= lInfo->UpdateQuota (LinkInfo::BWD, slice, quota);
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");
        }
    }

  // Notify the link sharing application
  m_sharingApp->NotifySlicesBuilt (controllers);
}

void
TransportController::NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices)
{
  NS_LOG_FUNCTION (this);

  // Create the link sharing application and aggregate it to controller node.
  m_sharingApp = CreateObject<LinkSharing> (Ptr<TransportController> (this));
  GetNode ()->AggregateObject (m_sharingApp);

  // Save the collection of transport switch devices.
  m_switchDevices = devices;
}

ofl_err
TransportController::HandleError (
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
            << " Transport controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with error message: " << msgStr
            << std::endl;
  return 0;
}

ofl_err
TransportController::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid << msg->stats->cookie);

  uint32_t teid = GlobalIds::CookieGetTeid (msg->stats->cookie);
  uint16_t prio = msg->stats->priority;

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free_flow_removed (msg, true, 0);

  NS_LOG_DEBUG ("Flow removed: " << msgStr);

  // Check for existing information for this bearer.
  Ptr<BearerInfo> bInfo = BearerInfo::GetPointer (teid);
  NS_ASSERT_MSG (bInfo, "Bearer metadata not found");

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer is inactive.
  if (!bInfo->IsActive ())
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for inactive bearer teid " << bInfo->GetTeidHex ());
      return 0;
    }

  // 2) The application is running and the bearer is active, but the bearer
  // priority was increased and this removed flow rule is an old one.
  if (bInfo->GetPriority () > prio)
    {
      NS_LOG_INFO ("Rule removed from switch dp " << swtch->GetDpId () <<
                   " for bearer teid " << bInfo->GetTeidHex () <<
                   " with old priority " << prio);
      return 0;
    }

  // 3) The application is running, the bearer is active, and the bearer
  // priority is the same of the removed rule. This is a critical situation!
  // For some reason, the flow rule was removed so we are going to abort the
  // program to avoid wrong results.
  NS_ASSERT_MSG (bInfo->GetPriority () == prio, "Invalid flow priority.");
  NS_ABORT_MSG ("Rule removed for active bearer. " <<
                "OpenFlow flow removed message: " << msgStr);
  return 0;
}

ofl_err
TransportController::HandlePacketIn (
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
            << " Transport controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with packet-in message: " << msgStr
            << std::endl;
  return 0;
}

void
TransportController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the OpenFlow switch datapath ID.
  uint64_t swDpId = swtch->GetDpId ();

  // TODO Differentiate between transport and eNB switch.

  // For each switch on the transport network, install the following rules:
  // -------------------------------------------------------------------------
  // Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by NotifyEpcAttach function.
  //
  // Table miss entry.
  // Send the packet to the classification table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << INPUT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET
        << " goto:"  << CLASS_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Classification table -- [from higher to lower priority]
  //
  // Classify GTP-U packets on the corresponding logical slice using
  // the system-wide GTP-U TEID masked value.
  // Send the packet to the corresponding slice table.
  for (int s = 0; s < N_SLICE_IDS; s++)
    {
      SliceId slice = static_cast<SliceId> (s);
      uint32_t teidSliceMask = GlobalIds::TeidSliceMask (slice);

      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=64"
          << ",table="      << CLASS_TAB
          << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET
          << " eth_type="   << IPV4_PROT_NUM
          << ",ip_proto="   << UDP_PROT_NUM
          << ",udp_src="    << GTPU_PORT
          << ",udp_dst="    << GTPU_PORT
          << ",gtpu_teid="  << teidSliceMask
          << "/"            << TEID_SLICE_MASK
          << " goto:"       << GetSliceTable (slice);
      DpctlExecute (swDpId, cmd.str ());
    }
  //
  // Entries will be installed here by the topology HandshakeSuccessful.

  // -------------------------------------------------------------------------
  // Slice tables (one for each slice) -- [from higher to lower priority]
  //
  // Entries will be installed here by BearerInstall function.

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Entries will be installed here by the link sharing application
  m_sharingApp->NotifyHandshakeSuccessful (swtch->GetDpId ());

  // Table miss entry.
  // Send the packet to the output table.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table="  << BANDW_TAB
        << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET
        << " goto:"   << OUTPT_TAB;
    DpctlExecute (swDpId, cmd.str ());
  }

  // -------------------------------------------------------------------------
  // Output table -- [from higher to lower priority]
  //
  // Classify IP packets on the corresponding output queue using
  // the IP DSCP value.
  // No goto instruction to trigger action set execution.
  if (GetQosQueuesMode () == OpMode::ON)
    {
      // QoS output queues rules.
      for (auto const &it : Dscp2QueueMap ())
        {
          std::ostringstream cmd;
          cmd << "flow-mod cmd=add,prio=32"
              << ",table="        << OUTPT_TAB
              << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
              << " eth_type="     << IPV4_PROT_NUM
              << ",ip_dscp="      << static_cast<uint16_t> (it.first)
              << " write:queue="  << static_cast<uint32_t> (it.second);
          DpctlExecute (swDpId, cmd.str ());
        }
    }
  //
  // Table miss entry.
  // No goto instruction to trigger action set execution.
  {
    std::ostringstream cmd;
    cmd << "flow-mod cmd=add,prio=0"
        << ",table=" << OUTPT_TAB
        << ",flags=" << FLAGS_REMOVED_OVERLAP_RESET;
    DpctlExecute (swDpId, cmd.str ());
  }
}

void
TransportController::SharingMeterApply (
  uint64_t swDpId, LinkInfo::LinkDir dir, SliceId slice)
{
  NS_LOG_FUNCTION (this << swDpId << dir << slice);
}

void
TransportController::SharingMeterInstall (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice << bitRate);

  // ---------------------------------------------------------------------
  // Meter table
  //
  uint32_t meterId = GlobalIds::MeterIdSlcCreate (slice, dir);
  int64_t meterKbps = Bps2Kbps (bitRate);
  bool success = lInfo->SetMetBitRate (dir, slice, meterKbps * 1000);
  NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

  NS_LOG_INFO ("Create slice " << SliceIdStr (slice) <<
               " direction "   << LinkInfo::LinkDirStr (dir) <<
               " meter ID "    << GetUint32Hex (meterId) <<
               " bitrate "     << meterKbps << " Kbps");

  std::ostringstream cmd;
  cmd << "meter-mod cmd=add"
      << ",flags="      << OFPMF_KBPS
      << ",meter="      << meterId
      << " drop:rate="  << meterKbps;
  DpctlExecute (lInfo->GetSwDpId (dir), cmd.str ());
}

void
TransportController::SharingMeterUpdate (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir, SliceId slice, int64_t bitRate)
{
  NS_LOG_FUNCTION (this << lInfo << dir << slice << bitRate);

  // ---------------------------------------------------------------------
  // Meter table
  //
  int64_t currBitRate = lInfo->GetMetBitRate (dir, slice);
  uint64_t diffBitRate = std::abs (currBitRate - bitRate);
  NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
                " direction "    << LinkInfo::LinkDirStr (dir) <<
                " diff rate "    << diffBitRate);

  if (diffBitRate >= m_meterStep.GetBitRate ())
    {
      uint32_t meterId = GlobalIds::MeterIdSlcCreate (slice, dir);
      int64_t meterKbps = Bps2Kbps (bitRate);
      bool success = lInfo->SetMetBitRate (dir, slice, meterKbps * 1000);
      NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

      NS_LOG_INFO ("Update slice " << SliceIdStr (slice) <<
                   " direction "   << LinkInfo::LinkDirStr (dir) <<
                   " meter ID "    << GetUint32Hex (meterId) <<
                   " bitrate "     << meterKbps << " Kbps");

      std::ostringstream cmd;
      cmd << "meter-mod cmd=mod"
          << ",flags="      << OFPMF_KBPS
          << ",meter="      << meterId
          << " drop:rate="  << meterKbps;
      DpctlExecute (lInfo->GetSwDpId (dir), cmd.str ());
    }
}

} // namespace ns3