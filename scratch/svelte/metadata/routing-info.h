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

#ifndef ROUTING_INFO_H
#define ROUTING_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../svelte-common.h"

namespace ns3 {

class RoutingInfo;
class UeInfo;

/** List of bearer routing information. */
typedef std::vector<Ptr<RoutingInfo> > RoutingInfoList_t;

/**
 * \ingroup svelteMeta
 * Metadata associated to the EPS bearer.
 */
class RoutingInfo : public Object
{
  friend class BackhaulController;
  friend class RingController;
  friend class SliceController;

public:
  /** The reason for any blocked request. */
  enum BlockReason
  {
    NONE      = 0,  //!< This bearer id not blocked.
    PGWTABLE  = 1,  //!< P-GW TFT flow table is full.
    PGWLOAD   = 2,  //!< P-GW TFT pipeline load is full.
    SGWTABLE  = 3,  //!< P-GW flow table is full.
    SGWLOAD   = 4,  //!< P-GW pipeline load is full.
    BACKTABLE = 5,  //!< At least one backhaul flow table is full.
    BACKLOAD  = 6,   //!< At least one backhaul pipeline load is full.
    BACKBAND  = 7   //!< At least one backahul link has no available bandwidth.
  };

  /**
   * Complete constructor.
   * \param teid The TEID value.
   * \param bearer The bearer context created.
   * \param ueInfo The UE metadata.
   * \param isDefault True for default bearer.
   */
  RoutingInfo (uint32_t teid, BearerCreated_t bearer,
               Ptr<UeInfo> ueInfo, bool isDefault);
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for bearer routing information.
   * \return The requested information.
   */
  //\{
  std::string GetBlockReasonStr (void) const;
  uint16_t    GetPgwTftIdx      (void) const;
  uint16_t    GetPriority       (void) const;
  SliceId     GetSliceId        (void) const;
  std::string GetSliceIdStr     (void) const;
  uint32_t    GetTeid           (void) const;
  std::string GetTeidHex        (void) const;
  uint16_t    GetTimeout        (void) const;
  bool        IsActive          (void) const;
  bool        IsAggregated      (void) const;
  bool        IsBlocked         (void) const;
  bool        IsDefault         (void) const;
  bool        IsTunnelInstalled (void) const;
  //\}

  /**
   * \name Private member accessors for traffic information.
   * \return The requested information.
   */
  //\{
  Ipv4Header::DscpType  GetDscp       (void) const;
  std::string           GetDscpStr    (void) const;
  uint16_t              GetDscpValue  (void) const;
  bool                  HasDlTraffic  (void) const;
  bool                  HasUlTraffic  (void) const;
  bool                  HasTraffic    (void) const;
  //\}

  /**
   * \name Private member accessors for bearer context information.
   * \return The requested information.
   */
  //\{
  uint8_t               GetBearerId   (void) const;
  EpsBearer             GetEpsBearer  (void) const;
  EpsBearer::Qci        GetQciInfo    (void) const;
  GbrQosInformation     GetQosInfo    (void) const;
  Ptr<EpcTft>           GetTft        (void) const;
  //\}

  /**
   * \name Private member accessors for bearer guaranteed bit rate information.
   *       These methods are used for reserving resources at backhaul network.
   * \return The requested information.
   */
  //\{
  uint64_t    GetGbrDlBitRate   (void) const;
  uint64_t    GetGbrUlBitRate   (void) const;
  bool        HasGbrDl          (void) const;
  bool        HasGbrUl          (void) const;
  bool        IsGbr             (void) const;
  bool        IsGbrReserved     (void) const;
  //\}

  /**
   * \name Private member accessors for bearer maximum bit rate information.
   *       These methods are used for installing meter entries at S/P-GW.
   * \return The requested information.
   */
  //\{
  std::string GetMbrDelCmd      (void) const;
  std::string GetMbrDlAddCmd    (void) const;
  std::string GetMbrUlAddCmd    (void) const;
  uint64_t    GetMbrDlBitRate   (void) const;
  uint64_t    GetMbrUlBitRate   (void) const;
  bool        HasMbrDl          (void) const;
  bool        HasMbrUl          (void) const;
  bool        IsMbrDlInstalled  (void) const;
  bool        IsMbrUlInstalled  (void) const;
  //\}

  /**
   * \name Private member accessors for UE information.
   * \return The requested information.
   */
  //\{
  Ipv4Address GetUeAddr         (void) const;
  uint64_t    GetUeImsi         (void) const;
  Ptr<UeInfo> GetUeInfo         (void) const;
  //\}

