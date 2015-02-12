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

#include "openflow-epc-controller.h"
#include "internet-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OpenFlowEpcController");
NS_OBJECT_ENSURE_REGISTERED (OpenFlowEpcController);

const int OpenFlowEpcController::m_defaultTimeout = 0; 
const int OpenFlowEpcController::m_dedicatedTimeout = 15;
const int OpenFlowEpcController::m_defaultPriority = 100;  
const int OpenFlowEpcController::m_dedicatedPriority = 1000;

OpenFlowEpcController::OpenFlowEpcController ()
  : m_gbrBearers (0),
    m_gbrBlocks (0)
{
  NS_LOG_FUNCTION (this);
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
    .AddAttribute ("OFNetwork",
                   "The OpenFlow network controlled by this app.",
                   PointerValue (),
                   MakePointerAccessor (&OpenFlowEpcController::m_ofNetwork),
                   MakePointerChecker<OpenFlowEpcNetwork> ())
  ;
  return tid;
}

void
OpenFlowEpcController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_arpTable.clear ();
  m_ipSwitchTable.clear ();
  m_connections.clear ();
  m_contexts.clear ();
  m_routes.clear ();
  m_ofNetwork = 0;
}

const Time
OpenFlowEpcController::GetDedicatedTimeout ()
{
  return Seconds (m_dedicatedTimeout);
}

double
OpenFlowEpcController::GetBlockRatioStatistics ()
{
  double ratio = (double)m_gbrBlocks / (double)m_gbrBearers;
  std::cout << "Number of GBR bearers request: " << m_gbrBearers  << std::endl
            << "Number of GBR bearers blocked: " << m_gbrBlocks   << std::endl
            << "Block ratio: "                   << ratio         << std::endl;
  return ratio;
}

void 
OpenFlowEpcController::NotifyNewIpDevice (Ptr<NetDevice> dev, Ipv4Address ip, 
    uint16_t switchIdx)
{
  NS_LOG_FUNCTION (this << dev << ip << switchIdx);

  { // Save the pair IP/MAC address in ARP table
    Mac48Address macAddr = Mac48Address::ConvertFrom (dev->GetAddress ());
    std::pair<Ipv4Address, Mac48Address> entry (ip, macAddr);
    std::pair <IpMacMap_t::iterator, bool> ret;
    ret = m_arpTable.insert (entry);
    if (ret.second == false)
      {
        NS_FATAL_ERROR ("This IP already exists in ARP table.");
      }
    NS_LOG_DEBUG ("New ARP entry: " << ip << " - " << macAddr);
  }

  { // Save the pair IP/Switch index in switch table
    std::pair<Ipv4Address, uint16_t> entry (ip, switchIdx);
    std::pair <IpSwitchMap_t::iterator, bool> ret;
    ret = m_ipSwitchTable.insert (entry);
    if (ret.second == false)
      {
        NS_FATAL_ERROR ("This IP already existis in switch index table.");
      }
    NS_LOG_DEBUG ("New IP/Switch entry: " << ip << " - " << switchIdx);
  }
}

void
OpenFlowEpcController::NotifyNewSwitchConnection (
    const Ptr<ConnectionInfo> connInfo)
{
  NS_LOG_FUNCTION (this << connInfo);
  
  // Save this connection info
  SwitchPair_t key;
  key.first  = std::min (connInfo->switchIdx1, connInfo->switchIdx2);
  key.second = std::max (connInfo->switchIdx1, connInfo->switchIdx2);
  std::pair<SwitchPair_t, Ptr<ConnectionInfo> > entry (key, connInfo);
  std::pair<ConnInfoMap_t::iterator, bool> ret;
  ret = m_connections.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Error saving connection info.");
    }
  NS_LOG_DEBUG ("New connection info saved: switch " << key.first << 
                " (" << connInfo->portNum1 << ") -- switch " << key.second << 
                " (" << connInfo->portNum2 << ")");
}

bool
OpenFlowEpcController::RequestNewDedicatedBearer (uint64_t imsi, 
    uint16_t cellId, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << imsi << cellId << tft);
  
  // Allowing any bearer creation
  return true;
}

