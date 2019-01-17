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

#include <iomanip>
#include <iostream>
#include <ns3/config-store-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "helpers/svelte-helper.h"

using namespace ns3;

/**
 * \defgroup svelte SVELTE architecture
 * Network simulation scenario for SVELTE architecture.
 */
NS_LOG_COMPONENT_DEFINE ("Main");


void ConfigureDefaults ();
void ForceDefaults ();
void EnableProgress (uint32_t);
void EnableVerbose (bool);
void EnableOfsLogs (bool);

// Prefixes used by input and output filenames.
static ns3::GlobalValue
  g_inputPrefix ("InputPrefix", "Common prefix for output filenames.",
                 ns3::StringValue (std::string ()),
                 ns3::MakeStringChecker ());

static ns3::GlobalValue
  g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                  ns3::StringValue (std::string ()),
                  ns3::MakeStringChecker ());

// Dump timeout for logging statistics.
static ns3::GlobalValue
  g_dumpTimeout ("DumpStatsTimeout", "Periodic statistics dump interval.",
                 ns3::TimeValue (Seconds (1)),
                 ns3::MakeTimeChecker ());

// Simulation lenght.
static ns3::GlobalValue
  g_simTime ("SimTime", "Simulation stop time.",
             ns3::TimeValue (Seconds (0)),
             ns3::MakeTimeChecker ());

int
main (int argc, char *argv[])
{
  bool        verbose  = false;
  bool        pcap     = false;
  bool        ofsLog   = false;
  bool        lteRem   = false;
  uint32_t    progress = 0;
  std::string prefix   = std::string ();

  // Configure some default attribute values. These values can be overridden by
  // users on the command line or in the configuration file.
  ConfigureDefaults ();

  // Parse command line arguments
  CommandLine cmd;
  cmd.AddValue ("Verbose",  "Enable verbose output.", verbose);
  cmd.AddValue ("Pcap",     "Enable pcap output.", pcap);
  cmd.AddValue ("OfsLog",   "Enable ofsoftswitch13 logs.", ofsLog);
  cmd.AddValue ("LteRem",   "Print LTE radio environment map.", lteRem);
  cmd.AddValue ("Progress", "Simulation progress interval (sec).", progress);
  cmd.AddValue ("Prefix",   "Common prefix for filenames.", prefix);
  cmd.Parse (argc, argv);

  // Update input and output prefixes from command line prefix parameter.
  NS_ASSERT_MSG (!prefix.empty (), "Unknown prefix.");
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
  EnableVerbose (verbose);
  EnableOfsLogs (ofsLog);

  // Create the SVELTE helper object, which is responsible for creating and
  // configuring the infrastructure and logical networks.
  NS_LOG_INFO ("Creating simulation scenario...");
  Ptr<SvelteHelper> svelteHelper = CreateObject<SvelteHelper> ();

  // Populating routing and ARP tables. The 'perfect' ARP used here comes from
  // the patch at https://www.nsnam.org/bugzilla/show_bug.cgi?id=187. This
  // patch uses a single ARP cache shared among all nodes. Some developers have
  // pointed that this implementation may fail if a node change what it thinks
  // that it's a local cache, or if there are global MAC hardware duplications.
  // Anyway, I've decided to use this to simplify the controller logic.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ArpCache::PopulateArpCaches ();

  // Print the LTE radio environment map.
  if (lteRem)
    {
      svelteHelper->PrintLteRem ();
    }

  // Enable pcap output.
  if (pcap)
    {
      svelteHelper->EnablePcap (outputPrefix.str (), true);
    }

  // Set stop time and run the simulation.
  std::cout << "Simulating..." << std::endl;
  EnableProgress (progress);

  TimeValue timeValue;
  GlobalValue::GetValueByName ("SimTime", timeValue);
  Time stopAt = timeValue.Get () + MilliSeconds (100);

  Simulator::Stop (stopAt);
  Simulator::Run ();

  // Finish the simulation.
  Simulator::Destroy ();
  svelteHelper->Dispose ();
  svelteHelper = 0;
  std::cout << "END OK" << std::endl;

  return 0;
}

