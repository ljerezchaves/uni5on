/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

namespace ns3 {

class RoutingInfo;

/** EPS bearer context created. */
typedef EpcS11SapMme::BearerContextCreated BearerContext_t;

/** List of bearer context created. */
typedef std::list<BearerContext_t> BearerContextList_t;

/** List of routing information. */
typedef std::list<Ptr<RoutingInfo> > RoutingInfoList_t;

/** Enumeration of available slices. */
typedef enum
{
  DFT = 0,  //!< Best-effort (default) slice.
  GBR = 1,  //!< HTC GBR slice.
  MTC = 2,  //!< MTC slice.
  ALL = 3   //!< ALL previous slices.
} Slice;

std::string SliceStr (Slice slice);

/**
 * \ingroup sdmn
 * \defgroup sdmnInfo Metadata
 * Metadata used by the SDMN architecture.
 */
/**
 * \ingroup sdmnInfo
 * Metadata associated to the S5 routing path between the S-GW and P-GW nodes
 * for a single EPS bearer.
 */
class RoutingInfo : public Object
{
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
   * \param imsi The UE IMSI.
   * \param isDefault True for default bearer.
   * \param isMtc True for MTC traffic.
   */
  RoutingInfo (uint32_t teid, BearerContext_t bearer, uint64_t imsi,
               bool isDefault, bool isMtc);
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  std::string GetBlockReasonStr (void) const;
  uint16_t GetDscp (void) const;
  uint64_t GetImsi (void) const;
  Ipv4Address GetPgwS5Addr (void) const;
  uint16_t GetPgwTftIdx (void) const;
  uint16_t GetPriority (void) const;
  Slice GetSlice (void) const;
  std::string GetSliceStr (void) const;
  Ipv4Address GetSgwS5Addr (void) const;
  uint32_t GetTeid (void) const;
  uint16_t GetTimeout (void) const;
  bool IsActive (void) const;
  bool IsAggregated (void) const;
  bool IsBlocked (void) const;
  bool IsDefault (void) const;
  bool IsHtc (void) const;
  bool IsInstalled (void) const;
  bool IsMtc (void) const;

  void SetActive (bool value);
  void SetBlocked (bool value, BlockReason reason = RoutingInfo::NOTBLOCKED);
  void SetDscp (uint16_t value);
  void SetInstalled (bool value);
  void SetPgwS5Addr (Ipv4Address value);
  void SetPgwTftIdx (uint16_t value);
  void SetPriority (uint16_t value);
  void SetSlice (Slice value);
  void SetSgwS5Addr (Ipv4Address value);
  void SetTimeout (uint16_t value);
  //\}

  /**
   *\name Accessors for bearer context information.
   * \return The requested information.
   */
  //\{
  EpsBearer GetEpsBearer (void) const;
  EpsBearer::Qci GetQciInfo (void) const;
  GbrQosInformation GetQosInfo (void) const;
  Ptr<EpcTft> GetTft (void) const;
  bool IsGbr (void) const;
  bool HasDownlinkTraffic (void) const;
  bool HasUplinkTraffic (void) const;
  //\}

  /**
   * Increase the priority value by one unit.
   */
  void IncreasePriority ();

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
   * Get a list of routing information for active bearers that are currently
   * installed in the OpenFlow switches at the P-GW and backhaul network.
   * \param pgwTftIdx The P-GW TFT index to filter the list.
   * \return The list of installed bearers.
   */
  static RoutingInfoList_t GetInstalledList (uint16_t pgwTftIdx = 0);

  /**
   * TracedCallback signature for Ptr<const RoutingInfo>.
   * \param rInfo The routing information.
   */
  typedef void (*TracedCallback)(Ptr<const RoutingInfo> rInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Register the routing information in global map for further usage.
   * \param rInfo The routing information to save.
   */
  static void RegisterRoutingInfo (Ptr<RoutingInfo> rInfo);

  BearerContext_t   m_bearer;       //!< EPS bearer information.
  BlockReason       m_blockReason;  //!< Bearer blocked reason.
  uint8_t           m_dscp;         //!< DiffServ DSCP value.
  uint64_t          m_imsi;         //!< UE IMSI.
  bool              m_isActive;     //!< Traffic active status.
  bool              m_isBlocked;    //!< Bearer request status.
  bool              m_isDefault;    //!< This is a default bearer.
  bool              m_isInstalled;  //!< Rules installed status.
  bool              m_isMtc;        //!< This is a MTC traffic.
  uint16_t          m_pgwTftIdx;    //!< P-GW TFT switch index.
  Ipv4Address       m_pgwS5Addr;    //!< P-GW S5 IPv4 address.
  uint16_t          m_priority;     //!< Flow rule priority.
  Slice             m_slice;        //!< Traffic backhaul slice.
  Ipv4Address       m_sgwS5Addr;    //!< S-GW S5 IPv4 address.
  uint32_t          m_teid;         //!< GTP TEID.
  uint16_t          m_timeout;      //!< Flow idle timeout.

  /** Map saving TEID / routing information. */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;
  static TeidRoutingMap_t m_globalInfoMap;  //!< Global routing info map.
};

};  // namespace ns3
#endif // ROUTING_INFO_H

