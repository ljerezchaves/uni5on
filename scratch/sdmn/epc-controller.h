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
#include "info/routing-info.h"
#include "info/connection-info.h"
#include "info/meter-info.h"
#include "info/gbr-info.h"
#include "info/ue-info.h"
#include "info/enb-info.h"
#include "stats/admission-stats-calculator.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * This is the abstract base class for the OpenFlow EPC controller, which
 * should be extended in accordance to OpenFlow backhaul network topology. This
 * controller implements the logic for traffic routing and engineering within
 * the OpenFlow backhaul network. It is also responsible for implementing the
 * P-GW control plane and for configuring the P-GW OpenFlow user plane.
 */
class EpcController : public OFSwitch13Controller
{
  friend class MemberEpcS11SapSgw<EpcController>;

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
   * \param teid The teid for this bearer, if already defined.
   * \param imsi uint64_t IMSI UE identifier.
   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful (the bearer creation process will proceed),
   *          false otherwise (the bearer creation process will abort).
   */
  virtual bool RequestDedicatedBearer (EpsBearer bearer, uint64_t imsi,
    uint16_t cellId, uint32_t teid);

  /**
   * Release a dedicated EPS bearer.
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the teid for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
   * \param teid The teid for this bearer, if already defined.
   * \param imsi uint64_t IMSI UE identifier.
   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns true if successful, false otherwise.
   */
  virtual bool ReleaseDedicatedBearer (EpsBearer bearer, uint64_t imsi,
    uint16_t cellId, uint32_t teid);

  /**
   * Notify this controller of the P-GW and Internet Web server connection over
   * the SGi interface. This function will configure the P-GW datapath.
   * \param pgwSwDev The OpenFlow P-GW switch device.
   * \param pgwSgiDev The SGi device attached to the OpenFlow P-GW switch.
   * \param pgwSgiIp The IPv4 address assigned to the P-GW SGi device.
   * \param sgiPortNo The SGi port number on the P-GW OpenFlow switch.
   * \param s5PortNo The S5 port number on the P-GW OpenFlow switch.
   * \param webSgiDev The SGi device attached to the Internet Web server.
   * \param webIp The IPv4 address assigned to the Internet Web server SGi dev.
   */
  virtual void NewPgwAttach (Ptr<OFSwitch13Device> pgwSwDev,
    Ptr<NetDevice> pgwSgiDev, Ipv4Address pgwSgiIp, uint32_t sgiPortNo,
    uint32_t s5PortNo, Ptr<NetDevice> webSgiDev, Ipv4Address webIp);

  /**
   * Notify this controller of a new S-GW or P-GW connected to the S5 OpenFlow
   * network over some switch port.
   * \param swtchDev The OpenFlow switch device.
   * \param portNo The port number created at the OpenFlow switch.
   * \param gwDev The device created at the S/P-GW node (this is not the one
   * added as port to switch, this is the 'other' end of this connection,
   * associated with the S-GW or P-GW node).
   * \param gwIp The IPv4 address assigned to the gwDev.
   */
  virtual void NewS5Attach (Ptr<OFSwitch13Device> swtchDev, uint32_t portNo,
    Ptr<NetDevice> gwDev, Ipv4Address gwIp);

  /**
   * Notify this controller of a new connection between two switches in the
   * OpenFlow network. The user is supposed to connect this function as trace
   * sink for EpcNetwork::NewSwitchConnection trace source.
   * \param cInfo The connection information and metadata.
   */
  virtual void NewSwitchConnection (Ptr<ConnectionInfo> cInfo);

  /**
   * Notify this controller that all connection between switches have already
   * been configure and the topology is finished. The user is supposed to
   * connect this function as trace sink for EpcNetwork::TopologyBuilt trace
   * source.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void TopologyBuilt (OFSwitch13DeviceContainer devices);

  /**
   * Notify this controller when the MME receives a context created response
   * message. This is used to notify this controller with the list of bearers
   * context created. With this information, the controller can configure the
   * switches for GTP routing. The user is supposed to connect this function as
   * trace sink for EpcMme::SessionCreated trace source.
   * \see 3GPP TS 29.274 7.2.1 for CreateSessionRequest message format.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB IPv4 address.
   * \param pgwAddr The P-GW IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  virtual void NotifySessionCreated (uint64_t imsi, uint16_t cellId,
    Ipv4Address enbAddr, Ipv4Address pgwAddr, BearerList_t bearerList);

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
   * \param enbAddr The eNB IPv4 address.
   * \param pgwAddr The P-GW IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  typedef void (*SessionCreatedTracedCallback)(uint64_t imsi, uint16_t cellId,
      Ipv4Address enbAddr, Ipv4Address pgwAddr, BearerList_t bearerList);

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
   * Configure the switches with OpenFlow commands for TEID routing, based on
   * network topology information.
   * \attention This function must increase the routing priority before
   *            installing the rules, to avoid conflicts with old entries.
   * \param rInfo The routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   * \return True if configuration succeeded, false otherwise.
   */
  virtual bool TopologyInstallRouting (Ptr<RoutingInfo> rInfo,
    uint32_t buffer = OFP_NO_BUFFER) = 0;

