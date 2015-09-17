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

#include "ns3/epc-application.h"
#include "openflow-epc-controller.h"
#include "openflow-epc-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcController");
NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcController);

const uint16_t OpenFlowEpcController::m_dedicatedTmo = 15;

OpenFlowEpcController::TeidBearerMap_t OpenFlowEpcController::m_bearersTable;

OpenFlowEpcController::OpenFlowEpcController ()
{
  NS_LOG_FUNCTION (this);

  // Connecting this controller to OpenFlowNetwork trace sources
  Ptr<OpenFlowEpcNetwork> network =
    Names::Find<OpenFlowEpcNetwork> ("/Names/OpenFlowNetwork");
  NS_ASSERT_MSG (network, "Network object not found.");
  NS_ASSERT_MSG (!network->IsTopologyCreated (),
                 "Network topology already created.");

  network->TraceConnectWithoutContext ("NewEpcAttach",
    MakeCallback (&OpenFlowEpcController::NotifyNewEpcAttach, this));
  network->TraceConnectWithoutContext ("TopologyBuilt",
    MakeCallback (&OpenFlowEpcController::NotifyTopologyBuilt, this));
  network->TraceConnectWithoutContext ("NewSwitchConnection",
    MakeCallback (&OpenFlowEpcController::NotifyNewSwitchConnection, this));

  // Connecting this controller to SgwPgwApplication trace sources
  Ptr<EpcSgwPgwApplication> gateway =
    Names::Find<EpcSgwPgwApplication> ("/Names/SgwPgwApplication");
  NS_ASSERT_MSG (gateway, "SgwPgw application not found.");

  gateway->TraceConnectWithoutContext ("ContextCreated",
    MakeCallback (&OpenFlowEpcController::NotifyContextCreated, this));
}

OpenFlowEpcController::~OpenFlowEpcController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
OpenFlowEpcController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OpenFlowEpcController")
    .SetParent (OFSwitch13Controller::GetTypeId ())
    .AddTraceSource ("BearerRequest",
                     "The bearer request trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcController::m_bearerRequestTrace),
                     "ns3::OpenFlowEpcController::BearerTracedCallback")
    .AddTraceSource ("BearerRelease",
                     "The bearer release trace source.",
                     MakeTraceSourceAccessor (&OpenFlowEpcController::m_bearerReleaseTrace),
                     "ns3::OpenFlowEpcController::BearerTracedCallback")
  ;
  return tid;
}

bool
OpenFlowEpcController::RequestDedicatedBearer (EpsBearer bearer, 
    uint64_t imsi, uint16_t cellId, uint32_t teid)
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
OpenFlowEpcController::ReleaseDedicatedBearer (EpsBearer bearer, 
    uint64_t imsi, uint16_t cellId, uint32_t teid)
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
OpenFlowEpcController::GetConstRoutingInfo (uint32_t teid) const
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
OpenFlowEpcController::GetEpsBearer (uint32_t teid)
{
  TeidBearerMap_t::iterator it;
  it = OpenFlowEpcController::m_bearersTable.find (teid);
  if (it != OpenFlowEpcController::m_bearersTable.end ())
    {
      return it->second;
    }
  else
    {
      NS_FATAL_ERROR ("No bearer information for teid " << teid);
    }
}

void
OpenFlowEpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_arpTable.clear ();
  m_ipSwitchTable.clear ();
  m_routes.clear ();
}

void 
OpenFlowEpcController::NotifyNewEpcAttach (Ptr<NetDevice> nodeDev, 
    Ipv4Address nodeIp, Ptr<OFSwitch13NetDevice> swtchDev, uint16_t swtchIdx, 
    uint32_t swtchPort)
{
  NS_LOG_FUNCTION (this << nodeIp << swtchIdx << swtchPort);

  // Save ARP and index information
  Mac48Address macAddr = Mac48Address::ConvertFrom (nodeDev->GetAddress ());
  SaveArpEntry (nodeIp, macAddr);
  SaveSwitchIndex (nodeIp, swtchIdx);

  ConfigureLocalPortRules (swtchDev, nodeDev, nodeIp, swtchPort);
}

void
OpenFlowEpcController::NotifyNewSwitchConnection (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_FUNCTION (this << cInfo);

  // Connecting this controller to ConnectionInfo trace source
  cInfo->TraceConnectWithoutContext ("NonGbrAdjusted",
    MakeCallback (&OpenFlowEpcController::NotifyNonGbrAdjusted, this));
}

void
OpenFlowEpcController::NotifyTopologyBuilt (NetDeviceContainer devices)
{
  NS_LOG_FUNCTION (this);

  m_ofDevices = devices;
  TopologyCreateSpanningTree ();
}

