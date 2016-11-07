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
#include "meter-info.h"
#include "gbr-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcController");
NS_OBJECT_ENSURE_REGISTERED (EpcController);

const uint16_t EpcController::m_dedicatedTmo = 15;

EpcController::TeidBearerMap_t EpcController::m_bearersTable;
EpcController::QciDscpMap_t EpcController::m_qciDscpTable;
EpcController::QciDscpInitializer EpcController::initializer;

EpcController::EpcController ()
  : m_controllerStats (0),
    m_teidCount (0x0000000F),
    m_s11SapMme (0)
{
  NS_LOG_FUNCTION (this);

  // Creating the admission stats calculator for this OpenFlow controller
  m_controllerStats = CreateObject<ControllerStatsCalculator> ();

  // The S-GW side of S11 AP
  m_s11SapSgw = new MemberEpcS11SapSgw<EpcController> (this);
}

EpcController::~EpcController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
EpcController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcController")
    .SetParent (OFSwitch13Controller::GetTypeId ())
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

    .AddTraceSource ("BearerRequest",
                     "The bearer request trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerRequestTrace),
                     "ns3::EpcController::BearerTracedCallback")
    .AddTraceSource ("BearerRelease",
                     "The bearer release trace source.",
                     MakeTraceSourceAccessor (
                       &EpcController::m_bearerReleaseTrace),
                     "ns3::EpcController::BearerTracedCallback")
  ;
  return tid;
}

bool
EpcController::RequestDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
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
      NS_LOG_DEBUG ("Routing path for " << teid << " is already installed.");
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
      return false;
    }

  // Everything is ok! Let's activate and install this bearer.
  rInfo->SetActive (true);
  return TopologyInstallRouting (rInfo);
}

bool
EpcController::ReleaseDedicatedBearer (
  EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid)
{
  NS_LOG_FUNCTION (this << imsi << cellId << teid);

  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
  NS_ASSERT_MSG (rInfo, "No routing information for teid.");

  // Is it a default bearer?
  if (rInfo->IsDefault ())
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource release, as default rules were supposed to remain
      // installed during entire simulation and must be Non-GBR.
      NS_ASSERT_MSG (rInfo->IsActive () && rInfo->IsInstalled (),
                     "Default bearer should be intalled and activated.");
      return true;
    }

  // Check for active bearer
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

