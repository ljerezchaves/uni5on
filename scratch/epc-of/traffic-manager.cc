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
  : m_imsi (0)
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
    .AddAttribute ("VoipTraffic",
                   "Enable/Disable VoIP traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_voipEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("RtVideoTraffic",
                   "Enable/Disable real-time video traffic during simulation.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_rtVideoEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("HttpTraffic",
                   "Enable/Disable http traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficManager::m_httpEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("StVideoTraffic",
                   "Enable/Disable stored video traffic during simulation.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficManager::m_stVideoEnable),
                   MakeBooleanChecker ())
    .AddAttribute ("Controller",
                   "The OpenFlow EPC controller.",
                   PointerValue (),
                   MakePointerAccessor (&TrafficManager::m_controller),
                   MakePointerChecker<OpenFlowEpcController> ())
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get application "
                   "inter-arrival start times.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&TrafficManager::m_poissonRng),
                   MakePointerChecker <ExponentialRandomVariable> ())
  ;
  return tid;
}

void
TrafficManager::AddEpcApplication (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save application and configure stop callback
  m_apps.push_back (app);
  app->TraceConnectWithoutContext ("AppStop", 
      MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Check for disabled application type
  if ((!m_httpEnable    && app->GetInstanceTypeId () == HttpClient::GetTypeId ())
   || (!m_voipEnable    && app->GetInstanceTypeId () == VoipClient::GetTypeId ())
   || (!m_stVideoEnable && app->GetInstanceTypeId () == StoredVideoClient::GetTypeId ())
   || (!m_rtVideoEnable && app->GetInstanceTypeId () == RealTimeVideoClient::GetTypeId ()))
    {
      // This application is disable, so we won't schedule first start.
      return;
    }

  // Schedule the first start attempt for this app.
  // Wait at least 2 seconds for simulation initial setup.
  Time startTime = Seconds (2) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (startTime, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::AppStartTry (Ptr<EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);

  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");

  bool authorized = true;

  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // No resource request for traffic over default bearer.
      authorized = m_controller->RequestDedicatedBearer (app->GetEpsBearer (),
                                                         m_imsi, m_cellId, appTeid);
    }

  // Before starting the traffic, let's set the next start attempt for this
  // same application. We will use this interval to limit the current traffic
  // duration, to avoid overlapping traffic which would not be possible. Doing
  // this, we can respect the inter-arrival times for the Poisson process.
  Time startInterval = Seconds (std::max (2.5, m_poissonRng->GetValue ())); 
  Simulator::Schedule (startInterval, &TrafficManager::AppStartTry, this, app);
  NS_LOG_DEBUG ("App " << app->GetAppName () << " at user " << m_imsi 
      << " is starting now (" << Simulator::Now ().GetSeconds () << ")."
      << " Next start will occur in +" << startInterval.GetSeconds ());

  if (authorized)
    {
      // Set the maximum traffic duration, considering an interval of 2 secs.
      Time duration = startInterval - Seconds (2);
      app->SetAttribute ("MaxDurationTime", TimeValue (duration));
      app->Start ();
    }
  
  // NOTE: In current implementation, no retries are performed for for
  //       non-authorized traffic.
}

void
TrafficManager::NotifyAppStop (Ptr<const EpcApplication> app)
{
  NS_LOG_FUNCTION (this << app);

  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // No resource release for traffic over default bearer.
      m_controller->ReleaseDedicatedBearer (app->GetEpsBearer (),
                                            m_imsi, m_cellId, appTeid);
    }

// TODO Remove.
//  // Find our non-constant app pointer to schedule next start attempt
//  std::vector<Ptr<EpcApplication> >::iterator it;
//  for (it = m_apps.begin (); *it != app && it != m_apps.end (); it++)
//    {
//      NS_ASSERT_MSG (it != m_apps.end (), "App not found!");
//    }
//
//  // Schedule next start attempt for this application.
//  // Wait at least 2 seconds for OpenFlow rule management.
//  Time idleTime = Seconds (std::max (2.0, m_poissonRng->GetValue ()));
//  Simulator::Schedule (idleTime, &TrafficManager::AppStartTry, this, *it);
}

void
TrafficManager::ContextCreatedCallback (uint64_t imsi, uint16_t cellId,
  Ipv4Address enbAddr, Ipv4Address sgwAddr, BearerList_t bearerList)
{
  NS_LOG_FUNCTION (this);

  // Check the imsi match for current manager
  if (imsi != m_imsi)
    {
      return;
    }

  m_cellId = cellId;
  m_defaultTeid = bearerList.front ().sgwFteid.teid;

  // For each application, set the corresponding teid
  std::vector<Ptr<EpcApplication> >::iterator appIt;
  for (appIt = m_apps.begin (); appIt != m_apps.end (); appIt++)
    {
      Ptr<EpcApplication> app = *appIt;

      // Using the tft to match bearers and apps
      Ptr<EpcTft> tft = app->GetTft ();
      if (tft)
        {
          BearerList_t::iterator it;
          for (it = bearerList.begin (); it != bearerList.end (); it++)
            {
              if (it->tft == tft)
                {
                  app->m_teid = it->sgwFteid.teid;
                }
            }
        }
      else
        {
          // This application uses the default bearer
          app->m_teid = m_defaultTeid;
        }
      NS_LOG_DEBUG ("Application " << app->GetAppName () << " [" << imsi 
          << "@" << cellId << "] set with teid " << app->GetTeid ());
    }
}

void
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_poissonRng = 0;
  m_controller = 0;
  m_apps.clear ();
}

};  // namespace ns3