void
OpenFlowEpcController::NotifyContextCreated (uint64_t imsi, uint16_t cellId,
    Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList)
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
  OpenFlowEpcController::RegisterBearer (teid, rInfo->GetEpsBearer ());

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
      OpenFlowEpcController::RegisterBearer (teid, rInfo->GetEpsBearer ());

      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();

      // Create (if necessary) the meter metadata
      if (gbrQoS.mbrDl || gbrQoS.mbrUl)
        {
          Ptr<MeterInfo> meterInfo = CreateObject<MeterInfo> (rInfo);
          rInfo->AggregateObject (meterInfo);
        }

      // For all GBR beares, create the GBR metadata.
      if (rInfo->IsGbr ())
        {
          Ptr<GbrInfo> gbrInfo = CreateObject<GbrInfo> (rInfo);
          rInfo->AggregateObject (gbrInfo);

          // FIXME: Set the appropriated DiffServ DSCP value for this bearer.
          gbrInfo->m_dscp = 46;
        }
    }
}

void
OpenFlowEpcController::NotifyNonGbrAdjusted (Ptr<ConnectionInfo> cInfo)
{
  NS_LOG_INFO (this << cInfo);
}

void
OpenFlowEpcController::ConnectionStarted (SwitchInfo swtch)
{
  NS_LOG_FUNCTION (this << swtch.ipv4);

  // This function is called after a successfully handshake between controller
  // and switch. So, let's start configure the switch tables in accordance to
  // our methodology.

  // Configure the switch to buffer packets and send only the first 128 bytes
  // to the controller.
  DpctlCommand (swtch, "set-config miss=128");


  // -------------------------------------------------------------------------
  // Table 0 -- Starting table -- [from higher to lower priority]

  // ARP request packets are sent to the controller.
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=45055 eth_type=0x0806"
                       " apply:output=ctrl");

  // More entries will be installed here by ConfigureLocalPortRules function.

  // GTP packets entering the switch from any port other then EPC ports are
  // sent to the forwarding table (table 2).
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=32 eth_type=0x800,"
                       "ip_proto=17,udp_src=2152,udp_dst=2152"
                       " goto:2");

  // Table miss entry. Send unmatched packets to the controller.
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");


  // -------------------------------------------------------------------------
  // Table 1 -- Classification table -- [from higher to lower priority]

  // More entries will be installed here by TopologyInstallRouting function.

  // Table miss entry. Send unmatched packets to the controller.
  DpctlCommand (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");


  // -------------------------------------------------------------------------
  // Table 2 -- Forwarding table -- [from higher to lower priority]

  // GBR packet classified at table 1. Forward the packet to the correct
  // routing group.
  DpctlCommand (swtch, "flow-mod cmd=add,table=2,prio=256"
                       " eth_type=0x800,ip_dscp=46,meta=0x1"
                       " write:group=1");
  DpctlCommand (swtch, "flow-mod cmd=add,table=2,prio=256"
                       " eth_type=0x800,ip_dscp=46,meta=0x2"
                       " write:group=2");

  // Non-GBR packet classified at table 1. Apply corresponding Non-GBR meter
  // band and forward the packet to the the correct routing group.
  DpctlCommand (swtch, "flow-mod cmd=add,table=2,prio=256"
                       " eth_type=0x800,ip_dscp=0,meta=0x1"
                       " meter:1 write:group=1");
  DpctlCommand (swtch, "flow-mod cmd=add,table=2,prio=256"
                       " eth_type=0x800,ip_dscp=0,meta=0x2"
                       " meter:2 write:group=2");

  // More entries will be installed here by NotifyTopologyBuilt function.

  // Table miss entry. Send unmatched packets to the controller.
  DpctlCommand (swtch, "flow-mod cmd=add,table=2,prio=0 apply:output=ctrl");
}