void 
OpenFlowEpcController::NotifyNewContextCreated (uint64_t imsi, uint16_t cellId,
    Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this << imsi << cellId << enbAddr << sgwAddr);

  // Create context info and save in context list.
  Ptr<ContextInfo> info = CreateObject<ContextInfo> ();
  info->m_imsi = imsi;
  info->m_cellId = cellId;
  info->m_enbIdx = GetSwitchIdxFromIp (enbAddr);
  info->m_sgwIdx = GetSwitchIdxFromIp (sgwAddr);
  info->m_enbAddr = enbAddr;
  info->m_sgwAddr = sgwAddr;
  info->m_bearerList = bearerList;
  m_contexts.push_back (info);

  // Create and save routing information for default bearer
  ContextBearer_t defaultBearer = bearerList.front ();
  NS_ASSERT_MSG (defaultBearer.epsBearerId == 1, "Not a default bearer.");
  
  uint32_t teid = defaultBearer.sgwFteid.teid;
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  NS_ASSERT_MSG (rInfo == 0, "Existing routing for default bearer " << teid);

  rInfo = CreateObject<RoutingInfo> ();
  rInfo->m_teid = teid;
  rInfo->m_sgwIdx = GetSwitchIdxFromIp (sgwAddr);
  rInfo->m_enbIdx = GetSwitchIdxFromIp (enbAddr);
  rInfo->m_sgwAddr = sgwAddr;
  rInfo->m_enbAddr = enbAddr;
  rInfo->m_app = 0;                       // No app for default bearer
  rInfo->m_priority = m_defaultPriority;  // Priority for default bearer
  rInfo->m_timeout = m_defaultTimeout;    // No timeout for default bearer
  rInfo->m_isInstalled = false;           // Bearer rules not installed yet
  rInfo->m_isActive = true;               // Default bearer is always active
  rInfo->m_isDefault = true;              // This is a default bearer
  rInfo->m_bearer = defaultBearer;
  SaveTeidRoutingInfo (rInfo);

  // Install rules for default bearer
  if (!InstallTeidRouting (rInfo))
    {
      NS_LOG_ERROR ("TEID rule installation failed!");
    }
}

bool 
OpenFlowEpcController::NotifyAppStart (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);

  ResetAppStatistics (app);
  uint32_t teid = GetTeidFromApplication (app);
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo == 0)
    {
      // This is the first time in simulation we are using this dedicated
      // bearer. Let's create and save it's routing metadata.
      NS_LOG_DEBUG ("First use of bearer TEID " << teid);
     
      Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
      ContextBearer_t dedicatedBearer = GetBearerFromTft (tft);
      Ptr<const ContextInfo> cInfo = GetContextFromTft (tft);
      rInfo = CreateObject<RoutingInfo> ();
      rInfo->m_teid = teid;
      rInfo->m_sgwIdx = cInfo->GetSgwIdx ();
      rInfo->m_enbIdx = cInfo->GetEnbIdx ();
      rInfo->m_sgwAddr = cInfo->GetSgwAddr ();
      rInfo->m_enbAddr = cInfo->GetEnbAddr ();
      rInfo->m_app = app;                      // App for this dedicated bearer
      rInfo->m_priority = m_dedicatedPriority; // Priority for dedicated bearer
      rInfo->m_timeout = m_dedicatedTimeout;   // Timeout for dedicated bearer
      rInfo->m_isInstalled = false;            // Switch rules not installed yet
      rInfo->m_isActive = false;               // Dedicated bearer not active yet
      rInfo->m_isDefault = false;              // This is a dedicated bearer
      rInfo->m_bearer = dedicatedBearer;
      SaveTeidRoutingInfo (rInfo);

      // Check for meter rules
      GbrQosInformation gbrQoS = rInfo->GetQosInfo ();
      if (gbrQoS.mbrDl || gbrQoS.mbrUl)
        {
          Ptr<MeterInfo> meterInfo = CreateObject<MeterInfo> (rInfo);
          meterInfo->m_teid = teid;
          if (gbrQoS.mbrDl)
            {
              meterInfo->m_hasDown = true;
              meterInfo->m_downSwitch = rInfo->m_sgwIdx;
              meterInfo->m_downBitRate = gbrQoS.mbrDl;
            }
          if (gbrQoS.mbrUl)
            {
              meterInfo->m_hasUp = true;
              meterInfo->m_upSwitch = rInfo->m_enbIdx;
              meterInfo->m_upBitRate = gbrQoS.mbrUl;
            }
          rInfo->AggregateObject (meterInfo);
        }
    }

  // Is it a default bearer?
  if (rInfo->m_isDefault)
    {
      // If the application traffic is sent over default bearer, there is no
      // need for resource reservation nor reinstall the switch rules, as
      // default rules were supposed to remain installed during entire
      // simulation and must be Non-GBR.
      NS_ASSERT_MSG (rInfo->m_isActive && rInfo->m_isInstalled, 
                     "Default bearer should be intalled and activated.");
      return true;
    }

  // Is it an active (aready configured) bearer?
  if (rInfo->m_isActive)
    {
      // This happens with VoIP application, which are installed in pairs and,
      // when the second application starts, the first one has already
      // configured the routing for this bearer and set the active flag.
      NS_ASSERT_MSG (rInfo->m_isInstalled, "Bearer should be installed.");
      NS_LOG_DEBUG ("Routing path for " << teid << " is already installed.");
      return true;
    }

  NS_ASSERT_MSG (!rInfo->m_isActive, "Bearer should be inactive.");
  // So, this bearer must be inactive and we are goind to reuse it's metadata.
  // Every time the application starts using an (old) existing bearer, let's
  // reinstall the rules on the switches, which will inscrease the bearer
  // priority. Doing this, we avoid problems with old 'expiring' rules, and we
  // can even use new routing paths when necessary.

  // For dedicated GBR bearers, let's first check for available resources. 
  if (rInfo->IsGbr ())
    {
      if (!GbrBearerRequest (rInfo))
        {
          return false;
        }
    }

  // Everything is ok! Let's activate and install this bearer.
  rInfo->m_isActive = true;
  return InstallTeidRouting (rInfo);
}

