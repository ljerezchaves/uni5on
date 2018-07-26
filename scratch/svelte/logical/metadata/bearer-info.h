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

#ifndef BEARER_INFO_H
#define BEARER_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../../slice-id.h"

namespace ns3 {

class BearerInfo;

/** EPS bearer context created. */
typedef EpcS11SapMme::BearerContextCreated BearerContext_t;

/** List of bearer context created. */
typedef std::list<BearerContext_t> BearerContextList_t;

/** List of bearer information. */
typedef std::list<Ptr<BearerInfo> > BearerInfoList_t;

/**
 * \ingroup svelteLogical
 * Metadata associated to the EPS bearer.
 */
class BearerInfo : public Object
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
   * \param sliceId The logical slice ID.
   * \param isDefault True for default bearer.
   */
  BearerInfo (uint32_t teid, BearerContext_t bearer, uint64_t imsi,
              SliceId sliceId, bool isDefault);
  virtual ~BearerInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint32_t GetTeid (void) const;
  uint64_t GetImsi (void) const;
  SliceId GetSliceId (void) const;
  std::string GetSliceIdStr (void) const;
  bool IsDefault (void) const;
  bool IsActive (void) const;
  bool IsBlocked (void) const;
  std::string GetBlockReasonStr (void) const;

  //\}

  /**
   * \name Accessors for bearer related information.
   * \return The requested information.
   */
  //\{
  EpsBearer GetEpsBearer (void) const;
  EpsBearer::Qci GetQciInfo (void) const;
  GbrQosInformation GetQosInfo (void) const;
  Ptr<EpcTft> GetTft (void) const;
  Ipv4Header::DscpType GetDscp (void) const;
  uint16_t GetDscpValue (void) const;
  bool IsGbr (void) const;
  bool HasDownlinkTraffic (void) const;
  bool HasUplinkTraffic (void) const;
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
   * TracedCallback signature for Ptr<const BearerInfo>.
   * \param bInfo The bearer information.
   */
  typedef void (*TracedCallback)(Ptr<const BearerInfo> bInfo);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /** \name Private member accessors. */
  //\{
  void SetActive (bool value);
  void SetBlocked (bool value, BlockReason reason = BearerInfo::NOTBLOCKED);
  //\}

private:
  /**
   * Register the bearer information in global map for further usage.
   * \param bInfo The bearer information to save.
   */
  static void RegisterBearerInfo (Ptr<BearerInfo> bInfo);

  uint32_t             m_teid;         //!< GTP TEID.
  BearerContext_t      m_bearer;       //!< EPS bearer information.
  uint64_t             m_imsi;         //!< UE IMSI.
  SliceId              m_sliceId;      //!< Logical network slice.
  bool                 m_isDefault;    //!< This is a default bearer.
  bool                 m_isActive;     //!< Bearer active status.
  bool                 m_isBlocked;    //!< Bearer request status.
  BlockReason          m_blockReason;  //!< Bearer blocked reason.

  /** Map saving TEID / bearer information. */
  typedef std::map<uint32_t, Ptr<BearerInfo> > TeidBearerMap_t;
  static TeidBearerMap_t m_bearerInfoByTeid;  //!< Global bearer info map.
};

} // namespace ns3
#endif // BEARER_INFO_H
