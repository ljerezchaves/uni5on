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
// #include "epc-s5-sap.h"
// #include "../info/enb-info.h"
// #include "../info/gbr-info.h"
// #include "../info/meter-info.h"
// #include "../info/routing-info.h"
// #include "../info/ue-info.h"

namespace ns3 {

class ConnectionInfo;

/** 
 * \ingroup svelteInfra
 * Enumeration of available operation modes.
 */
typedef enum
{
  OFF  = 0,   //!< Always off.
  ON   = 1,   //!< Always on.
  AUTO = 2    //!< Automatic.
} OperationMode;

/**
 * \ingroup svelteInfra
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OperationModeStr (OperationMode mode);

/**
 * \ingroup svelteInfra
 * This is the abstract base class for the OpenFlow backhaul controller, which
 * should be extended in accordance to the desired backhaul network topology.
 * This controller implements the logic for traffic routing and engineering
 * within the OpenFlow backhaul network.
 */
class BackhaulController : public OFSwitch13Controller
{
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
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the TEID for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
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
   * Notify this controller of a new EPC entity connected to the OpenFlow
   * backhaul network.
   * \param swDev The OpenFlow switch device on the backhaul network.
   * \param portNo The port number created at the OpenFlow switch.
   * \param epcDev The device created at the EPC node.
   */
  virtual void NotifyEpcAttach (
    Ptr<OFSwitch13Device> swDev, uint32_t portNo, Ptr<NetDevice> epcDev);

//  /**
//   * Notify this controller of a new S-GW connected to OpenFlow backhaul
//   * network over the S5 interface. This method is used only for configuring
//   * MTC traffic aggregation.
//   * \param gwDev The device created at the gateway node.
//   * \return The pair of MTC aggregation TEIDs on the uplink S5 interface.
//   */
//  virtual std::pair<uint32_t, uint32_t> NotifySgwAttach (Ptr<NetDevice> gwDev);

  /**
   * Notify this controller that all backhaul switches have already been
   * configured and the connections between them are finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow backhaul network.
   * \param cInfo The connection information.
   */
  virtual void NotifyTopologyConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * \name Internal mechanisms operation mode accessors.
   * \return The requested mechanism operation mode.
   */
  //\{
//  OperationMode GetHtcAggregMode (void) const;
//  OperationMode GetMtcAggregMode (void) const;
  OperationMode GetPriorityQueuesMode (void) const;
  OperationMode GetSlicingMode (void) const;
  //\}

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

// FIXME Comentando as funções de topologia pq por enquanto não estou usando o routingInfo.
//  /**
//   * \name Topology methods.
//   * These virtual methods must be implemented by topology subclasses, as they
//   * are dependent on the backhaul OpenFlow network topology.
//   */
//  //\{
//  /**
//   * Notify the topology controller of a new bearer context created.
//   * \param rInfo The routing information.
//   */
//  virtual void TopologyBearerCreated (Ptr<RoutingInfo> rInfo) = 0;
//
//  /**
//   * Process the bearer request, checking for available resources in the
//   * backhaul network and deciding for the best routing path.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo) = 0;
//
//  /**
//   * Release the bit rate for this bearer in the backhaul network.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool TopologyBitRateRelease (Ptr<RoutingInfo> rInfo) = 0;
//
//  /**
//   * Reserve the bit rate for this bearer in the backhaul network.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool TopologyBitRateReserve (Ptr<RoutingInfo> rInfo) = 0;
//
//  /**
//   * Install TEID routing OpenFlow match rules into backhaul switches.
//   * \attention To avoid conflicts with old entries, increase the routing
//   *            priority before installing OpenFlow rules.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool TopologyRoutingInstall (Ptr<RoutingInfo> rInfo) = 0;
//
//  /**
//   * Remove TEID routing OpenFlow match rules from backhaul switches.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool TopologyRoutingRemove (Ptr<RoutingInfo> rInfo) = 0;
//  //\}

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
//  /**
//   * Install OpenFlow match rules for this bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool BearerInstall (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Remove OpenFlow match rules for this bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool BearerRemove (Ptr<RoutingInfo> rInfo);

//  /**
//   * Install OpenFlow match rules for the aggregated MTC bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool MtcAggBearerInstall (Ptr<RoutingInfo> rInfo);

  /** Initialize static attributes only once. */
  static void StaticInitialize (void);

// FIXME Comentado para remover dependencia do routinginfo
// FIXME Deve ficar aqui ou no SliceController?
//  /** The bearer request trace source, fired at RequestDedicatedBearer. */
//  TracedCallback<Ptr<const RoutingInfo> > m_bearerRequestTrace;
//
//  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
//  TracedCallback<Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  // Internal mechanisms for performance improvement.
  OperationMode         m_priorityQueues; //!< DSCP priority queues mechanism.
  OperationMode         m_slicing;        //!< Network slicing mechanism.


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