bool
OpenFlowEpcController::NotifyAppStop (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);
 
  uint32_t teid = GetTeidFromApplication (app);
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  NS_ASSERT_MSG (rInfo, "No routing information for teid.");
  
  // Check for active bearer
  if (rInfo->m_isActive == true)
    {
      rInfo->m_isActive = false;
      rInfo->m_isInstalled = false;
      GbrBearerRelease (rInfo);
      // No need to remove the rules from switch. Wait for idle timeout.
    }

  PrintAppStatistics (app);
  return true;
}

void 
OpenFlowEpcController::ConfigurePortDelivery (Ptr<OFSwitch13NetDevice> swtch, 
    Ptr<NetDevice> device, Ipv4Address deviceIp, uint32_t devicePort)
{
  NS_LOG_FUNCTION (this << swtch << device << deviceIp << (int)devicePort);

  Mac48Address devMacAddr = Mac48Address::ConvertFrom (device->GetAddress ());
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=40000" <<
         " eth_type=0x800,eth_dst=" << devMacAddr <<
         ",ip_dst=" << deviceIp << " apply:output=" << devicePort;
  DpctlCommand (swtch, cmd.str ());
}

uint16_t 
OpenFlowEpcController::GetNSwitches ()
{
  return m_ofNetwork->GetNSwitches ();
}

Ptr<OFSwitch13NetDevice> 
OpenFlowEpcController::GetSwitchDevice (uint16_t index)
{
  return m_ofNetwork->GetSwitchDevice (index);
}

uint16_t 
OpenFlowEpcController::GetSwitchIdxForGateway ()
{
  return m_ofNetwork->GetSwitchIdxForGateway ();
}

uint16_t
OpenFlowEpcController::GetSwitchIdxFromDevice (Ptr<OFSwitch13NetDevice> dev)
{
  return m_ofNetwork->GetSwitchIdxForDevice (dev);
}

uint16_t 
OpenFlowEpcController::GetSwitchIdxFromIp (Ipv4Address addr)
{
  IpSwitchMap_t::iterator ret;
  ret = m_ipSwitchTable.find (addr);
  if (ret != m_ipSwitchTable.end ())
    {
      return (uint16_t)ret->second;
    }
  NS_FATAL_ERROR ("IP not registered in switch index table.");
}

Ptr<const ContextInfo>
OpenFlowEpcController::GetContextFromTeid (uint32_t teid)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      BearerList_t::iterator blsIt = cInfo->m_bearerList.begin (); 
      for ( ; blsIt != cInfo->m_bearerList.end (); blsIt++)
        {
          if (blsIt->sgwFteid.teid == teid)
            {
              return cInfo;
            }
        }
    }
  NS_FATAL_ERROR ("Couldn't find bearer for invalid teid.");
}

uint32_t 
OpenFlowEpcController::GetTeidFromApplication (Ptr<Application> app)
{
  Ptr<EpcTft> tft = app->GetObject<EpcTft> ();
  return GetBearerFromTft (tft).sgwFteid.teid;
}

