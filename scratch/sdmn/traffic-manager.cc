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
#include "epc-controller.h"

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
    .AddAttribute ("Controller",
                   "The OpenFlow EPC controller.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficManager::m_controller),
                   MakePointerChecker<EpcController> ())
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
TrafficManager::AddSdmnClientApp (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save application and configure stop callback
  m_apps.push_back (app);
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this app.
  // Wait at least 2 seconds for simulation initial setup.
  Time start = Seconds (2) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (start, &TrafficManager::AppStartTry, this, app);
  NS_LOG_DEBUG ("First start try for app " << app->GetAppName () <<
                " at user " << m_imsi << " with teid " << app->GetTeid () <<
                " will occur at " <<
                (Simulator::Now () + start).GetSeconds ());
}

void
TrafficManager::AppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);
  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");

  bool authorized = true;
  uint32_t appTeid = app->GetTeid ();

  NS_LOG_INFO ("Attemp to start traffic for bearer " << appTeid);
  if (appTeid != m_defaultTeid)
    {
      // No resource request for traffic over default bearer.
      authorized = m_controller->RequestDedicatedBearer (
          app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }

  //
  // Before starting the traffic, let's set the next start attempt for this
  // same application. We will use this interval to limit the current traffic
  // duration, to avoid overlapping traffic which would not be possible in
  // current implementation. Doing this, we can respect almost all
  // inter-arrival times for the Poisson process and reuse application and
  // bearers along the simulation. However, we must ensure a minimum interval
  // between start attempts so the network can prepare for application traffic
  // and release resources after that. In this implementation, we are using 1
  // second for traffic preparation, at least 3 seconds for traffic duration
  // and 4 seconds for release procedures. See the timeline bellow for better
  // understanding. Note that in current implementation, no retries are
  // performed for a non-authorized traffic.
  //
  //  Now  Now+1s              t-4s   t-3s   t-2s   t-1s    t
  //   |------|------ ... ------|------|------|------|------|---> time
  //   A      B                 C      D      E      F      G
  //           <-- MaxOnTime -->
  //           (at least 3 secs)
  //
  // A: This AppStartTry (install rules into switches)
  // B: Application starts (traffic begin)
  // C: Traffic generation ends (still have packets on the wire)
  // D: Application stops (fire dump statistics)
  // E: Resource release (remove rules from switches)
  // F: The socket will be effectivelly closed (Note 1)
  // G: Next AppStartTry (following Poisson proccess)
  //
  // Note 1: In this implementation the TCP socket maximum segment lifetime
  // attribute was adjusted to 1 second, which will allow the TCP state machine
  // to change from TIME_WAIT state to CLOSED state 2 seconds after the close
  // procedure.
  //
  Time nextStartTry = Seconds (std::max (8.0, m_poissonRng->GetValue ()));
  if (authorized)
    {
      // Set the maximum traffic duration.
      Time duration = nextStartTry - Seconds (5);
      app->SetAttribute ("MaxOnTime", TimeValue (duration));

      Simulator::Schedule (Seconds (1), &SdmnClientApp::Start, app);
      NS_LOG_DEBUG ("App " << app->GetAppName () << " at user " << m_imsi <<
                    " with teid " << app->GetTeid () << " will start at " <<
                    (Simulator::Now () + Seconds (1)).GetSeconds () <<
                    " with maximum duration of " << duration.GetSeconds ());
    }

  Simulator::Schedule (nextStartTry, &TrafficManager::AppStartTry, this, app);
  NS_LOG_DEBUG ("Next start try for app " << app->GetAppName () <<
                " at user " << m_imsi << " with teid " << app->GetTeid () <<
                " will occur at " <<
                (Simulator::Now () + nextStartTry).GetSeconds ());
}

void
TrafficManager::NotifyAppStop (Ptr<const SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // No resource release for traffic over default bearer.
      // Schedule the release for 1 second after application stops.
      Simulator::Schedule (
        Seconds (1), &EpcController::ReleaseDedicatedBearer,
        m_controller, app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }
}

void
TrafficManager::SessionCreatedCallback (
  uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address pgwAddr,
  BearerList_t bearerList)
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
  std::vector<Ptr<SdmnClientApp> >::iterator appIt;
  for (appIt = m_apps.begin (); appIt != m_apps.end (); appIt++)
    {
      Ptr<SdmnClientApp> app = *appIt;

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
      NS_LOG_INFO ("Application " << app->GetAppName () << " [" << imsi <<
                   "@" << cellId << "] set with teid " << app->GetTeid ());
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
