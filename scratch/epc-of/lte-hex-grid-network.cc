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

LteHexGridNetwork::LteHexGridNetwork (Ptr<EpcHelper> epcHelper)
  : m_epcHelper (epcHelper)
{
  NS_LOG_FUNCTION (this);

  // Adjust filenames for LTE trace files before creating the network.
  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();

  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename",
                      StringValue (prefix + "dl_rlc_lte.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename",
                      StringValue (prefix + "ul_rlc_lte.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename",
                      StringValue (prefix + "dl_pdcp_lte.txt"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename",
                      StringValue (prefix + "ul_pdcp_lte.txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::DlOutputFilename",
                      StringValue (prefix + "dl_mac_lte.txt"));
  Config::SetDefault ("ns3::MacStatsCalculator::UlOutputFilename",
                      StringValue (prefix + "ul_mac_lte.txt"));
  Config::SetDefault ("ns3::PhyStatsCalculator::DlRsrpSinrFilename",
                      StringValue (prefix + "dl_rsrp_sinr_lte.txt"));
  Config::SetDefault ("ns3::PhyStatsCalculator::UlSinrFilename",
                      StringValue (prefix + "ul_sinr_lte.txt"));
  Config::SetDefault ("ns3::PhyStatsCalculator::UlInterferenceFilename",
                      StringValue (prefix + "ul_interference_lte.txt"));
  Config::SetDefault ("ns3::PhyRxStatsCalculator::DlRxOutputFilename",
                      StringValue (prefix + "dl_rx_phy_lte.txt"));
  Config::SetDefault ("ns3::PhyRxStatsCalculator::UlRxOutputFilename",
                      StringValue (prefix + "ul_rx_phy_lte.txt"));
  Config::SetDefault ("ns3::PhyTxStatsCalculator::DlTxOutputFilename",
                      StringValue (prefix + "dl_tx_phy_lte.txt"));
  Config::SetDefault ("ns3::PhyTxStatsCalculator::UlTxOutputFilename",
                      StringValue (prefix + "ul_tx_phy_lte.txt"));
}

LteHexGridNetwork::LteHexGridNetwork ()
{
  NS_LOG_FUNCTION (this);
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
    .AddAttribute ("EnbMargin", "How much the eNB coverate area extends, "
                   "expressed as fraction of the inter-site distance.",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&LteHexGridNetwork::m_enbMargin),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("UeMobility", "Enable UE random mobility.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LteHexGridNetwork::m_ueMobility),
                   MakeBooleanChecker ())
    .AddAttribute ("PrintRem",
                   "Print the radio environment map.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LteHexGridNetwork::m_lteRem),
                   MakeBooleanChecker ())
    .AddAttribute ("LteTrace",
                   "Enable/Disable simulation LTE ASCII traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LteHexGridNetwork::m_lteTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("RemFilename",
                   "Filename for the radio enviroment map.",
                   StringValue ("rem_plot.txt"),
                   MakeStringAccessor (&LteHexGridNetwork::m_remFilename),
                   MakeStringChecker ())
    .AddAttribute ("BuildingsFilename",
                   "Filename for buildings positions.",
                   StringValue ("bld_plot.txt"),
                   MakeStringAccessor (&LteHexGridNetwork::m_bldsFilename),
                   MakeStringChecker ())
    .AddAttribute ("UesFilename",
                   "Filename for UE positions.",
                   StringValue ("ues_plot.txt"),
                   MakeStringAccessor (&LteHexGridNetwork::m_uesFilename),
                   MakeStringChecker ())
    .AddAttribute ("EnbsFilename",
                   "Filename for eNB positions.",
                   StringValue ("enb_plot.txt"),
                   MakeStringAccessor (&LteHexGridNetwork::m_enbsFilename),
                   MakeStringChecker ())
  ;
  return tid;
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
LteHexGridNetwork::GetLteHelper ()
{
  return m_lteHelper;
}

void
LteHexGridNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_topoHelper = 0;
  m_remHelper = 0;
  m_lteHelper = 0;
  m_epcHelper = 0;
  Object::DoDispose ();
}

void
LteHexGridNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Create LTE topology
  CreateTopology ();
  
  // Chain up
  Object::NotifyConstructionCompleted ();
}

void
LteHexGridNetwork::CreateTopology ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Topology with " << m_nSites << " macro eNBs sites.");

  // Creating the nodes for eNBs and UEs and set their names
  m_enbNodes.Create (m_nEnbs);
  for (uint32_t i = 0; i < m_nEnbs; i++)
    {
      std::ostringstream enbName;
      enbName << "enb" << i;
      Names::Add (enbName.str (), m_enbNodes.Get (i));
    }

  m_ueNodes.Create (m_nUes);
  for (uint32_t i = 0; i < m_nUes; i++)
    {
      std::ostringstream ueName;
      ueName << "ue" << i;
      Names::Add (ueName.str (), m_ueNodes.Get (i));
    }

  // Create the LTE helper for the radio network
  m_lteHelper = CreateObject<LteHelper> ();
  m_lteHelper->SetEpcHelper (m_epcHelper);

  // Use the hybrid pathloss model obtained through a combination of several
  // well known pathloss models in order to mimic different environmental
  // scenarios, considering the phenomenon of indoor/outdoor propagation in the
  // presence of buildings. Always use the LoS pathloss model.
  m_lteHelper->SetAttribute (
    "PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
  m_lteHelper->SetPathlossModelAttribute (
    "ShadowSigmaExtWalls", DoubleValue (0));
  m_lteHelper->SetPathlossModelAttribute (
    "ShadowSigmaOutdoor", DoubleValue (1.5));
  m_lteHelper->SetPathlossModelAttribute (
    "ShadowSigmaIndoor", DoubleValue (1.5));
  m_lteHelper->SetPathlossModelAttribute (
    "Los2NlosThr", DoubleValue (1e6));

  // Configuring the anntenas for the hexagonal grid topology
  m_lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
  m_lteHelper->SetEnbAntennaModelAttribute ("Beamwidth", DoubleValue (70));
  m_lteHelper->SetEnbAntennaModelAttribute (
    "MaxAttenuation", DoubleValue (20.0));

  // Create the topology helper used to group eNBs in three-sector sites layed
  // out on an hexagonal grid
  m_topoHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
  m_topoHelper->SetLteHelper (m_lteHelper);

  // Set the constant mobility model for eNB and UE positioning
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (m_enbNodes);

  // Position the nodes on a hex grid and install the corresponding
  // EnbNetDevices with antenna boresight configured properly.
  m_enbDevices = m_topoHelper->SetPositionAndInstallEnbDevice (m_enbNodes);

  // Create an X2 interface between all the eNBs in a given set.
  // FIXME m_lteHelper->AddX2Interface (macroEnbs);

  // After eNB positioning, identify the LTE radio coverage and spread the UEs
  // over the coverage area.
  m_coverageArea = IdentifyEnbsCoverageArea ();
  Ptr<RandomVariableStream> posX, posY, posZ;
  posX = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (m_coverageArea.xMin),
      "Max", DoubleValue (m_coverageArea.xMax));
  posY = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (m_coverageArea.yMin),
      "Max", DoubleValue (m_coverageArea.yMax));
  posZ = CreateObjectWithAttributes<ConstantRandomVariable> (
      "Constant", DoubleValue (m_ueHeight));

  Ptr<RandomBoxPositionAllocator> boxPosAllocator;
  boxPosAllocator = CreateObject<RandomBoxPositionAllocator> ();
  boxPosAllocator->SetAttribute ("X", PointerValue (posX));
  boxPosAllocator->SetAttribute ("Y", PointerValue (posY));
  boxPosAllocator->SetAttribute ("Z", PointerValue (posZ));
  mobilityHelper.SetPositionAllocator (boxPosAllocator);
  mobilityHelper.Install (m_ueNodes);

  // Install protocol stack into UE nodes
  m_ueDevices = m_lteHelper->InstallUeDevice (m_ueNodes);

  // Install TCP/IP protocol stack into UE nodes
  InternetStackHelper internet;
  internet.Install (m_ueNodes);
  m_epcHelper->AssignUeIpv4Address (m_ueDevices);

  // Specifying static routes for each UE to the default gateway
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t i = 0; i < m_nUes; i++)
    {
      Ptr<Node> ueNode = m_ueNodes.Get (i);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (
        m_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attaching UE to the eNBs using initial cell selection
  m_lteHelper->Attach (m_ueDevices);

  // Install the MobilityBuildingInfo into LTE nodes
  BuildingsHelper::Install (m_enbNodes);
  BuildingsHelper::Install (m_ueNodes);
  BuildingsHelper::MakeMobilityModelConsistent ();

  // If enable, print LTE radio environment map with node positions.
  if (m_lteRem)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();

      // Forcing initialization so we don't have to wait for Nodes to start
      // before positions are assigned (which is needed to output node
      // positions to file)
      NodeContainer::Iterator it;
      for (it = m_ueNodes.Begin (); it != m_ueNodes.End (); it++)
        {
          (*it)->Initialize ();
        }

      PrintBuildingListToFile (prefix + m_bldsFilename);
      PrintUeListToFile (prefix + m_uesFilename);
      PrintEnbListToFile (prefix + m_enbsFilename);
      PrintRadioEnvironmentMap (prefix + m_remFilename);
    }

  // If enable, print LTE ASCII traces
  if (m_lteTrace)
    {
      m_lteHelper->EnableTraces ();
    }
}

