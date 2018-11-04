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

#ifndef CUSTOM_CONTROLLER_H
#define CUSTOM_CONTROLLER_H

#include <ns3/ofswitch13-module.h>
#include <ns3/internet-module.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/lte-module.h>
#include "applications/svelte-client.h"

namespace ns3 {

class CustomController : public OFSwitch13Controller
{
public:
  CustomController ();            //!< Default constructor.
  virtual ~CustomController ();   //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Request a dedicated traffic. This is used to check for necessary resources
   * in the network. When returning false, it aborts the application start.
   * \param app The client application.
   * \param imsi The client identifier.
   * \return True if succeeded, false otherwise.
   */
  bool DedicatedBearerRequest (Ptr<SvelteClient> app, uint64_t imsi);

  /**
   * Release a dedicated traffic.
   * \param app The client application.
   * \param imsi The client identifier.
   * \return True if succeeded, false otherwise.
   */
  bool DedicatedBearerRelease (Ptr<SvelteClient> app, uint64_t imsi);

  /**
   * Notify this controller of the new OpenFlow switch device.
   * \param device The OpenFlow switch device.
   */
  void NotifySwitch (Ptr<OFSwitch13Device> device);

  /**
   * Notify this controller of a new host connected to the OpenFlow switch.
   * \param portNo The port number at the swithc.
   * \param ipAddr The host IP address.
   */
  //\{
  void NotifyClient (uint32_t portNo, Ipv4Address ipAddr);
  void NotifyServer (uint32_t portNo, Ipv4Address ipAddr);
  //\}

  /**
   * Notify this controller that all topology connections are done.
   */
  void NotifyTopologyBuilt ();

  /**
   * TracedCallback signature for request trace source.
   * \param teid The traffic ID.
   * \param accepted The traffic request status.
   */
  typedef void (*RequestTracedCallback)(uint32_t teid, bool accepted);

  /**
   * TracedCallback signature for release trace source.
   * \param teid The traffic ID.
   */
  typedef void (*ReleaseTracedCallback)(uint32_t teid);

protected:
  // Inherited from Object.
  virtual void DoDispose ();

private:
  Ptr<OFSwitch13Device>          m_switchDevice;  //!< Switch device.
  TracedCallback<uint32_t, bool> m_requestTrace;  //!< Request trace source.
  TracedCallback<uint32_t>       m_releaseTrace;  //!< Release trace source.
};

} // namespace ns3
#endif  // CUSTOM_CONTROLLER_H
