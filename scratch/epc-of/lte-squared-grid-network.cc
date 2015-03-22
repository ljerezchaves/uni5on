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

#include "lte-squared-grid-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteSquaredGridNetwork");
NS_OBJECT_ENSURE_REGISTERED (LteSquaredGridNetwork);

LteSquaredGridNetwork::LteSquaredGridNetwork ()
{
  NS_LOG_FUNCTION (this);
  ConfigureLteParameters ();
}

LteSquaredGridNetwork::~LteSquaredGridNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
LteSquaredGridNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::LteSquaredGridNetwork") 
    .SetParent<Object> ()
    .AddAttribute ("Enbs", 
                   "The number of eNBs in LTE Squared Grid Network",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteSquaredGridNetwork::m_nEnbs),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("EnbHeight", 
                   "The eNB antenna height in LTE Squared Grid Network",
                   DoubleValue (30.0),
                   MakeDoubleAccessor (&LteSquaredGridNetwork::m_enbHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UeHeight", 
                   "The UE antenna height in LTE Squared Grid Network",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&LteSquaredGridNetwork::m_ueHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RoomLength", 
                   "The room length of each grid in LTE Squared Grid Network",
                   DoubleValue (500.0),
                   MakeDoubleAccessor (&LteSquaredGridNetwork::m_roomLength),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UeFixedPos", 
                   "Fix all UEs close to its eNB, avoiding random positions.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&LteSquaredGridNetwork::m_fixedUes),
                   MakeBooleanChecker ())
    ;
  return tid; 
}

void
LteSquaredGridNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_lteHelper = 0;
  m_epcHelper = 0;
  Object::DoDispose ();
}

NodeContainer
LteSquaredGridNetwork::GetEnbNodes ()
{
  return m_enbNodes;
}

NodeContainer
LteSquaredGridNetwork::GetUeNodes ()
{
  return m_ueNodes;
}

NetDeviceContainer
LteSquaredGridNetwork::GetUeDevices ()
{
  return m_ueDevices;
}

Ptr<LteHelper>
LteSquaredGridNetwork::CreateTopology (Ptr<EpcHelper> epcHelper, 
    std::vector<uint32_t> nUes)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Topology with " << m_nEnbs << " eNBs");
  NS_ASSERT (epcHelper != 0);

  m_epcHelper = epcHelper;
  m_lteHelper = CreateObject<LteHelper> ();
  m_lteHelper->SetEpcHelper (m_epcHelper);

  m_nUesPerEnb = nUes;
  m_enbNodes.Create (m_nEnbs);
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      std::ostringstream enbName;
      enbName << "enb" << i;
      Names::Add (enbName.str(), m_enbNodes.Get (i));
      NS_LOG_INFO (" eNB #" << i << " with " << m_nUesPerEnb.at (i) << " UEs");
      NodeContainer ueNc;
      ueNc.Create (m_nUesPerEnb.at (i));
      m_ueNodesPerEnb.push_back (ueNc);
      m_ueNodes.Add (ueNc);
    }

  SetLteNodePositions ();
  InstallProtocolStack ();
  return m_lteHelper;
}

Ptr<LteHelper>
LteSquaredGridNetwork::GetLteHelper ()
{
  return m_lteHelper;
}

void
LteSquaredGridNetwork::EnableTraces ()
{
  m_lteHelper->EnableTraces ();
}

void
LteSquaredGridNetwork::ConfigureLteParameters ()
{
  // Increasing SrsPeriodicity to allow more UEs per eNB.
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

  // Downlink and uplink bandwidth: 100 RBs = 20Mhz
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (100));

  // Considering Band #1 @2110/1920 MHz (FDD)
  // http://niviuk.free.fr/lte_band.php
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (0));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18000));
 
  // Transmission power (eNB as macro cell)
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (18));

  Config::SetDefault ("ns3::LteHelper::PathlossModel", 
                      StringValue ("ns3::OhBuildingsPropagationLossModel"));
  Config::SetDefault ("ns3::LteHelper::Scheduler", 
                      StringValue ("ns3::CqaFfMacScheduler"));
}

