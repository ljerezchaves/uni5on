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
#include "simulation-scenario.h"
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

void ConfigureDefaults ();
void PrintCurrentTime (uint32_t);
void EnableVerbose (bool);

int
main (int argc, char *argv[])
{
  bool     verbose  = false;
  uint32_t progress = 0;
  uint32_t simTime  = 250;

  ConfigureDefaults ();
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  CommandLine cmd;
  cmd.AddValue ("verbose",    "Enable verbose output.", verbose);
  cmd.AddValue ("progress",   "Simulation progress interval [s].", progress);
  cmd.AddValue ("simTime",    "Simulation time [s].", simTime);
  cmd.AddValue ("topoFile",   "ns3::SimulationScenario::TopoFilename");
  cmd.AddValue ("prefix",     "ns3::SimulationScenario::CommonPrefix");
  cmd.AddValue ("pcap",       "ns3::SimulationScenario::PcapTrace");
  cmd.AddValue ("trace",      "ns3::SimulationScenario::LteTrace");
  cmd.AddValue ("radioMap",   "ns3::SimulationScenario::LteRem");
  cmd.AddValue ("liblog",     "ns3::SimulationScenario::SwitchLogs");
  cmd.AddValue ("voip",       "ns3::TrafficHelper::VoipTraffic");
  cmd.AddValue ("gbrLiveVid", "ns3::TrafficHelper::GbrLiveVideoTraffic");
  cmd.AddValue ("buffVid",    "ns3::TrafficHelper::BufferedVideoTraffic");
  cmd.AddValue ("nonLiveVid", "ns3::TrafficHelper::NonGbrLiveVideoTraffic");
  cmd.AddValue ("http",       "ns3::TrafficHelper::HttpTraffic");
  cmd.AddValue ("fast",       "ns3::TrafficHelper::FastTraffic");
  cmd.AddValue ("strategy",   "ns3::RingController::Strategy");
  cmd.AddValue ("ueFixed",    "ns3::LteHexGridNetwork::UeFixedPos");
  cmd.AddValue ("bandwidth",  "ns3::RingNetwork::SwitchLinkDataRate");
  cmd.Parse (argc, argv);

  // For debug purposes, enable verbose output and simulation progress report
  PrintCurrentTime (progress);
  EnableVerbose (verbose);

  // Creating the simulation scenario
  NS_LOG_INFO ("Creating simulation scenario...");
  Ptr<SimulationScenario> scenario = CreateObject<SimulationScenario> ();
  scenario->BuildRingTopology ();

  // Run the simulation
  NS_LOG_INFO ("Simulating...");
  Simulator::Stop (Seconds (simTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("End!");
}

void
ConfigureDefaults ()
{
  //
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns-3. To allow larger TCP packets, we defined
  // this value to 1400, based on 1500 bytes for Ethernet v2 MTU, and
  // considering 8 bytes for PPPoE header, 40 bytes for GTP/UDP/IP tunnel
  // headers, and 52 byter for default TCP/IP headers.
  //
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  //
  // When possible, use the Full Duplex CSMA channel to improve throughput.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  //
  // For network queues, use the byte mode and set default size to 128 KBytes.
  //
  Config::SetDefault ("ns3::DropTailQueue::Mode",
                      EnumValue (Queue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue (131072));

  //
  // For OpenFlow queues, use the priority queuing scheduling algorithm.
  //
  Config::SetDefault ("ns3::OFSwitch13Queue::Scheduling",
                      EnumValue (OFSwitch13Queue::PRIO));

  //
  // For the OpenFlow control channel, let's use point to point connections
  // between controller and switches.
  //
  Config::SetDefault ("ns3::OFSwitch13Helper::ChannelType",
                      EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we need to enable checksum computations (which are
  // disabled by default in ns-3).
  //
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  // --------------------------- LTE PARAMETERS --------------------------- //

  //
  // Increasing SrsPeriodicity to allow more UEs per eNB. Allowed values are:
  // {2, 5, 10, 20, 40, 80, 160, 320}. The default value (40) allows no more
  // than ~40 UEs for each eNB. Note that the value needs to be higher than the
  // actual number of UEs in your simulation program.  This is due to the need
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
  // using a lower value, with no power control.
  // See http://tinyurl.com/nlh6u3t and http://tinyurl.com/nlh6u3t
  //
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (18));
  Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl",
                      BooleanValue (false));

  //
  // Using a simplified model working only with Okumura Hata, considering the
  // phenomenon of indoor/outdoor propagation in the presence of buildings.
  //
  Config::SetDefault ("ns3::LteHelper::PathlossModel",
                      StringValue ("ns3::OhBuildingsPropagationLossModel"));

  //
  // Using the Channel and QoS Aware (CQA) Scheduler as the LTE MAC downlink
  // scheduling algorithm, which considers the head of line delay, the GBR
  // parameters and channel quality over different subbands.
  //
  Config::SetDefault ("ns3::LteHelper::Scheduler",
                      StringValue ("ns3::CqaFfMacScheduler"));

  //
  // Disabling error models for both control and data planes. This is necessary
  // for handover procedures.
  //
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                      BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                      BooleanValue (false));
}

void
PrintCurrentTime (uint32_t interval)
{
  if (interval)
    {
      std::cout << "Current simulation time: "
                << Simulator::Now ().As (Time::S)
                << std::endl;
      Simulator::Schedule (Seconds (interval), &PrintCurrentTime, interval);
    }
}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogComponentEnable ("Main", LOG_LEVEL_ALL);
      LogComponentEnable ("SimulationScenario", LOG_LEVEL_INFO);
      LogComponentEnable ("StatsCalculator", LOG_LEVEL_WARN);
      LogComponentEnable ("ConnectionInfo", LOG_LEVEL_WARN);

      LogComponentEnable ("OFSwitch13NetDevice", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_WARN);
      LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_WARN);

      LogComponentEnable ("OpenFlowEpcHelper", LOG_LEVEL_WARN);
      LogComponentEnable ("OpenFlowEpcNetwork", LOG_LEVEL_WARN);
      LogComponentEnable ("RingNetwork", LOG_LEVEL_WARN);
      LogComponentEnable ("LteSquaredGridNetwork", LOG_LEVEL_WARN);

      LogComponentEnable ("RoutingInfo", LOG_LEVEL_ALL);
      LogComponentEnable ("RoutingInfo", LOG_PREFIX_TIME);
      LogComponentEnable ("OpenFlowEpcController", LOG_LEVEL_ALL);
      LogComponentEnable ("OpenFlowEpcController", LOG_PREFIX_TIME);
      LogComponentEnable ("RingController", LOG_LEVEL_ALL);
      LogComponentEnable ("RingController", LOG_PREFIX_TIME);

      LogComponentEnable ("HttpClient", LOG_LEVEL_WARN);
      LogComponentEnable ("HttpServer", LOG_LEVEL_WARN);
      LogComponentEnable ("VoipClient", LOG_LEVEL_WARN);
      LogComponentEnable ("VoipServer", LOG_LEVEL_WARN);
      LogComponentEnable ("StoredVideoClient", LOG_LEVEL_WARN);
      LogComponentEnable ("StoredVideoServer", LOG_LEVEL_WARN);
      LogComponentEnable ("RealTimeVideoClient", LOG_LEVEL_WARN);
      LogComponentEnable ("RealTimeVideoServer", LOG_LEVEL_WARN);
      LogComponentEnable ("TrafficManager", LOG_LEVEL_ALL);
      LogComponentEnable ("TrafficHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("EpcApplication", LOG_LEVEL_ALL);
    }
}

