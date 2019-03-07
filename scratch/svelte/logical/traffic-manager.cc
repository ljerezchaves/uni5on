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
#include "slice-controller.h"
#include "../applications/svelte-client.h"
#include "../metadata/routing-info.h"
#include "../metadata/ue-info.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
  std::clog << "[User " << m_imsi << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficManager");
NS_OBJECT_ENSURE_REGISTERED (TrafficManager);

TrafficManager::TrafficManager ()
  : m_ctrlApp (0),
  m_imsi (0),
  m_defaultTeid (0)
{
  NS_LOG_FUNCTION (this);

  // Uniform probability to start applications
  m_startProbRng = CreateObject<UniformRandomVariable> ();
  m_startProbRng->SetAttribute ("Min", DoubleValue (0.0));
  m_startProbRng->SetAttribute ("Max", DoubleValue (1.0));
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
    .AddAttribute ("InterArrival",
                   "An random variable used to get inter-arrival start times.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=120.0]"),
                   MakePointerAccessor (&TrafficManager::m_interArrivalRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Restart applications after stop events.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficManager::m_restartApps),
                   MakeBooleanChecker ())
    .AddAttribute ("StartProb",
                   "The probability to start applications.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TrafficManager::m_startProb),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("StartTime",
                   "The time to start applications.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&TrafficManager::m_startTime),
                   MakeTimeChecker (Seconds (1)))
    .AddAttribute ("StopTime",
                   "The time to stop applications.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&TrafficManager::m_stopTime),
                   MakeTimeChecker (Time (0)))
  ;
  return tid;
}

void
TrafficManager::AddSvelteClient (Ptr<SvelteClient> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save the application pointer.
  std::pair<Ptr<SvelteClient>, Time> entry (app, Time ());
  auto ret = m_timeByApp.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Error when saving application.");

  // Connect to AppStop and AppError trace sources.
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&TrafficManager::NotifyAppStop, this));
  app->TraceConnectWithoutContext (
    "AppError", MakeCallback (&TrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this application.
  Time firstTry = m_startTime;
  firstTry += Seconds (std::abs (m_interArrivalRng->GetValue ()));
  Simulator::Schedule (firstTry, &TrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("First start attempt for app " << app->GetAppName () <<
               " will occur at " << firstTry.GetSeconds () << "s.");
}

void
TrafficManager::NotifySessionCreated (
  uint64_t imsi, BearerCreatedList_t bearerList)
{
  NS_LOG_FUNCTION (this);

  // Check the IMSI match for current manager.
  if (imsi != m_imsi)
    {
      return;
    }

  // Set the default TEID.
  Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);
  m_defaultTeid = ueInfo->GetDefaultTeid ();

  // For each application, set the corresponding TEID.
  for (auto const &ait : m_timeByApp)
    {
      Ptr<SvelteClient> app = ait.first;
      app->SetTeid (ueInfo->GetTeid (app->GetEpsBearerId ()));
      NS_LOG_INFO ("App " << app->GetNameTeid ());
    }
}

void
TrafficManager::SetController (Ptr<SliceController> controller)
{
  NS_LOG_FUNCTION (this << controller);

  m_ctrlApp = controller;
}

void
TrafficManager::SetImsi (uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi);

  m_imsi = imsi;
}

void
TrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_interArrivalRng = 0;
  m_startProbRng = 0;
  m_ctrlApp = 0;
  m_timeByApp.clear ();
  Object::DoDispose ();
}

void
TrafficManager::AppStartTry (Ptr<SvelteClient> app)
{
  NS_LOG_FUNCTION (this << app);

  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");
  NS_LOG_INFO ("Attempt to start app " << app->GetNameTeid ());
  bool authorized = true;

  // Set the absolute time of the next start attempt for this application.
  SetNextAppStartTry (app);

  // Check the stop time before (re)starting the application.
  // Aborted events prevents further start attempts for this application.
  if (!m_stopTime.IsZero () && Simulator::Now () > m_stopTime)
    {
      NS_LOG_INFO ("Application start try aborted by the stop time.");
      return;
    }

  // Check the start probability before (re)starting the application.
  // Aborted events allows further start attempts for this application.
  if (m_startProbRng->GetValue () > m_startProb)
    {
      NS_LOG_INFO ("Application start try aborted by the start probability.");
      return;
    }

  // Request resources only for traffic over dedicated bearers.
  uint32_t teid = app->GetTeid ();
  if (teid != m_defaultTeid)
    {
      authorized = m_ctrlApp->DedicatedBearerRequest (
          app->GetEpsBearer (), m_imsi, app->GetTeid ());

      // Update the active flag for this bearer.
      RoutingInfo::GetPointer (teid)->SetActive (authorized);
    }

  // Check the start authorization before (re)starting the application.
  // Aborted events allows further start attempts for this application.
  if (!authorized)
    {
      NS_LOG_INFO ("Application start try aborted by the authorization flag.");
      return;
    }

  // Schedule the application start for +1 second.
  Simulator::Schedule (Seconds (1), &SvelteClient::Start, app);
  NS_LOG_INFO ("App " << app->GetNameTeid () << " will start in +1sec with " <<
               "max duration set to " << app->GetMaxOnTime ().GetSeconds ());
}

