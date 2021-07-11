/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
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

#ifndef SCENARIO_TRAFFIC_H
#define SCENARIO_TRAFFIC_H

#include "../traffic/traffic-helper.h"

namespace ns3 {

/**
 * \ingroup uni5on
 * The helper to create and configure the network traffic for the
 * UNI5ON architecture.
 */
class ScenarioTraffic : public TrafficHelper
{
public:
  ScenarioTraffic ();           //!< Default constructor.
  virtual ~ScenarioTraffic ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase
  void ConfigureHelpers (void);
  void ConfigureUeTraffic (Ptr<UeInfo> ueInfo);

private:
  // Traffic manager attributes.
  Time                        m_fullProbAt;       //!< Time to 100% requests.
  Time                        m_halfProbAt;       //!< Time to 50% requests.
  Time                        m_zeroProbAt;       //!< Time to 0% requests.

  // Application helpers.
  ApplicationHelper           m_autPilotHelper;   //!< Auto-pilot helper.
  ApplicationHelper           m_bikeRaceHelper;   //!< Bicycle race helper.
  ApplicationHelper           m_gameOpenHelper;   //!< Open Arena helper.
  ApplicationHelper           m_gameTeamHelper;   //!< Team Fortress helper.
  ApplicationHelper           m_gpsTrackHelper;   //!< GPS tracking helper.
  ApplicationHelper           m_httpPageHelper;   //!< HTTP page helper.
  ApplicationHelper           m_livVideoHelper;   //!< Live video helper.
  ApplicationHelper           m_recVideoHelper;   //!< Recorded video helper.
  ApplicationHelper           m_voipCallHelper;   //!< VoIP call helper.
};

} // namespace ns3
#endif // SCENARIO_TRAFFIC_H
