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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef SLICE_CONTROLLER_H
#define SLICE_CONTROLLER_H

// Pipeline tables at OpenFlow S/P-GW switches.
#define PGW_MAIN_TAB  0
#define PGW_TFT_TAB   0
#define SGW_MAIN_TAB  0
#define SGW_DL_TAB    1
#define SGW_UL_TAB    2

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "../metadata/sgw-info.h"
#include "../uni5on-common.h"

namespace ns3 {

class BackhaulController;
class EnbInfo;
class RoutingInfo;
class Uni5onMme;
class SgwInfo;
class PgwInfo;

/** A list of slice controller applications. */
typedef std::vector<Ptr<SliceController> > SliceControllerList_t;

/**
 * \ingroup uni5onLogical
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
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \returns True if succeeded (the bearer creation process will proceed),
   *          false otherwise (the bearer creation process will abort).
   */
  virtual bool DedicatedBearerRequest (EpsBearer bearer, uint64_t imsi,
                                       uint32_t teid);

  /**
   * Release a dedicated EPS bearer.
   * \internal Current implementation assumes that each application traffic
   *           flow is associated with a unique bearer/tunnel. Because of that,
   *           we can use only the teid for the tunnel to prepare and install
   *           route. If we would like to aggregate traffic from several
   *           applications into same bearer we will need to revise this.
   * \param teid The teid for this bearer, if already defined.
   * \param imsi uint64_t IMSI UE identifier.
   * \param bearer EpsBearer bearer QoS characteristics of the bearer.
   * \return True if succeeded, false otherwise.
   */
  virtual bool DedicatedBearerRelease (EpsBearer bearer, uint64_t imsi,
                                       uint32_t teid);

  /**
   * Get the slice ID for this controller.
   * \return The slice ID.
   */
  SliceId GetSliceId (void) const;

  /**
   * \name Private member accessors for backhaul interface metadata.
   * \return The requested information.
   */
  //\{
  double    GetGbrBlockThs            (void) const;
  int       GetPriority               (void) const;
  int       GetQuota                  (void) const;
  OpMode    GetSharing                (void) const;
  OpMode    GetAggregation            (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW metadata.
   * \return The requested information.
   */
  //\{
  OpMode    GetPgwBlockPolicy         (void) const;
  double    GetPgwBlockThs            (void) const;
  OpMode    GetPgwTftLoadBal          (void) const;
  double    GetPgwTftJoinThs          (void) const;
  double    GetPgwTftSplitThs         (void) const;
  //\}

  /**
   * \name Private member accessors for S-GW metadata.
   * \return The requested information.
   */
  //\{
  OpMode    GetSgwBlockPolicy         (void) const;
  double    GetSgwBlockThs            (void) const;
  //\}

  /**
   * Get The S-GW side of the S11 SAP.
   * \return The S-GW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw (void) const;

  /**
   * Notify this controller of the P-GW connected to the OpenFlow backhaul
   * network over the S5 interface, and to the web server over the SGi
   * interface.
   * \param pgwInfo The P-GW metadata.
   * \param webSgiDev The SGi device on the Web server.
   */
  virtual void NotifyPgwAttach (Ptr<PgwInfo> pgwInfo,
                                Ptr<NetDevice> webSgiDev);

  /**
   * Notify this controller of the S-GW connected to the OpenFlow backhaul
   * network over the S1-U and S5 interfaces.
   * \param sgwInfo The S-GW metadata.
   */
  virtual void NotifySgwAttach (Ptr<SgwInfo> sgwInfo);

  /**
   * Configure this controller with slice network attributes.
   * \param ueAddr The UE network address.
   * \param ueMask The UE network mask.
   * \param webAddr The Internet network address.
   * \param webMask The Internet network mask.
   */
  void SetNetworkAttributes (Ipv4Address ueAddr, Ipv4Mask ueMask,
                             Ipv4Address webAddr, Ipv4Mask webMask);

  /**
   * TracedCallback signature for the P-GW TFT load balancing trace source.
   * \param pgwInfo The P-GW metadata.
   * \param nextLevel The load balacing level for next cycle.
   * \param bearersMoved The number of bearers moved.
   */
  typedef void (*PgwTftStatsTracedCallback)(
    Ptr<const PgwInfo> pgwInfo, uint32_t nextLevel, uint32_t bearersMoved);

  /**
   * TracedCallback signature for session created trace source.
   * \param imsi The IMSI UE identifier.
   * \param bearerList The list of bearer contexts created.
   */
  typedef void (*SessionCreatedTracedCallback)(
    uint64_t imsi, BearerCreatedList_t bearerList);

  /**
   * TracedCallback signature for session modified trace source.
   * \param imsi The IMSI UE identifier.
   * \param bearerList The list of bearer contexts modified.
   */
  typedef void (*SessionModifiedTracedCallback)(
    uint64_t imsi, BearerModifiedList_t bearerList);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Schedule a dpctl command to be executed after a delay.
   * \param delay The relative execution time for this command.
   * \param dpId The OpenFlow datapath ID.
   * \param textCmd The dpctl command to be executed.
   */
  void DpctlSchedule (Time delay, uint64_t dpId, const std::string textCmd);

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
   * Update OpenFlow match rules for this bearer.
   * \attention Don't update the ueInfo with the destination eNB metadata
   *            before invoking this method.
   * \internal This method must increase the rInfo priority.
   * \param rInfo The routing information to process.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  bool BearerUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo);

