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
#include "../info/routing-info.h"
#include "../info/connection-info.h"
#include "../info/meter-info.h"
#include "../info/gbr-info.h"
#include "../info/ue-info.h"
#include "../info/enb-info.h"

namespace ns3 {

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
  EpcController ();           //!< Default constructor.
  virtual ~EpcController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Request a new dedicated EPS bearer. This is used to check for necessary
   * resources in the network (mainly available data rate for GBR bearers).
   * When returning false, it aborts the bearer creation process.
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the teid for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \param teid The teid for this bearer, if already defined.
   * \returns True if succeeded (the bearer creation process will proceed),
   *          false otherwise (the bearer creation process will abort).
   */
  virtual bool RequestDedicatedBearer (EpsBearer bearer, uint32_t teid);

  /**
   * Release a dedicated EPS bearer.
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the teid for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \param teid The teid for this bearer, if already defined.
   * \return True if succeeded, false otherwise.
   */
  virtual bool ReleaseDedicatedBearer (EpsBearer bearer, uint32_t teid);

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
   * Notify this controller of a new connection between two switches in the
   * OpenFlow backhaul network.
   * \param cInfo The connection information.
   */
  virtual void NotifySwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this controller that all connections between switches have already
   * been configure and the topology is finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyTopologyBuilt (OFSwitch13DeviceContainer devices);

  /**
   * Get the active P-GW TFT index for a given traffic flow.
   * \param rInfo The routing information to process.
   * \return The P-GW TFT index.
   */
  uint16_t GetPgwTftIdx (Ptr<const RoutingInfo> rInfo) const;

  /**
   * Get the P-GW load balancing mechanism status.
   * \return The P-GW load balancing status.
   */
  bool GetPgwLoadBalancing (void) const;

  /**
   * Get The P-GW side of the S5 SAP.
   * \return The P-GW side of the S5 SAP.
   */
  EpcS5SapPgw* GetS5SapPgw (void) const;

  /**
   * Retrieve stored mapped value for a specific EPS QCI.
   * \param qci The EPS bearer QCI.
   * \return The IP DSCP mapped value for this QCI.
   */
  static uint16_t GetDscpValue (EpsBearer::Qci qci);

  /**
   * TracedCallback signature for new bearer request.
   * \param ok True when the bearer request/release processes succeeds.
   * \param rInfo The routing information for this bearer tunnel.
   */
  typedef void (*BearerTracedCallback)(bool ok, Ptr<const RoutingInfo> rInfo);

  /**
   * TracedCallback signature for session created trace source.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param bearerList The list of context bearers created.
   */
  typedef void (*SessionCreatedTracedCallback)(
    uint64_t imsi, uint16_t cellId, BearerContextList_t bearerList);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /** \name Topology methods.
   * These virtual methods must be implemented by topology subclasses, as they
   * are dependent on the backhaul OpenFlow network topology.
   */
  //\{
  /**
   * Install TEID routing OpenFlow match rules into backhaul switches.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Remove TEID routing OpenFlow match rules from backhaul switches.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyRemoveRouting (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer request and reserve backhaul bandwidth.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Release the backhaul bandwidth previously reserved for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  virtual bool TopologyBearerRelease (Ptr<RoutingInfo> rInfo) = 0;
  //\}

  // Inherited from OFSwitch13Controller.
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  virtual ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  // Inherited from OFSwitch13Controller.

  // Class attributes.
  bool m_voipQos;            //!< VoIP QoS with queues.
  bool m_nonGbrCoexistence;  //!< Non-GBR coexistence.

private:
  /**
   * Enable/Disable the P-GW TFT load balancing mechanism.
   * \param value The value to set.
   */
  void SetPgwLoadBalancing (bool value);

  /**
   * Install OpenFlow rules for downlink packet filtering on the P-GW TFT
   * switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \param pgwTftDpId The P-GW TFT datapath ID.
   * \param pgwTftS5PortNo The S5 port number on the P-GW TFT switch.
   * \param forceMeterInstall Force the meter entry installation even when the
   *        rInfo->IsDownInstalled () is true (use this only when moving rules
   *        between P-GW TFT switches).
   * \return True if succeeded, false otherwise.
   */
  bool InstallPgwSwitchRules (
    Ptr<RoutingInfo> rInfo, uint64_t pgwTftDpId, uint32_t pgwTftS5PortNo,
    bool forceMeterInstall = false);

  /**
   * Remove OpenFlow rules for downlink packet filtering from P-GW TFT switch.
   * \param rInfo The routing information to process.
   * \param pgwTftDpId The P-GW TFT datapath ID.
   * \param keepMeterFlag Don't update the rInfo->IsDownInstalled () flag when
   *                      removing the meter entry (use this only when moving
                          rules between P-GW TFT switches).
   * \return True if succeeded, false otherwise.
   */
  bool RemovePgwSwitchRules (
    Ptr<RoutingInfo> rInfo, uint64_t pgwTftDpId, bool keepMeterFlag = false);

  /**
   * Install OpenFlow match rules for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool InstallBearer (Ptr<RoutingInfo> rInfo);

  /**
   * Remove OpenFlow match rules for this bearer.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool RemoveBearer (Ptr<RoutingInfo> rInfo);

  /** \name Methods for the S5 SAP P-GW control plane. */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoModifyBearerRequest  (EpcS11SapSgw::ModifyBearerRequestMessage  msg);
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  msg);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage msg);
  //\}

  /** Initialize static attributes only once. */
  static void StaticInitialize (void);

  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, BearerContextList_t> m_sessionCreatedTrace;

  // P-GW metadata
  std::vector<uint64_t>   m_pgwDpIds;     //!< P-GW datapath IDs.
  std::vector<uint32_t>   m_pgwS5PortsNo; //!< P-GW S5 ports no.
  Ipv4Address             m_pgwS5Addr;    //!< P-GW S5 IP address for uplink.
  uint32_t                m_pgwSgiPortNo; //!< P-GW SGi port no.
  bool                    m_pgwTftLb;     //!< P-GW TFT load balancing.
  bool                    m_pgwAutoTftLb; //!< Auto P-GW TFT load balancing.

  // S-GW communication.
  EpcS5SapPgw*            m_s5SapPgw;     //!< P-GW side of the S5 SAP.

  /** Map saving EpsBearer::Qci / IP DSCP value. */
  typedef std::map<EpsBearer::Qci, uint16_t> QciDscpMap_t;

  /**
   * TEID counter, used to allocate new GTP-U TEID values.
   * \internal This counter is initialized at 0x0000000F, reserving the first
   *           values for controller usage.
   */
  static uint32_t         m_teidCount;
  static const uint16_t   m_flowTimeout;  //!< Timeout for flow entries.
  static QciDscpMap_t     m_qciDscpTable; //!< DSCP mapped values.
};

};  // namespace ns3
#endif // EPC_CONTROLLER_H
