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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#include "traffic-helper.h"
#include "traffic-manager.h"
#include "../infrastructure/radio-network.h"
#include "../metadata/ue-info.h"
#include "../slices/slice-controller.h"
#include "../slices/slice-network.h"
#include "../statistics/flow-stats-calculator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficHelper");
NS_OBJECT_ENSURE_REGISTERED (TrafficHelper);

// Initial port number
uint16_t TrafficHelper::m_port = 10000;

// ------------------------------------------------------------------------ //
TrafficHelper::TrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

TrafficHelper::~TrafficHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
TrafficHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficHelper")
    .SetParent<Object> ()

    // Slice.
    .AddAttribute ("SliceId", "The logical slice identification.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceId::UNKN),
                   MakeEnumAccessor (&TrafficHelper::m_sliceId),
                   MakeEnumChecker (SliceId::MBB, SliceIdStr (SliceId::MBB),
                                    SliceId::MTC, SliceIdStr (SliceId::MTC),
                                    SliceId::TMP, SliceIdStr (SliceId::TMP)))
    .AddAttribute ("SliceCtrl", "The logical slice controller pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_controller),
                   MakePointerChecker<SliceController> ())
    .AddAttribute ("SliceNet", "The logical slice network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_slice),
                   MakePointerChecker<SliceNetwork> ())

    // Infrastructure.
    .AddAttribute ("RadioNet", "The RAN network pointer.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   PointerValue (),
                   MakePointerAccessor (&TrafficHelper::m_radio),
                   MakePointerChecker<RadioNetwork> ())

    // Traffic helper attributes.
    .AddAttribute ("UseOnlyDefaultBearer",
                   "Use only the default EPS bearer for all traffic flows.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&TrafficHelper::m_useOnlyDefault),
                   MakeBooleanChecker ())

    // Traffic manager attributes.
    .AddAttribute ("InterArrival",
                   "A random variable to get inter-arrival start times.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::ExponentialRandomVariable[Mean=120.0]"),
                   MakePointerAccessor (&TrafficHelper::m_poissonRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("RestartApps",
                   "Continuously restart applications after stop events.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (true),
                   MakeBooleanAccessor (&TrafficHelper::m_restartApps),
                   MakeBooleanChecker ())
    .AddAttribute ("StartProb",
                   "The initial probability to start applications.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TrafficHelper::m_initialProb),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("StartTime",
                   "The time to start the applications.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&TrafficHelper::m_startAppsAt),
                   MakeTimeChecker (Time (0)))
    .AddAttribute ("StopTime",
                   "The time to stop the applications.",
                   TimeValue (Time (0)),
                   MakeTimeAccessor (&TrafficHelper::m_stopAppsAt),
                   MakeTimeChecker (Time (0)))
  ;
  return tid;
}

SliceId
TrafficHelper::GetSliceId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceId;
}

void
TrafficHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_radio = 0;
  m_slice = 0;
  m_controller = 0;
  m_poissonRng = 0;
  m_lteHelper = 0;
  m_webNode = 0;
  Object::DoDispose ();
}