  /**
   * \name Methods for the S11 SAP S-GW control plane.
   * \param msg The message sent here by the MME entity.
   * \internal Note the trick to avoid the need for allocating TEID on the S11
   *           interface using the IMSI as identifier.
   */
  //\{
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoDeleteBearerCommand  (EpcS11SapSgw::DeleteBearerCommandMessage  msg);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage msg);
  void DoModifyBearerRequest  (EpcS11SapSgw::ModifyBearerRequestMessage  msg);
  //\}

  /**
   * Get the active P-GW TFT index for a given traffic flow.
   * \param rInfo The routing information to process.
   * \param activeTfts The number of active P-GW TFT switches. When set to 0,
   *        the number of P-GW TFTs will be calculated considering the current
   *        load balancing level.
   * \return The P-GW TFT index.
   */
  uint16_t GetTftIdx (Ptr<const RoutingInfo> rInfo,
                      uint16_t activeTfts = 0) const;

  /**
   * Periodically check for the P-GW TFT processing load and flow table usage
   * to update the load balacing level.
   */
  void PgwTftLoadBalancing (void);

  /**
   * Check for available resources on P-GW TFT switch for this bearer request.
   * When any of the requested resources is not available, this method must set
   * the routing information with the block reason.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool PgwBearerRequest (Ptr<RoutingInfo> rInfo) const;

  /**
   * Install downlink packet filtering rules on the P-GW TFT OpenFlow switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesInstall (Ptr<RoutingInfo> rInfo);

  /**
   * Move downlink packet filtering rules from the source P-GW TFT OpenFlow
   * switch to the target one.
   * \param rInfo The routing information to process.
   * \param srcTftIdx The source P-GW TFT switch index.
   * \param dstTftIdx The target P-GW TFT switch index.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesMove (Ptr<RoutingInfo> rInfo, uint16_t srcTftIdx,
                     uint16_t dstTftIdx);

  /**
   * Remove downlink packet filtering rules from the P-GW TFT OpenFlow switch.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesRemove (Ptr<RoutingInfo> rInfo);

  /**
   * Check for available resources on S-GW switch for this bearer request. When
   * any of the requested resources is not available, this method must set
   * the routing information with the block reason.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool SgwBearerRequest (Ptr<RoutingInfo> rInfo) const;

  /**
   * Install packet forwarding rules on the S-GW OpenFlow switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing S-GW rules.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesInstall (Ptr<RoutingInfo> rInfo);

  /**
   * Remove packet forwarding rules from the S-GW OpenFlow switch.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesRemove (Ptr<RoutingInfo> rInfo);

  /**
   * Update packet forwarding rules on the S-GW OpenFlow switch after a
   * successful handover procedure.
   * \param rInfo The routing information to process.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesUpdate (Ptr<RoutingInfo> rInfo, Ptr<EnbInfo> dstEnbInfo);

  /**
   * Install individual TFT forwarding rules.
   * \param tft The Traffic Flow Template.
   * \param dir The traffic direction.
   * \param dpId The target switch datapath ID.
   * \param cmdStr The OpenFlow dpctl flow mod command.
   * \param actStr The OpenFlow dpctl action command
   * \return True if succeeded, false otherwise.
   */
  bool TftRulesInstall (Ptr<EpcTft> tft, Direction dir, uint64_t dpId,
                        std::string cmdStr, std::string actStr);

  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at DoCreateSessionRequest. */
  TracedCallback<uint64_t, BearerCreatedList_t> m_sessionCreatedTrace;

  /** The context modified trace source, fired at DoModifyBearerRequest. */
  TracedCallback<uint64_t, BearerModifiedList_t> m_sessionModifiedTrace;

  /** The P-GW TFT load balacing trace source, fired at PgwTftLoadBalancing. */
  TracedCallback<Ptr<const PgwInfo>, uint32_t, uint32_t> m_pgwTftLoadBalTrace;

  // Slice identification.
  SliceId                 m_sliceId;        //!< Logical slice ID.
  std::string             m_sliceIdStr;     //!< Slice ID string.

  // Infrastructure interface.
  Ptr<BackhaulController> m_backhaulCtrl;   //!< OpenFlow backhaul controller.
  double                  m_gbrBlockThs;    //!< Backhaul GBR block threshold.
  int                     m_slicePrio;      //!< slice priority.
  int                     m_linkQuota;      //!< Initial bandwitdh quota.
  OpMode                  m_linkSharing;    //!< Bandwitdh sharing mode.
  OpMode                  m_aggregation;    //!< Bearer traffic aggregation.

  // MME interface.
  Ptr<Uni5onMme>          m_mme;            //!< MME element.
  EpcS11SapMme*           m_s11SapMme;      //!< MME side of the S11 SAP.
  EpcS11SapSgw*           m_s11SapSgw;      //!< S-GW side of the S11 SAP.

  // Network configuration.
  Ipv4Address             m_ueAddr;         //!< UE network address.
  Ipv4Mask                m_ueMask;         //!< UE network mask.
  Ipv4Address             m_webAddr;        //!< Web network address.
  Ipv4Mask                m_webMask;        //!< Web network mask.

  // P-GW metadata and TFT load balancing mechanism.
  Ptr<PgwInfo>            m_pgwInfo;        //!< P-GW metadata for this slice.
  OpMode                  m_pgwBlockPolicy; //!< P-GW overload block policy.
  double                  m_pgwBlockThs;    //!< P-GW block threshold.
  OpMode                  m_tftLoadBal;     //!< P-GW TFT load balancing.
  double                  m_tftJoinThs;     //!< P-GW TFT join threshold.
  double                  m_tftSplitThs;    //!< P-GW TFT split threshold.
  bool                    m_tftStartMax;    //!< P-GW TFT start with maximum.
  Time                    m_tftTimeout;     //!< P-GW TFT load bal timeout.

  // S-GW metadata.
  Ptr<SgwInfo>            m_sgwInfo;        //!< S-GW metadata for this slice.
  OpMode                  m_sgwBlockPolicy; //!< S-GW overload block policy.
  double                  m_sgwBlockThs;    //!< S-GW block threshold.
};

} // namespace ns3
#endif // SLICE_CONTROLLER_H
