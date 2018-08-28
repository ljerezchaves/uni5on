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
class GbrInfo;
class MeterInfo;

/** List of bearer information. */
typedef std::list<Ptr<RoutingInfo> > RoutingInfoList_t;

/**
 * \ingroup svelteMeta
 * Metadata associated to the EPS bearer.
 */
class RoutingInfo : public Object
{
  friend class BackhaulController;
  friend class SliceController;

public:
  /** Block reason. */
  enum BlockReason
  {
    NOTBLOCKED   = 0,  //!< This bearer was not blocked.
    TFTTABLEFULL = 1,  //!< P-GW TFT flow table is full.
    TFTMAXLOAD   = 2,  //!< P-GW TFT pipeline load is maximum.
    NOBANDWIDTH  = 3   //!< No backahul bandwidth available.
  };

  /**
   * Complete constructor.
   * \param teid The TEID value.
   * \param bearer The bearer context.
   * \param ueInfo The UE metadata.
   * \param isDefault True for default bearer.
   */
  RoutingInfo (uint32_t teid, BearerContext_t bearer,
               Ptr<UeInfo> ueInfo, bool isDefault);
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint32_t GetTeid (void) const;
  std::string GetTeidHex (void) const;
  std::string GetBlockReasonStr (void) const;
  bool HasGbrInfo (void) const;
  bool HasMeterInfo (void) const;
  bool IsActive (void) const;
  bool IsAggregated (void) const;
  bool IsBlocked (void) const;
  bool IsDefault (void) const;
  bool IsInstalled (void) const;
  uint16_t GetPgwTftIdx (void) const;
  uint16_t GetPriority (void) const;
  uint16_t GetTimeout (void) const;
  Ptr<GbrInfo> GetGbrInfo (void) const;
  Ptr<MeterInfo> GetMeterInfo (void) const;
  Ptr<UeInfo> GetUeInfo (void) const;
  //\}

  /**
   * \name Private member accessors for bearer information.
   */
  //\{
  Ipv4Header::DscpType GetDscp (void) const;
  uint16_t GetDscpValue (void) const;
  EpsBearer GetEpsBearer (void) const;
  EpsBearer::Qci GetQciInfo (void) const;
  GbrQosInformation GetQosInfo (void) const;
  Ptr<EpcTft> GetTft (void) const;
  bool HasDownlinkTraffic (void) const;
  bool HasUplinkTraffic (void) const;
  bool IsGbr (void) const;
  //\}

  /**
   * \name Private member accessors for UE and routing information.
   */
  //\{
  uint64_t GetImsi (void) const;
  uint16_t GetCellId (void) const;
  SliceId GetSliceId (void) const;
  std::string GetSliceIdStr (void) const;
  Ipv4Address GetEnbS1uAddr (void) const;
  Ipv4Address GetPgwS5Addr (void) const;
  Ipv4Address GetSgwS1uAddr (void) const;
  Ipv4Address GetSgwS5Addr (void) const;
  Ipv4Address GetUeAddr (void) const;
  uint64_t GetSgwDpId (void) const;
  uint32_t GetSgwS1uPortNo (void) const;
  uint32_t GetSgwS5PortNo (void) const;
  uint16_t GetEnbInfraSwIdx (void) const;
  uint16_t GetPgwInfraSwIdx (void) const;
  uint16_t GetSgwInfraSwIdx (void) const;
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
   * TracedCallback signature for Ptr<const RoutingInfo>.
   * \param rInfo The bearer information.
   */
  typedef void (*TracedCallback)(Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /** \name Private member accessors. */
  //\{
  void SetActive (bool value);
  void SetBlocked (bool value, BlockReason reason = RoutingInfo::NOTBLOCKED);
  void SetInstalled (bool value);
  void SetPgwTftIdx (uint16_t value);
  void SetPriority (uint16_t value);
  void SetTimeout (uint16_t value);
  //\}

  /**
   * Get a list of the installed bearer routing information, optionally
   * filtered by LTE logical slice and P-GW TFT switch index.
   * \param slice The LTE logical slice ID.
   * \param pgwTftIdx The P-GW TFT index.
   * \return The list of installed bearers.
   */
  static RoutingInfoList_t GetInstalledList (SliceId slice = SliceId::NONE,
                                             uint16_t pgwTftIdx = 0);

  /**
   * Increase the priority value by one unit.
   */
  void IncreasePriority ();

private:
  /**
   * Register the routing information in global map for further usage.
   * \param rInfo The routing information to save.
   */
  static void RegisterRoutingInfo (Ptr<RoutingInfo> rInfo);

  uint32_t             m_teid;         //!< GTP TEID.
  BearerContext_t      m_bearer;       //!< EPS bearer information.
  BlockReason          m_blockReason;  //!< Bearer blocked reason.
  bool                 m_isActive;     //!< Bearer active status.
  bool                 m_isAggregated; //!< Bearer aggregation status.
  bool                 m_isBlocked;    //!< Bearer request status.
  bool                 m_isDefault;    //!< This is a default bearer.
  bool                 m_isInstalled;  //!< Rules installed status.
  uint16_t             m_pgwTftIdx;    //!< P-GW TFT switch index.
  uint16_t             m_priority;     //!< Flow rule priority.
  uint16_t             m_timeout;      //!< Flow idle timeout.
  Ptr<GbrInfo>         m_gbrInfo;      //!< GBR metadata.
  Ptr<MeterInfo>       m_meterInfo;    //!< Meter metadata.
  Ptr<UeInfo>          m_ueInfo;       //!< UE metadata.


  /** Map saving TEID / routing information. */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;
  static TeidRoutingMap_t m_routingInfoByTeid;  //!< Global routing info map.
};

} // namespace ns3
#endif // ROUTING_INFO_H
