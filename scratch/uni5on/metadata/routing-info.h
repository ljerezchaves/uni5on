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

#ifndef ROUTING_INFO_H
#define ROUTING_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class RoutingInfo;
class UeInfo;

/** List of bearer routing information. */
typedef std::vector<Ptr<RoutingInfo> > RoutingInfoList_t;

/**
 * \ingroup uni5onMeta
 * Metadata associated to the EPS bearer.
 */
class RoutingInfo : public Object
{
  friend class BackhaulController;
  friend class RingController;
  friend class SliceController;
  friend class TrafficManager;

public:
  /** The reason for any blocked request. */
  enum BlockReason
  {
    PGWTABLE  = (1U << 0),  //!< P-GW TFT flow table is full.
    PGWLOAD   = (1U << 1),  //!< P-GW TFT pipeline load is full.
    SGWTABLE  = (1U << 4),  //!< S-GW flow table is full.
    SGWLOAD   = (1U << 5),  //!< S-GW pipeline load is full.
    BACKTABLE = (1U << 8),  //!< At least one backhaul flow table is full.
    BACKLOAD  = (1U << 9),  //!< At least one backhaul pipeline load is full.
    BACKBAND  = (1U << 12)  //!< At least one backhaul link has no bandwidth.
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
   * \param iface The LTE logical interface.
   * \return The requested information.
   */
  //\{
  uint16_t    GetBlockReason    (void) const;
  std::string GetBlockReasonHex (void) const;
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
  bool        IsGwInstalled     (void) const;
  bool        IsIfInstalled     (LteIface iface) const;
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
  QosType               GetQosType    (void) const;
  std::string           GetQosTypeStr (void) const;
  Ptr<EpcTft>           GetTft        (void) const;
  //\}

  /**
   * \name Private member accessors for bearer guaranteed bit rate information.
   *       These methods are used for reserving resources at backhaul network.
   * \param iface The LTE logical interface.
   * \return The requested information.
   */
  //\{
  int64_t     GetGbrDlBitRate   (void) const;
  int64_t     GetGbrUlBitRate   (void) const;
  bool        HasGbrBitRate     (void) const;
  bool        HasGbrUlBitRate   (void) const;
  bool        HasGbrDlBitRate   (void) const;
  bool        IsGbr             (void) const;
  bool        IsNonGbr          (void) const;
  bool        IsGbrReserved     (LteIface iface) const;
  //\}

  /**
   * \name Private member accessors for bearer maximum bit rate information.
   *       These methods are used for installing meter entries at S/P-GW.
   * \param iface The LTE logical interface.
   * \return The requested information.
   */
  //\{
  int64_t     GetMbrDlBitRate   (void) const;
  int64_t     GetMbrUlBitRate   (void) const;
  bool        HasMbrDl          (void) const;
  bool        HasMbrUl          (void) const;
  bool        HasMbr            (void) const;
  bool        IsMbrDlInstalled  (void) const;
  bool        IsMbrUlInstalled  (void) const;
  bool        IsMbrDlInstalled  (LteIface iface) const;
  bool        IsMbrUlInstalled  (LteIface iface) const;
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
  uint64_t    GetPgwTftDpId     (void) const;
  uint32_t    GetPgwTftS5PortNo (void) const;
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
   * \name Private member accessors for infrastructure routing.
   * \param iface The LTE interface.
   * \return The requested information.
   */
  //\{
  uint16_t    GetDstDlInfraSwIdx  (LteIface iface) const;
  Ipv4Address GetDstDlAddr        (LteIface iface) const;
  uint16_t    GetDstUlInfraSwIdx  (LteIface iface) const;
  Ipv4Address GetDstUlAddr        (LteIface iface) const;
  uint16_t    GetSrcDlInfraSwIdx  (LteIface iface) const;
  Ipv4Address GetSrcDlAddr        (LteIface iface) const;
  uint16_t    GetSrcUlInfraSwIdx  (LteIface iface) const;
  Ipv4Address GetSrcUlAddr        (LteIface iface) const;
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
   * \param iface The LTE logical interface.
   * \param value The value to set.
   */
  //\{
  void SetActive          (bool value);
  void SetAggregated      (bool value);
  void SetGbrReserved     (LteIface iface, bool value);
  void SetMbrDlInstalled  (LteIface iface, bool value);
  void SetMbrUlInstalled  (LteIface iface, bool value);
  void SetPgwTftIdx       (uint16_t value);
  void SetPriority        (uint16_t value);
  void SetTimeout         (uint16_t value);
  void SetGwInstalled     (bool value);
  void SetIfInstalled     (LteIface iface, bool value);
  //\}

  /**
   * Increase the priority value by one unit.
   */
  void IncreasePriority (void);

  /**
   * Check the blocked status for the following reason.
   * \param reason The block reason.
   * \return True if the bearer is blocked for this reason, false otherwise.
   */
  bool IsBlocked (BlockReason reason) const;

  /**
   * Clear the blocked status.
   */
  void ResetBlocked (void);

  /**
   * Set the blocked status for the following reason.
   * \param reason The block reason.
   */
  void SetBlocked (BlockReason reason);

  /**
   * Unset the blocked status for the following reason.
   * \param reason The block reason.
   */
  void UnsetBlocked (BlockReason reason);

  /**
   * Get the list of bearer routing information, optionally filtered by the
   * LTE logical slice.
   * \param slice The LTE logical slice ID.
   * \param [out] returnList The list of installed bearers.
   */
  static void GetList (RoutingInfoList_t &returnList,
                       SliceId slice = SliceId::ALL);

private:
  /**
   * Register the routing information in global map for further usage.
   * \param rInfo The routing information to save.
   */
  static void RegisterRoutingInfo (Ptr<RoutingInfo> rInfo);

  BearerCreated_t  m_bearer;         //!< EPS bearer context created.
  uint16_t         m_blockReason;    //!< Bitmap for blocked reasons.
  bool             m_isActive;       //!< True for active bearer.
  bool             m_isAggregated;   //!< True for aggregated bearer.
  bool             m_isDefault;      //!< True for default bearer.
  bool             m_isGbrRes [2];   //!< True for GBR resources reserved.
  bool             m_isInstGw;       //!< True for installed gateway rules.
  bool             m_isInstIf [2];   //!< True for installed interface rules.
  bool             m_isMbrDlInst [2]; //!< True for downlink meter installed.
  bool             m_isMbrUlInst [2]; //!< True for uplink meter installed.
  uint16_t         m_pgwTftIdx;      //!< P-GW TFT switch index.
  uint16_t         m_priority;       //!< Flow table rule priority.
  SliceId          m_sliceId;        //!< Slice ID for this bearer.
  uint32_t         m_teid;           //!< GTP TEID.
  uint16_t         m_timeout;        //!< Flow table idle timeout.
  Ptr<UeInfo>      m_ueInfo;         //!< UE metadata pointer.

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
