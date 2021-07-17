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

#ifndef TRANSPORT_CONTROLLER_H
#define TRANSPORT_CONTROLLER_H

// Pipeline tables at OpenFlow transport switches.
#define INPUT_TAB 0
#define CLASS_TAB 1
#define SLICE_TAB_START 2
#define BANDW_TAB (SLICE_TAB_START + static_cast<int> (SliceId::ALL))
#define OUTPT_TAB (BANDW_TAB + 1)

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../mano-apps/global-ids.h"
#include "../metadata/link-info.h"
#include "../slices/slice-controller.h"
#include "../uni5on-common.h"

namespace ns3 {

class EnbInfo;
class LinkInfo;
class LinkSharing;

/**
 * \ingroup uni5onInfra
 * Abstract base class for the OpenFlow transport controller, which should be
 * extended to configure the desired transport network topology.
 */
class TransportController : public OFSwitch13Controller
{
  friend class TransportNetwork;
  friend class SliceController;
  friend class ScenarioConfig;
  friend class LinkSharing;

public:
  TransportController ();           //!< Default constructor.
  virtual ~TransportController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the OpenFlow datapath ID for a specific switch index.
   * \param idx The transport switch index.
   * \return The OpenFlow datapath ID.
   */
  uint64_t GetDpId (uint16_t idx) const;

  /**
   * Get the total number of OpenFlow switches in the transport network.
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
  OpMode    GetQosQueuesMode      (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Process the bearer request in the transport network.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRequest (Ptr<BearerInfo> bInfo) = 0;

  /**
   * Reserve the resources for this bearer.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerReserve (Ptr<BearerInfo> bInfo) = 0;

  /**
   * Release the resources for this bearer.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRelease (Ptr<BearerInfo> bInfo) = 0;

  /**
   * Install bearer routing rules into transport switches.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before invoking this method.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerInstall (Ptr<BearerInfo> bInfo) = 0;

  /**
   * Remove bearer routing rules from transport switches.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerRemove (Ptr<BearerInfo> bInfo) = 0;

  /**
   * Update bearer routing rules at transport switches.
   * \attention Don't increase the routing priority and don't update the ueInfo
   *            with the destination eNB metadata before invoking this method.
   * \param bInfo The bearer information.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  virtual bool BearerUpdate (Ptr<BearerInfo> bInfo,
                             Ptr<EnbInfo> dstEnbInfo) = 0;

  /**
   * Schedule a dpctl command to be executed after a delay.
   * \param delay The relative execution time for this command.
   * \param dpId The OpenFlow datapath ID.
   * \param textCmd The dpctl command to be executed.
   */
  void DpctlSchedule (Time delay, uint64_t dpId, const std::string textCmd);

  /**
   * Get the pipeline flow table usage for the given transport switch
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
   * Get the EWMA processing capacity usage for the given transport switch.
   * \param idx The switch index.
   * \return The pipeline capacity usage.
   */
  double GetEwmaCpuUse (uint16_t idx) const;

  /** A list of link information objects. */
  typedef std::vector<EpsBearer::Qci> QciList_t;

  /**
   * Get the list of Non-GBR QCIs.
   * \return The const reference to the list of Non-GBR QCIs.
   */
  static const QciList_t& GetNonGbrQcis (void);

  /**
   * Get the slice controller application for a given slice ID.
   * \param slice The network slice.
   * \return The slice controller application.
   */
  Ptr<SliceController> GetSliceController (SliceId slice) const;

  /**
   * Get the number of the OpenFlow pipeline table exclusively used by this
   * slice for GTP tunnel handling (routing and QoS).
   * \param slice The slice ID.
   * \return The pipeline table for this slice.
   */
  uint16_t GetSliceTable (SliceId slice) const;

  /**
   * Get the link sharing application.
   * \return The link sharing application.
   */
  Ptr<LinkSharing> GetSharingApp (void) const;

  /**
   * Notify this controller of a new bearer context created.
   * \param bInfo The bearer information.
   */
  virtual void NotifyBearerCreated (Ptr<BearerInfo> bInfo);

  /**
   * Notify this controller of a new EPC entity connected to the OpenFlow
   * transport network.
   * \param swDev The OpenFlow switch device on the transport network.
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
   * Notify this controller that all transport switches have already been
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
   * Apply the link sharing OpenFlow meter.
   * \param swDpId The transport switch datapath id.
   * \param dir The link direction.
   * \param slice The network slice.
   */
  virtual void SharingMeterApply (uint64_t swDpId, LinkInfo::LinkDir dir,
                                  SliceId slice);

  /**
   * Install the link sharing OpenFlow meter.
   * \param lInfo The link information.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The meter bit rate.
   */
  virtual void SharingMeterInstall (Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir,
                                    SliceId slice, int64_t bitRate);

  /**
   * Adjust the link sharing OpenFlow meter.
   * \param lInfo The link information.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The meter bit rate.
   */
  virtual void SharingMeterUpdate (Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir,
                                   SliceId slice, int64_t bitRate);

private:
  OpMode                    m_aggCheck;       //!< Check rate for agg bearers.
  DataRate                  m_meterStep;      //!< Meter adjustment step.
  OpMode                    m_qosQueues;      //!< QoS output queues mechanism.
  OpMode                    m_swBlockPolicy;  //!< Switch overload block policy.
  double                    m_swBlockThs;     //!< Switch block threshold.
  Ptr<LinkSharing>          m_sharingApp;     //!< Link sharing application.
  OFSwitch13DeviceContainer m_switchDevices;  //!< OpenFlow switch devices.

  /** Map saving Slice ID / Slice controller application. */
  typedef std::map<SliceId, Ptr<SliceController>> SliceIdCtrlAppMap_t;
  SliceIdCtrlAppMap_t       m_sliceCtrlById;  //!< Controller mapped values.

  static const QciList_t    m_nonQciList;     //!< List of Non-GBR QCIs.
};

} // namespace ns3
#endif // TRANSPORT_CONTROLLER_H
