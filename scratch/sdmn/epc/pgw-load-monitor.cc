/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/config.h>
#include <numeric>
#include "pgw-load-monitor.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PgwLoadMonitor");
NS_OBJECT_ENSURE_REGISTERED (PgwLoadMonitor);

PgwLoadMonitor::PgwLoadMonitor ()
  : m_loadBalEnable (false),
    m_avgFlowEntries (0),
    m_avgMeterEntries (0),
    m_avgGroupEntries (0)
{
  NS_LOG_FUNCTION (this);
}

PgwLoadMonitor::~PgwLoadMonitor ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PgwLoadMonitor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PgwLoadMonitor")
    .SetParent<Object> ()
    .AddConstructor<PgwLoadMonitor> ()
    .AddAttribute ("Timeout",
                   "The interval for verifying datapath load.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&PgwLoadMonitor::m_timeout),
                   MakeTimeChecker (Seconds (1)))
    .AddAttribute ("EwmaAlpha",
                   "The EWMA alpha parameter for averaging statistics.",
                   DoubleValue (0.85),
                   MakeDoubleAccessor (&PgwLoadMonitor::m_alpha),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("UpperThreshold",
                   "The upper bound on the number of datapath entries.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (1200),
                   MakeUintegerAccessor (&PgwLoadMonitor::m_upperThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LowerThreshold",
                   "The lower bound on the number of datapath entries.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (400),
                   MakeUintegerAccessor (&PgwLoadMonitor::m_lowerThreshold),
                   MakeUintegerChecker<uint32_t> ())

    .AddTraceSource ("LoadBalancingAdjust",
                     "Trace source indicating when the EPC controller should "
                     "enable or disable the P-GW load balancing mechanism.",
                     MakeTraceSourceAccessor (&PgwLoadMonitor::m_loadBalTrace),
                     "ns3::Boolean::TracedCallback")
  ;
  return tid;
}

void
PgwLoadMonitor::HookSinks (Ptr<OFSwitch13Device> device)
{
  NS_LOG_FUNCTION (this << device);

  device->TraceConnectWithoutContext (
    "FlowEntries", MakeCallback (
      &PgwLoadMonitor::NotifyFlowEntries,
      Ptr<PgwLoadMonitor> (this)));
  device->TraceConnectWithoutContext (
    "MeterEntries", MakeCallback (
      &PgwLoadMonitor::NotifyMeterEntries,
      Ptr<PgwLoadMonitor> (this)));
  device->TraceConnectWithoutContext (
    "GroupEntries", MakeCallback (
      &PgwLoadMonitor::NotifyGroupEntries,
      Ptr<PgwLoadMonitor> (this)));
}

void
PgwLoadMonitor::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
PgwLoadMonitor::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_upperThreshold > 2.1 * m_lowerThreshold,
                 "Ensure that the upper thresholds is at least 2.1 times "
                 "greater than the lower threshold.");

  // Schedule first verify load operation.
  Simulator::Schedule (m_timeout, &PgwLoadMonitor::VerifyLoad, this);

  // Chain up.
  Object::NotifyConstructionCompleted ();
}

void
PgwLoadMonitor::NotifyFlowEntries (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this << oldValue << newValue);

  m_avgFlowEntries = newValue * m_alpha + m_avgFlowEntries * (1 - m_alpha);
}

void
PgwLoadMonitor::NotifyMeterEntries (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this << oldValue << newValue);

  m_avgMeterEntries = newValue * m_alpha + m_avgMeterEntries * (1 - m_alpha);
}

void
PgwLoadMonitor::NotifyGroupEntries (uint32_t oldValue, uint32_t newValue)
{
  NS_LOG_FUNCTION (this << oldValue << newValue);

  m_avgGroupEntries = newValue * m_alpha + m_avgGroupEntries * (1 - m_alpha);
}

void
PgwLoadMonitor::VerifyLoad ()
{
  NS_LOG_FUNCTION (this);

  uint32_t entries = std::round (m_avgFlowEntries) +
    std::round (m_avgMeterEntries) + std::round (m_avgGroupEntries);

  if (!m_loadBalEnable && entries > m_upperThreshold)
    {
      NS_LOG_INFO ("P-GW datapath overload.");
      m_loadBalEnable = true;
      m_loadBalTrace (true);
    }
  else if (m_loadBalEnable && entries < m_lowerThreshold)
    {
      NS_LOG_INFO ("P-GW datapath underload.");
      m_loadBalEnable = false;
      m_loadBalTrace (false);
    }

  // Schedule next verify load operation.
  Simulator::Schedule (m_timeout, &PgwLoadMonitor::VerifyLoad, this);
}

} // Namespace ns3
