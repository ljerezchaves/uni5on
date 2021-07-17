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
#define PGW_ULDL_TAB  0
#define PGW_TFT_TAB   0
#define SGW_MAIN_TAB  0
#define SGW_DL_TAB    1
#define SGW_UL_TAB    2

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class SliceController;
class TransportController;
class BearerInfo;
class EnbInfo;
class PgwInfo;
class SgwInfo;
class PgwuScaling;
class StatelessMme;

/** A list of slice controller applications. */
typedef std::vector<Ptr<SliceController>> SliceControllerList_t;

/**
 * \ingroup uni5onLogical
 * The logical EPC network controller.
 */
class SliceController : public OFSwitch13Controller
{
  friend class MemberEpcS11SapSgw<SliceController>;
  friend class PgwuScaling;

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
   * \name Private member accessors for controller metadata.
   * \return The requested information.
   */
  //\{
  SliceId   GetSliceId                (void) const;
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
   * Get the S-GW side of the S11 SAP.
   * \return The S-GW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw (void) const;

  /**
   * Notify this controller of the P-GW connected to the OpenFlow transport
   * network over the S5 interface, and to the web server over the SGi
   * interface.
   * \param pgwInfo The P-GW metadata.
   * \param webSgiDev The SGi device on the Web server.
   */
  virtual void NotifyPgwAttach (Ptr<PgwInfo> pgwInfo, Ptr<NetDevice> webSgiDev);

  /**
   * Notify this controller of the S-GW connected to the OpenFlow transport
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
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool BearerInstall (Ptr<BearerInfo> bInfo);

  /**
   * Remove OpenFlow match rules for this bearer.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool BearerRemove (Ptr<BearerInfo> bInfo);

  /**
   * Update OpenFlow match rules for this bearer.
   * \attention Don't update the ueInfo with the destination eNB metadata
   *            before invoking this method.
   * \internal This method must increase the bInfo priority.
   * \param bInfo The bearer information.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  bool BearerUpdate (Ptr<BearerInfo> bInfo, Ptr<EnbInfo> dstEnbInfo);

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
   * Check for available resources on P-GW TFT switch for this bearer request.
   * When any of the requested resources is not available, this method must set
   * the routing information with the block reason.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool PgwBearerRequest (Ptr<BearerInfo> bInfo) const;

  /**
   * Install downlink packet filtering rules on the P-GW TFT OpenFlow switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesInstall (Ptr<BearerInfo> bInfo);

  /**
   * Move downlink packet filtering rules from the source P-GW TFT OpenFlow
   * switch to the target one.
   * \param bInfo The bearer information.
   * \param srcTftIdx The source P-GW TFT switch index.
   * \param dstTftIdx The target P-GW TFT switch index.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesMove (Ptr<BearerInfo> bInfo, uint16_t srcTftIdx,
                     uint16_t dstTftIdx);

  /**
   * Remove downlink packet filtering rules from the P-GW TFT OpenFlow switch.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesRemove (Ptr<BearerInfo> bInfo);

  /**
   * Update the P-GW TFT level by reconfiguring UL and DL switches.
   * \param level The next operation level.
   * \return True if succeed, false otherwise.
   */
  bool PgwTftLevelUpdate (uint16_t level);

  /**
   * Check for available resources on S-GW switch for this bearer request. When
   * any of the requested resources is not available, this method must set
   * the routing information with the block reason.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool SgwBearerRequest (Ptr<BearerInfo> bInfo) const;

  /**
   * Install packet forwarding rules on the S-GW OpenFlow switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing S-GW rules.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesInstall (Ptr<BearerInfo> bInfo);

  /**
   * Remove packet forwarding rules from the S-GW OpenFlow switch.
   * \param bInfo The bearer information.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesRemove (Ptr<BearerInfo> bInfo);

  /**
   * Update packet forwarding rules on the S-GW OpenFlow switch after a
   * successful handover procedure.
   * \param bInfo The bearer information.
   * \param dstEnbInfo The destination eNB after the handover procedure.
   * \return True if succeeded, false otherwise.
   */
  bool SgwRulesUpdate (Ptr<BearerInfo> bInfo, Ptr<EnbInfo> dstEnbInfo);

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
  TracedCallback<Ptr<const BearerInfo>> m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<Ptr<const BearerInfo>> m_bearerReleaseTrace;

  // Slice identification.
  SliceId                   m_sliceId;        //!< Logical slice ID.
  std::string               m_sliceIdStr;     //!< Slice ID string.

  // Infrastructure interface.
  Ptr<TransportController>  m_transportCtrl; //!< Transport controller.
  double                    m_gbrBlockThs;    //!< GBR block threshold.
  int                       m_slicePrio;      //!< Slice priority.
  int                       m_linkQuota;      //!< Initial link sharing quota.
  OpMode                    m_linkSharing;    //!< Bit rate sharing flag.
  OpMode                    m_aggregation;    //!< Bearer traffic aggregation.

  // MME interface.
  Ptr<StatelessMme>         m_mme;            //!< MME element.
  EpcS11SapMme*             m_s11SapMme;      //!< MME side of the S11 SAP.
  EpcS11SapSgw*             m_s11SapSgw;      //!< S-GW side of the S11 SAP.

  // Network configuration.
  Ipv4Address               m_ueAddr;         //!< UE network address.
  Ipv4Mask                  m_ueMask;         //!< UE network mask.
  Ipv4Address               m_webAddr;        //!< Web network address.
  Ipv4Mask                  m_webMask;        //!< Web network mask.

  // P-GW metadata and TFT load balancing mechanism.
  Ptr<PgwuScaling>          m_pgwScaling;     //!< P-GW scaling application.
  Ptr<PgwInfo>              m_pgwInfo;        //!< P-GW metadata for slice.
  OpMode                    m_pgwBlockPolicy; //!< P-GW overload block policy.
  double                    m_pgwBlockThs;    //!< P-GW block threshold.

  // S-GW metadata.
  Ptr<SgwInfo>              m_sgwInfo;        //!< S-GW metadata for slice.
  OpMode                    m_sgwBlockPolicy; //!< S-GW overload block policy.
  double                    m_sgwBlockThs;    //!< S-GW block threshold.
};

} // namespace ns3
#endif // SLICE_CONTROLLER_H