void
TrafficHelper::NotifyConstructionCompleted ()
{
  NS_LOG_FUNCTION (this);

  NS_ABORT_MSG_IF (m_sliceId == SliceId::UNKN, "Unknown slice ID.");
  NS_ABORT_MSG_IF (!m_radio, "No radio network.");
  NS_ABORT_MSG_IF (!m_slice, "No slice network.");
  NS_ABORT_MSG_IF (!m_controller, "No slice controller.");

  // Saving pointers.
  m_lteHelper = m_radio->GetLteHelper ();
  m_webNode = m_slice->GetWebNode ();

  // Saving server metadata.
  NS_ASSERT_MSG (m_webNode->GetNDevices () == 2, "Single device expected.");
  Ptr<NetDevice> webDev = m_webNode->GetDevice (1);
  m_webAddr = Ipv4AddressHelper::GetAddress (webDev);
  m_webMask = Ipv4AddressHelper::GetMask (webDev);

  // Configure the traffic manager object factory.
  m_managerFac.SetTypeId (TrafficManager::GetTypeId ());
  m_managerFac.Set ("InterArrival", PointerValue (m_poissonRng));
  m_managerFac.Set ("RestartApps", BooleanValue (m_restartApps));
  m_managerFac.Set ("StartProb", DoubleValue (m_initialProb));
  m_managerFac.Set ("StartTime", TimeValue (m_startAppsAt));
  m_managerFac.Set ("StopTime", TimeValue (m_stopAppsAt));

  // Configure the application helpers
  ConfigureHelpers ();

  for (auto const &imsi : m_slice->GetUeImsiList ())
    {
      Ptr<UeInfo> ueInfo = UeInfo::GetPointer (imsi);

      // Create a traffic manager for this UE.
      Ptr<TrafficManager> ueManager = m_managerFac.Create<TrafficManager> ();
      ueManager->SetController (m_controller);
      ueManager->SetImsi (imsi);
      ueInfo->SetTrafficManager (ueManager);

      // Configure the UE traffic.
      ConfigureUeTraffic (ueInfo);
    }

  Object::NotifyConstructionCompleted ();
}

void
TrafficHelper::InstallAppDedicated (
  Ptr<UeInfo> ueInfo, ApplicationHelper& helper,
  EpsBearer& bearer, EpcTft::PacketFilter& filter)
{
  NS_LOG_FUNCTION (this);

  // When enable, install all applications over the default UE EPS bearer.
  if (m_useOnlyDefault)
    {
      InstallAppDefault (ueInfo, helper);
      return;
    }

  // Create the client and server applications.
  uint16_t port = GetNextPortNo ();
  Ptr<BaseClient> clientApp = helper.Install (
      ueInfo->GetNode (), m_webNode,
      ueInfo->GetAddr (), m_webAddr,
      port, Dscp2Tos (Qci2Dscp (bearer.qci)));
  ueInfo->GetTrafficManager ()->AddClientApplication (clientApp);

  // Setup common packet filter parameters.
  filter.remoteAddress   = m_webAddr;
  filter.remoteMask      = m_webMask;
  filter.remotePortStart = port;
  filter.remotePortEnd   = port;
  filter.localAddress    = ueInfo->GetAddr ();
  filter.localMask       = ueInfo->GetMask ();
  filter.localPortStart  = 0;
  filter.localPortEnd    = 65535;

  // Create the TFT for this bearer.
  Ptr<EpcTft> tft = CreateObject<EpcTft> ();
  tft->Add (filter);

  // Create the dedicated bearer for this traffic.
  uint8_t bid = m_lteHelper->ActivateDedicatedEpsBearer (
      ueInfo->GetDevice (), bearer, tft);
  clientApp->SetEpsBearer (bearer);
  clientApp->SetEpsBearerId (bid);
}

void
TrafficHelper::InstallAppDefault (
  Ptr<UeInfo> ueInfo, ApplicationHelper& helper)
{
  NS_LOG_FUNCTION (this);

  // Get default EPS bearer information for this UE.
  uint8_t bid = ueInfo->GetDefaultBid ();
  EpsBearer bearer = ueInfo->GetEpsBearer (bid);

  // Create the client and server applications.
  uint16_t port = GetNextPortNo ();
  Ptr<BaseClient> clientApp = helper.Install (
      ueInfo->GetNode (), m_webNode,
      ueInfo->GetAddr (), m_webAddr,
      port, Dscp2Tos (Qci2Dscp (bearer.qci)));
  ueInfo->GetTrafficManager ()->AddClientApplication (clientApp);
  clientApp->SetEpsBearer (bearer);
  clientApp->SetEpsBearerId (bid);
}

uint16_t
TrafficHelper::GetNextPortNo ()
{
  NS_ABORT_MSG_IF (m_port == 0xFFFF, "No more ports available for use.");
  return m_port++;
}

} // namespace ns3