void
LteSquaredGridNetwork::SetLteNodePositions ()
{
  NS_LOG_FUNCTION (this);

  std::vector<Vector> enbPosition;

  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // eNBs positions
  uint32_t plantedEnb = 0;
  uint32_t nRooms = std::ceil (std::sqrt (m_nEnbs));
  Ptr<ListPositionAllocator> enbPosAllocator = 
      CreateObject<ListPositionAllocator> ();
  for (uint32_t row = 0; row < nRooms; row++)
    {
      for (uint32_t column = 0; column < nRooms && plantedEnb < m_nEnbs; 
          column++, plantedEnb++)
        {
          Vector pos (m_roomLength * (column + 0.5), 
              m_roomLength * (row + 0.5), m_enbHeight);
          enbPosAllocator->Add (pos);
          enbPosition.push_back (pos);
        }
    }
  mobilityHelper.SetPositionAllocator (enbPosAllocator);
  mobilityHelper.Install (m_enbNodes);

  // UEs positions for each eNB
  Ptr<RandomBoxPositionAllocator> uePosAllocator;
  Ptr<RandomVariableStream> posX, posY, posZ;
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      if (m_fixedUes)
        {
          posX = CreateObjectWithAttributes<ConstantRandomVariable> (
              "Constant", DoubleValue (enbPosition.at (i).x)); 
          posY = CreateObjectWithAttributes<ConstantRandomVariable> (
              "Constant", DoubleValue (enbPosition.at (i).y)); 
          posZ = CreateObjectWithAttributes<ConstantRandomVariable> (
              "Constant", DoubleValue (m_ueHeight)); 
        }
      else
        {
          posX = CreateObjectWithAttributes<UniformRandomVariable> (
              "Min", DoubleValue (enbPosition.at (i).x - m_roomLength * 0.5),
              "Max", DoubleValue (enbPosition.at (i).x + m_roomLength * 0.5));
          posY = CreateObjectWithAttributes<UniformRandomVariable> (
              "Min", DoubleValue (enbPosition.at (i).y - m_roomLength * 0.5),
              "Max", DoubleValue (enbPosition.at (i).y + m_roomLength * 0.5));
          posZ = CreateObjectWithAttributes<ConstantRandomVariable> (
              "Constant", DoubleValue (m_ueHeight));
        }
      uePosAllocator = CreateObjectWithAttributes<RandomBoxPositionAllocator> (
          "X", PointerValue (posX),
          "Y", PointerValue (posY),
          "Z", PointerValue (posZ));
      mobilityHelper.SetPositionAllocator (uePosAllocator);
      mobilityHelper.Install (m_ueNodesPerEnb.at (i));
    }
}

void
LteSquaredGridNetwork::InstallProtocolStack ()
{
  NS_ASSERT (m_epcHelper != 0);
  NS_ASSERT (m_lteHelper != 0);

  // Installing LTE protocol stack on the eNBs | eNB <-> EPC connection
  m_enbDevices = m_lteHelper->InstallEnbDevice (m_enbNodes);   
  
  // For each eNB, installing LTE protocol stack on the UEs
  InternetStackHelper internet;
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      NodeContainer ueNc = m_ueNodesPerEnb.at (i);
      NetDeviceContainer ueDev = m_lteHelper->InstallUeDevice (ueNc);
      m_ueDevices.Add (ueDev);
      internet.Install (ueNc);
      m_epcHelper->AssignUeIpv4Address (ueDev);

      // Specifying static routes for each UE (default gateway)
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t j = 0; j < m_nUesPerEnb.at (i); j++)
        {
          Ptr<Node> n = ueNc.Get (j);
          std::ostringstream ueName;
          ueName << "ue" << j << "@enb" << i;
          Names::Add (ueName.str(), n);
          Ptr<Ipv4StaticRouting> ueStaticRouting =
              ipv4RoutingHelper.GetStaticRouting (n->GetObject<Ipv4> ()); 
          ueStaticRouting->SetDefaultRoute (
              m_epcHelper->GetUeDefaultGatewayAddress (), 1); 
        }
 
      // Attaching UEs to the respective eNB (this activates the default EPS bearer)
      m_lteHelper->Attach (ueDev, m_enbDevices.Get (i));
    }
}

};  // namespace ns3