void ConfigureDefaults ()
{
  // Force some default attribute values
  ForceDefaults ();

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
  Config::SetDefault (
    "ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));

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
  Config::SetDefault (
    "ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (100));
  Config::SetDefault (
    "ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (100));

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
  Config::SetDefault (
    "ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault (
    "ns3::LteUeNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault (
    "ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (18100));

  //
  // We are configuring the eNB transmission power as a macro cell (46 dBm is
  // the maximum used value for the eNB for 20MHz channel). The max power that
  // the UE is allowed to use is set by the standard (23dBm). We are currently
  // using no power control.
  // See http://tinyurl.com/nlh6u3t and http://tinyurl.com/nlh6u3t
  //
  Config::SetDefault (
    "ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault (
    "ns3::LteUePhy::TxPower", DoubleValue (23));

  //
  // Disabling UE uplink power control.
  //
  Config::SetDefault (
    "ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue (false));

  //
  // Using the UE MIMO transmission diversity (Mode 2 with 4.2bB antenna gain).
  //
  Config::SetDefault (
    "ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue (1));

  //
  // Using the Proportional Fair (PF) Scheduler as the LTE MAC downlink
  // scheduling algorithm, which works by scheduling a user when its
  // instantaneous channel quality is high relative to its own average channel
  // condition over time.
  //
  Config::SetDefault (
    "ns3::LteHelper::Scheduler", StringValue ("ns3::PfFfMacScheduler"));

  //
  // Disabling error models for both control and data planes.
  //
  Config::SetDefault (
    "ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  Config::SetDefault (
    "ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));

  //
  // Set the LTE hexagonal grid layout topology to inter-site distance of 500m
  // with a single site in even rows.
  //
  Config::SetDefault (
    "ns3::LteHexGridEnbTopologyHelper::InterSiteDistance", DoubleValue (500));
  Config::SetDefault (
    "ns3::LteHexGridEnbTopologyHelper::SectorOffset", DoubleValue (0));
  Config::SetDefault (
    "ns3::LteHexGridEnbTopologyHelper::MinX", DoubleValue (500));
  Config::SetDefault (
    "ns3::LteHexGridEnbTopologyHelper::MinY", DoubleValue (250));
  Config::SetDefault (
    "ns3::LteHexGridEnbTopologyHelper::GridWidth", UintegerValue (2));
}

void ForceDefaults ()
{
  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we must enable checksum computations.
  //
  Config::SetGlobal (
    "ChecksumEnabled", BooleanValue (true));

  //
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns-3. To allow larger TCP packets, we defined
  // this value to 1400, based on 1500 bytes for Ethernet v2 MTU, and
  // considering 8 bytes for PPPoE header, 40 bytes for GTP/UDP/IP tunnel
  // headers, and 52 bytes for default TCP/IP headers. Don't use higher values
  // to avoid packet fragmentation.
  //
  Config::SetDefault (
    "ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  //
  // The default number of TCP connection attempts before returning a failure
  // is set to 6 in ns-3, with an interval of 3 seconds between each attempt.
  // We are going to keep the number of attempts but with a small interval of
  // 500 ms between them.
  //
  Config::SetDefault (
    "ns3::TcpSocket::ConnTimeout", TimeValue (MilliSeconds (500)));

  //
  // Increasing the default MTU for virtual network devices, which are used as
  // OpenFlow virtual port devices.
  //
  Config::SetDefault (
    "ns3::VirtualNetDevice::Mtu", UintegerValue (3000));

  //
  // Whenever possible, use the full-duplex CSMA channel to improve throughput.
  // The code will automatically fall back to half-duplex mode for more than
  // two devices in the same channel.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault (
    "ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  //
  // Fix the number of output priority queues on every switch port to 3.
  //
  Config::SetDefault (
    "ns3::OFSwitch13Queue::NumQueues", UintegerValue (3));

  //
  // Enable detailed OpenFlow datapath statistics.
  //
  Config::SetDefault (
    "ns3::OFSwitch13StatsCalculator::FlowTableDetails", BooleanValue (true));
}

void
EnableProgress (uint32_t interval)
{
  if (interval)
    {
      int64_t now = Simulator::Now ().ToInteger (Time::S);
      std::cout << "Current simulation time: +" << now << ".0s" << std::endl;
      Simulator::Schedule (Seconds (interval), &EnableProgress, interval);
    }
}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogLevel logLevelWarn = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN);
      NS_UNUSED (logLevelWarn);

      LogLevel logLevelWarnInfo = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN | LOG_INFO);
      NS_UNUSED (logLevelWarnInfo);

      LogLevel logLevelInfo = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_INFO);
      NS_UNUSED (logLevelInfo);

      LogLevel logLevelAll = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
      NS_UNUSED (logLevelAll);

      // Common components.
      LogComponentEnable ("Main",                     logLevelWarnInfo);
      LogComponentEnable ("SvelteCommon",             logLevelWarnInfo);

      // Helper components.
      LogComponentEnable ("SvelteHelper",             logLevelWarnInfo);
      LogComponentEnable ("TrafficHelper",            logLevelWarnInfo);

      // Infrastructure components.
      LogComponentEnable ("BackhaulController",       logLevelInfo);
      LogComponentEnable ("BackhaulNetwork",          logLevelWarnInfo);
      LogComponentEnable ("RadioNetwork",             logLevelWarnInfo);
      LogComponentEnable ("RingController",           logLevelInfo);
      LogComponentEnable ("RingNetwork",              logLevelWarnInfo);
      LogComponentEnable ("SvelteEnbApplication",     logLevelWarnInfo);

      // Logical components.
      LogComponentEnable ("GtpTunnelApp",             logLevelWarnInfo);
      LogComponentEnable ("PgwTunnelApp",             logLevelWarnInfo);
      LogComponentEnable ("SliceController",          logLevelWarnInfo);
      LogComponentEnable ("SliceNetwork",             logLevelWarnInfo);
      LogComponentEnable ("SvelteMme",                logLevelWarnInfo);
      LogComponentEnable ("TrafficManager",           logLevelWarnInfo);

      // Metadata components.
      LogComponentEnable ("EnbInfo",                  logLevelWarnInfo);
      LogComponentEnable ("LinkInfo",                 logLevelInfo);
      LogComponentEnable ("PgwInfo",                  logLevelWarnInfo);
      LogComponentEnable ("RingInfo",                 logLevelWarnInfo);
      LogComponentEnable ("RoutingInfo",              logLevelWarnInfo);
      LogComponentEnable ("SgwInfo",                  logLevelWarnInfo);
      LogComponentEnable ("UeInfo",                   logLevelWarnInfo);

      // Applications.
      LogComponentEnable ("BufferedVideoClient",      logLevelWarnInfo);
      LogComponentEnable ("BufferedVideoServer",      logLevelWarnInfo);
      LogComponentEnable ("HttpClient",               logLevelWarnInfo);
      LogComponentEnable ("HttpServer",               logLevelWarnInfo);
      LogComponentEnable ("LiveVideoClient",          logLevelWarnInfo);
      LogComponentEnable ("LiveVideoServer",          logLevelWarnInfo);
      LogComponentEnable ("SvelteClient",             logLevelWarnInfo);
      LogComponentEnable ("SvelteServer",             logLevelWarnInfo);
      LogComponentEnable ("SvelteUdpClient",          logLevelWarnInfo);
      LogComponentEnable ("SvelteUdpServer",          logLevelWarnInfo);

      // Statistic components.
      LogComponentEnable ("AdmissionStatsCalculator", logLevelWarnInfo);
      LogComponentEnable ("BackhaulStatsCalculator",  logLevelWarnInfo);
      LogComponentEnable ("FlowStatsCalculator",      logLevelWarnInfo);
      LogComponentEnable ("LteRrcStatsCalculator",    logLevelWarnInfo);
      LogComponentEnable ("PgwTftStatsCalculator",    logLevelWarnInfo);
      LogComponentEnable ("TrafficStatsCalculator",   logLevelWarnInfo);

      // OFSwitch13 module components.
      LogComponentEnable ("OFSwitch13Controller",     logLevelWarn);
      LogComponentEnable ("OFSwitch13Device",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Helper",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Interface",      logLevelWarn);
      LogComponentEnable ("OFSwitch13Port",           logLevelWarn);
      LogComponentEnable ("OFSwitch13Queue",          logLevelWarn);
      LogComponentEnable ("OFSwitch13SocketHandler",  logLevelWarn);
    }
}

void
EnableOfsLogs (bool enable)
{
  if (enable)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();
      ofs::EnableLibraryLog (true, prefix);
    }
}
