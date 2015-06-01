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
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_apps.clear ();
}

void
TrafficManager::SetController (Ptr<OpenFlowEpcController> controller)
{
  m_controller = controller;
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
  Time startTime = Seconds (std::abs (m_startRng->GetValue ()));
  Simulator::Schedule (startTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::NotifyAppStop (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);
  m_controller->NotifyAppStop (app);

  Time idleTime = Seconds (std::abs (m_idleRng->GetValue ()));
  Simulator::Schedule (idleTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::AppStartTry (Ptr<EpcApplication> app)
{
  if (m_controller->NotifyAppStart (app))
    {
      app->ResetQosStats ();
      app->Start ();
    }
  else
    {
      Time retryTime = Seconds (std::abs (m_startRng->GetValue ()));
      Simulator::Schedule (retryTime, &TrafficManager::AppStartTry, this, app);
    }
}

};  // namespace ns3
