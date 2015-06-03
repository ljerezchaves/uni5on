/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include "ns3/qos-stats-calculator.h"
#include "stats-calculator.h"
#include "output-logger.h"
#include <iomanip>
#include <iostream>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OutputLogger");
NS_OBJECT_ENSURE_REGISTERED (OutputLogger);

OutputLogger::OutputLogger ()
  : m_commonPrefix ("")
{
  NS_LOG_FUNCTION (this);

  m_admissionStats = CreateObject<AdmissionStatsCalculator> ();

  // Connecting to the trace sources
  Config::ConnectWithoutContext (
    "/NodeList/*/ApplicationList/*/$ns3::EpcApplication/AppStats",
    MakeCallback (&OutputLogger::ReportAppStats, this));

  m_admissionStats->TraceConnectWithoutContext ("AdmStats",
    MakeCallback (&OutputLogger::ReportAdmStats, this));
  
  m_admissionStats->TraceConnectWithoutContext ("BrqStats",
    MakeCallback (&OutputLogger::ReportBrqStats, this));

  Config::ConnectWithoutContext (
    "/Names/OpenFlowNetwork/EpcStats",
    MakeCallback (&OutputLogger::ReportEpcStats, this));
  
  Config::ConnectWithoutContext (
    "/Names/OpenFlowNetwork/PgwStats",
    MakeCallback (&OutputLogger::ReportPgwStats, this));
  
  Config::ConnectWithoutContext (
    "/Names/OpenFlowNetwork/BwdStats",
    MakeCallback (&OutputLogger::ReportBwdStats, this));

  Config::ConnectWithoutContext (
    "/Names/OpenFlowNetwork/SwtStats",
    MakeCallback (&OutputLogger::ReportSwtStats, this));

  Config::ConnectWithoutContext (
    "/Names/InternetNetwork/WebStats",
    MakeCallback (&OutputLogger::ReportWebStats, this));
}

