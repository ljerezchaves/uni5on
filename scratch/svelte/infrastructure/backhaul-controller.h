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

#ifndef BACKHAUL_CONTROLLER_H
#define BACKHAUL_CONTROLLER_H

// Pipeline tables at OpenFlow backhaul switches.
#define INPUT_TAB 0
#define CLASS_TAB 1
#define ROUTE_TAB 2
#define SLICE_TAB 3
#define OUTPT_TAB 4

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../metadata/link-info.h"
#include "../metadata/routing-info.h"
#include "../svelte-common.h"

namespace ns3 {

class LinkInfo;
class SliceController;

/**
 * \ingroup svelteInfra
 * This is the abstract base class for the OpenFlow backhaul controller, which
 * should be extended in accordance to the desired backhaul network topology.
 * This controller implements the logic for traffic routing and engineering
 * within the OpenFlow backhaul network.
 */
class BackhaulController : public OFSwitch13Controller
{
  friend class BackhaulNetwork;
  friend class SliceController;
  friend class SvelteHelper;

public:
  BackhaulController ();           //!< Default constructor.
  virtual ~BackhaulController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param idx The backhaul switch index.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDpId (uint16_t idx) const;

  /**
   * Get the total number of OpenFlow switches in the backhaul network.
   * \return The number of OpenFlow switches.
   */
  uint16_t GetNSwitches (void) const;

  /**
   * Get the priority output queues mechanism operation mode.
   * \return The operation mode.
   */
  OpMode GetPriorityQueuesMode (void) const;

  /**
   * Get the link slicing mechanism operation mode.
   * \return The operation mode.
   */
  OpMode GetLinkSlicingMode (void) const;

  /**
   * Get the average slice usage considering all links in the backhaul network.
   * \param slice The network slice.
   * \return The average slice usage.
   */
  double GetSliceUsage (SliceId slice) const;

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Process the bearer request, checking for the available resources in the
   * backhaul network, deciding for the best routing path, and reserving the
   * bit rate when necessary.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Release the resources for this bearer
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Block this bearer and notify the reason.
   * \param rInfo The routing information to process.
   * \param reason The reason for blocking this bearer.
   */
  void BlockBearer (Ptr<RoutingInfo> rInfo,
                    RoutingInfo::BlockReason reason) const;

  /**
   * Search for link information between two switches by their indexes.
   * \param idx1 First switch index.
   * \param idx2 Second switch index.
   * \return Pointer to link info saved.
   */
  Ptr<LinkInfo> GetLinkInfo (uint16_t idx1, uint16_t idx2) const;

  /**
   * Get the slice controller application for a given slice ID.
   * \param slice The network slice.
   * \return The slice controller application.
   */
  Ptr<SliceController> GetSliceController (SliceId slice) const;

  /**
   * Notify this controller of a new bearer context created.
   * \param rInfo The routing information to process.
   */
  virtual void NotifyBearerCreated (Ptr<RoutingInfo> rInfo);

  /**
   * Notify this controller of a new EPC entity connected to the OpenFlow
   * backhaul network.
   * \param swDev The OpenFlow switch device on the backhaul network.
   * \param portNo The port number created at the OpenFlow switch.
   * \param epcDev The device created at the EPC node.
   */
  virtual void NotifyEpcAttach (Ptr<OFSwitch13Device> swDev, uint32_t portNo,
                                Ptr<NetDevice> epcDev);

  /**
   * Notify this controller that all the logical slices have already been
   * configured and the slice controllers were created.
   * \param controllers The logical slice controllers.
   */
  virtual void NotifySlicesBuilt (ApplicationContainer &controllers);

  /**
   * Notify this controller that all backhaul switches have already been
   * configured and the connections between them are finished.
   * \param devices The OpenFlow switch devices.
   */
  virtual void NotifyTopologyBuilt (OFSwitch13DeviceContainer &devices);

  /**
   * Install TEID routing OpenFlow match rules into backhaul switches.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyRoutingInstall (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Remove TEID routing OpenFlow match rules from backhaul switches.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyRoutingRemove (Ptr<RoutingInfo> rInfo) = 0;

  // Inherited from OFSwitch13Controller.
  virtual ofl_err HandleError (
    struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  // Inherited from OFSwitch13Controller.

private:
  /**
   * Notify this controller when the reserved bit rate in any network link and
   * slice is adjusted, exceeding the AdjustmentStep attribute from LinkInfo
   * class. This is used to update infrastructure slicing meters.
   * \param lInfo The link information.
   * \param dir The link direction.
   * \param slice The network slice.
   */
  void SlicingMeterAdjusted (Ptr<const LinkInfo> lInfo,
                             LinkInfo::Direction dir, SliceId slice);

  /**
   * Install the infrastructure slicing meters. When the network slicing
   * operation mode is ON, the traffic of each slice will be independently
   * monitored by slicing meters. When the slicing operation mode is AUTO, the
   * traffic of all slices will be monitored together by the slicing meters,
   * ensuring a better bandwidth sharing among slices.
   *
   * \param swtch The OpenFlow switch information.
   */
  void SlicingMeterInstall (Ptr<const LinkInfo> lInfo);

  /** Initialize static attributes only once. */
  static void StaticInitialize (void);

  OFSwitch13DeviceContainer m_switchDevices;  //!< OpenFlow switch devices.

  // Internal mechanisms for performance improvement.
  OpMode                m_priorityQueues; //!< DSCP priority queues mechanism.
  OpMode                m_slicing;        //!< Network slicing mechanism.

  /** Map saving Slice ID / Slice controller application. */
  typedef std::map<SliceId, Ptr<SliceController> > SliceIdCtrlAppMap_t;
  SliceIdCtrlAppMap_t   m_sliceCtrlById;  //!< Slice controller mapped values.

  /** Map saving IP DSCP value / OpenFlow queue id. */
  typedef std::map<Ipv4Header::DscpType, uint32_t> DscpQueueMap_t;
  static DscpQueueMap_t m_queueByDscp;    //!< OpenFlow queue id mapped values.
};

} // namespace ns3
#endif // BACKHAUL_CONTROLLER_H