void
LteHexGridNetwork::SetNumSites (uint32_t sites)
{
  NS_LOG_FUNCTION (this << sites);

  m_nSites = sites;
  m_nEnbs = 3 * sites;
}

Rectangle
LteHexGridNetwork::IdentifyEnbsCoverageArea ()
{
  NS_LOG_FUNCTION (this);

  double xMin = std::numeric_limits<double>::max ();
  double yMin = std::numeric_limits<double>::max ();
  double xMax = std::numeric_limits<double>::min ();
  double yMax = std::numeric_limits<double>::min ();

  // Iterate over all eNBs checking for node positions.
  NodeList::Iterator it;
  for (it = m_enbNodes.Begin (); it != m_enbNodes.End (); it++)
    {
      Ptr<Node> node = *it;
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
      if (pos.x < xMin)
        {
          xMin = pos.x;
        }
      if (pos.x > xMax)
        {
          xMax = pos.x;
        }
      if (pos.y < yMin)
        {
          yMin = pos.y;
        }
      if (pos.y > yMax)
        {
          yMax = pos.y;
        }
    }

  DoubleValue doubleValue;
  m_topoHelper->GetAttribute ("InterSiteDistance", doubleValue);
  uint32_t adjust = m_enbMargin * doubleValue.Get ();
  Rectangle coverageArea (round (xMin - adjust), round (xMax + adjust),
                          round (yMin - adjust), round (yMax + adjust));
  
  NS_LOG_INFO ("Coverage area: " << coverageArea);
  return coverageArea;
}

