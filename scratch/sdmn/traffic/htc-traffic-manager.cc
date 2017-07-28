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

#include "htc-traffic-manager.h"
#include "../sdran/sdran-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HtcTrafficManager");
NS_OBJECT_ENSURE_REGISTERED (HtcTrafficManager);

HtcTrafficManager::HtcTrafficManager ()
  : m_ctrlApp (0),
    m_imsi (0),
    m_cellId (0),
    m_defaultTeid (0)
{
  NS_LOG_FUNCTION (this);
}

HtcTrafficManager::~HtcTrafficManager ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
HtcTrafficManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HtcTrafficManager")
    .SetParent<Object> ()
    .AddConstructor<HtcTrafficManager> ()
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get application "
                   "inter-arrival start times.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=180.0]"),
                   MakePointerAccessor (&HtcTrafficManager::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Continuously restart applications after stop events.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&HtcTrafficManager::m_restartApps),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
HtcTrafficManager::AddSdmnClientApp (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save the application pointer.
  std::pair<Ptr<SdmnClientApp>, Time> entry (app, Time ());
  std::pair<AppTimeMap_t::iterator, bool> ret;
  ret = m_appTable.insert (entry);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Can't save application pointer " << app);
    }

  // Connect to AppStop and AppError trace sources.
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&HtcTrafficManager::NotifyAppStop, this));
  app->TraceConnectWithoutContext (
    "AppError", MakeCallback (&HtcTrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this application (after the 1st sec).
  Time firstTry = Seconds (1) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (firstTry, &HtcTrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("First start attempt for app " << app->GetAppName () <<
               " will occur at " << firstTry.GetSeconds () << "s.");
}

void
HtcTrafficManager::SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
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
  for (aIt = m_appTable.begin (); aIt != m_appTable.end (); ++aIt)
    {
      Ptr<SdmnClientApp> app = aIt->first;
      app->SetTeid (m_defaultTeid);

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
      NS_LOG_INFO ("App " << app->GetAppName () <<
                   " over bearer teid " << app->GetTeid ());
    }
}

void
HtcTrafficManager::SetImsi (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_imsi = value;
}

void
HtcTrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_poissonRng = 0;
  m_ctrlApp = 0;
  m_appTable.clear ();
}

void
HtcTrafficManager::AppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");
  NS_LOG_INFO ("Attempt to start app " << app->GetNameTeid ());

  // Different from the MTC applications, before requesting for resources and
  // starting the application, let's set the next start attempt for this same
  // application. Depending on the next start attempt time, the application
  // will be forced to stops itself to avoid overlapping operations.
  SetNextAppStartTry (app);

  bool authorized = true;
  if (app->GetTeid () != m_defaultTeid)
    {
      // No resource request for traffic over default bearer.
      authorized = m_ctrlApp->DedicatedBearerRequest (
          app->GetEpsBearer (), m_imsi, m_cellId, app->GetTeid ());
    }

  // No retries are performed for a non-authorized traffic.
  if (authorized)
    {
      // Schedule the application start for +1 second.
      Simulator::Schedule (Seconds (1), &SdmnClientApp::Start, app);
      NS_LOG_INFO ("App " << app->GetNameTeid () << " will start in +1 sec.");
      if (app->GetMaxOnTime ().IsZero () == false)
        {
          NS_LOG_INFO ("App maximum duration set to " <<
                       app->GetMaxOnTime ().GetSeconds () << "s.");
        }
    }
}

void
HtcTrafficManager::NotifyAppStop (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // No resource release for traffic over default bearer.
  uint32_t appTeid = app->GetTeid ();
  if (appTeid != m_defaultTeid)
    {
      // Schedule the resource release procedure for +1 second.
      Simulator::Schedule (
        Seconds (1), &SdranController::DedicatedBearerRelease,
        m_ctrlApp, app->GetEpsBearer (), m_imsi, m_cellId, appTeid);
    }

  // Schedule the next start attempt for this application,
  // ensuring at least 2 seconds from now.
  if (m_restartApps)
    {
      Time now = Simulator::Now ();
      Time nextTry = GetNextAppStartTry (app) - now;
      if (nextTry < Seconds (2))
        {
          nextTry = Seconds (2);
          NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
                       " delayed to +2s.");
        }
      Simulator::Schedule (nextTry, &HtcTrafficManager::AppStartTry,
                           this, app);
    }
}

void
HtcTrafficManager::SetNextAppStartTry (Ptr<SdmnClientApp> app)
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
  double rngValue = std::abs (m_poissonRng->GetValue ());
  Time nextTry = Seconds (std::max (8.0, rngValue));

  // Save the absolute time into application table.
  AppTimeMap_t::iterator it = m_appTable.find (app);
  NS_ASSERT_MSG (it != m_appTable.end (), "Can't find app " << app);
  it->second = Simulator::Now () + nextTry;
  NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
               " should occur at " << it->second.GetSeconds () << "s.");

  // Set the maximum traffic duration.
  app->SetAttribute ("MaxOnTime", TimeValue (nextTry - Seconds (5.0)));
}

Time
HtcTrafficManager::GetNextAppStartTry (Ptr<SdmnClientApp> app) const
{
  NS_LOG_FUNCTION (this << app);

  AppTimeMap_t::const_iterator it = m_appTable.find (app);
  NS_ASSERT_MSG (it != m_appTable.end (), "Can't find app " << app);
  return it->second;
}

};  // namespace ns3