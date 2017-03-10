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
#include "epc-controller.h"

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
   * Notify this controller when the S-GW control plane implemented by this
   * SDRAN controller receives a create session request message. This
   * controller will forward this message to the EPC controller, so it can
   * configure the switches for GTP routing.
   * \param imsi The IMSI UE identifier.
   * \param cellId The eNB CellID to which the IMSI UE is attached to.
   * \param enbAddr The eNB S1-U IPv4 address.
   * \param sgwAddr The S-GW S1-U IPv4 address.
   * \param bearerList The list of context bearers created.
   */
  virtual void NotifySessionCreated (
    uint64_t imsi, uint16_t cellId, Ipv4Address enbAddr, Ipv4Address sgwAddr,
    BearerList_t bearerList);

  /**
   * Notify this controller of a new S-GW connected to the OpenFlow backhaul
   * network over over the S5 interface.
   * \param sgwS5PortNum The S5 port number on the S-GW OpenFlow switch.
   * \param sgwS5Dev The S5 device attached to the S-GW OpenFlow switch.
   */
  virtual void NotifySgwAttach (
    uint32_t sgwS5PortNum, Ptr<NetDevice> sgwS5Dev);

  /**
   * Notify this controller of a new or eNB connected to S-GW node over the
   * S1-U interface.
   * \param sgwS1uPortNum The S1-U port number on the S-GW OpenFlow switch.
   * TODO
   */
  virtual void NotifyEnbAttach (
    uint32_t sgwS1uPortNum);

  /**
   * Set the EPC controller application pointer on this SDRAN controller to
   * simplify the communication between controllers on the S5 interface.
   * \param epcCtrlApp The EPC controller application.
   */
  void SetEpcController (Ptr<EpcController> epcCtrlApp);

  /**
   * Get a pointer to the MME side of the S1-AP SAP.
   * \return the S1-AP SAP.
   */
  EpcS1apSapMme* GetS1apSapMme (void) const;

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
  void DoModifyBearerRequest  (EpcS11SapSgw::ModifyBearerRequestMessage  msg);
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  req);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req);
  //\}

private:
  Ptr<EpcController>    m_epcCtrlApp;   //!< EPC controller app.
  Ipv4Address           m_sgwS5Addr;    //!< S-GW S5 IP address.

  // MME communication.
  Ptr<SdmnMme>          m_mme;          //!< MME element.
  EpcS11SapMme*         m_s11SapMme;    //!< MME side of the S11 SAP.
  EpcS11SapSgw*         m_s11SapSgw;    //!< S-GW side of the S11 SAP.
};

};  // namespace ns3
#endif // SDRAN_CONTROLLER_H

