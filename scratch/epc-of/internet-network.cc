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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InternetNetwork");
NS_OBJECT_ENSURE_REGISTERED (InternetNetwork);

InternetNetwork::InternetNetwork ()
  : m_internetStats (0)
{
  NS_LOG_FUNCTION (this);
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
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&InternetNetwork::m_linkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The delay to be used for the Internet link",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&InternetNetwork::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu",
                   "The MTU of the Internet link",
                   UintegerValue (1492),  // PPPoE MTU
                   MakeUintegerAccessor (&InternetNetwork::m_linkMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("UseCsmaLink",
                   "If true, use CSMA link instead of P2P",
                   BooleanValue (true),
                   MakeBooleanAccessor (&InternetNetwork::m_csmaLink),
                   MakeBooleanChecker ())
  ;
  return tid;
}

Ptr<Node>
InternetNetwork::CreateTopology (Ptr<Node> pgw)
{
  NS_LOG_FUNCTION (this << pgw);

  // Configuring helpers
  m_p2pHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_p2pHelper.SetDeviceAttribute ("DataRate", DataRateValue (m_linkDataRate));
  m_p2pHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (m_linkMtu));
  m_csmaHelper.SetChannelAttribute ("DataRate",
                                    DataRateValue (m_linkDataRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (m_linkDelay));

  // Creating a single web node and connecting it to the EPC pgw over a
  // PointToPoint link.
  Ptr<Node> web = CreateObject<Node> ();
  Names::Add (InternetNetwork::GetServerName (), web);

  InternetStackHelper internet;
  internet.Install (web);

  m_webNodes.Add (pgw);
  m_webNodes.Add (web);

  if (m_csmaLink)
    {
      m_webDevices = m_csmaHelper.Install (m_webNodes);
      Ptr<CsmaNetDevice> webDev, pgwDev;
      pgwDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (0));
      webDev = DynamicCast<CsmaNetDevice> (m_webDevices.Get (1));

      Names::Add (Names::FindName (pgw) + "+" + Names::FindName (web), pgwDev);
      Names::Add (Names::FindName (web) + "+" + Names::FindName (pgw), webDev);

      Names::Add ("InternetNetwork/DownQueue", webDev->GetQueue ());
      Names::Add ("InternetNetwork/UpQueue", pgwDev->GetQueue ());
      m_internetStats->SetQueues (webDev->GetQueue (), pgwDev->GetQueue ());
    }
  else
    {
      m_webDevices = m_p2pHelper.Install (m_webNodes);
      Ptr<PointToPointNetDevice> webDev, pgwDev;
      pgwDev = DynamicCast<PointToPointNetDevice> (m_webDevices.Get (0));
      webDev = DynamicCast<PointToPointNetDevice> (m_webDevices.Get (1));

      Names::Add (Names::FindName (pgw) + "+" + Names::FindName (web), pgwDev);
      Names::Add (Names::FindName (web) + "+" + Names::FindName (pgw), webDev);

      Names::Add ("InternetNetwork/DownQueue", webDev->GetQueue ());
      Names::Add ("InternetNetwork/UpQueue", pgwDev->GetQueue ());
      m_internetStats->SetQueues (webDev->GetQueue (), pgwDev->GetQueue ());
    }

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("192.168.0.0", "255.255.255.0");
  ipv4h.Assign (m_webDevices);

  // Defining static routes to the web node
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
    ipv4RoutingHelper.GetStaticRouting (web->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (
    Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"),
    Ipv4Address ("192.168.0.1"), 1);

  return web;
}

void
InternetNetwork::EnablePcap (std::string prefix)
{
  NS_LOG_FUNCTION (this);
  if (m_csmaLink)
    {
      m_csmaHelper.EnablePcap (prefix, m_webDevices);
    }
  else
    {
      m_p2pHelper.EnablePcap (prefix, m_webDevices);
    }
}

const std::string
InternetNetwork::GetServerName ()
{
  return "srv";
}

const std::string
InternetNetwork::GetSgwPgwName ()
{
  return "pgw";
}

void
InternetNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_internetStats = 0;
  Object::DoDispose ();
}

void
InternetNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Creating the queue stats calculator for the Internet network
  m_internetStats = CreateObject<WebQueueStatsCalculator> ();
  
  Object::NotifyConstructionCompleted ();
}

};  // namespace ns3