Ptr<const RoutingInfo>
EpcController::GetConstRoutingInfo (uint32_t teid) const
{
  Ptr<const RoutingInfo> rInfo = 0;
  TeidRoutingMap_t::const_iterator ret;
  ret = m_routes.find (teid);
  if (ret != m_routes.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
}

EpsBearer
EpcController::GetEpsBearer (uint32_t teid)
{
  TeidBearerMap_t::iterator it;
  it = EpcController::m_bearersTable.find (teid);
  if (it != EpcController::m_bearersTable.end ())
    {
      return it->second;
    }
  NS_FATAL_ERROR ("No bearer information for teid " << teid);
}

uint16_t
EpcController::GetDscpMappedValue (EpsBearer::Qci qci)
{
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
EpcController::NewSgiAttach (
  Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwSgiDev,
  Ipv4Address pgwSgiAddr, uint32_t pgwSgiPort, uint32_t pgwS5Port,
  Ipv4Address webAddr)
{
  NS_LOG_FUNCTION (this << pgwSgiAddr << pgwSgiPort << webAddr);

  // Save ARP and index information
  Mac48Address macAddr = Mac48Address::ConvertFrom (pgwSgiDev->GetAddress ());
  SaveArpEntry (pgwSgiAddr, macAddr);

  ConfigurePgwRules (pgwSwDev, pgwSgiPort, pgwS5Port, webAddr);
}

void
EpcController::NewS5Attach (
  Ptr<NetDevice> nodeDev, Ipv4Address nodeIp, Ptr<OFSwitch13Device> swtchDev,
  uint16_t swtchIdx, uint32_t swtchPort)
{
  NS_LOG_FUNCTION (this << nodeIp << swtchIdx << swtchPort);

  // Save ARP and index information
  Mac48Address macAddr = Mac48Address::ConvertFrom (nodeDev->GetAddress ());
  SaveArpEntry (nodeIp, macAddr);
  SaveSwitchIndex (nodeIp, swtchIdx);

  ConfigureLocalPortRules (swtchDev, nodeDev, nodeIp, swtchPort);
}

void
EpcController::NewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  // Connecting this controller to ConnectionInfo trace source
  cInfo->TraceConnectWithoutContext (
    "NonGbrAdjusted", MakeCallback (
      &EpcController::NonGbrAdjusted, this));
}

void
EpcController::TopologyBuilt (OFSwitch13DeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  m_ofDevices = devices;
  TopologyCreateSpanningTree ();
}

void
EpcController::NotifySessionCreated (
  uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address sgwAddr,
  BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr << sgwAddr);

  // Create and save routing information for default bearer
  ContextBearer_t defaultBearer = bearerList.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");

  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
  NS_ASSERT_MSG (rInfo == 0, "Existing routing for default bearer " << teid);

  rInfo = CreateObject<RoutingInfo> ();
  rInfo->m_teid = teid;
  rInfo->m_imsi = imsi;
  rInfo->m_cellId = cellId;
  rInfo->m_sgwIdx = GetSwitchIndex (sgwAddr);
  rInfo->m_enbIdx = GetSwitchIndex (enbAddr);
  rInfo->m_sgwAddr = sgwAddr;
  rInfo->m_enbAddr = enbAddr;
  rInfo->m_priority = 127;                // Priority for default bearer
  rInfo->m_timeout = 0;                   // No timeout for default bearer
  rInfo->m_isInstalled = false;           // Bearer rules not installed yet
  rInfo->m_isActive = true;               // Default bearer is always active
  rInfo->m_isDefault = true;              // This is a default bearer
  rInfo->m_bearer = defaultBearer;
  SaveRoutingInfo (rInfo);
  EpcController::RegisterBearer (teid, rInfo->GetEpsBearer ());

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

      rInfo = CreateObject<RoutingInfo> ();
      rInfo->m_teid = teid;
      rInfo->m_imsi = imsi;
      rInfo->m_cellId = cellId;
      rInfo->m_sgwIdx = GetSwitchIndex (sgwAddr);
      rInfo->m_enbIdx = GetSwitchIndex (enbAddr);
      rInfo->m_sgwAddr = sgwAddr;
      rInfo->m_enbAddr = enbAddr;
      rInfo->m_priority = 127;              // Priority for dedicated bearer
      rInfo->m_timeout = m_dedicatedTmo;    // Timeout for dedicated bearer
      rInfo->m_isInstalled = false;         // Switch rules not installed
      rInfo->m_isActive = false;            // Dedicated bearer not active
      rInfo->m_isDefault = false;           // This is a dedicated bearer
      rInfo->m_bearer = dedicatedBearer;
      SaveRoutingInfo (rInfo);
      EpcController::RegisterBearer (teid, rInfo->GetEpsBearer ());

      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();

      // For all GBR beares, create the GBR metadata.
      if (rInfo->IsGbr ())
        {
          Ptr<GbrInfo> gbrInfo = CreateObject<GbrInfo> (rInfo);
          rInfo->AggregateObject (gbrInfo);

          // Set the appropriated DiffServ DSCP value for this bearer.
          gbrInfo->m_dscp =
            EpcController::GetDscpMappedValue (rInfo->GetQciInfo ());
        }

      // If necessary, create the meter metadata for maximum bit rate.
      if (gbrQoS.mbrDl || gbrQoS.mbrUl)
        {
          Ptr<MeterInfo> meterInfo = CreateObject<MeterInfo> (rInfo);
          rInfo->AggregateObject (meterInfo);
        }
    }
}

void
EpcController::NonGbrAdjusted (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_INFO (this << cInfo);
}

