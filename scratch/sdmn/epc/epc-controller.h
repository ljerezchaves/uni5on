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

#ifndef EPC_CONTROLLER_H
#define EPC_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "epc-s5-sap.h"
#include "../info/enb-info.h"
#include "../info/gbr-info.h"
#include "../info/meter-info.h"
#include "../info/routing-info.h"
#include "../info/ue-info.h"

namespace ns3 {

class ConnectionInfo;

/** Enumeration of available operation modes. */
typedef enum
{
  OFF  = 0,   //!< Always off.
  ON   = 1,   //!< Always on.
  AUTO = 2    //!< Automatic.
} OperationMode;

std::string OperationModeStr (OperationMode mode);

/**
 * \ingroup sdmnEpc
 * This is the abstract base class for the OpenFlow EPC controller, which
 * should be extended in accordance to OpenFlow backhaul network topology. This
 * controller implements the logic for traffic routing and engineering within
 * the OpenFlow backhaul network. It is also responsible for implementing the
 * P-GW control plane and for configuring the P-GW OpenFlow user plane.
 */
class EpcController : public OFSwitch13Controller
{
  friend class MemberEpcS5SapPgw<EpcController>;

public:
  /** P-GW adaptive mechanism statistics. */
  struct PgwTftStats
  {
    double   tableSize;       //!< The OpenFlow flow table size.
    double   maxEntries;      //!< The table size peak entries.
    double   sumEntries;      //!< The table size total entries.
    double   pipeCapacity;    //!< The OpenFlow pipeline capacity.
    double   maxLoad;         //!< The pipeline peak load;
    double   sumLoad;         //!< The pipeline total load;
    uint32_t currentLevel;    //!< The current mechanism level.
    uint32_t nextLevel;       //!< The mechanism level for next cycle.
    uint32_t maxLevel;        //!< The maximum mechanism level.
    uint32_t bearersMoved;    //!< The number of bearers moved between TFTs.
    double   blockThrs;       //!< The block threshold.
    double   joinThrs;        //!< The join threshold.
    double   splitThrs;       //!< The split threshold.
  };

  EpcController ();           //!< Default constructor.
  virtual ~EpcController ();  //!< Dummy destructor, see DoDispose.

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
   * Notify this controller that all P-GW switches have already been
   * configured and the connections between them are finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyPgwBuilt (OFSwitch13DeviceContainer devices);

  /**
   * Notify this controller of the P-GW main switch connected to the OpenFlow
   * backhaul network over the S5 interface, and to the web server over the SGi
   * interface.
   * \param pgwSwDev The OpenFlow P-GW main switch device.
   * \param pgwS5PortNo The S5 port number on the P-GW main switch.
   * \param pgwSgiPortNo The SGi port number on the P-GW main switch.
   * \param pgwS5Dev The S5 device attached to the P-GW main switch.
   * \param webSgiDev The SGi device attached to the Web server.
   */
  virtual void NotifyPgwMainAttach (
    Ptr<OFSwitch13Device> pgwSwDev, uint32_t pgwS5PortNo,
    uint32_t pgwSgiPortNo, Ptr<NetDevice> pgwS5Dev, Ptr<NetDevice> webSgiDev);

  /**
   * Notify this controller of a new P-GW TFT switch connected to the OpenFlow
   * backhaul network over the S5 interface and to the P-GW main switch over
   * internal interface.
   * \param pgwTftCounter The counter for P-GW TFT switches.
   * \param pgwSwDev The OpenFlow P-GW TFT switch device.
   * \param pgwS5PortNo The S5 port number on the P-GW TFT switch.
   * \param pgwMainPortNo The port number on the P-GW main switch.
   */
  virtual void NotifyPgwTftAttach (
    uint16_t pgwTftCounter, Ptr<OFSwitch13Device> pgwSwDev,
    uint32_t pgwS5PortNo, uint32_t pgwMainPortNo);