Ptr<ConnectionInfo>
OpenFlowEpcController::GetConnectionInfo (uint16_t sw1, uint16_t sw2)
{
  SwitchPair_t key;
  key.first = std::min (sw1, sw2);
  key.second = std::max (sw1, sw2);
  ConnInfoMap_t::iterator it = m_connections.find (key);
  if (it != m_connections.end ())
    {
      return it->second;
    }
  NS_FATAL_ERROR ("No connection information available.");
}

Ptr<RoutingInfo>
OpenFlowEpcController::GetTeidRoutingInfo (uint32_t teid)
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
OpenFlowEpcController::PrintAppStatistics (Ptr<Application> app)
{
  uint32_t teid = GetTeidFromApplication (app);
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);

  if (app->GetInstanceTypeId () == VoipPeer::GetTypeId ())
    {
      Ptr<VoipPeer> voipApp = DynamicCast<VoipPeer> (app);

      // Identifying Voip traffic direction
      std::string nodeName = Names::FindName (voipApp->GetNode ());
      bool downlink = InternetNetwork::GetServerName () == nodeName;
      uint16_t srcIdx = downlink ? rInfo->m_sgwIdx : rInfo->m_enbIdx;
      uint16_t dstIdx = downlink ? rInfo->m_enbIdx : rInfo->m_sgwIdx; 
      
      std::cout << 
        "VoIP (TEID " << teid << ") [" << srcIdx << " -> " << dstIdx << "]" << 
        " Duration " << voipApp->GetActiveTime ().ToInteger (Time::MS) << " ms -" << 
        " Loss " << voipApp->GetRxLossRatio () << " -" <<
        " Delay " << voipApp->GetRxDelay ().ToInteger (Time::MS) << " ms -" <<
        " Jitter " << voipApp->GetRxJitter ().ToInteger (Time::MS) << " ms -" << 
        " Goodput " << voipApp->GetRxGoodput () << 
      std::endl; 
    }
  else if (app->GetInstanceTypeId () == VideoClient::GetTypeId ())
    {
      // Get the relative UDP server for this client
      Ptr<VideoClient> videoApp = DynamicCast<VideoClient> (app);
      Ptr<UdpServer> serverApp = videoApp->GetServerApp ();
      std::cout << 
        "Video (TEID " << teid << ") [" <<  rInfo->m_sgwIdx << " -> " << rInfo->m_enbIdx << "]" << 
        " Duration " << serverApp->GetActiveTime ().ToInteger (Time::MS) << " ms -" << 
        " Loss " << serverApp->GetRxLossRatio () << " -" <<
        " Delay " << serverApp->GetRxDelay ().ToInteger (Time::MS) << " ms -" <<
        " Jitter " << serverApp->GetRxJitter ().ToInteger (Time::MS) << " ms -" << 
        " Goodput " << serverApp->GetRxGoodput () << 
      std::endl; 
    }
  else if (app->GetInstanceTypeId () == HttpClient::GetTypeId ())
    {
      Ptr<HttpClient> httpApp = DynamicCast<HttpClient> (app);
      std::cout << 
        "HTTP (TEID " << teid << ") [" <<  rInfo->m_sgwIdx << " <-> " << rInfo->m_enbIdx << "]" << 
        " Duration " << httpApp->GetActiveTime ().ToInteger (Time::MS) << " ms -" << 
        " Transfered " << httpApp->GetRxBytes () << " bytes -" <<
        " Goodput " << httpApp->GetRxGoodput () << 
      std::endl; 
    }
}

void
OpenFlowEpcController::ResetAppStatistics (Ptr<Application> app)
{
  NS_LOG_FUNCTION (this << app);
  
  if (app->GetInstanceTypeId () == VoipPeer::GetTypeId ())
    {
      DynamicCast<VoipPeer> (app)->ResetCounters ();
    }
  else if (app->GetInstanceTypeId () == VideoClient::GetTypeId ())
    {
      Ptr<VideoClient> videoApp = DynamicCast<VideoClient> (app);
      videoApp->ResetCounters ();
      videoApp->GetServerApp ()->ResetCounters ();
    }
  else if (app->GetInstanceTypeId () == HttpClient::GetTypeId ())
    {
      Ptr<HttpClient> httpApp = DynamicCast<HttpClient> (app);
      httpApp->ResetCounters ();
      httpApp->GetServerApp ()->ResetCounters ();
    }
}

void
OpenFlowEpcController::IncreaseGbrRequest ()
{
  m_gbrBearers++;
}