void
LteHexGridNetwork::PrintRadioEnvironmentMap (std::string filename)
{
  NS_LOG_FUNCTION (this);

  // Create the radio environment map helper and set output filename
  m_remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  m_remHelper->SetAttribute ("OutputFile", StringValue (filename));

  // Adjust LTE radio channel ID
  Ptr<LteEnbNetDevice> enbDevice =
    DynamicCast<LteEnbNetDevice> (m_enbDevices.Get (0));
  int id = enbDevice->GetPhy ()->GetDlSpectrumPhy ()->GetChannel ()->GetId ();
  std::ostringstream path;
  path << "/ChannelList/" << id;
  m_remHelper->SetAttribute ("ChannelPath", StringValue (path.str ()));

  // Adjust the channel frequency and bandwidth
  UintegerValue earfcnValue;
  enbDevice->GetAttribute ("DlEarfcn", earfcnValue);
  m_remHelper->SetAttribute ("Earfcn", earfcnValue);

  UintegerValue dlBandwidthValue ;
  enbDevice->GetAttribute ("DlBandwidth", dlBandwidthValue);
  m_remHelper->SetAttribute ("Bandwidth", dlBandwidthValue);

  // Adjust the LTE radio coverage area
  m_remHelper->SetAttribute ("XMin", DoubleValue (m_coverageArea.xMin));
  m_remHelper->SetAttribute ("XMax", DoubleValue (m_coverageArea.xMax));
  m_remHelper->SetAttribute ("YMin", DoubleValue (m_coverageArea.yMin));
  m_remHelper->SetAttribute ("YMax", DoubleValue (m_coverageArea.yMax));
  m_remHelper->SetAttribute ("Z", DoubleValue (m_ueHeight));

  // Adjust plot resolution
  uint32_t xResolution = m_coverageArea.xMax - m_coverageArea.xMin + 1;
  uint32_t yResolution = m_coverageArea.yMax - m_coverageArea.yMin + 1;
  m_remHelper->SetAttribute ("XRes", UintegerValue (xResolution));
  m_remHelper->SetAttribute ("YRes", UintegerValue (yResolution));

  m_remHelper->Install ();
}

