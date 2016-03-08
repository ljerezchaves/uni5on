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

  ConfigureLteDefaults ();
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
    .AddAttribute ("NumSites", "The total number of macro eNBs sites.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteHexGridNetwork::SetNumSites),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumUes", "The total number of UEs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&LteHexGridNetwork::m_nUes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("UeHeight", "The UE antenna height [m].",
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_ueHeight),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

void
LteHexGridNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_remHelper = 0;
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
LteHexGridNetwork::CreateTopology (Ptr<EpcHelper> epcHelper)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Topology with " << m_nSites << " macro eNBs sites.");
  NS_ASSERT (epcHelper != 0);
  
  m_lteHelper  = CreateObject<LteHelper> ();
  m_topoHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
  m_remHelper  = CreateObject<RadioEnvironmentMapHelper> ();

  m_topoHelper->SetAttribute ("InterSiteDistance", DoubleValue (500));
  m_topoHelper->SetAttribute ("MinX", DoubleValue (500));
  m_topoHelper->SetAttribute ("MinY", DoubleValue (250));
  m_topoHelper->SetAttribute ("GridWidth", UintegerValue (2));

   
  m_nEnbs = 3 * m_nSites;
  m_enbNodes.Create (m_nEnbs);
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      std::ostringstream enbName;
      enbName << "enb" << i;
      Names::Add (enbName.str (), m_enbNodes.Get (i));
    }
  m_epcHelper = epcHelper;

  // According to 3GPP R4-092042
  m_lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
  m_lteHelper->SetEnbAntennaModelAttribute ("Beamwidth", DoubleValue (70));
  m_lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation", DoubleValue (20.0));
  
  m_lteHelper->SetEpcHelper (m_epcHelper);
  
  m_topoHelper->SetLteHelper (m_lteHelper);

  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (m_enbNodes);
  BuildingsHelper::Install (m_enbNodes);
  
  m_enbDevices = m_topoHelper->SetPositionAndInstallEnbDevice (m_enbNodes);
  for (size_t i = 0; i < m_enbNodes.GetN (); i++)
    {
      NS_LOG_DEBUG (
        "Enb " << i << " at " <<
        m_enbNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
    }

  m_ueDevices = m_lteHelper->InstallUeDevice (m_ueNodes);
  InternetStackHelper internet;
  m_epcHelper->AssignUeIpv4Address (m_ueDevices);

  // Attaching UE to the eNBs using initial cell selection
  internet.Install (m_ueNodes);
  m_lteHelper->Attach (m_ueDevices);

  // BuildingsHelper::Install (m_ueNodes);



  for (size_t i = 0; i < m_ueNodes.GetN (); i++)
    {
      NS_LOG_DEBUG (
        "UE " << i << " at " <<
        m_ueNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
    }


  // m_nUesPerEnb = nUes;
  // m_enbNodes.Create (m_nEnbs);
  // for (uint32_t i = 0; i < m_nEnbs; i++)
  //   {
  //     std::ostringstream enbName;
  //     enbName << "enb" << i;
  //     Names::Add (enbName.str (), m_enbNodes.Get (i));
  //     NS_LOG_INFO (" eNB #" << i << " with " << m_nUesPerEnb.at (i) << " UEs");
  //     NodeContainer ueNc;
  //     ueNc.Create (m_nUesPerEnb.at (i));
  //     m_ueNodesPerEnb.push_back (ueNc);
  //     m_ueNodes.Add (ueNc);
  //   }

  // SetLteNodePositions ();
  // InstallProtocolStack ();

  BuildingsHelper::MakeMobilityModelConsistent ();
  
  
  return m_lteHelper;
}

Ptr<LteHelper>
LteHexGridNetwork::GetLteHelper ()
{
  return m_lteHelper;
}

void
LteHexGridNetwork::EnableTraces ()
{
  m_lteHelper->EnableTraces ();
}

void
LteHexGridNetwork::PrintRadioEnvironmentMap ()
{
  NS_LOG_FUNCTION (this);

  Ptr<LteEnbNetDevice> enbDevice =
    DynamicCast<LteEnbNetDevice> (m_enbDevices.Get (0));

  int id = enbDevice->GetPhy ()->GetDlSpectrumPhy ()->GetChannel ()->GetId ();
  std::ostringstream path;
  path << "/ChannelList/" << id;
  m_remHelper->SetAttribute ("ChannelPath", StringValue (path.str ()));

  UintegerValue earfcnValue;
  enbDevice->GetAttribute ("DlEarfcn", earfcnValue);
  m_remHelper->SetAttribute ("Earfcn", earfcnValue);

  UintegerValue dlBandwidthValue ;
  enbDevice->GetAttribute ("DlBandwidth", dlBandwidthValue);
  m_remHelper->SetAttribute ("Bandwidth", dlBandwidthValue);

  m_remHelper->SetAttribute ("XMax", DoubleValue (1500.0));
  m_remHelper->SetAttribute ("XRes", UintegerValue (500));

  m_remHelper->SetAttribute ("YMax", DoubleValue (1500.0));
  m_remHelper->SetAttribute ("YRes", UintegerValue (500));

  //m_remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
  //m_remHelper->SetAttribute ("RbId", IntegerValue (remRbId));


  m_remHelper->SetAttribute ("Z", DoubleValue (m_ueHeight));

  m_remHelper->Install ();
}

void
LteHexGridNetwork::SetNumSites (uint32_t sites)
{
  NS_LOG_FUNCTION (this << sites);
  m_nSites = sites;
  m_nUes = 3 * sites;
}


void
LteHexGridNetwork::ConfigureLteDefaults ()
{
  NS_LOG_FUNCTION (this);

  //
  // Increasing SrsPeriodicity to allow more UEs per eNB. Allowed values are:
  // {2, 5, 10, 20, 40, 80, 160, 320}. The default value (40) allows no more
  // than ~40 UEs for each eNB. Note that the value needs to be higher than the
  // actual number of UEs in your simulation program. This is due to the need
  // of accommodating some temporary user context for random access purposes
  // (the maximum number of UEs in a single eNB supported by ns-3 is ~320).
  // Note that for a 20MHz bandwidth channel (the largest one), the practical
  // number of active users supported is something like 200 UEs.
  // See http://tinyurl.com/pg9nfre for discussion.
  // ** Considering maximum value: 320
  //
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

  //
  // In the ns-3 LTE simulator, the channel bandwidth is set by the number of
  // RBs. The correlation table is:
  //    1.4 MHz —   6 PRBs
  //    3.0 MHz —  15 PRBs
  //    5.0 MHz —  25 PRBs
  //   10.0 MHz —  50 PRBs
  //   15.0 MHz —  75 PRBs
  //   20.0 MHz — 100 PRBs.
  // ** Considering downlink and uplink bandwidth: 100 RBs = 20Mhz.
  //
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth",
                      UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth",
                      UintegerValue (100));

  //
  // LTE supports a wide range of different frequency bands. In Brazil, current
  // band in use is #7 (@2600MHz). This is a high-frequency band, with reduced
  // coverage. This configuration is normally used only in urban areas, with a
  // high number of cells with reduced radius, lower eNB TX power and small
  // channel bandwidth. For simulations, we are using the reference band #1.
  // See http://niviuk.free.fr/lte_band.php for LTE frequency bands and Earfcn
  // calculation.
  // ** Considering Band #1 @2110/1920 MHz (FDD)
  //
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (0));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18000));

  //
  // We are configuring the eNB transmission power as a macro cell (46 dBm is
  // the maximum used value for the eNB for 20MHz channel). The max power that
  // the UE is allowed to use is set by the standard (23dBm). We are currently
  // using no power control.
  // See http://tinyurl.com/nlh6u3t and http://tinyurl.com/nlh6u3t
  //
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23));
  Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl",
                      BooleanValue (false));

  //
  // Using the UE MIMO transmission diversity (Mode 2 with 4.2bB antenna gain).
  //
  Config::SetDefault ("ns3::LteEnbRrc::DefaultTransmissionMode",
                      UintegerValue (1));

  //
  // Using the Channel and QoS Aware (CQA) Scheduler as the LTE MAC downlink
  // scheduling algorithm, which considers the head of line delay, the GBR
  // parameters and channel quality over different subbands.
  //
  Config::SetDefault ("ns3::LteHelper::Scheduler",
                      StringValue ("ns3::CqaFfMacScheduler"));

  //
  // Disabling the frequency reuse algorithms.
  //
  Config::SetDefault ("ns3::LteHelper::FfrAlgorithm",
                      StringValue ("ns3::LteFrNoOpAlgorithm"));

  //
  // Disabling handover algorithms.
  //
  Config::SetDefault ("ns3::LteHelper::HandoverAlgorithm",
                      StringValue ("ns3::NoOpHandoverAlgorithm"));

  //
  // Using the hybrid model obtained through a combination of several well
  // known pathloss models in order to mimic different environmental scenarios,
  // considering the phenomenon of indoor/outdoor propagation in the presence
  // of buildings. Configuring pathloss internal attributes too, using always
  // LoS model.
  //
  Config::SetDefault ("ns3::LteHelper::PathlossModel",
      StringValue ("ns3::HybridBuildingsPropagationLossModel"));
  Config::SetDefault ("ns3::BuildingsPropagationLossModel::ShadowSigmaOutdoor",
                      DoubleValue (1.5));
  Config::SetDefault ("ns3::BuildingsPropagationLossModel::ShadowSigmaIndoor",
                      DoubleValue (1.5));
  Config::SetDefault ("ns3::BuildingsPropagationLossModel::ShadowSigmaExtWalls",
                      DoubleValue (0));
  Config::SetDefault ("ns3::HybridBuildingsPropagationLossModel::Los2NlosThr",
                      DoubleValue (1e6));

  //
  // Disabling error models for both control and data planes.
  //
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                      BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                      BooleanValue (false));
}

