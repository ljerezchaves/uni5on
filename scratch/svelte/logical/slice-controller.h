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
#include "../infrastructure/backhaul-controller.h"

namespace ns3 {

class BackhaulController;
class BackhaulNetwork;
class SvelteMme;

// FIXME Temporário aqui enquanto não estou usando o routing info.
/** EPS bearer context created. */
typedef EpcS11SapMme::BearerContextCreated BearerContext_t;

/** List of bearer context created. */
typedef std::list<BearerContext_t> BearerContextList_t;

/**
 * \ingroup svelteLogical
 * Enumeration of available SVELTE logical slices IDs.
 * \internal Slice IDs are restricted to the range [0x01, 0xFF] by the current
 * TEID allocation strategy.
 */
typedef enum
{
  NONE = 0,   //!< Undefined slice.
  HTC  = 1,   //!< Slice for HTC UEs.
  MTC  = 2    //!< Slice for MTC UEs.
} SliceId;

/**
 * \ingroup svelteLogical
 * Get the slice ID name.
 * \param slice The slice ID.
 * \return The string with the slice ID name.
 */
std::string SliceIdStr (SliceId slice);

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

  SliceController ();           //!< Default constructor.
  virtual ~SliceController ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

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
  virtual bool DedicatedBearerRelease (
    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);

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
  virtual bool DedicatedBearerRequest (
    EpsBearer bearer, uint64_t imsi, uint16_t cellId, uint32_t teid);

  /**
   * Notify this controller of the S-GW connected to the OpenFlow backhaul
   * network over the S1-U and S5 interfaces.
   * \param sgwSwDev The OpenFlow S-GW switch device.
   * \param sgwS1uDev The S1-U device on the S-GW OpenFlow switch.
   * \param sgwS1uPortNo The S1-U port number on the S-GW OpenFlow switch.
   * \param sgwS5Dev The S5 device configure on the S-GW OpenFlow switch.
   * \param sgwS5PortNo The S5 port number on the S-GW OpenFlow switch.
   */
  virtual void NotifySgwAttach (
    Ptr<OFSwitch13Device> sgwSwDev, Ptr<NetDevice> sgwS1uDev,
    uint32_t sgwS1uPortNo, Ptr<NetDevice> sgwS5Dev, uint32_t sgwS5PortNo);

  /**
   * Notify this controller of the P-GW main switch connected to the OpenFlow
   * backhaul network over the S5 interface, and to the web server over the SGi
   * interface.
   * \param pgwSwDev The P-GW main OpenFlow switch device.
   * \param pgwS5Dev The S5 device on the P-GW OpenFlow switch.
   * \param pgwS5PortNo The S5 port number on the P-GW OpenFlow switch.
   * \param pgwSgiDev The SGi device on the P-GW OpenFlow switch.
   * \param pgwSgiPortNo The SGi port number on the P-GW OpenFlow switch.
   * \param webSgiDev The SGi device on the Web server.
   */
  virtual void NotifyPgwMainAttach (
    Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwS5Dev,
    uint32_t pgwS5PortNo, Ptr<NetDevice> pgwSgiDev, uint32_t pgwSgiPortNo,
    Ptr<NetDevice> webSgiDev);

  /**
   * Notify this controller of a new P-GW TFT switch connected to the OpenFlow
   * backhaul network over the S5 interface and to the P-GW main switch over
   * an internal interface.
   * \param pgwSwDev The P-GW TFT OpenFlow switch device.
   * \param pgwS5Dev The S5 device on the P-GW OpenFlow switch.
   * \param pgwS5PortNo The S5 port number on the P-GW OpenFlow switch.
   * \param pgwMainPortNo The internal port number on the P-GW main switch.
   * \param pgwTftCounter The counter (index) for this P-GW TFT switch.
   */
  virtual void NotifyPgwTftAttach (
    Ptr<OFSwitch13Device> pgwSwDev, Ptr<NetDevice> pgwS5Dev,
    uint32_t pgwS5PortNo, uint32_t pgwMainPortNo, uint16_t pgwTftCounter);

  /**
   * Notify this controller that all P-GW switches have already been
   * configured and the connections between them are finished.
   * \param devices The OFSwitch13DeviceContainer for OpenFlow switch devices.
   */
  virtual void NotifyPgwBuilt (OFSwitch13DeviceContainer devices);

  /**
   * \name Internal mechanisms operation mode accessors.
   * \return The requested mechanism operation mode.
   */
  //\{
  OperationMode GetAggregationMode (void) const;
  OperationMode GetPgwAdaptiveMode (void) const;
  //\}

  /**
   * Get The S-GW side of the S11 SAP.
   * \return The S-GW side of the S11 SAP.
   */
  EpcS11SapSgw* GetS11SapSgw (void) const;

  /**
   * Configure this controller with slice network attributes.
   * \param nPgwTfts The number of P-GW TFT switches available for use.
   * \param ueAddr The UE network address.
   * \param ueMask The UE network mask.
   * \param webAddr The Internet network address.
   * \param webMask The Internet network mask.
   */
  void SetNetworkAttributes (
    uint16_t nPgwTfts, Ipv4Address ueAddr, Ipv4Mask ueMask,
    Ipv4Address webAddr, Ipv4Mask webMask);

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
//  /**
//   * Install OpenFlow match rules for this bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool BearerInstall (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Remove OpenFlow match rules for this bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool BearerRemove (Ptr<RoutingInfo> rInfo);

  /**
   * Periodic controller timeout operations.
   */
  void ControllerTimeout (void);

