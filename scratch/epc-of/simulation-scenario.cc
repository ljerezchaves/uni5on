/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#include "simulation-scenario.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulationScenario");
NS_OBJECT_ENSURE_REGISTERED (SimulationScenario);

SimulationScenario::SimulationScenario ()
  : m_opfNetwork (0),
    m_epcHelper (0),
    m_lteNetwork (0),
    m_lteHelper (0),
    m_webNetwork (0),
    m_webHost (0)
{
  NS_LOG_FUNCTION (this);
}

SimulationScenario::~SimulationScenario ()
{
  NS_LOG_FUNCTION (this);
}

void
SimulationScenario::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_opfNetwork = 0;
  m_epcHelper = 0;
  m_lteNetwork = 0;
  m_webNetwork = 0;
  m_lteHelper = 0;
  m_webHost = 0;

  m_epcS1uStats = 0;
}

TypeId
SimulationScenario::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulationScenario")
    .SetParent<Object> ()
    .AddConstructor<SimulationScenario> ()
    .AddAttribute ("PcapTrace",
                   "Enable/Disable simulation PCAP traces.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimulationScenario::m_pcapTrace),
                   MakeBooleanChecker ())
  ;
  return tid;
}

void
SimulationScenario::BuildRingTopology ()
{
  NS_LOG_FUNCTION (this);

  // Create the OpenFlow network
  m_opfNetwork = CreateObject<RingNetwork> ();

  // 7) Create LTE radio access network and build topology
  m_lteNetwork = CreateObject<LteHexGridNetwork> ();
  m_lteHelper = m_lteNetwork->CreateTopology (m_opfNetwork->GetEpcHelper ());

  // 8) Create Internet network and build topology
  m_webNetwork = CreateObject<InternetNetwork> ();
  Names::Add ("InternetNetwork", m_webNetwork);
  m_webHost = m_webNetwork->CreateTopology (m_opfNetwork->GetGatewayNode ());

  // 9) Install applications and traffic manager
  Ptr<TrafficHelper> tfcHelper =
    CreateObject<TrafficHelper> (m_webHost, m_lteHelper, m_opfNetwork->GetControllerApp ());
  tfcHelper->Install (m_lteNetwork->GetUeNodes (),
                      m_lteNetwork->GetUeDevices ());

  // 10) Set up output ofsoftswitch13 logs and ns-3 traces
  EnableTraces ();

  // 11) Creating remaining stats calculator for output dump
  m_epcS1uStats = CreateObject<EpcS1uStatsCalculator> ();
  m_epcS1uStats->SetController (m_opfNetwork->GetControllerApp ());
}

void
SimulationScenario::EnableTraces ()
{
  NS_LOG_FUNCTION (this);

  if (m_pcapTrace)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();

      m_webNetwork->EnablePcap (prefix);
      m_opfNetwork->EnablePcap (prefix);
    }
}

};  // namespace ns3
