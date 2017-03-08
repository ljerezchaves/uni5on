/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef SDRAN_CONTROLLER_H
#define SDRAN_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "info/routing-info.h"
#include "info/ue-info.h"
#include "info/enb-info.h"
#include "sdmn-mme.h"

namespace ns3 {

/**
 * \ingroup sdmn
 * This is the class for the OpenFlow SDRAN controller. This controller is
 * responsible for implementing the S-GW control plane and for configuring the
 * S-GW OpenFlow user plane.
 */
class SdranController : public OFSwitch13Controller
{
  friend class MemberEpcS11SapSgw<SdranController>;

public:
  SdranController ();           //!< Default constructor.
  virtual ~SdranController ();  //!< Dummy destructor, see DoDispose.

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
  virtual bool RequestDedicatedBearer (
    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);

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
  virtual bool ReleaseDedicatedBearer (
    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);

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
  virtual void NotifySessionCreated (
    uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address pgwAddr,
    BearerList_t bearerList);

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
  virtual void NotifyS5Attach (
    Ptr<OFSwitch13Device> swtchDev, uint32_t portNo, Ptr<NetDevice> gwDev,
    Ipv4Address gwIp);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  // Inherited from OFSwitch13Controller.
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  virtual ofl_err HandlePacketIn (
    ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  // Inherited from OFSwitch13Controller.

private:
  /** \name Methods for the S11 SAP S-GW control plane. */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoModifyBearerRequest (EpcS11SapSgw::ModifyBearerRequestMessage msg);
  void DoDeleteBearerCommand (EpcS11SapSgw::DeleteBearerCommandMessage req);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req);
  //\}

protected:
  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<bool, Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, Ipv4Address, Ipv4Address, BearerList_t>
  m_sessionCreatedTrace;

private:
  /**
   * TEID counter, used to allocate new GTP-U TEID values.
   * \internal This counter is initialized at 0x0000000F, reserving the first
   *           values for controller usage.
   */
  uint32_t m_teidCount;

  // MME communication.
  Ptr<SdmnMme>  m_mme;            //!< MME element.
  EpcS11SapMme* m_s11SapMme;      //!< MME side of the S11 SAP.
  EpcS11SapSgw* m_s11SapSgw;      //!< S-GW side of the S11 SAP.
};

};  // namespace ns3
#endif // SDRAN_CONTROLLER_H

