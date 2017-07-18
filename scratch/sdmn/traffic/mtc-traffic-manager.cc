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
#define NS_LOG_APPEND_CONTEXT \
  { std::clog << "[User " << m_imsi << " at cell " << m_cellId << "] "; }

#include "mtc-traffic-manager.h"
#include "../sdran/sdran-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MtcTrafficManager");
NS_OBJECT_ENSURE_REGISTERED (MtcTrafficManager);

MtcTrafficManager::MtcTrafficManager ()
  : m_ctrlApp (0),
    m_imsi (0),
    m_cellId (0),
    m_defaultTeid (0)
{
  NS_LOG_FUNCTION (this);
}

MtcTrafficManager::~MtcTrafficManager ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MtcTrafficManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MtcTrafficManager")
    .SetParent<Object> ()
    .AddConstructor<MtcTrafficManager> ()
    .AddAttribute ("PoissonInterArrival",
                   "An exponential random variable used to get application "
                   "inter-arrival start times.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=20.0]"),
                   MakePointerAccessor (&MtcTrafficManager::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Continuously restart applications after stop events.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MtcTrafficManager::m_restartApps),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
MtcTrafficManager::AddSdmnClientApp (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  // Save the application pointer.
  std::pair<AppSet_t::iterator, bool> ret;
  ret = m_appTable.insert (app);
  if (ret.second == false)
    {
      NS_FATAL_ERROR ("Can't save application pointer " << app);
    }

  // Connect to AppStop and AppError trace sources.
  app->TraceConnectWithoutContext (
    "AppStop", MakeCallback (&MtcTrafficManager::NotifyAppStop, this));
  app->TraceConnectWithoutContext (
    "AppError", MakeCallback (&MtcTrafficManager::NotifyAppStop, this));

  // Schedule the first start attempt for this application (after the 1st sec).
  Time firstTry = Seconds (1) + Seconds (std::abs (m_poissonRng->GetValue ()));
  Simulator::Schedule (firstTry, &MtcTrafficManager::AppStartTry, this, app);
  NS_LOG_INFO ("First start attempt for app " << app->GetAppName () <<
               " will occur at " << firstTry.GetSeconds () << "s.");
}

void
MtcTrafficManager::SessionCreatedCallback (uint64_t imsi, uint16_t cellId,
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
  AppSet_t::iterator aIt;
  for (aIt = m_appTable.begin (); aIt != m_appTable.end (); ++aIt)
    {
      Ptr<SdmnClientApp> app = *aIt;
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
MtcTrafficManager::SetImsi (uint64_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_imsi = value;
}

void
MtcTrafficManager::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_poissonRng = 0;
  m_ctrlApp = 0;
  m_appTable.clear ();
}

void
MtcTrafficManager::AppStartTry (Ptr<SdmnClientApp> app)
{
  NS_LOG_FUNCTION (this << app);

  NS_ASSERT_MSG (!app->IsActive (), "Can't start an active application.");
  NS_LOG_INFO ("Attempt to start app " << app->GetNameTeid ());

  // Different from the HTC applications, we don't set the next start for MTC
  // applications. We will wait until the application stops by itself, and then
  // we use the Poisson inter arrival RNG to get the next start time.
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
    }
}

void
MtcTrafficManager::NotifyAppStop (Ptr<SdmnClientApp> app)
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
      double rngValue = std::abs (m_poissonRng->GetValue ());
      Time nextTry = Seconds (std::max (2.0, rngValue));
      NS_LOG_INFO ("Next start try for app " << app->GetNameTeid () <<
                   " will occur at " << nextTry.GetSeconds () << "s.");
      Simulator::Schedule (nextTry, &MtcTrafficManager::AppStartTry,
                           this, app);
    }
}

};  // namespace ns3
