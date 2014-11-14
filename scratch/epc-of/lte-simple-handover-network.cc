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

#include "lte-simple-handover-network.h"

NS_LOG_COMPONENT_DEFINE ("LteSimpleHandoverNetwork");

namespace ns3 {

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}


NS_OBJECT_ENSURE_REGISTERED (LteSimpleHandoverNetwork)
  ;


LteSimpleHandoverNetwork::LteSimpleHandoverNetwork ()
  : m_nEnbs (2),
    m_nUes (1)
{
  NS_LOG_FUNCTION (this);
}


LteSimpleHandoverNetwork::~LteSimpleHandoverNetwork ()
{
  NS_LOG_FUNCTION (this);
}


TypeId 
LteSimpleHandoverNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::LteSimpleHandoverNetwork") 
    .SetParent<Object> ()
    .AddAttribute ("EnbDistance", 
                   "The distance between two eNBs",
                   DoubleValue (100.0),
                   MakeDoubleAccessor (&LteSimpleHandoverNetwork::m_enbDistance),
                   MakeDoubleChecker<double> ())
    ;
  return tid; 
}


void
LteSimpleHandoverNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_lteHelper = 0;
  m_epcHelper = 0;
  Object::DoDispose ();
}


NodeContainer
LteSimpleHandoverNetwork::GetEnbNodes ()
{
  return m_enbNodes;
}


NodeContainer
LteSimpleHandoverNetwork::GetUeNodes ()
{
  return m_ueNodes;
}


void
LteSimpleHandoverNetwork::EnableTraces ()
{
  m_lteHelper->EnableTraces ();
}


void
LteSimpleHandoverNetwork::CreateTopology (Ptr<EpcHelper> epcHelper)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (epcHelper != 0);

  m_epcHelper = epcHelper;
  m_lteHelper = CreateObject<LteHelper> ();
  m_lteHelper->SetEpcHelper (m_epcHelper);

  m_enbNodes.Create (m_nEnbs);
  m_ueNodes.Create (m_nUes);

  Names::Add ("Enb0", m_enbNodes.Get (0));
  Names::Add ("Enb1", m_enbNodes.Get (1));
  Names::Add ("UE",   m_ueNodes.Get (0));
  
  SetLteNodePositions ();
  InstallProtocolStack ();
  SetHandoverConfiguration ();
}


Ptr<LteHelper>
LteSimpleHandoverNetwork::GetLteHelper ()
{
  return m_lteHelper;
}

NetDeviceContainer
LteSimpleHandoverNetwork::GetUeDevices ()
{
  return m_ueDevices;
}


void
LteSimpleHandoverNetwork::SetLteNodePositions ()
{
  NS_LOG_FUNCTION (this);
  
  Ptr<ListPositionAllocator> positionAlloc = 
      CreateObject<ListPositionAllocator> ();
  MobilityHelper mobility;
  
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // eNBs positions
  positionAlloc->Add (Vector (  0, 0, 0));
  positionAlloc->Add (Vector (200, 0, 0));
  
  // UE position
  positionAlloc->Add (Vector (100, 0, 0));
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (m_enbNodes);
  mobility.Install (m_ueNodes);
}


void
LteSimpleHandoverNetwork::InstallProtocolStack ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_epcHelper != 0);
  NS_ASSERT (m_lteHelper != 0);

  // Installing LTE protocol stack on the eNBs
  m_enbDevices = m_lteHelper->InstallEnbDevice (m_enbNodes);  // eNB <--> EPC connection
  
  // Installing LTE protocol stack on the UE
  m_ueDevices = m_lteHelper->InstallUeDevice (m_ueNodes);
      
  InternetStackHelper internet;
  internet.Install (m_ueNodes);
  m_epcHelper->AssignUeIpv4Address (m_ueDevices);

  // Specifying static routes for UE (default gateway)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Node> ue = m_ueNodes.Get (0);
  Ptr<Ipv4StaticRouting> ueStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ()); 
  ueStaticRouting->SetDefaultRoute (m_epcHelper->GetUeDefaultGatewayAddress (), 1); 
 
  // Attaching UE to the first eNB (this activates the default EPS bearer)
  m_lteHelper->Attach (m_ueDevices.Get (0), m_enbDevices.Get (0));
}

void
LteSimpleHandoverNetwork::SetHandoverConfiguration ()
{
  NS_LOG_FUNCTION (this);

  // Add X2 inteface
  m_lteHelper->AddX2Interface (m_enbNodes);

  // X2-based Handover
  m_lteHelper->HandoverRequest (Seconds (2.00), m_ueDevices.Get (0), 
      m_enbDevices.Get (0), m_enbDevices.Get (1));

  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));
}


};  // namespace ns3