  /**
   * Remove TEID routing rules from switches.
   * \param rInfo The routing information to remove.
   * \return True if remove succeeded, false otherwise.
   */
  virtual bool TopologyRemoveRouting (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer resource request and bit rate allocation based on
   * network topology information.
   * \param rInfo The routing information to process.
   * \return True when the request is satisfied.
   */
  virtual bool TopologyBearerRequest (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * Process the bearer and bit rate release based on network topology
   * information.
   * \param rInfo The routing information to process.
   * \return True when the resources are successfully released.
   */
  virtual bool TopologyBearerRelease (Ptr<RoutingInfo> rInfo) = 0;

  /**
   * To avoid flooding problems when broadcasting packets (like in ARP
   * protocol), let's find a Spanning Tree and drop packets at selected ports
   * when flooding (OFPP_FLOOD). This is accomplished by configuring the port
   * with OFPPC_NO_FWD flag (0x20).
   */
  virtual void TopologyCreateSpanningTree () = 0;
  //\}

  /**
   * Configure the P-GW with OpenFlow rules for downlink TFT packet filtering.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing P-GW TFT rules.
   * \param rInfo The routing information to configure.
   * \param buffer The buffered packet to apply this rule to.
   */
  void InstallPgwTftRules (Ptr<RoutingInfo> rInfo,
                           uint32_t buffer = OFP_NO_BUFFER);

  // Inherited from OFSwitch13Controller.
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  virtual ofl_err HandlePacketIn (
    ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  // Inherited from OFSwitch13Controller.

private:
  /**
   * EpcController inner friend utility class
   * used to initialize static DSCP map table.
   */
  class QciDscpInitializer
  {
  public:
    /** Initializer function. */
    QciDscpInitializer ();
  };
  friend class QciDscpInitializer;

  /**
   * Static instance of Initializer. When this is created, its constructor
   * initializes the EpcController s' static DSCP map table.
   */
  static QciDscpInitializer initializer;

//
// Member variables
//
protected:
  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, Ipv4Address, Ipv4Address, BearerList_t>
    m_sessionCreatedTrace;

  bool                  m_voipQos;            //!< VoIP QoS with queues.
  bool                  m_nonGbrCoexistence;  //!< Non-GBR coexistence.
  static const uint16_t m_flowTimeout;        //!< Timeout for flow entries.

private:
  Ptr<AdmissionStatsCalculator> m_admissionStats; //!< Admission statistics.
  uint64_t                      m_pgwDpId;        //!< P-GW datapath ID.
  uint32_t                      m_pgwS5Port;      //!< P-GW S5 port no.

  /** Map saving EpsBearer::Qci / IP DSCP value. */
  typedef std::map<EpsBearer::Qci, uint16_t> QciDscpMap_t;
  static QciDscpMap_t           m_qciDscpTable;   //!< DSCP mapped values.

//
// Everything below is from S-GW control plane.
// FIXME Move to the SDRAN controller.
//
public:
  /** \name Methods for the S-GW control plane. */
  //\{
  /**
   * Set the MME side of the S11 SAP.
   * \param s the MME side of the S11 SAP.
   */
  void SetS11SapMme (EpcS11SapMme * s);

  /**
   * Get the S-GW side of the S11 SAP.
   * \return the SGW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw ();
  //\}

private:
  /** \name Methods for the S11 SAP S-GW control plane. */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoModifyBearerRequest (EpcS11SapSgw::ModifyBearerRequestMessage msg);
  void DoDeleteBearerCommand (EpcS11SapSgw::DeleteBearerCommandMessage req);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req);
  //\}

  /**
   * TEID counter, used to allocate new GTP-U TEID values.
   * \internal This counter is initialized at 0x0000000F, reserving the first
   *           values for controller usage.
   */
  uint32_t m_teidCount;

  EpcS11SapMme* m_s11SapMme;      //!< MME side of the S11 SAP
  EpcS11SapSgw* m_s11SapSgw;      //!< S-GW side of the S11 SAP
};

};  // namespace ns3
#endif // EPC_CONTROLLER_H

