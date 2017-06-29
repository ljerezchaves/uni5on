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

#include <ns3/core-module.h>
#include <ns3/config-store-module.h>
#include <iomanip>
#include <iostream>
#include "lte-network.h"
#include "traffic-helper.h"
#include "epc/ring-network.h"
#include "stats/admission-stats-calculator.h"
#include "stats/backhaul-stats-calculator.h"
#include "stats/connection-stats-calculator.h"
#include "stats/pgw-tft-stats-calculator.h"
#include "stats/traffic-stats-calculator.h"

using namespace ns3;
using namespace ns3::ofs;

/**
 * \defgroup sdmn SDMN architecture
 * Network simulation scenario for Software-Defined Mobile network
 * architecture. This scenario comprises an LTE EPC network using a OpenFlow
 * 1.3 backhaul infrastructure.
 */
NS_LOG_COMPONENT_DEFINE ("Main");

void PrintCurrentTime (uint32_t);
void ConfigureDefaults ();
void ForceDefaults ();
void EnableVerbose (bool);
void EnableLibLogs (bool);

// Prefixes used by input and output filenames.
static ns3::GlobalValue
g_inputPrefix ("InputPrefix", "Common prefix for output filenames.",
               ns3::StringValue (""),
               ns3::MakeStringChecker ());

static ns3::GlobalValue
g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                ns3::StringValue (""),
                ns3::MakeStringChecker ());

// Dump timeout for logging statistics
static GlobalValue
g_dumpTimeout ("DumpStatsTimeout", "Periodic statistics dump interval.",
               TimeValue (Seconds (10)),
               ns3::MakeTimeChecker ());

int
main (int argc, char *argv[])
{
  bool        verbose  = false;
  bool        pcap     = false;
  bool        libLog   = false;
  uint32_t    progress = 0;
  uint32_t    simTime  = 250;
  std::string prefix   = "";

  // Configure some default attribute values. These values can be overridden by
  // users on the command line or in the configuration file.
  ConfigureDefaults ();

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("Verbose",  "Enable verbose output.", verbose);
  cmd.AddValue ("Pcap",     "Enable pcap output.", pcap);
  cmd.AddValue ("LibLog",   "Enable ofsoftswitch13 logs.", libLog);
  cmd.AddValue ("Progress", "Simulation progress interval [s].", progress);
  cmd.AddValue ("SimTime",  "Simulation stop time [s].", simTime);
  cmd.AddValue ("Prefix",   "Common prefix for filenames.", prefix);
  cmd.Parse (argc, argv);

  // Update input and output prefixes from command line prefix parameter.
  NS_ASSERT_MSG (prefix != "", "Unknown prefix.");
  std::ostringstream inputPrefix, outputPrefix;
  inputPrefix << prefix;
  char lastChar = *prefix.rbegin ();
  if (lastChar != '-')
    {
      inputPrefix << "-";
    }
  outputPrefix << inputPrefix.str () << RngSeedManager::GetRun () << "-";
  Config::SetGlobal ("InputPrefix", StringValue (inputPrefix.str ()));
  Config::SetGlobal ("OutputPrefix", StringValue (outputPrefix.str ()));

  // Read the configuration file. The file existence is mandatory.
  std::string cfgFilename = prefix + ".topo";
  std::ifstream testFile (cfgFilename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (testFile.good (), "Invalid topology file " << cfgFilename);
  testFile.close ();

  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (cfgFilename));
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // Parse command line again so users can override values from configuration
  // file, and force some default attribute values that cannot be overridden.
  cmd.Parse (argc, argv);
  ForceDefaults ();

  // Enable verbose output and progress report for debug purposes.
  PrintCurrentTime (progress);
  EnableVerbose (verbose);
  EnableLibLogs (libLog);

  // Create the simulation scenario.
  // The following objects must be created in this order:
  // * The OpenFlow EPC backhaul network
  // * The LTE radio access network
  // * The traffic helper for applications
  // * The stats calculators
  NS_LOG_INFO ("Creating simulation scenario...");

  Ptr<RingNetwork>   ofNetwork;
  Ptr<LteNetwork>    lteNetwork;
  Ptr<TrafficHelper> trafficHelper;
  ofNetwork     = CreateObject<RingNetwork> ();
  lteNetwork    = CreateObject<LteNetwork> (ofNetwork);
  trafficHelper = CreateObject<TrafficHelper> (lteNetwork,
                                               ofNetwork->GetWebNode ());

  Ptr<AdmissionStatsCalculator>   admissionStats;
  Ptr<BackhaulStatsCalculator>    backhaulStats;
  Ptr<ConnectionStatsCalculator>  connectionStats;
  Ptr<PgwTftStatsCalculator>      pgwTftStats;
  Ptr<TrafficStatsCalculator>     trafficStats;
  admissionStats  = CreateObject<AdmissionStatsCalculator> ();
  backhaulStats   = CreateObject<BackhaulStatsCalculator> ();
  connectionStats = CreateObject<ConnectionStatsCalculator> ();
  pgwTftStats     = CreateObject<PgwTftStatsCalculator> ();
  trafficStats    = CreateObject<TrafficStatsCalculator> ();

  // Populating routing and ARP tables. The 'perfect' ARP used here comes from
  // the patch at https://www.nsnam.org/bugzilla/show_bug.cgi?id=187. This
  // patch uses a single ARP cache shared among all nodes. Some developers have
  // pointed that this implementation may fail if a node change what it thinks
  // that it's a local cache, or if there are global MAC hardware duplications.
  // Anyway, I've decided to use this to simplify the controller logic.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ArpCache::PopulateArpCaches ();

  // If necessary, enable pcap output
  if (pcap)
    {
      ofNetwork->EnablePcap (outputPrefix.str (), true);
      lteNetwork->EnablePcap (outputPrefix.str (), true);
    }

  // Run the simulation.
  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
PrintCurrentTime (uint32_t interval)
{
  if (interval)
    {
      int64_t now = Simulator::Now ().ToInteger (Time::S);
      std::cout << "Current simulation time: +" << now << ".0s" << std::endl;
      Simulator::Schedule (Seconds (interval), &PrintCurrentTime, interval);
    }
}

void ConfigureDefaults ()
{
  // Force some default attribute values
  ForceDefaults ();

  //
  // Increasing the default MTU for virtual network devices, which are used as
  // OpenFlow virtual port devices.
  //
  Config::SetDefault ("ns3::VirtualNetDevice::Mtu", UintegerValue (3000));

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
  // ** Considering Band #1 @2100 MHz (FDD)
  //
  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn",  UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18100));

  //
  // We are configuring the eNB transmission power as a macro cell (46 dBm is
  // the maximum used value for the eNB for 20MHz channel). The max power that
  // the UE is allowed to use is set by the standard (23dBm). We are currently
  // using no power control.
  // See http://tinyurl.com/nlh6u3t and http://tinyurl.com/nlh6u3t
  //
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::LteUePhy::TxPower",  DoubleValue (23));

  //
  // Disabling UE uplink power control.
  //
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
  // Disabling error models for both control and data planes.
  //
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                      BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                      BooleanValue (false));

  //
  // Set the LTE hexagonal grid layout topology to inter-site distance of 500m
  // with a single site in even rows.
  //
  Config::SetDefault ("ns3::LteNetwork::EnbMargin", DoubleValue (0.5));
  Config::SetDefault ("ns3::LteHexGridEnbTopologyHelper::InterSiteDistance",
                      DoubleValue (500));
  Config::SetDefault ("ns3::LteHexGridEnbTopologyHelper::SectorOffset",
                      DoubleValue (0));
  Config::SetDefault ("ns3::LteHexGridEnbTopologyHelper::MinX",
                      DoubleValue (500));
  Config::SetDefault ("ns3::LteHexGridEnbTopologyHelper::MinY",
                      DoubleValue (250));
  Config::SetDefault ("ns3::LteHexGridEnbTopologyHelper::GridWidth",
                      UintegerValue (1));
}