void
LteHexGridNetwork::PrintBuildingListToFile (std::string filename)
{
  Ptr<OutputStreamWrapper> fileWrapper;
  fileWrapper = Create<OutputStreamWrapper> (filename, std::ios::out);

  uint32_t index = 0;
  BuildingList::Iterator it;
  for (it = BuildingList::Begin (); it != BuildingList::End (); it++)
    {
      index++;
      Box box = (*it)->GetBoundaries ();

      *fileWrapper->GetStream ()
      << "set object " << index
      << " rect from " << box.xMin  << "," << box.yMin
      << " to "   << box.xMax  << "," << box.yMax
      << " front fs empty "
      << std::endl;
    }
  fileWrapper = 0;
}

void
LteHexGridNetwork::PrintUeListToFile (std::string filename)
{
  Ptr<OutputStreamWrapper> fileWrapper;
  fileWrapper = Create<OutputStreamWrapper> (filename, std::ios::out);

  NetDeviceContainer::Iterator it;
  for (it = m_ueDevices.Begin (); it != m_ueDevices.End (); it++)
    {
      Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (*it);
      Ptr<Node> node = ueDev->GetNode ();
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();

      *fileWrapper->GetStream ()
      << "set label \"" << ueDev->GetImsi ()
      << "\" at "<< pos.x << "," << pos.y
      << " left font \"Helvetica,4\" textcolor rgb \"grey\" "
      << "front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
      << std::endl;
    }
  fileWrapper = 0;
}

void
LteHexGridNetwork::PrintEnbListToFile (std::string filename)
{
  Ptr<OutputStreamWrapper> fileWrapper;
  fileWrapper = Create<OutputStreamWrapper> (filename, std::ios::out);

  NetDeviceContainer::Iterator it;
  for (it = m_enbDevices.Begin (); it != m_enbDevices.End (); it++)
    {
      Ptr<LteEnbNetDevice> enbDev = DynamicCast<LteEnbNetDevice> (*it);
      Ptr<Node> node = enbDev->GetNode ();
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();

      *fileWrapper->GetStream ()
      << "set label \"" << enbDev->GetCellId ()
      << "\" at "<< pos.x << "," << pos.y
      << " left font \"Helvetica,4\" textcolor rgb \"white\" "
      << "front point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
      << std::endl;
    }
  fileWrapper = 0;
}

};  // namespace ns3
