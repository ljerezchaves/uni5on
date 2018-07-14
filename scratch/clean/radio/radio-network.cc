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

#include "radio-network.h"
#include "../backhaul/backhaul-network.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RadioNetwork");
NS_OBJECT_ENSURE_REGISTERED (RadioNetwork);

RadioNetwork::RadioNetwork (Ptr<BackhaulNetwork> epcNetwork)
  : m_epcHelper (epcNetwork)
{
  NS_LOG_FUNCTION (this << epcNetwork);

  // Adjust filenames for LTE trace files before creating the network.
  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();

  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlRlcOutputFilename",
                      StringValue (prefix + "dl_rlc_lte.log"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlRlcOutputFilename",
                      StringValue (prefix + "ul_rlc_lte.log"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::DlPdcpOutputFilename",
                      StringValue (prefix + "dl_pdcp_lte.log"));
  Config::SetDefault ("ns3::RadioBearerStatsCalculator::UlPdcpOutputFilename",
                      StringValue (prefix + "ul_pdcp_lte.log"));
  Config::SetDefault ("ns3::MacStatsCalculator::DlOutputFilename",
                      StringValue (prefix + "dl_mac_lte.log"));
  Config::SetDefault ("ns3::MacStatsCalculator::UlOutputFilename",
                      StringValue (prefix + "ul_mac_lte.log"));
  Config::SetDefault ("ns3::PhyStatsCalculator::DlRsrpSinrFilename",
                      StringValue (prefix + "dl_rsrp_sinr_lte.log"));
  Config::SetDefault ("ns3::PhyStatsCalculator::UlSinrFilename",
                      StringValue (prefix + "ul_sinr_lte.log"));
  Config::SetDefault ("ns3::PhyStatsCalculator::UlInterferenceFilename",
                      StringValue (prefix + "ul_interference_lte.log"));
  Config::SetDefault ("ns3::PhyRxStatsCalculator::DlRxOutputFilename",
                      StringValue (prefix + "dl_rx_phy_lte.log"));
  Config::SetDefault ("ns3::PhyRxStatsCalculator::UlRxOutputFilename",
                      StringValue (prefix + "ul_rx_phy_lte.log"));
  Config::SetDefault ("ns3::PhyTxStatsCalculator::DlTxOutputFilename",
                      StringValue (prefix + "dl_tx_phy_lte.log"));
  Config::SetDefault ("ns3::PhyTxStatsCalculator::UlTxOutputFilename",
                      StringValue (prefix + "ul_tx_phy_lte.log"));
}

RadioNetwork::RadioNetwork ()
{
  NS_LOG_FUNCTION (this);
}

RadioNetwork::~RadioNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RadioNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RadioNetwork")
    .SetParent<Object> ()
    .AddAttribute ("NumCellSites", "The total number of cell sites.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&RadioNetwork::m_nSites),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumHtcUes", "The total number of HTC UEs, randomly "
                   "distributed within the coverage area boundaries.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&RadioNetwork::m_nHtcUes),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("NumMtcUes", "The total number of MTC UEs, randomly "
                   "distributed within the coverage area boundaries.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1),
                   MakeUintegerAccessor (&RadioNetwork::m_nMtcUes),
                   MakeUintegerChecker<uint32_t> (0, 65535))
    .AddAttribute ("UeHeight", "The UE antenna height [m].",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (1.5),
                   MakeDoubleAccessor (&RadioNetwork::m_ueHeight),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("EnbMargin", "How much the eNB coverage area extends, "
                   "expressed as fraction of the inter-site distance.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&RadioNetwork::m_enbMargin),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("HtcUeMobility", "Enable HTC UE random mobility.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&RadioNetwork::m_htcUeMobility),
                   MakeBooleanChecker ())
    .AddAttribute ("MtcUeMobility", "Enable MTC UE random mobility.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&RadioNetwork::m_mtcUeMobility),
                   MakeBooleanChecker ())
    .AddAttribute ("LteTrace", "Enable LTE ASCII traces.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&RadioNetwork::m_lteTrace),
                   MakeBooleanChecker ())
    .AddAttribute ("PrintRem", "Print the radio environment map.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&RadioNetwork::m_lteRem),
                   MakeBooleanChecker ())
    .AddAttribute ("RemFilename", "Filename for the radio map (no extension).",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("radio-map"),
                   MakeStringAccessor (&RadioNetwork::m_remFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

Ptr<LteHelper>
RadioNetwork::GetLteHelper (void) const
{
  NS_LOG_FUNCTION (this);

  return m_lteHelper;
}

NodeContainer
RadioNetwork::GetEnbNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_enbNodes;
}

NodeContainer
RadioNetwork::GetHtcUeNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_htcUeNodes;
}

NodeContainer
RadioNetwork::GetMtcUeNodes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mtcUeNodes;
}