void
TrafficManager::NotifyAppStop (Ptr<SvelteClient> app)
{
  NS_LOG_FUNCTION (this << app);

  // Release resources only for traffic over dedicated bearers.
  uint32_t teid = app->GetTeid ();
  if (teid != m_defaultTeid)
    {
      // Update the active flag for this bearer.
      RoutingInfo::GetPointer (teid)->SetActive (false);

      // Schedule the resource release procedure for +1 second.
      Simulator::Schedule (
        Seconds (1), &SliceController::DedicatedBearerRelease,
        m_ctrlApp, app->GetEpsBearer (), m_imsi, teid);
    }

  // Check the restart application flag.
  if (!m_restartApps)
    {
      NS_LOG_INFO ("Application next start try aborted by the restart flag.");
      return;
    }

  // Schedule the next start attempt for this application,
  // ensuring at least 2 seconds from now.
  Time nextTry = GetNextAppStartTry (app) - Simulator::Now ();
  if (nextTry < Seconds (2))
    {
      nextTry = Seconds (2);
      NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
                   " delayed to +2secs.");
    }
  Simulator::Schedule (nextTry, &TrafficManager::AppStartTry, this, app);
}

void
TrafficManager::SetNextAppStartTry (Ptr<SvelteClient> app)
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
  //    scheduled to start in A + 1 second.
  //
  // B: The application effectively starts and the traffic begins.
  //
  // C: The application traffic stops. This event occurs naturally when there's
  //    no more data to be transmitted by the application, or it can be forced
  //    by the MaxOnTime app attribute value. At this point no more data is
  //    sent by the applications, but we may have pending data on socket
  //    buffers and packets on the fly.
  //
  // D: The application reports itself as stopped. For applications on top of
  //    UDP sockets, this happens at C + 1 second (this is enough time for
  //    packets on the fly to reach their destinations). For applications on
  //    top of TCP sockets, this happens when all pending data on buffers were
  //    successfully transmitted. This event will fire dump statistics and the
  //    resource release procedure will be scheduled for D + 1 second.
  //
  // E: The resources are released and switch rules are removed.
  //
  // F: This is the next AppStartTry, following the Poisson process.
  //
  // So, a minimum of 8 seconds must be ensured between two consecutive start
  // attempts to guarantee the following intervals:
  //    A-B: 1 sec
  //    B-C: at least 3 secs of traffic
  //    C-D: 2 secs for stop report
  //    D-E: 1 sec
  //    E-F: at least 1 sec
  //
  double randValue = std::abs (m_interArrivalRng->GetValue ());
  Time nextTry = Seconds (std::max (8.0, randValue));

  // Save the absolute time into application table.
  auto it = m_timeByApp.find (app);
  NS_ASSERT_MSG (it != m_timeByApp.end (), "Can't find app " << app);
  it->second = Simulator::Now () + nextTry;
  NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
               " should occur at " << it->second.GetSeconds () << "s.");

  // Set the maximum traffic duration, forcing the application to stops itself
  // to avoid overlapping operations.
  app->SetAttribute ("MaxOnTime", TimeValue (nextTry - Seconds (5.0)));
}

Time
TrafficManager::GetNextAppStartTry (Ptr<SvelteClient> app) const
{
  NS_LOG_FUNCTION (this << app);

  auto it = m_timeByApp.find (app);
  NS_ASSERT_MSG (it != m_timeByApp.end (), "Can't find app " << app);
  return it->second;
}

} // namespace ns3