  /**
   * Notify this controller of a new S-GW or P-GW connected to OpenFlow
   * backhaul network over the S5 interface.
   * \param swtchDev The OpenFlow switch device on the backhaul network.
   * \param portNo The port number created at the OpenFlow switch.
   * \param gwDev The device created at the gateway node.
   */
  virtual void NotifyS5Attach (
    Ptr<OFSwitch13Device> swtchDev, uint32_t portNo, Ptr<NetDevice> gwDev);

  /**
   * Notify this controller of a new S-GW connected to OpenFlow backhaul
   * network over the S5 interface. This method is used only for configuring
   * MTC traffic aggregation.
   * \param gwDev The device created at the gateway node.
   * \return The MTC aggregation TEID on the uplink S5 interface.
   */
  virtual uint32_t NotifySgwAttach (Ptr<NetDevice> gwDev);

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
  OperationMode GetHtcAggregMode (void) const;
  OperationMode GetMtcAggregMode (void) const;
  OperationMode GetPgwAdaptiveMode (void) const;
  OperationMode GetPriorityQueuesMode (void) const;
  OperationMode GetSlicingMode (void) const;
  //\}

  /**
   * Get The P-GW side of the S5 SAP.
   * \return The P-GW side of the S5 SAP.
   */
  EpcS5SapPgw* GetS5SapPgw (void) const;

  /**
   * Retrieve stored mapped IP ToS for a specific DSCP. We are mapping the DSCP
   * value (RFC 2474) to the IP Type of Service (ToS) (RFC 1349) field because
   * the pfifo_fast queue discipline from the traffic control module still uses
   * the old IP ToS definition. Thus, we are "translating" the DSCP values so
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

  /**
   * TracedCallback signature for the P-GW TFT stats trace source.
   * \param stats The P-GW TST statistics from the last interval.
   */
  typedef void (*PgwTftStatsTracedCallback)(struct PgwTftStats stats);

  /**
   * TracedCallback signature for session created trace source.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param bearerList The list of context bearers created.
   */
  typedef void (*SessionCreatedTracedCallback)(
    uint64_t imsi, uint16_t cellId, BearerContextList_t bearerList);

  static const uint32_t m_teidStart;    //!< First valid TEID value.
  static const uint32_t m_teidEnd;      //!< Last valid TEID value.

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * \name Topology methods.
   * These virtual methods must be implemented by topology subclasses, as they
   * are dependent on the backhaul OpenFlow network topology.
   */
  //\{
  /**
   * Notify the topology controller of a new bearer context created.
   * \param rInfo The routing information.
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

private:
  /**
   * Install OpenFlow match rules for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool BearerInstall (Ptr<RoutingInfo> rInfo);

  /**
   * Remove OpenFlow match rules for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool BearerRemove (Ptr<RoutingInfo> rInfo);

  /**
   * Periodic timeout operation.
   */
  void ControllerTimeout (void);

