/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef SCENARIO_SWITCH_HELPER_H
#define SCENARIO_SWITCH_HELPER_H

#include <ns3/ofswitch13-module.h>

namespace ns3 {

/**
 * \ingroup uni5onInfra
 * Custom OFSwitch13 switch helper for handling eNBs switches simultaneously
 * managed by infrastructure and slice controllers.
 */
class SwitchHelper : public OFSwitch13InternalHelper
{
public:
  SwitchHelper ();          //!< Default constructor.
  virtual ~SwitchHelper (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * This method adds the given OpenFlow switch into the internal container
   * for further connection with the already configured OpenFlow controller.
   * \param device The OpenFlow device.
   */
  void AddSwitch (Ptr<OFSwitch13Device> device);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();
};

} // namespace ns3
#endif /* SCENARIO_SWITCH_HELPER_H */

