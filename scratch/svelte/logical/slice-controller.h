/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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

#ifndef SLICE_CONTROLLER_H
#define SLICE_CONTROLLER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class SvelteMme;

/**
 * \ingroup svelteLogical
 * This is the abstract base class for a logical LTE slice controller, which
 * should be extended in accordance to the desired network configuration. This
 * slice controller is responsible for implementing S/P-GW control planes and
 * configuring the S/P-GW OpenFlow switches at user plane.
 */
class SliceController : public OFSwitch13Controller
{
  friend class MemberEpcS11SapSgw<SliceController>;

public:
  SliceController ();           //!< Default constructor.
  virtual ~SliceController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

//  /**
//   * Release a dedicated EPS bearer.
//   * \internal Current implementation assumes that each application traffic
//   *           flow is associated with a unique bearer/tunnel. Because of that,
//   *           we can use only the teid for the tunnel to prepare and install
//   *           route. If we would like to aggregate traffic from several
//   *           applications into same bearer we will need to revise this.
//   * \param teid The teid for this bearer, if already defined.
//   * \param imsi uint64_t IMSI UE identifier.
//   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
//   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
//   * \return True if succeeded, false otherwise.
//   */
//  virtual bool DedicatedBearerRelease (
//    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);
//
//  /**
//   * Request a new dedicated EPS bearer. This is used to check for necessary
//   * resources in the network (mainly available data rate for GBR bearers).
//   * When returning false, it aborts the bearer creation process.
//   * \internal Current implementation assumes that each application traffic
//   *           flow is associated with a unique bearer/tunnel. Because of that,
//   *           we can use only the teid for the tunnel to prepare and install
//   *           route. If we would like to aggregate traffic from several
//   *           applications into same bearer we will need to revise this.
//   * \param teid The teid for this bearer, if already defined.
//   * \param imsi uint64_t IMSI UE identifier.
//   * \param cellId uint16_t eNB CellID to which the IMSI UE is attached to.
//   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
//   * \returns True if succeeded (the bearer creation process will proceed),
//   *          false otherwise (the bearer creation process will abort).
//   */
//  virtual bool DedicatedBearerRequest (
//    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);
//
//  /**
//   * Notify this controller of a new or eNB connected to S-GW node over the
//   * S1-U interface.
//   * \param cellID The eNB cell ID.
//   * \param sgwS1uPortNo The S1-U port number on the S-GW OpenFlow switch.
//   */
//  virtual void NotifyEnbAttach (uint16_t cellId, uint32_t sgwS1uPortNo);
//
//  /**
//   * Notify this controller of the S-GW connected to the OpenFlow backhaul
//   * network over over the S5 interface.
//   * \param sgwS5PortNo The S5 port number on the S-GW OpenFlow switch.
//   * \param sgwS5Dev The S5 device attached to the S-GW OpenFlow switch.
//   * \param mtcGbrTeid The TEID for the MTC GBR aggregation tunnel.
//   * \param mtcNonTeid The TEID for the MTC Non-GBR aggregation tunnel.
//   */
//  virtual void NotifySgwAttach (
//    uint32_t sgwS5PortNo, Ptr<NetDevice> sgwS5Dev, uint32_t mtcGbrTeid,
//    uint32_t mtcNonTeid);
//
//  /**
//   * Notify this controller that all P-GW switches have already been
//   * configured and the connections between them are finished.
//   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
//   */
//  virtual void NotifyPgwBuilt (OFSwitch13DeviceContainer devices);
//
//  /**
//   * Notify this controller of the P-GW main switch connected to the OpenFlow
//   * backhaul network over the S5 interface, and to the web server over the SGi
//   * interface.
//   * \param pgwSwDev The OpenFlow P-GW main switch device.
//   * \param pgwS5PortNo The S5 port number on the P-GW main switch.
//   * \param pgwSgiPortNo The SGi port number on the P-GW main switch.
//   * \param pgwS5Dev The S5 device attached to the P-GW main switch.
//   * \param webSgiDev The SGi device attached to the Web server.
//   */
//  virtual void NotifyPgwMainAttach (
//    Ptr<OFSwitch13Device> pgwSwDev, uint32_t pgwS5PortNo,
//    uint32_t pgwSgiPortNo, Ptr<NetDevice> pgwS5Dev, Ptr<NetDevice> webSgiDev);
//
//  /**
//   * Notify this controller of a new P-GW TFT switch connected to the OpenFlow
//   * backhaul network over the S5 interface and to the P-GW main switch over
//   * internal interface.
//   * \param pgwTftCounter The counter for P-GW TFT switches.
//   * \param pgwSwDev The OpenFlow P-GW TFT switch device.
//   * \param pgwS5PortNo The S5 port number on the P-GW TFT switch.
//   * \param pgwMainPortNo The port number on the P-GW main switch.
//   */
//  virtual void NotifyPgwTftAttach (
//    uint16_t pgwTftCounter, Ptr<OFSwitch13Device> pgwSwDev,
//    uint32_t pgwS5PortNo, uint32_t pgwMainPortNo);
//
//  /**
//   * Get the SDRAN controller pointer from the global map for this cell ID.
//   * \param cellID The eNB cell ID.
//   * \return The SDRAN controller pointer.
//   */
//  static Ptr<SliceController> GetPointer (uint16_t cellId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from OFSwitch13Controller.
  virtual ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);
  virtual void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);
  // Inherited from OFSwitch13Controller.

private:
  /** \name Methods for the S11 SAP S-GW control plane. */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  msg);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage msg);
  void DoModifyBearerRequest  (EpcS11SapSgw::ModifyBearerRequestMessage  msg);
  //\}

//  /**
//   * Configure the S-GW with OpenFlow rules for packet forwarding.
//   * \attention To avoid conflicts with old entries, increase the routing
//   *            priority before installing S-GW rules.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool SgwRulesInstall (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Remove OpenFlow rules for packet forwarding from S-GW.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool SgwRulesRemove (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Register the SDRAN controller into global map for further usage.
//   * \param ctrl The SDRAN controller pointer.
//   * \param cellId The cell ID used to index the map.
//   */
//  static void RegisterController (Ptr<SliceController> ctrl, uint16_t cellId);

  // MME communication.
  Ptr<SvelteMme>        m_mme;          //!< MME element.
  EpcS11SapMme*         m_s11SapMme;    //!< MME side of the S11 SAP.
  EpcS11SapSgw*         m_s11SapSgw;    //!< S-GW side of the S11 SAP.

//  /** Map saving cell ID / SDRAN controller pointer. */
//  typedef std::map<uint16_t, Ptr<SliceController> > CellIdCtrlMap_t;
//  static CellIdCtrlMap_t m_cellIdCtrlMap; //!< Global SDRAN ctrl by cell ID.
};

} // namespace ns3
#endif // SLICE_CONTROLLER_H
