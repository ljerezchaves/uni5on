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

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../svelte-common.h"

namespace ns3 {

class LinkInfo;
class RoutingInfo;

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

public:
  BackhaulController ();           //!< Default constructor.
  virtual ~BackhaulController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Release a dedicated EPS bearer.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \param teid The teid for this bearer, if already defined.
   * \return True if succeeded, false otherwise.
   */
  virtual bool DedicatedBearerRelease (EpsBearer bearer, uint32_t teid);

  /**
   * Request a new dedicated EPS bearer. This is used to check for necessary
   * resources in the network (mainly available data rate for GBR bearers).
   * When returning false, it aborts the bearer creation process.
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the TEID for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \param teid The teid for this bearer, if already defined.
   * \returns True if succeeded (the bearer creation process will proceed),
   *          false otherwise (the bearer creation process will abort).
   */
  virtual bool DedicatedBearerRequest (EpsBearer bearer, uint32_t teid);

  /**
   * \name Internal mechanisms operation mode accessors.
   * \return The requested mechanism operation mode.
   */
  //\{
  OpMode GetPriorityQueuesMode (void) const;
  OpMode GetSlicingMode (void) const;
  //\}

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
   * Get the slice usage considering the maximum usage in any backhaul link.
   * \param slice The link slice.
   * \return The maximum slice usage.
   */
  double GetSliceUsage (LinkSlice slice) const;

  /**
   * Retrieve stored mapped IP ToS for a specific DSCP. We are mapping the DSCP
   * value (RFC 2474) to the IP Type of Service (ToS) (RFC 1349) field because
   * the pfifo_fast queue discipline from the traffic control module still uses
   * the old IP ToS definition. Thus, we are 'translating' the DSCP values so
   * we can keep the priority queueing consistency both on traffic control
   * module and OpenFlow port queues.
   * \param dscp The IP DSCP value.
   * \return The IP ToS mapped for this DSCP.
   */
  static uint8_t Dscp2Tos (Ipv4Header::DscpType dscp);

  /**
   * Retrieve stored mapped DSCP for a specific EPS QCI.
   * \param qci The EPS bearer QCI.
   * \return The IP DSCP mapped for this QCI.
   */
  static Ipv4Header::DscpType Qci2Dscp (EpsBearer::Qci qci);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Search for link information between two switches by their indexes.
   * \param idx1 First switch index.
   * \param idx2 Second switch index.
   * \return Pointer to link info saved.
   */
  Ptr<LinkInfo> GetLinkInfo (uint16_t idx1, uint16_t idx2) const;

  /**
   * Notify this controller of a new EPC entity connected to the OpenFlow
   * backhaul network.
   * \param swDev The OpenFlow switch device on the backhaul network.
   * \param portNo The port number created at the OpenFlow switch.
   * \param epcDev The device created at the EPC node.
   */
  virtual void NotifyEpcAttach (
    Ptr<OFSwitch13Device> swDev, uint32_t portNo, Ptr<NetDevice> epcDev);

  /**
   * \name Topology methods.
   * These virtual methods must be implemented by topology subclasses, as
   * they are dependent on the OpenFlow backhaul network topology.
   */
  //\{
  /**
   * Notify this controller that all backhaul switches have already been
   * configured and the connections between them are finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices) = 0;

  /**
   * Notify this controller of a new bearer context created.
   * \param rInfo The routing information to process.
   */
  virtual void TopologyBearerCreated (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer request, checking for available resources in the
   * backhaul network and deciding for the best routing path.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Release the bit rate for this bearer in the backhaul network.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyBitRateRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Reserve the bit rate for this bearer in the backhaul network.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyBitRateReserve (Ptr<RoutingInfo> rInfo) = 0;

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
  //\}

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

  OFSwitch13DeviceContainer m_switchDevices;  //!< OpenFlow switch devices.

private:
  /** Initialize static attributes only once. */
  static void StaticInitialize (void);

  // Internal mechanisms for performance improvement.
  OpMode                m_priorityQueues; //!< DSCP priority queues mechanism.
  OpMode                m_slicing;        //!< Network slicing mechanism.

  /** Map saving EPS QCI / IP DSCP value. */
  typedef std::map<EpsBearer::Qci, Ipv4Header::DscpType> QciDscpMap_t;

  /** Map saving IP DSCP value / OpenFlow queue id. */
  typedef std::map<Ipv4Header::DscpType, uint32_t> DscpQueueMap_t;

  /** Map saving IP DSCP value / IP ToS. */
  typedef std::map<Ipv4Header::DscpType, uint8_t> DscpTosMap_t;

  static QciDscpMap_t   m_qciDscpTable;   //!< DSCP mapped values.
  static DscpQueueMap_t m_dscpQueueTable; //!< OpenFlow queue id mapped values.
  static DscpTosMap_t   m_dscpTosTable;   //!< IP ToS mapped values.
};

} // namespace ns3
#endif // BACKHAUL_CONTROLLER_H