void ForceDefaults ()
{
  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we must enable checksum computations.
  //
  Config::SetGlobal ("ChecksumEnabled", BooleanValue (true));

  //
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns-3. To allow larger TCP packets, we defined
  // this value to 1400, based on 1500 bytes for Ethernet v2 MTU, and
  // considering 8 bytes for PPPoE header, 40 bytes for GTP/UDP/IP tunnel
  // headers, and 52 bytes for default TCP/IP headers. Don't use higher values
  // to avoid packet fragmentation.
  //
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  //
  // The default number of TCP connection attempts before returning a failure
  // is set to 6 in ns-3, with an interval of 3 seconds between each attempt.
  // We are going to keep the number of attempts but with a small interval of
  // 500 ms between them.
  //
  Config::SetDefault ("ns3::TcpSocket::ConnTimeout",
                      TimeValue (MilliSeconds (500)));

  //
  // The default TCP minimum retransmit timeout value is set to 1 second in
  // ns-3, according to RFC 6298. However, Linux uses 200 ms as the default
  // value, and we are going to keep up with this fast retransmission
  // approach.
  //
  Config::SetDefault ("ns3::TcpSocketBase::MinRto",
                      TimeValue (MilliSeconds (200)));

  //
  // Whenever possible, use the full-duplex CSMA channel to improve throughput.
  // The code will automatically fall back to half-duplex mode for more than
  // two devices in the same channel.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));
}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogComponentEnable ("EpcNetwork",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("EpcController",            LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("LteNetwork",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("Main",                     LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("PgwTunnelApp",             LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RingController",           LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RingNetwork",              LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("SdranCloud",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("SdranController",          LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("SdranMme",                 LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("GtpTunnelApp",             LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("TrafficHelper",            LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("TrafficManager",           LOG_ERROR_WARN_INFO_FT);

      LogComponentEnable ("HttpClient",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("HttpServer",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RealTimeVideoClient",      LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RealTimeVideoServer",      LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("SdmnClientApp",            LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("SdmnServerApp",            LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("StoredVideoClient",        LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("StoredVideoServer",        LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("VoipClient",               LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("VoipServer",               LOG_ERROR_WARN_INFO_FT);

      LogComponentEnable ("ConnectionInfo",           LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("EnbInfo",                  LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("GbrInfo",                  LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("MeterInfo",                LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RingRoutingInfo",          LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("RoutingInfo",              LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("UeInfo",                   LOG_ERROR_WARN_INFO_FT);

      LogComponentEnable ("AdmissionStatsCalculator", LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("BackhaulStatsCalculator",  LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("PgwTftStatsCalculator",    LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("QosStatsCalculator",       LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("TrafficStatsCalculator",   LOG_ERROR_WARN_INFO_FT);

      LogComponentEnable ("OFSwitch13Controller",     LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13Device",         LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13Helper",         LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13Interface",      LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13Port",           LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13SocketHandler",  LOG_ERROR_WARN_INFO_FT);
      LogComponentEnable ("OFSwitch13Queue",          LOG_ERROR_WARN_INFO_FT);
    }
}

void
EnableLibLogs (bool enable)
{
  if (enable)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();
      ofs::EnableLibraryLog (true, prefix);
    }
}
