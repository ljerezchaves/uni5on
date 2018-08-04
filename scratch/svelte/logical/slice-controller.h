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
#include "../svelte-common.h"

namespace ns3 {

class BackhaulController;
class RoutingInfo;
class SvelteMme;
class SgwInfo;
class PgwInfo;

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
   * Get the P-GW adaptive mechanisms operation mode.
   * \return The operation mode.
   */
  OpMode GetPgwAdaptiveMode (void) const;

  /**
   * Get The S-GW side of the S11 SAP.
   * \return The S-GW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw (void) const;

  /**
   * Get the slice ID for this controller.
   * \return The slice ID.
   */
  SliceId GetSliceId (void) const;

  /**
   * Notify this controller of the P-GW  connected to the OpenFlow backhaul
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
   * TracedCallback signature for the P-GW adaptive mechanism trace source.
   * \param pgwInfo The P-GW metadata.
   * \param currentLevel The current mechanism level.
   * \param nextLevel The mechanism level for next cycle.
   * \param maxLevel The maximum mechanism level.
   * \param bearersMoved The number of bearers moved.
   * \param blockThrs The block threshold.
   * \param joinThrs The join threshold.
   * \param splitThrs The split threshold.
   */
  typedef void (*PgwTftStatsTracedCallback)(
    Ptr<const PgwInfo> pgwInfo, uint32_t currentLevel, uint32_t nextLevel,
    uint32_t maxLevel, uint32_t bearersMoved, double blockThrs,
    double joinThrs, double splitThrs);

  /**
   * TracedCallback signature for session created trace source.
   * \param imsi The IMSI UE identifier.
   * \param bearerList The list of context bearers created.
   */
  typedef void (*SessionCreatedTracedCallback)(
    uint64_t imsi, BearerContextList_t bearerList);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

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
   * Periodic controller timeout operations.
   */
  void ControllerTimeout (void);

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
   * Get the S-GW metadata for a specific backhaul switch index.
   * \param infaSwIdx The backhaul switch index.
   * \return The S-GW metadata.
   */
  Ptr<SgwInfo> GetSgwInfo (uint16_t infraSwIdx);

  /**
   * Get the active P-GW TFT index for a given traffic flow.
   * \param rInfo The routing information to process.
   * \param activeTfts The number of active P-GW TFT switches. When set to 0,
   *        the number of P-GW TFTs will be calculated considering the current
   *        adaptive mechanism level.
   * \return The P-GW TFT index.
   */
  uint16_t GetTftIdx (Ptr<const RoutingInfo> rInfo,
                      uint16_t activeTfts = 0) const;

  /**
   * Periodically check for the P-GW TFT processing load and flow table usage
   * to update the adaptive mechanism.
   */
  void PgwAdaptiveMechanism (void);

  /**
   * Check for available resources on P-GW TFT switch for this bearer request.
   * \param rInfo The routing information to process.
   * \return True if succeeded, false otherwise.
   */
  bool PgwBearerRequest (Ptr<RoutingInfo> rInfo);

  /**
   * Install downlink packet filtering rules on the P-GW TFT OpenFlow switch.
   * \attention To avoid conflicts with old entries, increase the routing
   *            priority before installing OpenFlow rules.
   * \param rInfo The routing information to process.
   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
   *        be get from rInfo->GetPgwTftIdx ().
   * \param moveFlag When true, forces the meter entry install even when
   *        meterInfo->IsDownInstalled () is true (useful when moving rules
   *        among TFT switches).
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesInstall (Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
                        bool moveFlag = false);

  /**
   * Remove downlink packet filtering rules from the P-GW TFT OpenFlow switch.
   * \param rInfo The routing information to process.
   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
   *        be get from rInfo->GetPgwTftIdx ().
   * \param moveFlag When true, don't set the meterInfo->IsDownInstalled ()
   *        flag to false when removing the meter entry (useful when moving
   *        rules among TFT switches).
   * \return True if succeeded, false otherwise.
   */
  bool PgwRulesRemove (Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
                       bool moveFlag = false);

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

  /** The bearer request trace source, fired at RequestDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerRequestTrace;

  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
  TracedCallback<Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, BearerContextList_t> m_sessionCreatedTrace;

  /** The P-GW TFT adaptive trace source, fired at PgwAdaptiveMechanism. */
  TracedCallback<Ptr<const PgwInfo>, uint32_t, uint32_t, uint32_t, uint32_t,
                 double, double, double> m_pgwTftAdaptiveTrace;

  // Slice identification.
  SliceId                 m_sliceId;        //!< Logical slice ID.

  // Infrastructure interface.
  Ptr<BackhaulController> m_backhaulCtrl;   //!< OpenFlow backhaul controller.

  // MME interface.
  Ptr<SvelteMme>          m_mme;            //!< MME element.
  EpcS11SapMme*           m_s11SapMme;      //!< MME side of the S11 SAP.
  EpcS11SapSgw*           m_s11SapSgw;      //!< S-GW side of the S11 SAP.

  // Network configuration.
  Ipv4Address             m_ueAddr;         //!< UE network address.
  Ipv4Mask                m_ueMask;         //!< UE network mask.
  Ipv4Address             m_webAddr;        //!< Web network address.
  Ipv4Mask                m_webMask;        //!< Web network mask.

  // P-GW metadata and TFT adaptive mechanism.
  Ptr<PgwInfo>            m_pgwInfo;        //!< P-GW metadata for this slice.
  OpMode                  m_tftAdaptive;    //!< P-GW adaptive mechanism.
  uint8_t                 m_tftLevel;       //!< Current adaptive level.
  OpMode                  m_tftBlockPolicy; //!< Overload block policy.
  double                  m_tftBlockThs;    //!< Block threshold.
  double                  m_tftJoinThs;     //!< Join threshold.
  double                  m_tftSplitThs;    //!< Split threshold.
  uint16_t                m_tftSwitches;    //!< Max number of TFT switches.

  // Internal members and attributes.
  Time                    m_timeout;        //!< Controller internal timeout.
  static const uint16_t   m_flowTimeout;    //!< Timeout for flow entries.

  /** Map saving backhaul switch index / S-GW metadata. */
  typedef std::map<uint16_t, Ptr<SgwInfo> > SwIdxSgwInfoMap_t;
  SwIdxSgwInfoMap_t       m_sgwInfoBySwIdx; //!< S-GW info map by switch idx.
};

} // namespace ns3
#endif // SLICE_CONTROLLER_H