void
EpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_controllerStats = 0;
  m_arpTable.clear ();
  m_ipSwitchTable.clear ();
  m_routes.clear ();
  m_enbInfoByCellId.clear ();
  m_ueInfoByAddrMap.clear ();
  m_ueInfoByImsiMap.clear ();
 
  m_mme = 0;

  delete (m_s11SapSgw);

  Object::DoDispose ();
}

void
EpcController::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // TODO In the future, this will be moved to the RAN controller

  // Create the MME object for this controller
  m_mme = CreateObject<EpcMme> ();

  // Connect the MME to the S-GW via S11 interface
  m_mme->SetS11SapSgw (GetS11SapSgw ());
  SetS11SapMme (m_mme->GetS11SapMme ());

  // Connect the admission stats calculator
  TraceConnectWithoutContext (
    "BearerRequest", MakeCallback (
      &ControllerStatsCalculator::NotifyBearerRequest, m_controllerStats));

  // Chain up
  ObjectBase::NotifyConstructionCompleted ();
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

  if (swtch->GetDpId () == m_pgwDpId)
    {
      // Don't install the following rules on the P-GW switch.
      return;
    }

  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // Entries will be installed here by ConfigureLocalPortRules function.

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
  // Entries will be installed here by ConfigureLocalPortRules function.
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
      int dscpVoip =
        EpcController::GetDscpMappedValue (EpsBearer::GBR_CONV_VOICE);

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
  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // When a flow is removed, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->IsActive ())
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for stopped application.");
      return 0;
    }

  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->GetPriority () > prio)
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for old rule.");
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

uint64_t
EpcController::GetDatapathId (uint16_t index) const
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return m_ofDevices.Get (index)->GetDatapathId ();
}

uint64_t
EpcController::GetPgwDatapathId () const
{
  return m_pgwDpId;
}

void
EpcController::SaveRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);

  uint32_t teid = rInfo->GetTeid ();
  std::pair <uint32_t, Ptr<RoutingInfo> > entry (teid, rInfo);
  std::pair <TeidRoutingMap_t::iterator, bool> ret;
  ret = m_routes.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing routing information for teid " << teid);
    }
}

Ptr<RoutingInfo>
EpcController::GetRoutingInfo (uint32_t teid)
{
  Ptr<RoutingInfo> rInfo = 0;
  TeidRoutingMap_t::iterator ret;
  ret = m_routes.find (teid);
  if (ret != m_routes.end ())
    {
      rInfo = ret->second;
    }
  return rInfo;
}

void
EpcController::SaveSwitchIndex (Ipv4Address ipAddr, uint16_t index)
{
  std::pair<Ipv4Address, uint16_t> entry (ipAddr, index);
  std::pair <IpSwitchMap_t::iterator, bool> ret;
  ret = m_ipSwitchTable.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New IP/Switch entry: " << ipAddr << " - " << index);
      return;
    }
  NS_FATAL_ERROR ("This IP already existis in switch index table.");
}

uint16_t
EpcController::GetSwitchIndex (Ipv4Address addr)
{
  IpSwitchMap_t::iterator ret;
  ret = m_ipSwitchTable.find (addr);
  if (ret != m_ipSwitchTable.end ())
    {
      return static_cast<uint16_t> (ret->second);
    }
  NS_FATAL_ERROR ("IP not registered in switch index table.");
}

void
EpcController::SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr)
{
  std::pair<Ipv4Address, Mac48Address> entry (ipAddr, macAddr);
  std::pair <IpMacMap_t::iterator, bool> ret;
  ret = m_arpTable.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_DEBUG ("New ARP entry: " << ipAddr << " - " << macAddr);
      return;
    }
  NS_FATAL_ERROR ("This IP already exists in ARP table.");
}

Mac48Address
EpcController::GetArpEntry (Ipv4Address ip)
{
  IpMacMap_t::iterator ret;
  ret = m_arpTable.find (ip);
  if (ret != m_arpTable.end ())
    {
      NS_LOG_DEBUG ("Found ARP entry: " << ip << " - " << ret->second);
      return ret->second;
    }
  NS_FATAL_ERROR ("No ARP information for this IP.");
}

