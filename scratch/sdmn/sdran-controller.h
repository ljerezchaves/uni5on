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
#include "epc-s5-sap.h"

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
  friend class MemberEpcS5SapSgw<SdranController>;

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
   * \returns True if succeeded (the bearer creation process will proceed),
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
   * \return True if succeeded, false otherwise.
   */
  virtual bool ReleaseDedicatedBearer (
    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);

  /**
   * Notify this controller of a new S-GW connected to the OpenFlow backhaul
   * network over over the S5 interface.
   * \param sgwS5PortNo The S5 port number on the S-GW OpenFlow switch.
   * \param sgwS5Dev The S5 device attached to the S-GW OpenFlow switch.
   */
  virtual void NotifySgwAttach (uint32_t sgwS5PortNo, Ptr<NetDevice> sgwS5Dev);

  /**
   * Notify this controller of a new or eNB connected to S-GW node over the
   * S1-U interface.
   * \param cellID The eNB cell ID.
   * \param sgwS1uPortNo The S1-U port number on the S-GW OpenFlow switch.
   */
  virtual void NotifyEnbAttach (uint16_t cellId, uint32_t sgwS1uPortNo);

  /** \name Private member accessors. */
  //\{
  Ipv4Address GetSgwS5Addr (void) const;
  EpcS1apSapMme* GetS1apSapMme (void) const;
  EpcS5SapSgw* GetS5SapSgw (void) const;

  void SetEpcCtlrApp (Ptr<EpcController> value);
  void SetSgwDpId (uint64_t value);
  //\}

  /**
   * Get the SDRAN controller pointer from the global map for this cell ID.
   * \param cellID The eNB cell ID.
   * \return The SDRAN controller pointer.
   */
  static Ptr<SdranController> GetPointer (uint16_t cellId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * Configure the S-GW with OpenFlow rules for packet forwarding.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing S-GW rules.
   * \param rInfo The routing information to configure.
   * \return True if succeeded, false otherwise.
   */
  virtual bool InstallSgwSwitchRules (Ptr<RoutingInfo> rInfo);

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
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  msg);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage msg);
  //\}

  /** \name Methods for the S5 SAP S-GW control plane. */
  //\{
  void DoCreateSessionResponse (EpcS11SapMme::CreateSessionResponseMessage msg);
  void DoModifyBearerResponse (EpcS11SapMme::ModifyBearerResponseMessage  msg);
  void DoDeleteBearerRequest  (EpcS11SapMme::DeleteBearerRequestMessage   msg);
  //\}

  /**
   * Register the SDRAN controller into global map for further usage.
   * \param ctrl The SDRAN controller pointer.
   * \param cellId The cell ID used to index the map.
   */
  static void RegisterController (Ptr<SdranController> ctrl, uint16_t cellId);

  uint64_t              m_sgwDpId;      //!< S-GW datapath ID.
  Ipv4Address           m_sgwS5Addr;    //!< S-GW S5 IP address.
  uint32_t              m_sgwS5PortNo;  //!< S-GW S5 port number.

  // P-GW communication.
  Ptr<EpcController>    m_epcCtrlApp;   //!< EPC controller app.
  EpcS5SapPgw*          m_s5SapPgw;     //!< P-GW side of the S5 SAP.
  EpcS5SapSgw*          m_s5SapSgw;     //!< S-GW side of the S5 SAP.

  // MME communication.
  Ptr<SdmnMme>          m_mme;          //!< MME element.
  EpcS11SapMme*         m_s11SapMme;    //!< MME side of the S11 SAP.
  EpcS11SapSgw*         m_s11SapSgw;    //!< S-GW side of the S11 SAP.

  /** Map saving cell ID / SDRAN controller pointer. */
  typedef std::map<uint16_t, Ptr<SdranController> > CellIdCtrlMap_t;
  static CellIdCtrlMap_t m_cellIdCtrlMap; //!< Global SDRAN ctrl by cell ID.
};

};  // namespace ns3
#endif // SDRAN_CONTROLLER_H
