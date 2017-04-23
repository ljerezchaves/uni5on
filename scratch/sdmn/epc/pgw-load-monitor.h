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

#ifndef PGW_LOAD_MONITOR_H
#define PGW_LOAD_MONITOR_H

#include <ns3/ofswitch13-device.h>

namespace ns3 {

/**
 * \ingroup sdmnEpc
 * This class monitors the average load of P-GW OpenFlow switch to enable or
 * disable the P-GW TFT load balancing mechanism.
 */
class PgwLoadMonitor : public Object
{
public:
  PgwLoadMonitor ();          //!< Default constructor.
  virtual ~PgwLoadMonitor (); //!< Default destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Hook switch device trace sources to internal stats calculator trace sinks.
   * \param device The OpenFlow switch device to monitor.
   */
  void HookSinks (Ptr<OFSwitch13Device> device);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * \name Trace sinks for monitoring traced values.
   * Trace sinks used to monitor traced values on the OpenFlow switch datapath.
   * \param oldValue The old value.
   * \param newValue The new just updated value.
   */
  //\{
  void NotifyFlowEntries    (uint32_t oldValue, uint32_t newValue);
  void NotifyMeterEntries   (uint32_t oldValue, uint32_t newValue);
  void NotifyGroupEntries   (uint32_t oldValue, uint32_t newValue);
  //\}

  /**
   * Verify the current datapath load and fire the LoadBalancing trace source
   * to enable or disable the mechanism when necessary.
   */
  void VerifyLoad (void);

  Time                  m_timeout;          //!< Update timeout.
  double                m_alpha;            //!< EWMA alpha parameter.
  uint32_t              m_upperThreshold;   //!< Upper bound threshold limit.
  uint32_t              m_lowerThreshold;   //!< Lower bound threshold limit.
  bool                  m_loadBalEnable;    //!< P-GW load balancing enable.
  double                m_avgFlowEntries;   //!< Avg number of flow entries.
  double                m_avgMeterEntries;  //!< Avg number of meter entries.
  double                m_avgGroupEntries;  //!< Avg number of group entries.
  TracedCallback<bool>  m_loadBalTrace;     //!< The LB trace source.
};

} // namespace ns3
#endif /* PGW_LOAD_MONITOR_H */