void
EpcController::ConfigureLocalPortRules (
  Ptr<OFSwitch13Device> swtchDev, Ptr<NetDevice> nodeDev, Ipv4Address nodeIp,
  uint32_t swtchPort)
{
  NS_LOG_FUNCTION (this << swtchDev << nodeDev << nodeIp << swtchPort);

  // -------------------------------------------------------------------------
  // Table 0 -- Input table -- [from higher to lower priority]
  //
  // GTP packets entering the ring network from any EPC port.
  // Send to the Classification table.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64"
        << " eth_type=0x800,ip_proto=17,udp_src=2152,udp_dst=2152"
        << ",in_port=" << swtchPort
        << " goto:1";
  DpctlSchedule (swtchDev->GetDatapathId (), cmdIn.str ());

  // -------------------------------------------------------------------------
  // Table 2 -- Routing table -- [from higher to lower priority]
  //
  // GTP packets addressed to EPC elements connected to this switch over EPC
  // ports. Write the output port epcPort into action set. Send the packet
  // directly to Output table.
//  Mac48Address devMacAddr = Mac48Address::ConvertFrom (nodeDev->GetAddress ()); //FIXME
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=2,prio=256 eth_type=0x800"
//         << ",eth_dst=" << devMacAddr << ",ip_dst=" << nodeIp
         << ",ip_dst=" << nodeIp
         << " write:output=" << swtchPort
         << " goto:4";
  DpctlSchedule (swtchDev->GetDatapathId (), cmdOut.str ());
}

void
EpcController::ConfigurePgwRules (
  Ptr<OFSwitch13Device> pgwDev, uint32_t pgwSgiPort, uint32_t pgwS5Port,
  Ipv4Address webAddr)
{
  NS_LOG_FUNCTION (this << pgwDev << pgwSgiPort << pgwS5Port << webAddr);
  m_pgwDpId = pgwDev->GetDatapathId ();

  // -------------------------------------------------------------------------
  // Table 0 -- P-GW default table -- [from higher to lower priority]
  //
  // IP packets coming from the Internet (SGI) and addressed to the LTE network
  // (S5) are sent to table 1, where TFT rules will match the flow and set both
  // TEID and eNB addr within tunnel metadata.
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
//        << ",in_port=" << pgwSgiPort << " goto:1";
        << ",in_port=" << pgwSgiPort
        << " apply:output=" << pgwS5Port;  
  DpctlSchedule (m_pgwDpId, cmdIn.str ());

  // IP packets coming from the LTE network and addressed to the Internet are
  // forwared from the S5 to the SGi interface port.
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=0,prio=64 eth_type=0x800"
         << ",in_port=" << pgwS5Port
         << ",ip_dst=" << webAddr
         << " apply:output=" << pgwSgiPort;
  DpctlSchedule (m_pgwDpId, cmdOut.str ());

  // ARP request packets. Send to controller.
  DpctlSchedule (m_pgwDpId, "flow-mod cmd=add,table=0,prio=16 eth_type=0x0806 "
                 "apply:output=ctrl");

  // Table miss entry. Send to controller.
  DpctlSchedule (m_pgwDpId, "flow-mod cmd=add,table=0,prio=0 "
                 "apply:output=ctrl");

  // -------------------------------------------------------------------------
  // Table 1 -- P-GW TFT table -- [from higher to lower priority]
  //
  // Entries will be installed here by TopologyInstallRouting function.
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

  // ARP header
  ArpHeader arp;
  arp.SetReply (srcMac, srcIp, dstMac, dstIp);
  packet->AddHeader (arp);

  // Ethernet header
  EthernetHeader eth (false);
  eth.SetSource (srcMac);
  eth.SetDestination (dstMac);
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }
  eth.SetLengthType (ArpL3Protocol::PROT_NUMBER);
  packet->AddHeader (eth);

  // Ethernet trailer
  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);

  return packet;
}

