/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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
 * Author: Rafael G. Motta <rafaelgmotta@gmail.com>
 *         Luciano J. Chaves <ljerezchaves@gmail.com>
 */

#include "custom-controller.h"
#include <iomanip>
#include <iostream>

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomController");
NS_OBJECT_ENSURE_REGISTERED (CustomController);

CustomController::CustomController ()
{
  NS_LOG_FUNCTION (this);
}

CustomController::~CustomController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CustomController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomController")
    .SetParent<OFSwitch13Controller> ()
    .AddConstructor<CustomController> ()
    .AddTraceSource ("Request", "The request trace source.",
                     MakeTraceSourceAccessor (&CustomController::m_requestTrace),
                     "ns3::CustomController::RequestTracedCallback")
    .AddTraceSource ("Release", "The release trace source.",
                     MakeTraceSourceAccessor (&CustomController::m_releaseTrace),
                     "ns3::CustomController::ReleaseTracedCallback")
  ;
  return tid;
}

bool
CustomController::DedicatedBearerRequest (Ptr<SvelteClient> app, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << app << imsi);

  m_requestTrace (app->GetTeid (), true);
  return true;
}

bool
CustomController::DedicatedBearerRelease (Ptr<SvelteClient> app, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << app << imsi);

  m_releaseTrace (app->GetTeid ());
  return true;
}

void
CustomController::NotifySwitch (Ptr<OFSwitch13Device> device)
{
  NS_LOG_FUNCTION (this << device);

  m_switchDevice = device;

  // O switch tem 3 tabelas:
  // Tabela 0: Identifica a direção do tráfego (downlink ou uplink)
  // Tabela 1: Faz o mapeamento de portas para o tráfego de downlink.
  // Tabela 2: Faz o mapemamento de portas para o tráfego de uplink,
}

void
CustomController::NotifyClient (uint32_t portNo, Ipv4Address ipAddr)
{
  NS_LOG_FUNCTION (this << portNo << ipAddr);

  std::ostringstream cmd1, cmd2;
  cmd1 << "flow-mod cmd=add,prio=64,table=1"
       << " eth_type=0x800,ip_dst=" << ipAddr
       << " apply:output=" << portNo;
  cmd2 << "flow-mod cmd=add,prio=64,table=0"
       << " eth_type=0x800,in_port=" << portNo
       << " goto:2";
  DpctlSchedule (m_switchDevice->GetDatapathId (), cmd1.str ());
  DpctlSchedule (m_switchDevice->GetDatapathId (), cmd2.str ());
}

void
CustomController::NotifyServer (uint32_t portNo, Ipv4Address ipAddr)
{
  NS_LOG_FUNCTION (this << portNo << ipAddr);

  std::ostringstream cmd1, cmd2;
  cmd1 << "flow-mod cmd=add,prio=64,table=2"
       << " eth_type=0x800,ip_dst=" << ipAddr
       << " apply:output=" << portNo;
  cmd2 << "flow-mod cmd=add,prio=64,table=0"
       << " eth_type=0x800,in_port=" << portNo
       << " goto:1";
  DpctlSchedule (m_switchDevice->GetDatapathId (), cmd1.str ());
  DpctlSchedule (m_switchDevice->GetDatapathId (), cmd2.str ());
}

void
CustomController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_switchDevice = 0;
  OFSwitch13Controller::DoDispose ();
}

} // namespace ns3
