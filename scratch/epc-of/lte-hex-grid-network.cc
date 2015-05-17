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

#include "lte-hex-grid-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteHexGridNetwork");
NS_OBJECT_ENSURE_REGISTERED (LteHexGridNetwork);

LteHexGridNetwork::LteHexGridNetwork ()
{
  NS_LOG_FUNCTION (this);
  ConfigureLteParameters ();
}

LteHexGridNetwork::~LteHexGridNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
LteHexGridNetwork::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::LteHexGridNetwork") 
    .SetParent<Object> ()
    .AddAttribute ("Enbs", "The number of eNBs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteHexGridNetwork::m_nEnbs),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("EnbHeight", "The eNB antenna height [m].",
                   DoubleValue (30.0),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_enbHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UeHeight", "The UE antenna height [m].",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_ueHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("EnbDistance", "The distance [m] between nearby eNBs.",
                   DoubleValue (500),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_interSite),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UeFixedPos", "Fix all UEs close to its eNB.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&LteHexGridNetwork::m_fixedUes),
                   MakeBooleanChecker ())
    .AddAttribute ("MinX", "The x coordinate where the hex grid starts.",
                   DoubleValue (500.0),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_xMin),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinY", "The y coordinate where the hex grid starts.",
                   DoubleValue (250.0),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_yMin),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("GridWidth", "The number of eNBs in even rows. "
                   "Odd rows will have one additional eNB.",
                   UintegerValue (2),
                   MakeUintegerAccessor (&LteHexGridNetwork::m_gridWidth),
                   MakeUintegerChecker<uint32_t> ())
    ;
  return tid; 
}

void
LteHexGridNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_lteHelper = 0;
  m_epcHelper = 0;
  Object::DoDispose ();
}

NodeContainer
LteHexGridNetwork::GetEnbNodes ()
{
  return m_enbNodes;
}

NodeContainer
LteHexGridNetwork::GetUeNodes ()
{
  return m_ueNodes;
}

NetDeviceContainer
LteHexGridNetwork::GetUeDevices ()
{
  return m_ueDevices;
}

Ptr<LteHelper>
LteHexGridNetwork::CreateTopology (Ptr<EpcHelper> epcHelper, 
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
LteHexGridNetwork::GetLteHelper ()
{
  return m_lteHelper;
}

void
LteHexGridNetwork::EnableTraces (std::string prefix)
{
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename", 
    StringValue (prefix + "lte_dl_rlc.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename", 
    StringValue (prefix + "lte_ul_rlc.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename", 
    StringValue (prefix + "lte_dl_pdcp.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename", 
    StringValue (prefix + "lte_ul_pdcp.txt"));

  m_lteHelper->EnablePdcpTraces ();
  m_lteHelper->EnableRlcTraces ();
}

void
LteHexGridNetwork::ConfigureLteParameters ()
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
  
  //Disable Uplink Power Control
  Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl", 
                      BooleanValue (false));

  // Propagation model
  Config::SetDefault ("ns3::LteHelper::PathlossModel", 
                      StringValue ("ns3::OhBuildingsPropagationLossModel"));
  
  // Downlink scheduler
  Config::SetDefault ("ns3::LteHelper::Scheduler", 
                      StringValue ("ns3::CqaFfMacScheduler"));
  
  // Disabling error models
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", 
                      BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", 
                      BooleanValue (false));
}

void
LteHexGridNetwork::SetLteNodePositions ()
{
  NS_LOG_FUNCTION (this);

  // Compute eNBs positions
  const double xydfactor = std::sqrt (0.75);
  double yd = xydfactor * m_interSite;
  Ptr<ListPositionAllocator> listPosAllocator = CreateObject<ListPositionAllocator> ();
  std::vector<Vector> enbPosition;
  
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      uint32_t biRowIndex = (i / (m_gridWidth + m_gridWidth + 1));
      uint32_t biRowRemainder = i % (m_gridWidth + m_gridWidth + 1);
      uint32_t rowIndex = biRowIndex * 2;
      uint32_t colIndex = biRowRemainder; 
      if (biRowRemainder >= m_gridWidth)
        {
          ++rowIndex;
          colIndex -= m_gridWidth;
        }
      double y = m_yMin + yd * rowIndex;
      double x;
      if ((rowIndex % 2) == 0) 
	{
	  x = m_xMin + m_interSite * colIndex;
	}
      else
	{
	  x = m_xMin - (0.5 * m_interSite) + m_interSite * colIndex;
	}
      Vector pos (x, y, m_enbHeight);
      listPosAllocator->Add (pos);
      enbPosition.push_back (pos);
      NS_LOG_LOGIC ("eNB site " << i << " at " << pos);
    }

  // Set eNB initial positions
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.SetPositionAllocator (listPosAllocator);
  mobilityHelper.Install (m_enbNodes);
 
  // Set UE initial positions (same as its eNBs)
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      listPosAllocator = CreateObject<ListPositionAllocator> ();
      listPosAllocator->Add (enbPosition.at (i));
      mobilityHelper.SetPositionAllocator (listPosAllocator);
      mobilityHelper.Install (m_ueNodesPerEnb.at (i));
    }

  // Spreading UEs around the eNB, when necessary
  if (!m_fixedUes)
    {
      Ptr<RandomBoxPositionAllocator> boxPosAllocator;
      Ptr<RandomVariableStream> posX, posY, posZ;
      for (uint32_t i = 0; i < m_nEnbs; i++)
        {
          posX = CreateObjectWithAttributes<UniformRandomVariable> (
              "Min", DoubleValue (enbPosition.at (i).x - m_interSite * 0.5),
              "Max", DoubleValue (enbPosition.at (i).x + m_interSite * 0.5));
          posY = CreateObjectWithAttributes<UniformRandomVariable> (
              "Min", DoubleValue (enbPosition.at (i).y - m_interSite * 0.5),
              "Max", DoubleValue (enbPosition.at (i).y + m_interSite * 0.5));
          posZ = CreateObjectWithAttributes<ConstantRandomVariable> (
              "Constant", DoubleValue (m_ueHeight));
          
          boxPosAllocator = CreateObject<RandomBoxPositionAllocator> ();
          boxPosAllocator->SetAttribute ("X", PointerValue (posX));
          boxPosAllocator->SetAttribute ("Y", PointerValue (posY));
          boxPosAllocator->SetAttribute ("Z", PointerValue (posZ));
          mobilityHelper.SetPositionAllocator (boxPosAllocator);
          mobilityHelper.Install (m_ueNodesPerEnb.at (i));
        }
    } 
  
  // Buildings module
  BuildingsHelper::Install (m_enbNodes);
  BuildingsHelper::Install (m_ueNodes);
  BuildingsHelper::MakeMobilityModelConsistent ();

  for (size_t i = 0; i < m_enbNodes.GetN (); i++)
    {
      NS_LOG_DEBUG ("Enb " << i << " at " 
        << m_enbNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
    }

  for (size_t i = 0; i < m_ueNodes.GetN (); i++)
    {
      NS_LOG_DEBUG ("UE " << i << " at " 
        << m_ueNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
    }
}

void
LteHexGridNetwork::InstallProtocolStack ()
{
  NS_ASSERT (m_epcHelper != 0);
  NS_ASSERT (m_lteHelper != 0);

  // Installing LTE protocol stack on the eNBs | eNB <-> EPC connection
  m_enbDevices = m_lteHelper->InstallEnbDevice (m_enbNodes);   
  
  // For each eNB, installing LTE protocol stack on its UEs
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

