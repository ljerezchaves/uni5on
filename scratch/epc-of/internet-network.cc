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
                   "The data rate to be used for the Internet PointToPoint link",
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&InternetNetwork::m_LinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The delay to be used for the Internet PointToPoint link",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&InternetNetwork::m_LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("LinkMtu", 
                   "The MTU of the Internet PointToPoint link",
                   UintegerValue (1492),  // PPPoE MTU 
                   MakeUintegerAccessor (&InternetNetwork::m_LinkMtu),
                   MakeUintegerChecker<uint16_t> ())
    ;
  return tid; 
}

Ptr<Node>
InternetNetwork::CreateTopology (Ptr<Node> pgw)
{
  NS_LOG_FUNCTION (this << pgw);
  
  // Creating a single web node and connecting it to the EPC pgw over a
  // PointToPoint link.
  Ptr<Node> web = CreateObject<Node> ();
  InternetStackHelper internet;
  internet.Install (web);
  
  Names::Add (InternetNetwork::GetServerName (), web);

  m_webNodes.Add (pgw);
  m_webNodes.Add (web);

  m_p2pHeler.SetDeviceAttribute ("DataRate", DataRateValue (m_LinkDataRate));
  m_p2pHeler.SetDeviceAttribute ("Mtu", UintegerValue (m_LinkMtu));
  m_p2pHeler.SetChannelAttribute ("Delay", TimeValue (m_LinkDelay));
  m_p2pHeler.SetQueue ("ns3::CoDelQueue");
  
  m_webDevices = m_p2pHeler.Install (m_webNodes);
  Ptr<PointToPointNetDevice> webDev, pgwDev;
  pgwDev = DynamicCast<PointToPointNetDevice> (m_webDevices.Get (0));
  webDev = DynamicCast<PointToPointNetDevice> (m_webDevices.Get (1));
 
  Names::Add (Names::FindName (pgw) + "+" + Names::FindName (web), pgwDev);
  Names::Add (Names::FindName (web) + "+" + Names::FindName (pgw), webDev);
  
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("192.168.0.0", "255.255.255.0");
  ipv4h.Assign (m_webDevices);

  // Defining static routes to the web node
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> webHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (web->GetObject<Ipv4> ());
  webHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), 
      Ipv4Mask ("255.0.0.0"), Ipv4Address("192.168.0.1"), 1);

  // Registering trace sink for queue drop packets
  pgwDev->GetQueue ()->TraceConnect ("Drop", Names::FindName (pgw),
    MakeCallback (&InternetNetwork::QueueDropPacket, this));
  webDev->GetQueue ()->TraceConnect ("Drop", Names::FindName (web),
    MakeCallback (&InternetNetwork::QueueDropPacket, this));

  return web;
}

void
InternetNetwork::EnablePcap (std::string prefix)
{
  NS_LOG_FUNCTION (this);
  m_p2pHeler.EnablePcap (prefix, m_webDevices);
}

void
InternetNetwork::QueueDropPacket (std::string context, 
                                  Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet->GetUid ());
  // TODO Report dropped packets.
}

void
InternetNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Object::DoDispose ();
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

};  // namespace ns3

