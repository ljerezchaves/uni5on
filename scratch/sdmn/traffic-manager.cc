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
#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[User " << m_imsi << " at cell " << m_cellId << "] "; }

#include "traffic-manager.h"
#include "sdran-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficManager");
NS_OBJECT_ENSURE_REGISTERED (TrafficManager);

TrafficManager::TrafficManager ()
  : m_ctrlApp (0),
    m_imsi (0),
    m_cellId (0),
    m_defaultTeid (0)
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

  // Save the application into map.
  std::pair<Ptr<SdmnClientApp>, Time> entry (app, Time ());
  std::pair<AppTimeMap_t::iterator, bool> ret;
  ret = m_appStartTable.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Can't insert application " << app << " into map.");
    }

  // Configure the application stop callback.
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this application (after the 1st sec).
  Time firstTry = Seconds (1) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (firstTry, &TrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("First start attempt for app " << app->GetAppName () <<
               " will occur at " << firstTry.GetSeconds () << "s.");
}

void
TrafficManager::SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
                                        BearerContextList_t bearerList)
{
  NS_LOG_FUNCTION (this);

  // Check the IMSI match for current manager.
  if (imsi != m_imsi)
    {
      return;
    }

  m_cellId = cellId;
  m_ctrlApp = SdranController::GetPointer (cellId);
  m_defaultTeid = bearerList.front ().sgwFteid.teid;

  // For each application, set the corresponding TEID.
  AppTimeMap_t::iterator aIt;
  for (aIt = m_appStartTable.begin (); aIt != m_appStartTable.end (); ++aIt)
    {
      Ptr<SdmnClientApp> app = aIt->first;

      // Using the TFT to match bearers and applications.
      Ptr<EpcTft> tft = app->GetTft ();
      if (tft)
        {
          BearerContextList_t::iterator bIt;
          for (bIt = bearerList.begin (); bIt != bearerList.end (); bIt++)
            {
              if (bIt->tft == tft)
                {
                  app->SetTeid (bIt->sgwFteid.teid);
                }
            }
        }
      else
        {
          // This application uses the default bearer.
          app->SetTeid (m_defaultTeid);
        }
      NS_LOG_INFO ("App " << app->GetAppName () <<
                   " set with teid " << app->GetTeid ());
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
  m_appStartTable.clear ();
}

void
TrafficManager::AppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");
  NS_LOG_INFO ("Attempt to start app " << app->GetNameTeid ());

  // Before requesting for resources and starting the traffic, let's set the
  // next start attempt for this same application. We will use this interval to
  // limit the current traffic duration to avoid overlapping traffic, which
  // would not be possible in current implementation. This is necessary to
  // respect inter-arrival times for the Poisson process and reuse application
  // and bearers along the simulation.
  SetNextAppStartTry (app);

  // No resource request for traffic over default bearer.
  bool authorized = true;
  if (app->GetTeid () != m_defaultTeid)
    {
      authorized = m_ctrlApp->RequestDedicatedBearer (
          app->GetEpsBearer (), m_imsi, m_cellId, app->GetTeid ());
    }

  // No retries are performed for a non-authorized traffic.
  if (authorized)
    {
      // Schedule the application start for +1 second.
      Simulator::Schedule (Seconds (1), &SdmnClientApp::Start, app);
      NS_LOG_INFO ("App " << app->GetNameTeid () <<
                   " will start in +1 sec with maximum duration of " <<
                   app->GetMaxOnTime ().GetSeconds () << "s.");
    }
}

void
TrafficManager::NotifyAppStop (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // No resource release for traffic over default bearer.
  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // Schedule the resource release procedure for +1 second.
      Simulator::Schedule (
        Seconds (1), &SdranController::ReleaseDedicatedBearer,
        m_ctrlApp, app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }

  // Schedule the next start attempt for this application, ensuring at least 2
  // seconds from now.
  Time now = Simulator::Now ();
  Time nextTry = Max (Seconds (2), GetNextAppStartTry (app) - now);
  Simulator::Schedule (nextTry, &TrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("Schedule next start try for app " << app->GetNameTeid () <<
               " in " << nextTry.As (Time::S));
}

void
TrafficManager::SetNextAppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // We must ensure a minimum interval between two consecutive start attempts
  // for the same application. The timeline bellow exposes the time
  // requirements for this.
  //
  //     1sec                               1sec
  //   |------|------ ... ------|-- ... --|------|-- ... --|---> Time
  //   A      B                 C         D      E         F
  // (Now)     <-- MaxOnTime -->                  <- ... ->
  //           (at least 3 secs)               (at least 1sec)
  //
  // A: This is the current AppStartTry. If the resources requested were
  //    accepted, the switch rules are installed and the application is
  //    scheduled to start in +1 second.
  //
  // B: The application effectively starts and the traffic begins.
  //
  // C: The application traffic stops. This event occurs naturally when there's
  //    no more data to be transmitted by the application, or it can be forced
  //    by the MaxOnTime app attribute value. At this point no more data is
  //    sent by the applications, but we may have packets on the fly.
  //
  // D: The application reports itself as stopped. For applications running on
  //    top of UDP sockets, this happens +1 second after C (this is enough time
  //    for packets on the fly to reach their destinations). For applications
  //    on top of TCP sockets, this report will occur when the client socket is
  //    completely closed and the endpoint is deallocated*. This event will
  //    fire dump statistics and the resource release procedure will be
  //    scheduled for +1 second.
  //
  //   *In current configuration, the TCP socket maximum segment lifetime
  //    attribute was adjusted to 1 sec, which will allow the TCP finite state
  //    machine to change from TIME_WAIT to CLOSED state 2 secs after receiving
  //    all pending data on the client side (the client is responsible for
  //    closing the connections).
  //
  // E: The resources are released and switch rules are removed.
  //
  // F: This is the next AppStartTry, following the Poisson process.
  //
  // So, a minimum of 9 seconds must be ensured between two consecutive start
  // attempts to guarantee the following intervals:
  //    A-B: 1 sec
  //    B-C: at least 3 secs of traffic
  //    C-D: 3 secs for stop report, as TCP normally takes 2.x secs
  //    D-E: 1 sec
  //    E-F: at least 1 sec
  //
  Time nextTry = Seconds (std::max (9.0, m_poissonRng->GetValue ()));

  // Save the absolute time into map.
  AppTimeMap_t::iterator it = m_appStartTable.find (app);
  NS_ASSERT_MSG (it != m_appStartTable.end (), "Can't find app " << app);
  it->second = Simulator::Now () + nextTry;
  NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
               " should occur at " << it->second.GetSeconds () << "s.");

  // Set the maximum traffic duration.
  app->SetAttribute ("MaxOnTime", TimeValue (nextTry - Seconds (6.0)));
}

Time
TrafficManager::GetNextAppStartTry (Ptr<SdmnClientApp> app) const
{
  NS_LOG_FUNCTION (this << app);

  AppTimeMap_t::const_iterator it = m_appStartTable.find (app);
  NS_ASSERT_MSG (it != m_appStartTable.end (), "Can't find app " << app);
  return it->second;
}

};  // namespace ns3