  /** \name Methods for the S5 SAP P-GW control plane. */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  msg);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage msg);
  void DoModifyBearerRequest  (EpcS11SapSgw::ModifyBearerRequestMessage  msg);
  //\}

  /**
   * Get the P-GW main datapath ID.
   * \return The P-GW main datapath ID.
   */
  uint64_t GetPgwMainDpId (void) const;

  /**
   * Get the P-GW TFT datapath ID for a given index.
   * \param idx The P-GW TFT index.
   * \return The P-GW TFT datapath ID.
   */
  uint64_t GetPgwTftDpId (uint16_t idx) const;

  /**
   * Get the active P-GW TFT index for a given traffic flow.
   * \param rInfo The routing information to process.
   * \param activeTfts The number of active P-GW TFT switches. When set to 0,
   *        the number of P-GW TFTs will be calculated considering the current
   *        adaptive mechanism level.
   * \return The P-GW TFT index.
   */
  uint16_t GetPgwTftIdx (
    Ptr<const RoutingInfo> rInfo, uint16_t activeTfts = 0) const;

  /**
   * Install OpenFlow match rules for the aggregated MTC bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool MtcAggBearerInstall (Ptr<RoutingInfo> rInfo);

  /**
   * Check for available resources on P-GW TFT switch for this bearer request.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool PgwBearerRequest (Ptr<RoutingInfo> rInfo);

  /**
   * Install OpenFlow rules for downlink packet filtering on the P-GW TFT
   * switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
   *        be get from rInfo->GetPgwTftIdx ().
   * \param forceMeterInstall Force the meter entry installation even when the
   *        meterInfo->IsDownInstalled () is true.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesInstall (
    Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
    bool forceMeterInstall = false);

  /**
   * Remove OpenFlow rules for downlink packet filtering from P-GW TFT switch.
   * \param rInfo The routing information to process.
   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
   *        be get from rInfo->GetPgwTftIdx ().
   * \param keepMeterFlag Don't set the meterInfo->IsDownInstalled () flag to
   *        false when removing the meter entry.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesRemove (
    Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
    bool keepMeterFlag = false);

  /**
   * Periodically check for the P-GW TFT processing load and flow table usage
   * to update the adaptive mechanism.
   */
  void PgwTftCheckUsage (void);

  /** Initialize static attributes only once. */
  static void StaticInitialize (void);

  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, BearerContextList_t>
  m_sessionCreatedTrace;

  /** The P-GW TFT stats trace source, fired at PgwTftCheckUsage. */
  TracedCallback<struct PgwTftStats> m_pgwTftStatsTrace;

  // Internal mechanisms for performance improvement.
  OperationMode         m_priorityQueues; //!< DSCP priority queues mechanism.
  OperationMode         m_slicing;        //!< Network slicing mechanism.

  // P-GW metadata.
  std::vector<uint64_t> m_pgwDpIds;       //!< Datapath IDs.
  Ipv4Address           m_pgwS5Addr;      //!< S5 IP address for uplink.
  std::vector<uint32_t> m_pgwS5PortsNo;   //!< S5 port numbers.
  uint32_t              m_pgwSgiPortNo;   //!< SGi port number.

  // P-GW TFT adaptive mechanism.
  OperationMode         m_tftAdaptive;    //!< P-GW adaptive mechanism.
  uint8_t               m_tftLevel;       //!< Current adaptive level.
  OperationMode         m_tftBlockPolicy; //!< Overload block policy.
  double                m_tftBlockThs;    //!< Block threshold.
  double                m_tftJoinThs;     //!< Join threshold.
  double                m_tftSplitThs;    //!< Split threshold.
  uint16_t              m_tftSwitches;    //!< Max number of TFT switches.
  DataRate              m_tftMaxLoad;     //!< Processing capacity.
  uint32_t              m_tftTableSize;   //!< Flow table size.

  // Traffic aggregation mechanisms.
  OperationMode         m_mtcAggregation; //!< MTC aggregation mechanism.
  OperationMode         m_htcAggregation; //!< HTC aggregation mechanism.
  double                m_htcAggGbrThs;   //!< Bandwidth threshold for GBR.
  double                m_htcAggNonThs;   //!< Bandwidth threshold for Non-GBR.

  // Internal members and attributes.
  EpcS5SapPgw*          m_s5SapPgw;       //!< P-GW side of the S5 SAP.
  Time                  m_timeout;        //!< Controller internal timeout.

  /** Map saving EPS QCI / IP DSCP value. */
  typedef std::map<EpsBearer::Qci, Ipv4Header::DscpType> QciDscpMap_t;

  /** Map saving IP DSCP value / OpenFlow queue id. */
  typedef std::map<Ipv4Header::DscpType, uint32_t> DscpQueueMap_t;

  /** Map saving IP DSCP value / IP ToS. */
  typedef std::map<Ipv4Header::DscpType, uint8_t> DscpTosMap_t;

  static const uint16_t m_flowTimeout;    //!< Timeout for flow entries.
  static uint32_t       m_teidCount;      //!< TEID counter.
  static QciDscpMap_t   m_qciDscpTable;   //!< DSCP mapped values.
  static DscpQueueMap_t m_dscpQueueTable; //!< OpenFlow queue id mapped values.
  static DscpTosMap_t   m_dscpTosTable;   //!< IP ToS mapped values.
};

};  // namespace ns3
#endif // EPC_CONTROLLER_H