void
LteHexGridNetwork::SetLteNodePositions ()
{
//   NS_LOG_FUNCTION (this);
// 
//   // Compute eNBs positions
//   const double xydfactor = std::sqrt (0.75);
//   double yd = xydfactor * m_interSite;
//   Ptr<ListPositionAllocator> listPosAllocator =
//     CreateObject<ListPositionAllocator> ();
//   std::vector<Vector> enbPosition;
// 
//   for (uint32_t i = 0; i < m_nEnbs; i++)
//     {
//       uint32_t biRowIndex = (i / (m_gridWidth + m_gridWidth + 1));
//       uint32_t biRowRemainder = i % (m_gridWidth + m_gridWidth + 1);
//       uint32_t rowIndex = biRowIndex * 2;
//       uint32_t colIndex = biRowRemainder;
//       if (biRowRemainder >= m_gridWidth)
//         {
//           ++rowIndex;
//           colIndex -= m_gridWidth;
//         }
//       double y = m_yMin + yd * rowIndex;
//       double x;
//       if ((rowIndex % 2) == 0)
//         {
//           x = m_xMin + m_interSite * colIndex;
//         }
//       else
//         {
//           x = m_xMin - (0.5 * m_interSite) + m_interSite * colIndex;
//         }
//       Vector pos (x, y, m_enbHeight);
//       listPosAllocator->Add (pos);
//       enbPosition.push_back (pos);
//       NS_LOG_LOGIC ("eNB site " << i << " at " << pos);
//     }
// 
//   // Set eNB initial positions
//   MobilityHelper mobilityHelper;
//   mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//   mobilityHelper.SetPositionAllocator (listPosAllocator);
//   mobilityHelper.Install (m_enbNodes);
// 
//   // Set UE initial positions (same as its eNBs)
//   for (uint32_t i = 0; i < m_nEnbs; i++)
//     {
//       listPosAllocator = CreateObject<ListPositionAllocator> ();
//       listPosAllocator->Add (enbPosition.at (i));
//       mobilityHelper.SetPositionAllocator (listPosAllocator);
//       mobilityHelper.Install (m_ueNodesPerEnb.at (i));
//     }
// 
//   // Spreading UEs around the eNB, when necessary
//   if (!m_fixedUes)
//     {
//       Ptr<RandomBoxPositionAllocator> boxPosAllocator;
//       Ptr<RandomVariableStream> posX, posY, posZ;
//       for (uint32_t i = 0; i < m_nEnbs; i++)
//         {
//           posX = CreateObjectWithAttributes<UniformRandomVariable> (
//               "Min", DoubleValue (enbPosition.at (i).x - m_interSite * 0.5),
//               "Max", DoubleValue (enbPosition.at (i).x + m_interSite * 0.5));
//           posY = CreateObjectWithAttributes<UniformRandomVariable> (
//               "Min", DoubleValue (enbPosition.at (i).y - m_interSite * 0.5),
//               "Max", DoubleValue (enbPosition.at (i).y + m_interSite * 0.5));
//           posZ = CreateObjectWithAttributes<ConstantRandomVariable> (
//               "Constant", DoubleValue (m_ueHeight));
// 
//           boxPosAllocator = CreateObject<RandomBoxPositionAllocator> ();
//           boxPosAllocator->SetAttribute ("X", PointerValue (posX));
//           boxPosAllocator->SetAttribute ("Y", PointerValue (posY));
//           boxPosAllocator->SetAttribute ("Z", PointerValue (posZ));
//           mobilityHelper.SetPositionAllocator (boxPosAllocator);
//           mobilityHelper.Install (m_ueNodesPerEnb.at (i));
//         }
//     }
// 
//   // Buildings module
//   BuildingsHelper::Install (m_enbNodes);
//   BuildingsHelper::Install (m_ueNodes);
//   BuildingsHelper::MakeMobilityModelConsistent ();
// 
//   for (size_t i = 0; i < m_enbNodes.GetN (); i++)
//     {
//       NS_LOG_DEBUG (
//         "Enb " << i << " at " <<
//         m_enbNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
//     }
// 
//   for (size_t i = 0; i < m_ueNodes.GetN (); i++)
//     {
//       NS_LOG_DEBUG (
//         "UE " << i << " at " <<
//         m_ueNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ());
//     }
}