void
OpenFlowEpcController::IncreaseGbrBlocks ()
{
  m_gbrBlocks++;
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

void
OpenFlowEpcController::ConnectionStarted (SwitchInfo swtch)
{
  NS_LOG_FUNCTION (this << swtch.ipv4);
  
  // Set the switch to buffer packets and send only the first 128 bytes
  DpctlCommand (swtch, "set-config miss=128");

  // After a successfull handshake, let's install some default entries: 
  // Table miss entry and ARP handling entry.
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl");
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=1 eth_type=0x0806 "
                       "apply:output=ctrl");

  // Handling GTP tunnels at table #1
  DpctlCommand (swtch, "flow-mod cmd=add,table=0,prio=2 eth_type=0x800,"
                       "ip_proto=17,udp_src=2152,udp_dst=2152 goto:1");
  DpctlCommand (swtch, "flow-mod cmd=add,table=1,prio=0 apply:output=ctrl");
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
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo == 0)
    {
      NS_FATAL_ERROR ("Routing info for TEID " << teid << " not found.");
      return 0;
    }

  // When a rule expires due to idle timeout, check the following situations:
  // 1) The application is stopped and the bearer must be inactive.
  if (!rInfo->m_isActive)
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for stopped application.");
      return 0;
    }
  
  // 2) The application is running and the bearer is active, but the
  // application has already been stopped since last rule installation. In this
  // case, the bearer priority should have been increased to avoid conflicts.
  if (rInfo->m_priority > prio)
    {
      NS_LOG_DEBUG ("Flow " << teid << " removed for old rule.");
      return 0;
    }

  // 3) The application is running and the bearer is active. This is the
  // critical situation. For some reason, the traffic absence lead to flow
  // expiration, and we need to reinstall the rules with higher priority to
  // avoid problems. 
  NS_ASSERT_MSG (rInfo->m_priority == prio, "Invalid flow priority.");
  if (rInfo->m_isActive)
    {
      NS_LOG_DEBUG ("Flow " << teid << " is still active. Reinstall rules...");
      if (!InstallTeidRouting (rInfo))
        {
          NS_LOG_ERROR ("TEID rule installation failed!");
        }
      return 0;
    }

  NS_ABORT_MSG ("Should not get here :/");
}


Ptr<const ContextInfo>
OpenFlowEpcController::GetContextFromTft (Ptr<EpcTft> tft)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      BearerList_t::iterator blsIt = cInfo->m_bearerList.begin (); 
      for ( ; blsIt != cInfo->m_bearerList.end (); blsIt++)
        {
          if (blsIt->tft == tft)
            {
              return cInfo;
            }
        }
    }
  NS_FATAL_ERROR ("Couldn't find context for invalid tft.");
}

ContextBearer_t 
OpenFlowEpcController::GetBearerFromTft (Ptr<EpcTft> tft)
{
  Ptr<ContextInfo> cInfo = 0;
  ContextInfoList_t::iterator ctxIt;
  for (ctxIt = m_contexts.begin (); ctxIt != m_contexts.end (); ctxIt++)
    {
      cInfo = *ctxIt;
      BearerList_t::iterator blsIt = cInfo->m_bearerList.begin (); 
      for ( ; blsIt != cInfo->m_bearerList.end (); blsIt++)
        {
          if (blsIt->tft == tft)
            {
              return *blsIt;
            }
        }
    }
  NS_FATAL_ERROR ("Couldn't find bearer for invalid tft.");
}

void 
OpenFlowEpcController::SaveTeidRoutingInfo (Ptr<RoutingInfo> rInfo)
{
  NS_LOG_FUNCTION (this << rInfo);
  
  std::pair <uint32_t, Ptr<RoutingInfo> > entry (rInfo->m_teid, rInfo);
  std::pair <TeidRoutingMap_t::iterator, bool> ret;
  ret = m_routes.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Existing routing information for teid " << rInfo->m_teid);
    }
}

ofl_err
OpenFlowEpcController::HandleGtpuTeidPacketIn (ofl_msg_packet_in *msg, 
    SwitchInfo swtch, uint32_t xid, uint32_t teid)
{
  NS_LOG_FUNCTION (this << swtch.ipv4 << xid << teid);

  // Let's check for active routing path
  Ptr<RoutingInfo> rInfo = GetTeidRoutingInfo (teid);
  if (rInfo && rInfo->m_isActive)
    {
      NS_LOG_WARN ("Not supposed to happen, but we can handle this.");
      if (!InstallTeidRouting (rInfo, msg->buffer_id))
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
      Mac48Address dstMac = ArpLookup (dstIp);
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

Mac48Address 
OpenFlowEpcController::ArpLookup (Ipv4Address ip)
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

};  // namespace ns3