void
EpcController::RegisterBearer (uint32_t teid, EpsBearer bearer)
{
  std::pair <uint32_t, EpsBearer> entry (teid, bearer);
  std::pair <TeidBearerMap_t::iterator, bool> ret;
  ret = EpcController::m_bearersTable.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing bearer information for teid " << teid);
    }
}

void
EpcController::UnregisterBearer (uint32_t teid)
{
  TeidBearerMap_t::iterator it;
  it = EpcController::m_bearersTable.find (teid);
  if (it != EpcController::m_bearersTable.end ())
    {
      EpcController::m_bearersTable.erase (it);
    }
  else
    {
      NS_FATAL_ERROR ("Error removing bearer information for teid " << teid);
    }
}

EpcController::QciDscpInitializer::QciDscpInitializer ()
{
  // FIXME: Agora no ns-3 tem os valores do DSCP definidos. Substituir.
  NS_LOG_FUNCTION_NOARGS ();

  std::pair <EpsBearer::Qci, uint16_t> entries [9];
  entries [0] = std::make_pair (EpsBearer::GBR_CONV_VOICE, 44);      // Voice
  entries [1] = std::make_pair (EpsBearer::GBR_CONV_VIDEO, 12);      // AF 12
  entries [2] = std::make_pair (EpsBearer::GBR_GAMING, 18);          // AF 21
  entries [3] = std::make_pair (EpsBearer::GBR_NON_CONV_VIDEO, 18);  // AF 11

  // Currently we are mapping all Non-GBR bearers to best effort DSCP traffic.
  entries [4] = std::make_pair (EpsBearer::NGBR_IMS, 0);
  entries [5] = std::make_pair (EpsBearer::NGBR_VIDEO_TCP_OPERATOR, 0);
  entries [6] = std::make_pair (EpsBearer::NGBR_VOICE_VIDEO_GAMING, 0);
  entries [7] = std::make_pair (EpsBearer::NGBR_VIDEO_TCP_PREMIUM, 0);
  entries [8] = std::make_pair (EpsBearer::NGBR_VIDEO_TCP_DEFAULT, 0);

  std::pair <QciDscpMap_t::iterator, bool> ret;
  for (int i = 0; i < 9; i++)
    {
      ret = EpcController::m_qciDscpTable.insert (entries [i]);
      NS_ASSERT_MSG (ret.second, "Can't insert DSCP map value.");
    }
}











EpcController::UeInfo::UeInfo ()
{
  NS_LOG_FUNCTION (this);
}

EpcController::UeInfo::~UeInfo ()
{
  NS_LOG_FUNCTION (this);
}

void
EpcController::UeInfo::AddBearer (Ptr<EpcTft> tft, uint8_t bearerId,
                                  uint32_t teid)
{
  NS_LOG_FUNCTION (this << tft << teid);
  m_teidByBearerIdMap[bearerId] = teid;
  return m_tftClassifier.Add (tft, teid);
}

void
EpcController::UeInfo::RemoveBearer (uint8_t bearerId)
{
  NS_LOG_FUNCTION (this << bearerId);
  m_teidByBearerIdMap.erase (bearerId);
}

uint32_t
EpcController::UeInfo::Classify (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  // we hardcode DOWNLINK direction since the PGW is espected to
  // classify only downlink packets (uplink packets will go to the
  // internet without any classification).
  return m_tftClassifier.Classify (p, EpcTft::DOWNLINK);
}

Ipv4Address
EpcController::UeInfo::GetEnbAddr ()
{
  return m_enbAddr;
}

void
EpcController::UeInfo::SetEnbAddr (Ipv4Address enbAddr)
{
  m_enbAddr = enbAddr;
}

Ipv4Address
EpcController::UeInfo::GetUeAddr ()
{
  return m_ueAddr;
}

void
EpcController::UeInfo::SetUeAddr (Ipv4Address ueAddr)
{
  m_ueAddr = ueAddr;
}




