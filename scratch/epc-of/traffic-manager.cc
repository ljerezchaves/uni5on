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

#include "traffic-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficManager");
NS_OBJECT_ENSURE_REGISTERED (TrafficManager);

TrafficManager::TrafficManager ()
{
  NS_LOG_FUNCTION (this);
}

TrafficManager::~TrafficManager ()
{
  NS_LOG_FUNCTION (this);
}

TypeId 
TrafficManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficManager")
    .SetParent<Object> ()
    .AddConstructor<TrafficManager> ()
    .AddAttribute ("HttpTraffic",
                   "Enable/Disable http traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_httpEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("VoipTraffic",
                   "Enable/Disable VoIP traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_voipEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("StVideoTraffic",
                   "Enable/Disable stored video traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_stVideoEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("RtVideoTraffic",
                   "Enable/Disable real-time video traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_rtVideoEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("Controller",
                   "The OpenFlow EPC controller.",
                   PointerValue (),
                   MakePointerAccessor (&TrafficManager::m_controller),
                   MakePointerChecker<OpenFlowEpcController> ())
    .AddAttribute ("Network",
                   "The OpenFlow EPC network.",
                   PointerValue (),
                   MakePointerAccessor (&TrafficManager::m_network),
                   MakePointerChecker<OpenFlowEpcNetwork> ())
    .AddAttribute ("IdleRng",
                   "A random variable used to set idle time.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&TrafficManager::m_idleRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("StartRng",
                   "A random variable used to set start time.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=20.0]"),
                   MakePointerAccessor (&TrafficManager::m_startRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddTraceSource ("AppStats",
                     "Application QoS trace source.",
                     MakeTraceSourceAccessor (&TrafficManager::m_appTrace),
                     "ns3::TrafficManager::QosTracedCallback")
  ;
  return tid;
}

void
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_apps.clear ();
  m_ueDevice = 0;
}

void 
TrafficManager::SetLteUeDevice (Ptr<LteUeNetDevice> ueDevice)
{
  m_ueDevice = ueDevice;
}

void
TrafficManager::AddEpcApplication (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);
  m_apps.push_back (app);

  // Configure stop callback
  app->SetStopCallback (MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Check for application type and traffic enabled/disabled
  if (!m_httpEnable && app->GetInstanceTypeId () == HttpClient::GetTypeId ()) return;
  if (!m_voipEnable && app->GetInstanceTypeId () == VoipClient::GetTypeId ()) return;
  if (!m_stVideoEnable && app->GetInstanceTypeId () == StoredVideoClient::GetTypeId ()) return;
  if (!m_rtVideoEnable && app->GetInstanceTypeId () == RealTimeVideoClient::GetTypeId ()) return;

  // Schedule the first start for this app
  Time startTime = Seconds (5) + Seconds (std::abs (m_startRng->GetValue ()));
  Simulator::Schedule (startTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::NotifyAppStop (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);
  m_controller->NotifyAppStop (app);

  // Dump application and EPC statistics
  bool bidirectional = false;
  if (app->GetInstanceTypeId () == VoipClient::GetTypeId ())
    {
      bidirectional = true;
    }
  uint32_t teid = m_controller->GetTeidFromApplication (app); //FIXME
  DumpAppStatistics (app);
  m_network->DumpEpcStatistics (teid, GetAppDescription (app), bidirectional);

  Time idleTime = Seconds (std::abs (m_idleRng->GetValue ()));
  Simulator::Schedule (idleTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::AppStartTry (Ptr<EpcApplication> app)
{
  if (m_controller->NotifyAppStart (app))
    {
      uint32_t teid = m_controller->GetTeidFromApplication (app);
      m_network->ResetEpcStatistics (teid); // FIXME
      app->ResetQosStats ();
      app->Start ();
    }
  else
    {
      Time retryTime = Seconds (std::abs (m_startRng->GetValue ()));
      Simulator::Schedule (retryTime, &TrafficManager::AppStartTry, this, app);
    }
}

void
TrafficManager::DumpAppStatistics (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);


  uint32_t teid = m_controller->GetTeidFromApplication (app);
  Ptr<RoutingInfo> rInfo = m_controller->GetTeidRoutingInfo (teid);
  Ptr<const QosStatsCalculator> appStats;

  if (app->GetInstanceTypeId () == VoipClient::GetTypeId ())
    {
      Ptr<VoipClient> voipApp = DynamicCast<VoipClient> (app);
      appStats = voipApp->GetServerApp ()->GetQosStats ();
      m_appTrace (GetAppDescription (app) + "ul", teid, appStats);
    }
  appStats = app->GetQosStats ();
  m_appTrace (GetAppDescription (app) + "dl", teid, appStats);
}

std::string 
TrafficManager::GetAppDescription (Ptr<const EpcApplication> app)
{
  std::ostringstream desc;
  desc << app->GetAppName () << " [" << m_ueDevice->GetImsi () << "]";
       //<< "@" << m_ueDevice->GetTargetEnb ()->GetCellId ();
  return desc.str ();
}

};  // namespace ns3