  /**
   * \name Private member accessors for eNB information.
   * \return The requested information.
   */
  //\{
  uint16_t    GetEnbCellId      (void) const;
  uint16_t    GetEnbInfraSwIdx  (void) const;
  Ipv4Address GetEnbS1uAddr     (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW information.
   * \return The requested information.
   */
  //\{
  uint32_t    GetPgwId          (void) const;
  uint16_t    GetPgwInfraSwIdx  (void) const;
  Ipv4Address GetPgwS5Addr      (void) const;
  //\}

  /**
   * \name Private member accessors for S-GW information.
   * \return The requested information.
   */
  //\{
  uint64_t    GetSgwDpId        (void) const;
  uint32_t    GetSgwId          (void) const;
  uint16_t    GetSgwInfraSwIdx  (void) const;
  Ipv4Address GetSgwS1uAddr     (void) const;
  uint32_t    GetSgwS1uPortNo   (void) const;
  Ipv4Address GetSgwS5Addr      (void) const;
  uint32_t    GetSgwS5PortNo    (void) const;
  //\}

  /**
   * Get the string representing the block reason.
   * \param reason The block reason.
   * \return The block reason string.
   */
  static std::string BlockReasonStr (BlockReason reason);

  /**
   * Get stored information for a specific EPS bearer.
   * \param teid The GTP tunnel ID.
   * \return The EpsBearer information for this teid.
   */
  static EpsBearer GetEpsBearer (uint32_t teid);

  /**
   * Get the routing information from the global map for a specific TEID.
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  static Ptr<RoutingInfo> GetPointer (uint32_t teid);

  /**
   * Get the slice ID for this GTP TEID.
   * \param teid The GTP tunnel ID.
   * \return The slice ID for this tunnel.
   */
  static SliceId GetSliceId (uint32_t teid);

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * TracedCallback signature for Ptr<const RoutingInfo>.
   * \param rInfo The bearer information.
   */
  typedef void (*TracedCallback)(Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * \name Private member accessors for updating internal metadata.
   * \param value The value to set.
   */
  //\{
  void SetActive          (bool value);
  void SetAggregated      (bool value);
  void SetGbrReserved     (bool value);
  void SetMbrDlInstalled  (bool value);
  void SetMbrUlInstalled  (bool value);
  void SetPgwTftIdx       (uint16_t value);
  void SetPriority        (uint16_t value);
  void SetTimeout         (uint16_t value);
  void SetTunnelInstalled (bool value);
  //\}

  /**
   * Increase the priority value by one unit.
   */
  void IncreasePriority (void);

  /**
   * Set the blocked status including the reason.
   * \param value The value to set.
   * \param reason The reason for this value.
   */
  void SetBlocked (bool value, BlockReason reason = RoutingInfo::NONE);

  /**
   * Get a list of the installed bearer routing information, optionally
   * filtered by the LTE logical slice and by the P-GW TFT switch index.
   * \param slice The LTE logical slice ID.
   * \param pgwTftIdx The P-GW TFT index.
   * \param [out] returnList The list of installed bearers.
   */
  static void GetInstalledList (RoutingInfoList_t &returnList,
                                SliceId slice = SliceId::ALL,
                                uint16_t pgwTftIdx = 0);

private:
  /**
   * Register the routing information in global map for further usage.
   * \param rInfo The routing information to save.
   */
  static void RegisterRoutingInfo (Ptr<RoutingInfo> rInfo);

  BearerCreated_t      m_bearer;       //!< EPS bearer context created.
  BlockReason          m_blockReason;  //!< Reason for blocked request.
  bool                 m_isActive;     //!< True for active bearer.
  bool                 m_isAggregated; //!< True for aggregated bearer.
  bool                 m_isBlocked;    //!< True for blocked request.
  bool                 m_isDefault;    //!< True for default bearer.
  bool                 m_isGbrRes;     //!< True for GBR resources reserved.
  bool                 m_isMbrDlInst;  //!< True fir downlink meter installed.
  bool                 m_isMbrUlInst;  //!< True for uplink meter installed.
  bool                 m_isTunnelInst; //!< True for GTP tunnel installed.
  uint16_t             m_pgwTftIdx;    //!< P-GW TFT switch index.
  uint16_t             m_priority;     //!< Flow table rule priority.
  SliceId              m_sliceId;      //!< Slice ID for this bearer.
  uint32_t             m_teid;         //!< GTP TEID.
  uint16_t             m_timeout;      //!< Flow table idle timeout.
  Ptr<UeInfo>          m_ueInfo;       //!< UE metadata pointer.

  /** Map saving TEID / routing information. */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;
  static TeidRoutingMap_t m_routingInfoByTeid;  //!< Global routing info map.
};

/**
 * Print the routing metadata on an output stream.
 * \param os The output stream.
 * \param rInfo The RoutingInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the RoutingInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const RoutingInfo &rInfo);

} // namespace ns3
#endif // ROUTING_INFO_H
