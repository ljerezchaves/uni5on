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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef BACKHAUL_CONTROLLER_H
#define BACKHAUL_CONTROLLER_H

// Pipeline tables at OpenFlow backhaul switches.
#define INPUT_TAB 0
#define CLASS_TAB 1
#define BANDW_TAB (static_cast<int> (SliceId::ALL) + 2)
#define OUTPT_TAB (static_cast<int> (SliceId::ALL) + 3)

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../logical/slice-controller.h"
#include "../metadata/link-info.h"
#include "../metadata/routing-info.h"
#include "../uni5on-common.h"

namespace ns3 {

class EnbInfo;
class LinkInfo;

/**
 * \ingroup uni5onInfra
 * This is the abstract base class for the OpenFlow backhaul controller, which
 * should be extended in accordance to the desired backhaul network topology.
 * This controller implements the logic for traffic routing and engineering
 * within the OpenFlow backhaul network.
 */
class BackhaulController : public OFSwitch13Controller
{
  friend class BackhaulNetwork;
  friend class SliceController;
  friend class ScenarioHelper;

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
   * \name Attribute accessors.
   * \return The requested information.
   */
  //\{
  OpMode    GetAggBitRateCheck    (void) const;
  OpMode    GetSwBlockPolicy      (void) const;
  double    GetSwBlockThreshold   (void) const;
  SliceMode GetInterSliceMode     (void) const;
  OpMode    GetQosQueuesMode      (void) const;
  OpMode    GetSpareUseMode       (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Process the bearer request, deciding for the best routing path and
   * checking for the available resources in the backhaul network.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Reserve the resources for this bearer.
   * \param rInfo The ring routing information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerReserve (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Release the resources for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Install TEID routing OpenFlow match rules into backhaul switches.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerInstall (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Remove TEID routing OpenFlow match rules from backhaul switches.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRemove (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Update TEID routing OpenFlow match rules from backhaul switches after a
   * successful handover procedure.
   * \attention Don't increase the rInfo priority and don't update the ueInfo
   *            with the destination eNB metadata before invoking this method.
   * \param rInfo The routing information to process.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerUpdate (Ptr<RoutingInfo> rInfo,
                             Ptr<EnbInfo> dstEnbInfo) = 0;

  /**
   * Schedule a dpctl command to be executed after a delay.
   * \param delay The relative execution time for this command.
   * \param dpId The OpenFlow datapath ID.
   * \param textCmd The dpctl command to be executed.
   */
  void DpctlSchedule (Time delay, uint64_t dpId, const std::string textCmd);

  /**
   * Get the pipeline flow table usage for the given backhaul switch index
   * and pipeline flow table ID.
   * \param idx The switch index.
   * \param tableId The pipeline slice table ID.
   * \return The flow table usage.
   */
  double GetFlowTableUse (uint16_t idx, uint8_t tableId) const;

  /**
   * Search for link information between two switches by their indexes.
   * \param idx1 First switch index.
   * \param idx2 Second switch index.
   * \return A tuple with:
   *         1st: A pointer to the link information;
   *         2nd: The link direction from idx1 to idx2;
   *         3rd: The link direction from idx2 to idx1.
   */
  std::tuple<Ptr<LinkInfo>, LinkInfo::LinkDir, LinkInfo::LinkDir> GetLinkInfo (
    uint16_t idx1, uint16_t idx2) const;

  /**
   * Get the EWMA processing capacity usage for the given backhaul switch.
   * \param idx The switch index.
   * \return The pipeline capacity usage.
   */
  double GetEwmaCpuUse (uint16_t idx) const;

  /**
   * Get the slice controller application for a given slice ID.
   * \param slice The network slice.
   * \return The slice controller application.
   */
  Ptr<SliceController> GetSliceController (SliceId slice) const;

  /**
   * Get the list of slice controller applications.
   * \param sharing Filter controller applications with enabled backhaul
   *        bandwdith sharing flag.
   * \return The list of controller applications.
   */
  const SliceControllerList_t& GetSliceControllerList (
    bool sharing = false) const;

  /**
   * Get the number of the OpenFlow pipeline table exclusively used by this
   * slice for GTP tunnel handling (routing and QoS).
   * \param slice The slice ID.
   * \return The pipeline table for this slice.
   */
  uint16_t GetSliceTable (SliceId slice) const;

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

  /**
   * Periodically check for infrastructure bandwidth utilization over backhaul
   * links to adjust extra bit rate when in dynamic inter-slice operation mode.
   */
  void SlicingDynamicTimeout (void);

  /**
   * Adjust the infrastructure inter-slicing extra bit rate, depending on the
   * ExtraStep attribute value and current link configuration.
   * \param lInfo The link information.
   * \param dir The link direction.
   */
  void SlicingExtraAdjust (Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir);

  /**
   * Adjust the infrastructure inter-slicing OpenFlow meter, depending on the
   * MeterStep attribute value and current link configuration.
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void SlicingMeterAdjust (Ptr<LinkInfo> lInfo, SliceId slice);

  /**
   * Install the infrastructure inter-slicing OpenFlow meters.
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void SlicingMeterInstall (Ptr<LinkInfo> lInfo, SliceId slice);

private:
  OFSwitch13DeviceContainer m_switchDevices;  //!< OpenFlow switch devices.

  // Internal mechanisms metadata.
  OpMode                m_aggCheck;       //!< Check bit rate for agg bearers.
  DataRate              m_extraStep;      //!< Extra adjustment step.
  DataRate              m_guardStep;      //!< Dynamic slice link guard.
  DataRate              m_meterStep;      //!< Meter adjustment step.
  OpMode                m_qosQueues;      //!< QoS output queues mechanism.
  SliceMode             m_sliceMode;      //!< Inter-slicing operation mode.
  Time                  m_sliceTimeout;   //!< Dynamic slice timeout interval.
  OpMode                m_spareUse;       //!< Spare bit rate sharing mode.
  OpMode                m_swBlockPolicy;  //!< Switch overload block policy.
  double                m_swBlockThs;     //!< Switch block threshold.

  /** Slice controllers sorted by increasing priority. */
  SliceControllerList_t m_sliceCtrlsAll;

  /** Slice controllers with enabled sharing sorted by increasing priority. */
  SliceControllerList_t m_sliceCtrlsSha;

  /** Map saving Slice ID / Slice controller application. */
  typedef std::map<SliceId, Ptr<SliceController> > SliceIdCtrlAppMap_t;
  SliceIdCtrlAppMap_t   m_sliceCtrlById;  //!< Slice controller mapped values.
};

} // namespace ns3
#endif // BACKHAUL_CONTROLLER_H