OutputLogger::~OutputLogger ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
OutputLogger::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OutputLogger")
    .SetParent<Object> ()
    .AddConstructor<OutputLogger> ()
    .AddAttribute ("AppStatsFilename",
                   "Filename for application QoS statistics.",
                   StringValue ("app_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_appStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("EpcStatsFilename",
                   "Filename for EPC QoS statistics.",
                   StringValue ("epc_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_epcStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("PgwStatsFilename",
                   "Filename for packet gateway traffic statistics.",
                   StringValue ("pgw_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_pgwStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("SwtStatsFilename",
                   "FilName for flow table entries statistics.",
                   StringValue ("swt_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_swtStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("AdmStatsFilename",
                   "Filename for bearer admission control statistics.",
                   StringValue ("adm_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_admStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("WebStatsFilename",
                   "Filename for internet queue statistics.",
                   StringValue ("web_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_webStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BwdStatsFilename",
                   "Filename for network bandwidth statistics.",
                   StringValue ("bwd_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_bwdStatsFilename),
                   MakeStringChecker ())
    .AddAttribute ("BrqStatsFilename",
                   "Filename for bearer request statistics.",
                   StringValue ("brq_stats.txt"),
                   MakeStringAccessor (&OutputLogger::m_brqStatsFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
OutputLogger::SetCommonPrefix (std::string prefix)
{
  ostringstream ss;
  ss << prefix << RngSeedManager::GetRun () << "-";
  m_commonPrefix = ss.str ();
}

void
OutputLogger::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_admissionStats = 0;
}

std::string
OutputLogger::GetCompleteName (std::string name)
{
  return m_commonPrefix + name;
}

void 
OutputLogger::ReportAppStats (std::string description, uint32_t teid,
                              Ptr<const QosStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this << teid);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_appStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (12) << "Active (s)"
              << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"
              << setw (9)  << "Rx Pkts"
              << setw (12) << "Loss ratio"
              << setw (6)  << "Losts"
              << setw (10) << "Rx Bytes"
              << setw (8)  << "Throughput (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << description
          << setw (6)  << teid
          << setw (12) << stats->GetActiveTime ().GetSeconds ()
          << setw (12) << fixed << stats->GetRxDelay ().GetSeconds () * 1000
          << setw (12) << fixed << stats->GetRxJitter ().GetSeconds () * 1000
          << setw (9)  << fixed << stats->GetRxPackets ()
          << setw (12) << fixed << stats->GetLossRatio ()
          << setw (6)  << fixed << stats->GetLostPackets ()
          << setw (10) << fixed << stats->GetRxBytes ()
          << setw (8)  << fixed << (double)(stats->GetRxThroughput ().GetBitRate ()) / 1024
          << std::endl;
  outFile.close ();
}

void 
OutputLogger::ReportEpcStats (std::string description, uint32_t teid,
                              Ptr<const QosStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this << teid);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_epcStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (12) << "Active (s)"
              << setw (12) << "Delay (ms)"
              << setw (12) << "Jitter (ms)"
              << setw (9)  << "Rx Pkts"
              << setw (12) << "Loss ratio"
              << setw (7)  << "Losts"
              << setw (7)  << "Meter"
              << setw (7)  << "Queue"
              << setw (10) << "Rx Bytes"
              << setw (8)  << "Throughput (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << description
          << setw (6)  << teid
          << setw (12) << stats->GetActiveTime ().GetSeconds ()
          << setw (12) << fixed << stats->GetRxDelay ().GetSeconds () * 1000
          << setw (12) << fixed << stats->GetRxJitter ().GetSeconds () * 1000
          << setw (9)  << fixed << stats->GetRxPackets ()
          << setw (12) << fixed << stats->GetLossRatio ()
          << setw (7)  << fixed << stats->GetLostPackets ()
          << setw (7)  << fixed << stats->GetMeterDrops ()
          << setw (7)  << fixed << stats->GetQueueDrops ()
          << setw (10) << fixed << stats->GetRxBytes ()
          << setw (8)  << fixed << (double)(stats->GetRxThroughput ().GetBitRate ()) / 1024
          << std::endl;
  outFile.close ();
}

void
OutputLogger::ReportAdmStats (Ptr<const AdmissionStatsCalculator> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_admStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (27) << "GBR"
              << setw (27) << "Non-GBR"
              << std::endl
              << setw (12) << " "
              << setw (9) << "Requests"
              << setw (9)  << "Blocks"
              << setw (9)  << "Ratio"
              << setw (9)  << "Requests"
              << setw (9)  << "Blocks"
              << setw (9)  << "Ratio"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (9)  << stats->GetGbrRequests ()
          << setw (9)  << stats->GetGbrBlocked ()
          << setw (9)  << stats->GetGbrBlockRatio ()
          << setw (9)  << stats->GetNonGbrRequests ()
          << setw (9)  << stats->GetNonGbrBlocked ()
          << setw (9)  << stats->GetNonGbrBlockRatio ()
          << std::endl;
  outFile.close ();
}

void 
OutputLogger::ReportPgwStats (DataRate downTraffic, DataRate upTraffic)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_pgwStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (17) << "Downlink (kbps)"
              << setw (14) << "Uplink (kbps)"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << (double)downTraffic.GetBitRate () / 1024
          << setw (14) << (double)upTraffic.GetBitRate () / 1024
          << std::endl;
  outFile.close ();
}

void 
OutputLogger::ReportSwtStats (std::vector<uint32_t> teid)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;
  size_t switches = teid.size ();

  std::string name = GetCompleteName (m_swtStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)" 
              << setw (10)  << "Pgw"
              << setw (48) << "eNB switches"
              << std::endl
              << setw (12) << " "
              << setw (10) << " ";
      for (size_t i = 1; i < switches; i++)
        {
          outFile << setw (5) << i;
        }
      outFile << setw (12) << "Average" << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  double enbSum = 0;
  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ();
  
  std::vector<uint32_t>::iterator it = teid.begin ();
  outFile << setw (10) << *it;
  for (it++; it != teid.end (); it++)
    {
      outFile << setw (5) << *it;
      enbSum += *it;
    }
  
  outFile << setw (12) << (enbSum / (switches - 1)) << std::endl;
  outFile.close ();
}

void
OutputLogger::ReportWebStats (Ptr<const Queue> downlink, 
                              Ptr<const Queue> uplink)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_webStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s) " 
              << setw (48) << "Downlink"
              << setw (48) << "Uplink"
              << std::endl
              << setw (12) << " " 
              << setw (12) << "Pkts"
              << setw (12) << "Bytes"
              << setw (12) << "Pkts drop"
              << setw (12) << "Bytes drop"
              << setw (12) << "Pkts"
              << setw (12) << "Bytes"
              << setw (12) << "Pkts drop"
              << setw (12) << "Bytes drop"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (12) << downlink->GetTotalReceivedPackets ()
          << setw (12) << downlink->GetTotalReceivedBytes ()
          << setw (12) << downlink->GetTotalDroppedPackets ()
          << setw (12) << downlink->GetTotalDroppedBytes ()
          << setw (12) << uplink->GetTotalReceivedPackets ()
          << setw (12) << uplink->GetTotalReceivedBytes ()
          << setw (12) << uplink->GetTotalDroppedPackets ()
          << setw (12) << uplink->GetTotalDroppedBytes ()
          << std::endl;
  outFile.close ();
}

void
OutputLogger::ReportBwdStats (std::vector<BandwidthStats_t> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_bwdStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)";
      std::vector<BandwidthStats_t>::iterator it;
      for (it = stats.begin (); it != stats.end (); it++)        
        {
          BandwidthStats_t entry = *it;
          SwitchPair_t pair = entry.first;
          outFile << setw (1) << pair.first
                  << "-" 
                  << setw (7) << pair.second;
        }
      outFile << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ();
  std::vector<BandwidthStats_t>::iterator it;
  for (it = stats.begin (); it != stats.end (); it++)        
    {
      BandwidthStats_t entry = *it;
      outFile << setw (5) << fixed << entry.second << " ";
    }
  outFile << std::endl;
  outFile.close ();
}

void
OutputLogger::ReportBrqStats (Ptr<const BearerRequestStats> stats)
{
  NS_LOG_FUNCTION (this);
  static bool firstWrite = true;

  std::string name = GetCompleteName (m_brqStatsFilename);
  std::ofstream outFile;
  if (firstWrite == true)
    {
      outFile.open (name.c_str ());
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
      firstWrite = false;
      outFile << left 
              << setw (12) << "Time (s)"
              << setw (17) << "Description"
              << setw (6)  << "TEID"
              << setw (10) << "Accepted?"
              << setw (12) << "Down (kbps)"
              << setw (10) << "Up (kbps)"
              << setw (40) << "Routing paths"
              << std::endl;
    }
  else
    {
      outFile.open (name.c_str (), std::ios_base::app);
      if (!outFile.is_open ())
        {
          NS_LOG_ERROR ("Can't open file " << name);
          return;
        }
    }

  outFile << left
          << setw (12) << Simulator::Now ().GetSeconds ()
          << setw (17) << stats->GetDescription ()
          << setw (6)  << stats->GetTeid ()
          << setw (10) << (stats->IsAccepted () ? "yes" : "no")
          << setw (12) << (double)(stats->GetDownDataRate ().GetBitRate ()) / 1024
          << setw (10) << (double)(stats->GetUpDataRate ().GetBitRate ()) / 1024
          << setw (40) << stats->GetRoutingPaths ()
          << std::endl;
  outFile.close ();
}

};  // namespace ns3

