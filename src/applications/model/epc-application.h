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

#ifndef EPC_APPLICATION_H
#define EPC_APPLICATION_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "qos-stats-calculator.h"

namespace ns3 {

/**
 * \ingroup applications
 * This class extends the Application with QosStatsCalculator and stop
 * callback. It was designed to be used by client applications for EPC +
 * OpenFlow simulations.
 */
class EpcApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  
  EpcApplication ();            //!< Default constructor
  virtual ~EpcApplication ();   //!< Dummy destructor, see DoDipose

  /**
   * Get QoS statistics
   * \return Get the const pointer to QosStatsCalculator
   */
  Ptr<const QosStatsCalculator> GetQosStats (void) const;

  /**
   * Reset the QoS statistics
   */
  virtual void ResetQosStats ();

  /**
   * \brief Start this application at any time.
   */
  virtual void Start (void);
  
  /**
   * Stop callback signature. 
   */
  typedef Callback<void, Ptr<EpcApplication> > StopCb_t;

  /**
   * Set the stop callback
   * \param cb The callback to invoke when traffic stops.
   */
  void SetStopCallback (StopCb_t cb);

  /**
   * Get the application name;
   * \return The application name;
   */
  virtual std::string GetAppName (void) const;

protected:
  virtual void DoDispose (void);        //!< Destructor implementation

  Ptr<QosStatsCalculator> m_qosStats;   //!< QoS statistics
  StopCb_t                m_stopCb;     //!< Stop callback
};

} // namespace ns3
#endif /* EPC_APPLICATION_H */
