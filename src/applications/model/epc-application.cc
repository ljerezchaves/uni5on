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

#include "epc-application.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcApplication");

TypeId
EpcApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcApplication")
    .SetParent<Application> ()
    .AddConstructor<EpcApplication> ()
  ;
  return tid;
}

EpcApplication::EpcApplication()
{
  NS_LOG_FUNCTION (this);
  m_qosStats = Create<QosStatsCalculator> ();
}

EpcApplication::~EpcApplication()
{

}

Ptr<const QosStatsCalculator>
EpcApplication::GetQosStats (void) const
{
  return m_qosStats;
}

void
EpcApplication::ResetQosStats ()
{
  NS_LOG_FUNCTION (this);
  m_qosStats->ResetCounters ();
}

void
EpcApplication::Start ()
{
}

void
EpcApplication::SetStopCallback (StopCb_t cb)
{
  NS_LOG_FUNCTION (this);
  m_stopCb = cb;
}

void
EpcApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_qosStats = 0;
  m_stopCb = MakeNullCallback<void, Ptr<EpcApplication> > ();
  Application::DoDispose ();
}

} // namespace ns3