NetDeviceContainer
RadioNetwork::GetHtcUeDevices (void) const
{
  NS_LOG_FUNCTION (this);

  return m_htcUeDevices;
}

NetDeviceContainer
RadioNetwork::GetMtcUeDevices (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mtcUeDevices;
}

void
RadioNetwork::EnablePcap (std::string prefix, bool promiscuous)
{
  NS_LOG_FUNCTION (this << prefix << promiscuous);

  // Enable pcap on CSMA devices.
  CsmaHelper helper;
  // helper.EnablePcap (prefix + "enb-s1u", m_s1uDevices, promiscuous); // FIXME Vai usar mesmo aqui?
}

void
RadioNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_topoHelper = 0;
  m_remHelper = 0;
  m_lteHelper = 0;
  m_epcHelper = 0;
  Object::DoDispose ();
}

void
RadioNetwork::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  // Set the number of eNBs based on the number of cell sites.
  m_nEnbs = 3 * m_nSites;
  NS_LOG_INFO ("LTE RAN with " << m_nSites <<
               " sites and " << m_nEnbs << " eNBs.");

  // Automatically configure the LTE network (don't change the order below).
  ConfigureHelpers ();
  ConfigureEnbs ();
  ConfigureUes ();

  // Make the buildings mobility model consistent.
  BuildingsHelper::MakeMobilityModelConsistent ();

  // Chain up.
  Object::NotifyConstructionCompleted ();

  // If enable, print the LTE radio environment map.
  if (m_lteRem)
    {
      PrintRadioEnvironmentMap ();
    }

  // If enable, print the LTE ASCII trace files.
  if (m_lteTrace)
    {
      m_lteHelper->EnableTraces ();
    }
}

void
RadioNetwork::ConfigureHelpers ()
{
  NS_LOG_FUNCTION (this);

  // Create the LTE helper for the radio network.
  m_lteHelper = CreateObject<LteHelper> ();
  // m_lteHelper->SetEpcHelper (m_epcHelper); // FIXME Preciso de um EPC helper.

  // Use the hybrid path loss model obtained through a combination of several
  // well known path loss models in order to mimic different environmental
  // scenarios, considering the phenomenon of indoor/outdoor propagation in the
  // presence of buildings. Always use the LoS path loss model.
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

  // Configure the antennas for the hexagonal grid topology.
  m_lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
  m_lteHelper->SetEnbAntennaModelAttribute ("Beamwidth", DoubleValue (70));
  m_lteHelper->SetEnbAntennaModelAttribute (
    "MaxAttenuation", DoubleValue (20.0));

  // Create the topology helper used to group eNBs in three-sector sites layed
  // out on an hexagonal grid.
  m_topoHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
  m_topoHelper->SetLteHelper (m_lteHelper);
}