Ipv4Address
EpcController::GetEnbAddr (Ipv4Address ueAddr)
{
  NS_LOG_FUNCTION (this << ueAddr);

  // Find corresponding UeInfo address
  IpUeInfoMap_t::iterator ret;
  ret = m_ueInfoByAddrMap.find (ueAddr);
  if (ret != m_ueInfoByAddrMap.end ())
    {
      return ret->second->GetEnbAddr ();
    }
  NS_FATAL_ERROR ("UE address registered in map.");
}

uint32_t
EpcController::GetTeid (Ipv4Address ueAddr, Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << ueAddr << packet);

  // Find corresponding UeInfo address
  IpUeInfoMap_t::iterator ret;
  ret = m_ueInfoByAddrMap.find (ueAddr);
  if (ret != m_ueInfoByAddrMap.end ())
    {
      return ret->second->Classify (packet);
    }
  NS_FATAL_ERROR ("UE address registered in map.");
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

  // Create and insert eNB info into map.
  EnbInfo enbInfo;
  enbInfo.enbAddr = enbAddr;
  enbInfo.sgwAddr = sgwAddr;

  std::pair<uint16_t, EnbInfo> entry (cellId, enbInfo);
  std::pair <CellIdEnbInfo_t::iterator, bool> ret;
  ret = m_enbInfoByCellId.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing information for cellId " << cellId);
    }
}

void
EpcController::AddUe (uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi);

  // Create and insert UE info into map.
  std::pair<uint64_t, Ptr<UeInfo> > entry (imsi, Create<UeInfo> ());
  std::pair <ImsiUeInfoMap_t::iterator, bool> ret;
  ret = m_ueInfoByImsiMap.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing information for ISMI " << imsi);
    }
}

void
EpcController::SetUeAddress (uint64_t imsi, Ipv4Address ueAddr)
{
  NS_LOG_FUNCTION (this << imsi << ueAddr);

  // Find UE info by ISMI
  ImsiUeInfoMap_t::iterator imsiIt = m_ueInfoByImsiMap.find (imsi);
  if (imsiIt == m_ueInfoByImsiMap.end ())
    {
      NS_FATAL_ERROR ("Unknown IMSI " << imsi);
    }
  imsiIt->second->SetUeAddr (ueAddr);

  // Save UE info by eNB address (only in the first time)
  IpUeInfoMap_t::iterator ipIt = m_ueInfoByAddrMap.find (ueAddr);
  if (ipIt != m_ueInfoByAddrMap.end ())
    {
      NS_ASSERT_MSG (ipIt->second == imsiIt->second, "Invalid UeInfo");
    }
  else
    {
      std::pair<Ipv4Address, Ptr<UeInfo> > entry (ueAddr, imsiIt->second);
      std::pair <IpUeInfoMap_t::iterator, bool> ret;
      ret = m_ueInfoByAddrMap.insert (entry);
      if (ret.second == false)
        {
          NS_FATAL_ERROR ("Existing information for IP " << ueAddr);
        }
    }
}

void
EpcController::DoCreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.imsi);

  ImsiUeInfoMap_t::iterator ueit = m_ueInfoByImsiMap.find (req.imsi);
  NS_ASSERT (ueit != m_ueInfoByImsiMap.end ());
  
  uint16_t cellId = req.uli.gci;
  CellIdEnbInfo_t::iterator enbit = m_enbInfoByCellId.find (cellId);
  NS_ASSERT (enbit != m_enbInfoByCellId.end ());

  Ipv4Address enbAddr = enbit->second.enbAddr;
  ueit->second->SetEnbAddr (enbAddr);

  EpcS11SapMme::CreateSessionResponseMessage res;
  res.teid = req.imsi; // trick to avoid the need for allocating TEIDs on the S11 interface

  std::list<EpcS11SapSgw::BearerContextToBeCreated>::iterator bit;
  for (bit = req.bearerContextsToBeCreated.begin ();
       bit != req.bearerContextsToBeCreated.end ();
       ++bit)
    {
      // simple sanity check. If you ever need more than 4M teids
      // throughout your simulation, you'll need to implement a smarter teid
      // management algorithm.
      NS_ABORT_IF (m_teidCount == 0xFFFFFFFF);
      uint32_t teid = ++m_teidCount;
      ueit->second->AddBearer (bit->tft, bit->epsBearerId, teid);

      EpcS11SapMme::BearerContextCreated bearerContext;
      bearerContext.sgwFteid.teid = teid;
      bearerContext.sgwFteid.address = enbit->second.sgwAddr;
      bearerContext.epsBearerId =  bit->epsBearerId;
      bearerContext.bearerLevelQos = bit->bearerLevelQos;
      bearerContext.tft = bit->tft;
      res.bearerContextsCreated.push_back (bearerContext);
    }

  // Notify the controller of the new create session request accepted.
  NotifySessionCreated (req.imsi, cellId, enbit->second.enbAddr,
                        enbit->second.sgwAddr, res.bearerContextsCreated);

  // Send the response message.
  m_s11SapMme->CreateSessionResponse (res);
}

