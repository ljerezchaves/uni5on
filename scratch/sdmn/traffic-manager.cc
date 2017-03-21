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
#include "sdran-controller.h"

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
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get application "
                   "inter-arrival start times.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&TrafficManager::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

void
TrafficManager::AddSdmnClientApp (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save application and configure stop callback.
  m_apps.push_back (app);
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this application.
  // Wait at least 2 seconds for simulation initial setup.
  Time start = Seconds (2) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (start, &TrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("First start try for app " << app->GetAppName () <<
               " at user " << m_imsi << " with teid " << app->GetTeid () <<
               " will occur at " << (Simulator::Now () + start).GetSeconds ());
}

void
TrafficManager::AppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);
  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");

  bool authorized = true;
  uint32_t appTeid = app->GetTeid ();
  NS_LOG_INFO ("Attemp to start traffic for bearer " << appTeid);

  // No resource request for traffic over default bearer.
  if (appTeid != m_defaultTeid)
    {
      authorized = m_ctrlApp->RequestDedicatedBearer (
          app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }

  //
  // Before starting the traffic, let's set the next start attempt for this
  // same application. We will use this interval to limit the current traffic
  // duration, to avoid overlapping traffic which would not be possible in
  // current implementation. Doing this, we can respect almost all
  // inter-arrival times for the Poisson process and reuse application and
  // bearers along the simulation. However, we must ensure a minimum interval
  // between start attempts so the network can prepare for application
  // traffic and release resources after that. See the timeline bellow for
  // better understanding. Note that in current implementation, no retries are
  // performed for a non-authorized traffic.
  //
  //     1sec                                                            time
  //   |------|------ ... ------|------|------|------|------ ... ------|--->
  //   A      B                 C      D             E                 F
  // (Now)     <-- MaxOnTime -->                      <----- ... ----->
  //           (at least 3 secs)                      (at least 2 secs)
  //
  // A: This is the current AppStartTry. If the resources requested were
  //    accepted, the switch rules are installed and the application is
  //    scheduled to start in 1 second.
  //
  // B: The application effectively starts and the traffic begins.
  //
  // C: The application prepares to stop. No more traffic is generated, but we
  //    may have packets on the fly.
  //
  // D: The application effectively stops. This will fire dump statistics and
  //    sockets will be closed. The resource release is scheduled for +2 secs.
  //
  // E: The resources are released and switch rules are removed. Note that at
  //    this point, TCP socket are already effectively in closed, as the TCP
  //    socket maximum segment lifetime attribute was adjusted to 900 ms which
  //    will allow the TCP state machine to change from TIME_WAIT state to
  //    CLOSED state before 2 secs after the close procedure.
  //
  // F: This is the next AppStartTry, following the Poisson process.
  //
  Time nextStartTry = Seconds (std::max (9.0, m_poissonRng->GetValue ()));
  if (authorized)
    {
      // Set the maximum traffic duration.
      Time duration = nextStartTry - Seconds (6.0);
      app->SetAttribute ("MaxOnTime", TimeValue (duration));

      Simulator::Schedule (Seconds (1), &SdmnClientApp::Start, app);
      NS_LOG_INFO ("App " << app->GetAppName () << " at user " << m_imsi <<
                   " with teid " << app->GetTeid () << " will start at " <<
                   (Simulator::Now () + Seconds (1)).GetSeconds () <<
                   " with maximum duration of " << duration.GetSeconds ());
    }

  Simulator::Schedule (nextStartTry, &TrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("Next start try for app " << app->GetAppName () <<
               " at user " << m_imsi << " with teid " << app->GetTeid () <<
               " will occur at " <<
               (Simulator::Now () + nextStartTry).GetSeconds ());
}

void
TrafficManager::NotifyAppStop (Ptr<const SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // No resource release for traffic over default bearer.
  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // Schedule the release for +2 seconds.
      Simulator::Schedule (
        Seconds (2), &SdranController::ReleaseDedicatedBearer,
        m_ctrlApp, app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }
}

void
TrafficManager::SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
                                        BearerContextList_t bearerList)
{
  NS_LOG_FUNCTION (this);

  // Check the imsi match for current manager.
  if (imsi != m_imsi)
    {
      return;
    }

  m_cellId = cellId;
  m_ctrlApp = SdranController::GetPointer (cellId);
  m_defaultTeid = bearerList.front ().sgwFteid.teid;

  // For each application, set the corresponding teid.
  std::vector<Ptr<SdmnClientApp> >::iterator appIt;
  for (appIt = m_apps.begin (); appIt != m_apps.end (); appIt++)
    {
      Ptr<SdmnClientApp> app = *appIt;

      // Using the TFT to match bearers and apps.
      Ptr<EpcTft> tft = app->GetTft ();
      if (tft)
        {
          BearerContextList_t::iterator it;
          for (it = bearerList.begin (); it != bearerList.end (); it++)
            {
              if (it->tft == tft)
                {
                  app->SetTeid (it->sgwFteid.teid);
                }
            }
        }
      else
        {
          // This application uses the default bearer.
          app->SetTeid (m_defaultTeid);
        }
      NS_LOG_INFO ("Application " << app->GetAppName () << " [" << imsi <<
                   "@" << cellId << "] set with teid " << app->GetTeid ());
    }
}

void
TrafficManager::SetImsi (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_imsi = value;
}

void
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_poissonRng = 0;
  m_ctrlApp = 0;
  m_apps.clear ();
}

};  // namespace ns3