void
RadioNetwork::ConfigureEnbs ()
{
  NS_LOG_FUNCTION (this);

  // Create the eNBs nodes and set their names.
  m_enbNodes.Create (m_nEnbs);
  uint32_t enbCounter = 0;
  NodeContainer::Iterator it;
  for (it = m_enbNodes.Begin (); it != m_enbNodes.End (); ++it)
    {
      std::ostringstream enbName;
      enbName << "enb" << ++enbCounter;
      Names::Add (enbName.str (), *it);
    }

  // Attach each eNB to the backhaul network.
  // m_epcNetwork->AttachSdranCloud (); // FIXME Fazer um for aqui!
  // FIXME Anteriormente isto aqui conectava o SGW de cada SDRAN no backhaul.
  // Agora eu vou conectar cada eNB diretamente lá. Ainda não sei como informar
  // em qual switch conectar. Onde será que essa informação deve ficar?

  // Set the constant mobility model for eNB positioning
  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityHelper.Install (m_enbNodes);

  // Set eNB nodes positions on the hex grid and install the corresponding eNB
  // devices with antenna bore sight properly configured.
  m_enbDevices = m_topoHelper->SetPositionAndInstallEnbDevice (m_enbNodes);
  BuildingsHelper::Install (m_enbNodes);

  // Create an X2 interface between all the eNBs in a given set.
  // m_lteHelper->AddX2Interface (m_enbNodes); // FIXME ???? Isso aqui funciona?

  // Identify the LTE radio coverage area based on eNB nodes positions.
  std::vector<double> xPos, yPos;
  for (NodeList::Iterator it = m_enbNodes.Begin ();
       it != m_enbNodes.End (); it++)
    {
      Vector pos = (*it)->GetObject<MobilityModel> ()->GetPosition ();
      xPos.push_back (pos.x);
      yPos.push_back (pos.y);
    }

  // Get the minimum and maximum X and Y positions.
  double xMin = *std::min_element (xPos.begin (), xPos.end ());
  double yMin = *std::min_element (yPos.begin (), yPos.end ());
  double xMax = *std::max_element (xPos.begin (), xPos.end ());
  double yMax = *std::max_element (yPos.begin (), yPos.end ());

  // Calculate the coverage area considering the eNB margin parameter.
  DoubleValue doubleValue;
  m_topoHelper->GetAttribute ("InterSiteDistance", doubleValue);
  uint32_t adjust = m_enbMargin * doubleValue.Get ();
  m_coverageArea = Rectangle (round (xMin - adjust), round (xMax + adjust),
                              round (yMin - adjust), round (yMax + adjust));
  NS_LOG_INFO ("eNBs coverage area: " << m_coverageArea);
}

void
RadioNetwork::ConfigureUes ()
{
  NS_LOG_FUNCTION (this);

  // Create the HTC UE nodes and set their names.
  NS_LOG_INFO ("LTE RAN with " << m_nHtcUes << " HTC UEs.");
  m_htcUeNodes.Create (m_nHtcUes);
  for (uint32_t i = 0; i < m_nHtcUes; i++)
    {
      std::ostringstream ueName;
      ueName << "htcUe" << i + 1;
      Names::Add (ueName.str (), m_htcUeNodes.Get (i));
    }

  // Create the MTC UE nodes and set their names.
  NS_LOG_INFO ("LTE RAN with " << m_nMtcUes << " MTC UEs.");
  m_mtcUeNodes.Create (m_nMtcUes);
  for (uint32_t i = 0; i < m_nMtcUes; i++)
    {
      std::ostringstream ueName;
      ueName << "mtcUe" << i + 1;
      Names::Add (ueName.str (), m_mtcUeNodes.Get (i));
    }

  // Configure the fixed mobility helper for UEs.
  MobilityHelper fixedMobilityHelper;
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

  // Spread HTC UEs under eNBs coverage area.
  MobilityHelper htcMobHelper;
  htcMobHelper.SetPositionAllocator (boxPosAllocator);
  if (m_htcUeMobility)
    {
      htcMobHelper.SetMobilityModel (
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=15.0]"),
        "Pause", StringValue ("ns3::ExponentialRandomVariable[Mean=25.0]"),
        "PositionAllocator", PointerValue (boxPosAllocator));
    }
  htcMobHelper.Install (m_htcUeNodes);
  BuildingsHelper::Install (m_htcUeNodes);

  // Spread MTC UEs under eNBs coverage area.
  MobilityHelper mtcMobHelper;
  mtcMobHelper.SetPositionAllocator (boxPosAllocator);
  if (m_mtcUeMobility)
    {
      mtcMobHelper.SetMobilityModel (
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=30.0]"),
        "Pause", StringValue ("ns3::ExponentialRandomVariable[Mean=10.0]"),
        "PositionAllocator", PointerValue (boxPosAllocator));
    }
  mtcMobHelper.Install (m_mtcUeNodes);
  BuildingsHelper::Install (m_mtcUeNodes);

  // Install LTE protocol stack into UE nodes.
  m_htcUeDevices = m_lteHelper->InstallUeDevice (m_htcUeNodes);
  m_mtcUeDevices = m_lteHelper->InstallUeDevice (m_mtcUeNodes);

  // Install TCP/IP protocol stack into UE nodes.
  InternetStackHelper internet;
  internet.Install (m_htcUeNodes);
  internet.Install (m_mtcUeNodes);

  // FIXME Quem é o responsável por atribuir endereços IP??
  // m_epcHelper->AssignHtcUeIpv4Address (m_htcUeDevices);
  // m_epcHelper->AssignMtcUeIpv4Address (m_mtcUeDevices);

  // Specify static routes for each UE to its default S-GW.
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (NodeContainer::Iterator it = m_htcUeNodes.Begin ();
       it != m_htcUeNodes.End (); it++)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      // FIXME O endereço IP do PGW pode vir de outro lugar se for preciso
      // ueStaticRouting->SetDefaultRoute (
      //   m_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
  for (NodeContainer::Iterator it = m_mtcUeNodes.Begin ();
       it != m_mtcUeNodes.End (); it++)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting ((*it)->GetObject<Ipv4> ());
      // FIXME O endereço IP do PGW pode vir de outro lugar se for preciso
      // ueStaticRouting->SetDefaultRoute (
      //   m_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach UE to the eNBs using initial cell selection.
  m_lteHelper->Attach (m_htcUeDevices);
  m_lteHelper->Attach (m_mtcUeDevices);
}