  /**
   * \name Methods for the S11 SAP S-GW control plane.
   * \param msg The message sent here by the MME entity.
   */
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

//  /**
//   * Get the active P-GW TFT index for a given traffic flow.
//   * \param rInfo The routing information to process.
//   * \param activeTfts The number of active P-GW TFT switches. When set to 0,
//   *        the number of P-GW TFTs will be calculated considering the current
//   *        adaptive mechanism level.
//   * \return The P-GW TFT index.
//   */
//  uint16_t GetPgwTftIdx (
//    Ptr<const RoutingInfo> rInfo, uint16_t activeTfts = 0) const;
//
//  /**
//   * Install OpenFlow match rules for the aggregated MTC bearer.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool MtcAggBearerInstall (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Check for available resources on P-GW TFT switch for this bearer request.
//   * \param rInfo The routing information to process.
//   * \return True if succeeded, false otherwise.
//   */
//  bool PgwBearerRequest (Ptr<RoutingInfo> rInfo);
//
//  /**
//   * Install OpenFlow rules for downlink packet filtering on the P-GW TFT
//   * switch.
//   * \attention To avoid conflicts with old entries, increase the routing
//   *            priority before installing OpenFlow rules.
//   * \param rInfo The routing information to process.
//   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
//   *        be get from rInfo->GetPgwTftIdx ().
//   * \param forceMeterInstall Force the meter entry installation even when the
//   *        meterInfo->IsDownInstalled () is true.
//   * \return True if succeeded, false otherwise.
//   */
//  bool PgwRulesInstall (
//    Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
//    bool forceMeterInstall = false);
//
//  /**
//   * Remove OpenFlow rules for downlink packet filtering from P-GW TFT switch.
//   * \param rInfo The routing information to process.
//   * \param pgwTftIdx The P-GW TFT switch index. When set to 0, the index will
//   *        be get from rInfo->GetPgwTftIdx ().
//   * \param keepMeterFlag Don't set the meterInfo->IsDownInstalled () flag to
//   *        false when removing the meter entry.
//   * \return True if succeeded, false otherwise.
//   */
//  bool PgwRulesRemove (
//    Ptr<RoutingInfo> rInfo, uint16_t pgwTftIdx = 0,
//    bool keepMeterFlag = false);

  /**
   * Periodically check for the P-GW TFT processing load and flow table usage
   * to update the adaptive mechanism.
   */
  void PgwTftCheckUsage (void);

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

// FIXME comentado por causa da dependencia com o rinfo.
//  /** The bearer request trace source, fired at RequestDedicatedBearer. */
//  TracedCallback<Ptr<const RoutingInfo> > m_bearerRequestTrace;
//
//  /** The bearer release trace source, fired at ReleaseDedicatedBearer. */
//  TracedCallback<Ptr<const RoutingInfo> > m_bearerReleaseTrace;

  /** The context created trace source, fired at NotifySessionCreated. */
  TracedCallback<uint64_t, uint16_t, BearerContextList_t>
  m_sessionCreatedTrace;

  /** The P-GW TFT stats trace source, fired at PgwTftCheckUsage. */
  TracedCallback<struct PgwTftStats> m_pgwTftStatsTrace;

  // Slice identification.
  SliceId                 m_sliceId;        //!< Logical slice ID.

  // MME interface.
  Ptr<SvelteMme>          m_mme;            //!< MME element.
  EpcS11SapMme*           m_s11SapMme;      //!< MME side of the S11 SAP.
  EpcS11SapSgw*           m_s11SapSgw;      //!< S-GW side of the S11 SAP.

  // Infrastructure interface.
  // FIXME Vou precisar disso?
  // Ptr<BackhaulController> m_backhaulCtrl;   //!< OpenFlow backhaul controller.
  // Ptr<BackhaulNetwork>    m_backhaulNet;    //!< OpenFlow backhaul network.

  // Network configuration.
  Ipv4Address             m_ueAddr;         //!< UE network address.
  Ipv4Mask                m_ueMask;         //!< UE network mask.
  Ipv4Address             m_webAddr;        //!< Web network address.
  Ipv4Mask                m_webMask;        //!< Web network mask.

  // P-GW user plane.
  std::vector<uint64_t>   m_pgwDpIds;       //!< Datapath IDs.
  Ipv4Address             m_pgwS5Addr;      //!< S5 IP address for uplink.
  std::vector<uint32_t>   m_pgwS5PortsNo;   //!< S5 port numbers.
  uint32_t                m_pgwSgiPortNo;   //!< SGi port number.

  // P-GW TFT adaptive mechanism.
  OperationMode           m_tftAdaptive;    //!< P-GW adaptive mechanism.
  uint8_t                 m_tftLevel;       //!< Current adaptive level.
  OperationMode           m_tftBlockPolicy; //!< Overload block policy.
  double                  m_tftBlockThs;    //!< Block threshold.
  double                  m_tftJoinThs;     //!< Join threshold.
  double                  m_tftSplitThs;    //!< Split threshold.
  uint16_t                m_tftSwitches;    //!< Max number of TFT switches.
  DataRate                m_tftMaxLoad;     //!< Processing capacity.
  uint32_t                m_tftTableSize;   //!< Flow table size.

  // Internal members and attributes.
  OperationMode           m_aggregation;    //!< Aggregation mechanism.
  Time                    m_timeout;        //!< Controller internal timeout.

  static const uint16_t   m_flowTimeout;    //!< Timeout for flow entries.
};

} // namespace ns3
#endif // SLICE_CONTROLLER_H
