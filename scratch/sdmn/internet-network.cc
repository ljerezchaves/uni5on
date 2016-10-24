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

#include "internet-network.h"
#include "stats-calculator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InternetNetwork");
NS_OBJECT_ENSURE_REGISTERED (InternetNetwork);

InternetNetwork::InternetNetwork ()
  : m_internetStats (0)
{
  NS_LOG_FUNCTION (this);

  // Creating the queue stats calculator
  ObjectFactory statsFactory;
  statsFactory.SetTypeId (LinkQueuesStatsCalculator::GetTypeId ());
  statsFactory.Set ("LnkStatsFilename", StringValue ("web_stats.txt"));
  m_internetStats = statsFactory.Create<LinkQueuesStatsCalculator> ();
}

InternetNetwork::~InternetNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
InternetNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::InternetNetwork")
    .SetParent<Object> ()
    .AddAttribute ("LinkDataRate",
                   "The data rate to be used for the Internet link",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&InternetNetwork::m_linkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The delay to be used for the Internet link",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&InternetNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU of the Internet link",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1492),  // PPPoE MTU
                   MakeUintegerAccessor (&InternetNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

Ptr<Node>
InternetNetwork::CreateTopology (Ptr<Node> pgw)
{
  NS_LOG_FUNCTION (this << pgw);

  // Configuring helpers
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate",
                                    DataRateValue (m_linkDataRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Creating a single web node and connecting it to the EPC PGW node
  Ptr<Node> web = CreateObject<Node> ();
  Names::Add ("srv", web);

  InternetStackHelper internet;
  internet.Install (web);

  m_webNodes.Add (pgw);
  m_webNodes.Add (web);

  m_webDevices = m_csmaHelper.Install (m_webNodes);
  Ptr<CsmaNetDevice> webDev, pgwDev;
  pgwDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (0));
  webDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (1));

  Names::Add (Names::FindName (pgw) + "+" + Names::FindName (web), pgwDev);
  Names::Add (Names::FindName (web) + "+" + Names::FindName (pgw), webDev);

  m_internetStats->SetQueues (webDev->GetQueue (), pgwDev->GetQueue ());

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("192.168.0.0", "255.255.255.0");
  ipv4h.Assign (m_webDevices);

  // Defining static routes at the web node to the LTE network
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (web->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"),
    Ipv4Address ("192.168.0.1"), 1);

  return web;
}

Ptr<Node>
InternetNetwork::GetServerNode ()
{
  return m_webNodes.Get (1);
}

void
InternetNetwork::EnablePcap (std::string prefix)
{
  NS_LOG_FUNCTION (this);
  m_csmaHelper.EnablePcap (prefix, m_webDevices);
}

void
InternetNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_internetStats = 0;
  Object::DoDispose ();
}

};  // namespace ns3
