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
  ;
  return tid;
}


void
TrafficManager::AddEpcApplication (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save application and configure stop callback
  m_apps.push_back (app);
  app->SetStopCallback (MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Check for disabled application type
  if ( (!m_httpEnable    && app->GetInstanceTypeId () == HttpClient::GetTypeId ())
    || (!m_voipEnable    && app->GetInstanceTypeId () == VoipClient::GetTypeId ())
    || (!m_stVideoEnable && app->GetInstanceTypeId () == StoredVideoClient::GetTypeId ())
    || (!m_rtVideoEnable && app->GetInstanceTypeId () == RealTimeVideoClient::GetTypeId ()))
    {
      // This application is disable.
      return;
    }

  // Schedule the first start attemp for this app (wait at least 1 second)
  Time startTime = Seconds (1) + Seconds (std::abs (m_startRng->GetValue ()));
  Simulator::Schedule (startTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::AppStartTry (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);

  bool authorized = m_controller->NotifyAppStart (app); // FIXME
  if (authorized)
    {
      // ResetEpcStatistics.
      // NOTE: This teid approach only works because we currently have a single
      // application at each bearer/tunnel. If we would like to aggregate
      // traffic from several applications into same bearer we will need to
      // revise this.
      m_network->ResetEpcStatistics (app->GetTeid ());
      app->Start ();
    }
  else
    {
      // Reschedule start attempt for this application
      Time retryTime = Seconds (std::abs (m_startRng->GetValue ()));
      Simulator::Schedule (retryTime, &TrafficManager::AppStartTry, this, app);
    }
}

void
TrafficManager::NotifyAppStop (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);
 
  m_controller->NotifyAppStop (app); // FIXME

  // DumpEpcStatistcs
  // NOTE: Currently, only Voip application needs uplink stats
  bool uplink = (app->GetInstanceTypeId () == VoipClient::GetTypeId ());
  m_network->DumpEpcStatistics (app->GetTeid (), app->GetDescription (), uplink);

  // Schedule next start attempt for this application
  Time idleTime = Seconds (std::abs (m_idleRng->GetValue ()));
  Simulator::Schedule (idleTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_idleRng = 0;
  m_startRng = 0;
  m_controller = 0;
  m_network = 0;
  m_apps.clear ();
}

};  // namespace ns3
