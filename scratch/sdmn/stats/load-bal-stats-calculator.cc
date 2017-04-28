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

#include <iomanip>
#include <iostream>
#include "load-bal-stats-calculator.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LoadBalStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (LoadBalStatsCalculator);

LoadBalStatsCalculator::LoadBalStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::EpcController/LoadBalFinished",
    MakeCallback (&LoadBalStatsCalculator::NotifyLoadBalFinished, this));
}

LoadBalStatsCalculator::~LoadBalStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LoadBalStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LoadBalStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<LoadBalStatsCalculator> ()
    .AddAttribute ("LbmStatsFilename",
                   "Filename for EPC P-GW load balancing statistics.",
                   StringValue ("pgw-load-balancing.log"),
                   MakeStringAccessor (&LoadBalStatsCalculator::m_lbmFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
LoadBalStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_lbmWrapper = 0;
}

void
LoadBalStatsCalculator::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("LbmStatsFilename", StringValue (prefix + m_lbmFilename));

  m_lbmWrapper = Create<OutputStreamWrapper> (m_lbmFilename, std::ios::out);
  *m_lbmWrapper->GetStream ()
  << fixed << setprecision (4) << boolalpha
  << left
  << setw (11) << "Time(s)"
  << setw (13) << "Status"
  << setw (11) << "ListOfBearers"
  << std::endl;

  Object::NotifyConstructionCompleted ();
}

void
LoadBalStatsCalculator::NotifyLoadBalFinished (
  std::string context, bool status, RoutingInfoList_t bearerList)
{
  NS_LOG_FUNCTION (this << context << status);

  *m_lbmWrapper->GetStream ()
  << left
  << setw (10) << Simulator::Now ().GetSeconds () << " "
  << setw (12) << status                          << " ";

  // Print the list of TEIDs moved from one switch to another.
  RoutingInfoList_t::iterator it;
  for (it = bearerList.begin (); it != bearerList.end (); )
    {
      *m_lbmWrapper->GetStream () << (*it)->GetTeid ();
      if (++it != bearerList.end ())
        {
          *m_lbmWrapper->GetStream () << ",";
        }
    }
  *m_lbmWrapper->GetStream () << std::endl;
}

} // Namespace ns3