void
RadioNetwork::PrintRadioEnvironmentMap ()
{
  NS_LOG_FUNCTION (this);

  // Force UE initialization so we don't have to wait for nodes to start before
  // positions are assigned (which is needed to output node positions to plot).
  for (NodeContainer::Iterator it = m_htcUeNodes.Begin ();
       it != m_htcUeNodes.End (); it++)
    {
      (*it)->Initialize ();
    }
  for (NodeContainer::Iterator it = m_mtcUeNodes.Begin ();
       it != m_mtcUeNodes.End (); it++)
    {
      (*it)->Initialize ();
    }

  StringValue prefixValue;
  GlobalValue::GetValueByName ("OutputPrefix", prefixValue);
  std::string filename = prefixValue.Get () + m_remFilename;

  // Create the radio environment map helper and set output filename.
  m_remHelper = CreateObject<RadioEnvironmentMapHelper> ();
  m_remHelper->SetAttribute ("OutputFile", StringValue (filename + ".dat"));

  // Adjust LTE radio channel ID.
  Ptr<LteEnbNetDevice> enbDevice =
    DynamicCast<LteEnbNetDevice> (m_enbDevices.Get (0));
  int id = enbDevice->GetPhy ()->GetDlSpectrumPhy ()->GetChannel ()->GetId ();
  std::ostringstream path;
  path << "/ChannelList/" << id;
  m_remHelper->SetAttribute ("ChannelPath", StringValue (path.str ()));

  // Adjust the channel frequency and bandwidth.
  UintegerValue earfcnValue;
  enbDevice->GetAttribute ("DlEarfcn", earfcnValue);
  m_remHelper->SetAttribute ("Earfcn", earfcnValue);

  UintegerValue dlBandwidthValue;
  enbDevice->GetAttribute ("DlBandwidth", dlBandwidthValue);
  m_remHelper->SetAttribute ("Bandwidth", dlBandwidthValue);

  // Adjust the LTE radio coverage area.
  Rectangle area = m_coverageArea;
  m_remHelper->SetAttribute ("XMin", DoubleValue (area.xMin));
  m_remHelper->SetAttribute ("XMax", DoubleValue (area.xMax));
  m_remHelper->SetAttribute ("YMin", DoubleValue (area.yMin));
  m_remHelper->SetAttribute ("YMax", DoubleValue (area.yMax));
  m_remHelper->SetAttribute ("Z", DoubleValue (m_ueHeight));

  // Adjust plot resolution.
  uint32_t xResolution = area.xMax - area.xMin + 1;
  uint32_t yResolution = area.yMax - area.yMin + 1;
  m_remHelper->SetAttribute ("XRes", UintegerValue (xResolution));
  m_remHelper->SetAttribute ("YRes", UintegerValue (yResolution));

  // Prepare the GNUPlot script file.
  Ptr<OutputStreamWrapper> fileWrapper;
  fileWrapper = Create<OutputStreamWrapper> (filename + ".gpi", std::ios::out);

  size_t begin = filename.rfind ("/");
  size_t end = filename.length ();
  std::string localname = filename.substr (begin + 1, end - 1);

  *fileWrapper->GetStream ()
    << "set term pdfcairo enhanced color dashed rounded" << std::endl
    << "set output '" << localname << ".pdf'" << std::endl
    << "unset key" << std::endl
    << "set view map;" << std::endl
    << "set xlabel 'x-coordinate (m)'" << std::endl
    << "set ylabel 'y-coordinate (m)'" << std::endl
    << "set cbrange [-5:20]" << std::endl
    << "set cblabel 'SINR (dB)'" << std::endl
    << "set xrange [" << area.xMin << ":" << area.xMax << "]" << std::endl
    << "set yrange [" << area.yMin << ":" << area.yMax << "]" << std::endl;

  // Buildings.
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin ();
       it != BuildingList::End (); it++)
    {
      index++;
      Box box = (*it)->GetBoundaries ();

      *fileWrapper->GetStream ()
        << "set object " << index << " rect"
        << " from " << box.xMin  << "," << box.yMin
        << " to "   << box.xMax  << "," << box.yMax
        << " front fs empty "
        << std::endl;
    }

  // HTC UEs positions.
  for (NetDeviceContainer::Iterator it = m_htcUeDevices.Begin ();
       it != m_htcUeDevices.End (); it++)
    {
      Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (*it);
      Ptr<Node> node = ueDev->GetNode ();
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();

      *fileWrapper->GetStream ()
        << "set label '" << ueDev->GetImsi () << "' "
        << "at " << pos.x << "," << pos.y << " "
        << "left font ',5' textcolor rgb 'grey' "
        << "front point pt 1 lw 2 ps 0.3 lc rgb 'grey'"
        << std::endl;
    }
  for (NetDeviceContainer::Iterator it = m_mtcUeDevices.Begin ();
       it != m_mtcUeDevices.End (); it++)
    {
      Ptr<LteUeNetDevice> ueDev = DynamicCast<LteUeNetDevice> (*it);
      Ptr<Node> node = ueDev->GetNode ();
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();

      *fileWrapper->GetStream ()
        << "set label '" << ueDev->GetImsi () << "' "
        << "at " << pos.x << "," << pos.y << " "
        << "left font ',5' textcolor rgb 'grey' "
        << "front point pt 1 lw 2 ps 0.3 lc rgb 'grey'"
        << std::endl;
    }

  // Cell site positions.
  for (NetDeviceContainer::Iterator it = m_enbDevices.Begin ();
       it != m_enbDevices.End (); it++, it++, it++)
    {
      Ptr<LteEnbNetDevice> enbDev = DynamicCast<LteEnbNetDevice> (*it);
      Ptr<Node> node = enbDev->GetNode ();
      Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
      uint32_t site = enbDev->GetCellId ();

      *fileWrapper->GetStream ()
        << "set label '" << site << "," << site + 1 << "," << site + 2 << "' "
        << "at " << pos.x << "," << pos.y << " "
        << "left font ',5' textcolor rgb 'white' "
        << "front point pt 7 ps 0.4 lc rgb 'white'"
        << std::endl;
    }

  // Radio map.
  *fileWrapper->GetStream ()
    << "plot '" << localname << ".dat' using 1:2:(10*log10($4)) with image"
    << std::endl;
  fileWrapper = 0;

  // Install the REM generator.
  m_remHelper->Install ();
}

} // namespace ns3
