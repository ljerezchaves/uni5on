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

#ifndef BEARER_INFO_H
#define BEARER_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../uni5on-common.h"

namespace ns3 {

class BearerInfo;
class UeInfo;

/** List of bearer information. */
typedef std::vector<Ptr<BearerInfo>> BearerInfoList_t;

/**
 * \ingroup uni5onMeta
 * Metadata associated to an EPC bearer.
 */
class BearerInfo : public Object
{
  friend class TransportController;
  friend class RingController;
  friend class SliceController;
  friend class TrafficManager;
  friend class PgwuScaling;

public:
  /** The reason for any blocked request. */
  enum BlockReason
  {
    BRPGWTAB = (1U << 0),  //!< P-GW flow table.
    BRPGWCPU = (1U << 1),  //!< P-GW pipeline load.
    BRSGWTAB = (1U << 4),  //!< S-GW flow table.
    BRSGWCPU = (1U << 5),  //!< S-GW pipeline load.
    BRTPNTAB = (1U << 8),  //!< Transport switch flow table.
    BRTPNCPU = (1U << 9),  //!< Transport switch pipeline.
    BRTPNBWD = (1U << 12)  //!< Transport link bandwidth.
  };

  /**
   * Complete constructor.
   * \param teid The TEID value.
   * \param bearer The bearer context created.
   * \param ueInfo The UE metadata.
   * \param isDefault True for default bearer.
   */
  BearerInfo (uint32_t teid, BearerCreated_t bearer,
              Ptr<UeInfo> ueInfo, bool isDefault);
  virtual ~BearerInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for bearer information.
   * \param iface The logical interface.
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
  bool        IsIfInstalled     (EpsIface iface) const;
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
   *       These methods are used for reserving resources at transport network.
   * \param iface The logical interface.
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
  bool        IsGbrReserved     (EpsIface iface) const;
  //\}

  /**
   * \name Private member accessors for bearer maximum bit rate information.
   *       These methods are used for installing meter entries at S/P-GW.
   * \param iface The logical interface.
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
  bool        IsMbrDlInstalled  (EpsIface iface) const;
  bool        IsMbrUlInstalled  (EpsIface iface) const;
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
  uint32_t    GetPgwId            (void) const;
  uint16_t    GetPgwInfraSwIdx    (void) const;
  Ipv4Address GetPgwS5Addr        (void) const;
  uint64_t    GetPgwTftDpId       (void) const;
  uint32_t    GetPgwTftToUlPortNo (void) const;
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
   * \name Private member accessors for infrastructure.
   * \param iface The logical interface.
   * \return The requested information.
   */
  //\{
  uint16_t    GetDstDlInfraSwIdx  (EpsIface iface) const;
  Ipv4Address GetDstDlAddr        (EpsIface iface) const;
  uint16_t    GetDstUlInfraSwIdx  (EpsIface iface) const;
  Ipv4Address GetDstUlAddr        (EpsIface iface) const;
  uint16_t    GetSrcDlInfraSwIdx  (EpsIface iface) const;
  Ipv4Address GetSrcDlAddr        (EpsIface iface) const;
  uint16_t    GetSrcUlInfraSwIdx  (EpsIface iface) const;
  Ipv4Address GetSrcUlAddr        (EpsIface iface) const;
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
   * Get the bearer information from the global map for a specific TEID.
   * \param teid The GTP tunnel ID.
   * \return The bearer information for this tunnel.
   */
  static Ptr<BearerInfo> GetPointer (uint32_t teid);

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * TracedCallback signature for Ptr<const BearerInfo>.
   * \param bInfo The bearer information.
   */
  typedef void (*TracedCallback)(Ptr<const BearerInfo> bInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * \name Private member accessors for updating internal metadata.
   * \param iface The logical interface.
   * \param value The value to set.
   */
  //\{
  void SetActive          (bool value);
  void SetAggregated      (bool value);
  void SetGbrReserved     (EpsIface iface, bool value);
  void SetMbrDlInstalled  (EpsIface iface, bool value);
  void SetMbrUlInstalled  (EpsIface iface, bool value);
  void SetPgwTftIdx       (uint16_t value);
  void SetPriority        (uint16_t value);
  void SetTimeout         (uint16_t value);
  void SetGwInstalled     (bool value);
  void SetIfInstalled     (EpsIface iface, bool value);
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
   * Get the list of bearer information, optionally filtered by the
   * logical slice.
   * \param slice The logical slice ID.
   * \param [out] returnList The list of installed bearers.
   */
  static void GetList (BearerInfoList_t &returnList,
                       SliceId slice = SliceId::ALL);

private:
  /**
   * Register the bearer information in global map for further usage.
   * \param bInfo The bearer information to save.
   */
  static void RegisterBearerInfo (Ptr<BearerInfo> bInfo);

  BearerCreated_t  m_bearer;          //!< EPS bearer context created.
  uint16_t         m_blockReason;     //!< Bitmap for blocked reasons.
  bool             m_isActive;        //!< True for active bearer.
  bool             m_isAggregated;    //!< True for aggregated bearer.
  bool             m_isDefault;       //!< True for default bearer.
  bool             m_isGbrRes [2];    //!< True for GBR resources reserved.
  bool             m_isInstGw;        //!< True for installed gateway rules.
  bool             m_isInstIf [2];    //!< True for installed interface rules.
  bool             m_isMbrDlInst [2]; //!< True for downlink meter installed.
  bool             m_isMbrUlInst [2]; //!< True for uplink meter installed.
  uint16_t         m_pgwTftIdx;       //!< P-GW TFT switch index.
  uint16_t         m_priority;        //!< Flow table rule priority.
  SliceId          m_sliceId;         //!< Slice ID for this bearer.
  uint32_t         m_teid;            //!< GTP TEID.
  uint16_t         m_timeout;         //!< Flow table idle timeout.
  Ptr<UeInfo>      m_ueInfo;          //!< UE metadata pointer.

  /** Map saving TEID / bearer information. */
  typedef std::map<uint32_t, Ptr<BearerInfo>> TeidBearerMap_t;
  static TeidBearerMap_t m_BearerInfoByTeid;  //!< Global bearer info map.
};

/**
 * Print the bearer metadata on an output stream.
 * \param os The output stream.
 * \param bInfo The BearerInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the BearerInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const BearerInfo &bInfo);

} // namespace ns3
#endif // BEARER_INFO_H