void
LteHexGridNetwork::InstallProtocolStack ()
{
//   NS_ASSERT (m_epcHelper != 0);
//   NS_ASSERT (m_lteHelper != 0);
// 
//   // Installing LTE protocol stack on the eNBs | eNB <-> EPC connection
//   m_enbDevices = m_lteHelper->InstallEnbDevice (m_enbNodes);
// 
//   // For each eNB, installing LTE protocol stack on its UEs
//   InternetStackHelper internet;
//   for (uint32_t i = 0; i < m_nEnbs; i++)
//     {
//       NodeContainer ueNc = m_ueNodesPerEnb.at (i);
//       NetDeviceContainer ueDev = m_lteHelper->InstallUeDevice (ueNc);
//       m_ueDevices.Add (ueDev);
//       internet.Install (ueNc);
//       m_epcHelper->AssignUeIpv4Address (ueDev);
// 
//       // Specifying static routes for each UE (default gateway)
//       Ipv4StaticRoutingHelper ipv4RoutingHelper;
//       for (uint32_t j = 0; j < m_nUesPerEnb.at (i); j++)
//         {
//           Ptr<Node> n = ueNc.Get (j);
//           std::ostringstream ueName;
//           ueName << "ue" << j << "@enb" << i;
//           Names::Add (ueName.str (), n);
//           Ptr<Ipv4StaticRouting> ueStaticRouting =
//             ipv4RoutingHelper.GetStaticRouting (n->GetObject<Ipv4> ());
//           ueStaticRouting->SetDefaultRoute (
//             m_epcHelper->GetUeDefaultGatewayAddress (), 1);
//         }
// 
//       // Attaching UEs to the respective eNB
//       // (this activates the default EPS bearer)
//       m_lteHelper->Attach (ueDev, m_enbDevices.Get (i));
//     }
}

};  // namespace ns3