void
EpcController::DoModifyBearerRequest (EpcS11SapSgw::ModifyBearerRequestMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);
  uint64_t imsi = req.teid; // trick to avoid the need for allocating TEIDs on the S11 interface
  ImsiUeInfoMap_t::iterator ueit = m_ueInfoByImsiMap.find (imsi);
  NS_ASSERT_MSG (ueit != m_ueInfoByImsiMap.end (), "unknown IMSI " << imsi);
  uint16_t cellId = req.uli.gci;
  CellIdEnbInfo_t::iterator enbit = m_enbInfoByCellId.find (cellId);
  NS_ASSERT_MSG (enbit != m_enbInfoByCellId.end (), "unknown CellId " << cellId);
  Ipv4Address enbAddr = enbit->second.enbAddr;
  ueit->second->SetEnbAddr (enbAddr);
  // no actual bearer modification: for now we just support the minimum needed for path switch request (handover)
  EpcS11SapMme::ModifyBearerResponseMessage res;
  res.teid = imsi; // trick to avoid the need for allocating TEIDs on the S11 interface
  res.cause = EpcS11SapMme::ModifyBearerResponseMessage::REQUEST_ACCEPTED;
  m_s11SapMme->ModifyBearerResponse (res);
}

void
EpcController::DoDeleteBearerCommand (EpcS11SapSgw::DeleteBearerCommandMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);
  uint64_t imsi = req.teid; // trick to avoid the need for allocating TEIDs on the S11 interface
  ImsiUeInfoMap_t::iterator ueit = m_ueInfoByImsiMap.find (imsi);
  NS_ASSERT_MSG (ueit != m_ueInfoByImsiMap.end (), "unknown IMSI " << imsi);

  EpcS11SapMme::DeleteBearerRequestMessage res;
  res.teid = imsi;

  for (std::list<EpcS11SapSgw::BearerContextToBeRemoved>::iterator bit = req.bearerContextsToBeRemoved.begin ();
       bit != req.bearerContextsToBeRemoved.end ();
       ++bit)
    {
      EpcS11SapMme::BearerContextRemoved bearerContext;
      bearerContext.epsBearerId =  bit->epsBearerId;
      res.bearerContextsRemoved.push_back (bearerContext);
    }
  //schedules Delete Bearer Request towards MME
  m_s11SapMme->DeleteBearerRequest (res);
}

void
EpcController::DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req)
{
  NS_LOG_FUNCTION (this << req.teid);
  uint64_t imsi = req.teid; // trick to avoid the need for allocating TEIDs on the S11 interface
  ImsiUeInfoMap_t::iterator ueit = m_ueInfoByImsiMap.find (imsi);
  NS_ASSERT_MSG (ueit != m_ueInfoByImsiMap.end (), "unknown IMSI " << imsi);

  for (std::list<EpcS11SapSgw::BearerContextRemovedSgwPgw>::iterator bit = req.bearerContextsRemoved.begin ();
       bit != req.bearerContextsRemoved.end ();
       ++bit)
    {
      //Function to remove de-activated bearer contexts from S-Gw and P-Gw side
      ueit->second->RemoveBearer (bit->epsBearerId);
    }
}


};  // namespace ns3
