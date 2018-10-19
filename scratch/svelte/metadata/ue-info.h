/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef UE_INFO_H
#define UE_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "../svelte-common.h"

namespace ns3 {

class EnbInfo;
class PgwInfo;
class SgwInfo;
class RoutingInfo;
class SliceController;

/** Map saving Bearer ID / Routing information. */
typedef std::map<uint8_t, Ptr<RoutingInfo> > BidRInfoMap_t;

/**
 * \ingroup svelte
 * \defgroup svelteMeta Metadata
 * SVELTE architecture metadata.
 */

/**
 * \ingroup svelteMeta
 * Metadata associated to a UE.
 */
class UeInfo : public Object
{
  friend class PgwTunnelApp;
  friend class RoutingInfo;
  friend class SliceController;
  friend class SvelteHelper;
  friend class SvelteMme;
  friend class TrafficStatsCalculator;

public:
  /**
   * Complete constructor.
   * \param imsi The IMSI identifier.
   * \param addr The UE IP address.
   * \param sliceCtrl The slice controller application.
   */
  UeInfo (uint64_t imsi, Ipv4Address addr, Ptr<SliceController> sliceCtrl);
  virtual ~UeInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Hold info on an EPS bearer to be activated. */
  struct BearerInfo
  {
    Ptr<EpcTft> tft;
    EpsBearer   bearer;
    uint8_t     bearerId;
  };

  /**
   * \name Private member accessors for UE information.
   * \return The requested information.
   */
  //\{
  Ipv4Address           GetAddr       (void) const;
  uint16_t              GetEnbCellId  (void) const;
  Ptr<EnbInfo>          GetEnbInfo    (void) const;
  uint64_t              GetEnbUeS1Id  (void) const;
  uint64_t              GetImsi       (void) const;
  uint64_t              GetMmeUeS1Id  (void) const;
  uint16_t              GetNBearers   (void) const;
  Ptr<PgwInfo>          GetPgwInfo    (void) const;
  EpcS11SapSgw*         GetS11SapSgw  (void) const;
  EpcS1apSapEnb*        GetS1apSapEnb (void) const;
  Ptr<SgwInfo>          GetSgwInfo    (void) const;
  Ptr<SliceController>  GetSliceCtrl  (void) const;
  SliceId               GetSliceId    (void) const;
  uint32_t              GetDefaultTeid  (void) const;
  //\}

  /**
   * Get the bearer for this bearer ID.
   * \param bearerId The bearer ID.
   * \return The const reference to the bearer.
   */
  const BearerInfo& GetBearer (uint8_t bearerId) const;

  /**
   * Get the list of bearers for this UE.
   * \return The const reference to the list of bearers.
   */
  const std::vector<BearerInfo>& GetBearerList (void) const;

  /**
   * Get the routing information for this bearer ID.
   * \param bearerId The bearer ID.
   * \return The const reference to the routing information.
   */
  Ptr<const RoutingInfo> GetRoutingInfo (uint8_t bearerId) const;

  /**
   * Get the map of routing information for this UE.
   * \return The const reference to the map of routing information.
   */
  const BidRInfoMap_t& GetRoutingInfoMap (void) const;

  /**
   * Get the UE information from the global map for a specific IMSI.
   * \param imsi The IMSI identifier for this UE.
   * \return The UE information for this IMSI.
   */
  static Ptr<UeInfo> GetPointer (uint64_t imsi);

  /**
   * Get the UE information from the global map for a specific UE IPv4.
   * \param addr The UE IP address.
   * \return The UE information for this IP.
   */
  static Ptr<UeInfo> GetPointer (Ipv4Address addr);

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * Get the empty string for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintNull (std::ostream &os);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * \name Private member accessors for updating internal metadata.
   * \param value The value to set.
   */
  //\{
  void SetEnbUeS1Id (uint64_t value);
  void SetEnbInfo   (Ptr<EnbInfo> value);
  void SetPgwInfo   (Ptr<PgwInfo> value);
  void SetSgwInfo   (Ptr<SgwInfo> value);
  //\}

  /**
   * Add an EPS bearer to the list of bearers for this UE. The bearer will be
   * activated when the UE enters the ECM connected state.
   * \param bearer The bearer info.
   * \return The bearer ID.
   */
  uint8_t AddBearer (BearerInfo bearer);

  /**
   * Add an EPS routing metadata to the list of routing contexts for this UE.
   * The corresponding TFT will be automatically added to the TFT classifier.
   * \param rInfo The routing info.
   */
  void AddRoutingInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Classify the packet using the UE TFT classifier.
   * \param packet The IP packet to be classified.
   * \return The GTP tunnel ID for this packet.
   */
  uint32_t Classify (Ptr<Packet> packet);

  /**
   * Register the UE information in global map for further usage.
   * \param ueInfo The UE information to save.
   */
  static void RegisterUeInfo (Ptr<UeInfo> ueInfo);

  // UE metadata.
  Ipv4Address             m_addr;                 //!< UE IP address.
  uint64_t                m_imsi;                 //!< UE IMSI.
  Ptr<EnbInfo>            m_enbInfo;              //!< Serving eNB info.
  Ptr<PgwInfo>            m_pgwInfo;              //!< Serving P-GW info.
  Ptr<SgwInfo>            m_sgwInfo;              //!< Serving S-GW info.

  // Control-plane communication.
  Ptr<SliceController>    m_sliceCtrl;            //!< LTE logical slice ctrl.
  uint64_t                m_mmeUeS1Id;            //!< ID for S1-AP at MME.
  uint16_t                m_enbUeS1Id;            //!< ID for S1-AP at eNB.

  // Bearers and TFTs.
  std::vector<BearerInfo> m_bearersList;          //!< Bearer contexts.
  EpcTftClassifier        m_tftClassifier;        //!< P-GW TFT classifier.
  BidRInfoMap_t           m_rInfoByBid;           //!< Routing info map by BID.

  /** Map saving UE IMSI / UE information. */
  typedef std::map<uint64_t, Ptr<UeInfo> > ImsiUeInfoMap_t;
  static ImsiUeInfoMap_t  m_ueInfoByImsi;   //!< Global UE info map by IMSI.

  /** Map saving UE IPv4 / UE information. */
  typedef std::map<Ipv4Address, Ptr<UeInfo> > Ipv4UeInfoMap_t;
  static Ipv4UeInfoMap_t  m_ueInfoByAddr;   //!< Global UE info map by IPv4.
};

/**
 * Print the UE metadata on an output stream.
 * \param os The output stream.
 * \param ueInfo The UeInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the UeInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const UeInfo &ueInfo);

} // namespace ns3
#endif // UE_INFO_H