ofl_err
OpenFlowEpcController::HandlePacketIn (ofl_msg_packet_in *msg,
                                       SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << xid);
  ofl_match_tlv *tlv;

  enum ofp_packet_in_reason reason = msg->reason;
  if (reason == OFPR_NO_MATCH)
    {
      char *m = ofl_structs_match_to_string (msg->match, 0);
      NS_LOG_INFO ("Packet in match: " << m);
      free (m);

      // (Table #1 is used only for GTP TEID routing)
      uint8_t tableId = msg->table_id;
      if (tableId == 1)
        {
          uint32_t teid;
          tlv = oxm_match_lookup (OXM_OF_GTPU_TEID, (ofl_match*)msg->match);
          memcpy (&teid, tlv->value, OXM_LENGTH (OXM_OF_GTPU_TEID));

          NS_LOG_LOGIC ("TEID routing miss packet: " << teid);
          return HandleGtpuTeidPacketIn (msg, swtch, xid, teid);
        }
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
OpenFlowEpcController::HandleFlowRemoved (ofl_msg_flow_removed *msg,
                                          SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << xid);

  uint8_t table = msg->stats->table_id;
  uint32_t teid = msg->stats->cookie;
  uint16_t prio = msg->stats->priority;

  NS_LOG_FUNCTION (swtch.ipv4 << teid);

  char *m = ofl_msg_to_string ((ofl_msg_header*)msg, 0);
  // NS_LOG_DEBUG ("Flow removed: " << m);
  free (m);

  // Since handlers must free the message when everything is ok,
  // let's remove it now, as we already got the necessary information.
  ofl_msg_free_flow_removed (msg, true, 0);

  // Ignoring flows removed from tables other than TEID table #1
  if (table != 1)
    {
      NS_LOG_WARN ("Ignoring flow removed from table " << table);
      return 0;
    }

  // Check for existing routing information for this bearer
  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // When a rule expires due to idle timeout, check the following situations:
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
  // TODO: With HTTP traffic, when MaxPages > 1, the random reading time can be
  // larger than current rule idle timeout. This will trigger this rule
  // reinstallation process, but it could be avoided.
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

Ptr<OFSwitch13NetDevice>
OpenFlowEpcController::GetSwitchDevice (uint16_t index)
{
  NS_ASSERT (index < m_ofDevices.GetN ());
  return DynamicCast<OFSwitch13NetDevice> (m_ofDevices.Get (index));
}

void
OpenFlowEpcController::SaveRoutingInfo (Ptr<RoutingInfo> rInfo)
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
OpenFlowEpcController::GetRoutingInfo (uint32_t teid)
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
OpenFlowEpcController::SaveSwitchIndex (Ipv4Address ipAddr, uint16_t index)
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
OpenFlowEpcController::GetSwitchIndex (Ipv4Address addr)
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
OpenFlowEpcController::SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr)
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
OpenFlowEpcController::GetArpEntry (Ipv4Address ip)
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
OpenFlowEpcController::ConfigureLocalPortRules (
  Ptr<OFSwitch13NetDevice> swtchDev, Ptr<NetDevice> nodeDev,
  Ipv4Address nodeIp, uint32_t swtchPort)
{
  NS_LOG_FUNCTION (this << swtchDev << nodeDev << nodeIp << swtchPort);

  // -------------------------------------------------------------------------
  // Table 0 -- Starting table -- [from higher to lower priority]
  //
  // GTP packets addressed to the EPC entity connected to this switch over
  // EPC switchPort are sent to this EPC switchPort to leave the ring
  // network.
  Mac48Address devMacAddr = Mac48Address::ConvertFrom (nodeDev->GetAddress ());
  std::ostringstream cmdOut;
  cmdOut << "flow-mod cmd=add,table=0,prio=128"
         << " eth_type=0x800,ip_proto=17,udp_src=2152,udp_dst=2152"
         << ",eth_dst=" << devMacAddr << ",ip_dst=" << nodeIp
         << " apply:output=" << swtchPort;
  DpctlCommand (swtchDev, cmdOut.str ());

  // GTP packets entering the switch from EPC switchPort are sent to the
  // classification table (table 1).
  std::ostringstream cmdIn;
  cmdIn << "flow-mod cmd=add,table=0,prio=64"
        << " eth_type=0x800,ip_proto=17,udp_src=2152,udp_dst=2152"
        << ",in_port=" << swtchPort
        << " goto:1";
  DpctlCommand (swtchDev, cmdIn.str ());
}

ofl_err
OpenFlowEpcController::HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
    SwitchInfo swtch, uint32_t xid, uint32_t teid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << xid << teid);

  // Let's check for active routing path
  Ptr<RoutingInfo> rInfo = GetRoutingInfo (teid);
  if (rInfo && rInfo->IsActive ())
    {
      NS_LOG_WARN ("Not supposed to happen, but we can handle this.");
      if (!TopologyInstallRouting (rInfo, msg->buffer_id))
        {
          NS_LOG_ERROR ("TEID rule installation failed!");
        }
    }
  else
    {
      NS_LOG_WARN ("Ignoring TEID packet sent to controller.");
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((ofl_msg_header*)msg, 0 /*dp->exp*/);
  return 0;
}

ofl_err
OpenFlowEpcController::HandleArpPacketIn (ofl_msg_packet_in *msg,
                                          SwitchInfo swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << xid);

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

      int error = SendToSwitch (&swtch, (ofl_msg_header*)&reply, xid);
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
OpenFlowEpcController::ExtractIpv4Address (uint32_t oxm_of, ofl_match* match)
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
OpenFlowEpcController::CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp,
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
OpenFlowEpcController::RegisterBearer (uint32_t teid, EpsBearer bearer)
{
  std::pair <uint32_t, EpsBearer> entry (teid, bearer);
  std::pair <TeidBearerMap_t::iterator, bool> ret;
  ret = OpenFlowEpcController::m_bearersTable.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing bearer information for teid " << teid);
    }
}

void
OpenFlowEpcController::UnregisterBearer (uint32_t teid)
{
  TeidBearerMap_t::iterator it;
  it = OpenFlowEpcController::m_bearersTable.find (teid);
  if (it != OpenFlowEpcController::m_bearersTable.end ())
    {
      OpenFlowEpcController::m_bearersTable.erase (it);
    }
  else
    {
      NS_FATAL_ERROR ("Error removing bearer information for teid " << teid);
    }
}

};  // namespace ns3
