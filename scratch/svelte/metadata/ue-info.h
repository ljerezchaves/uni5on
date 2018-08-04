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
class SliceController;

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
  friend class SliceController;
  friend class SvelteMme;

public:
  /**
   * Complete constructor.
   * \param imsi The IMSI identifier.
   * \param ueAddr The UE IP address.
   * \param sliceCtrl The slice controller application.
   */
  UeInfo (uint64_t imsi, Ipv4Address ueAddr, Ptr<SliceController> sliceCtrl);
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

  /** \name Private member accessors. */
  //\{
  uint64_t GetImsi (void) const;
  SliceId GetSliceId (void) const;
  Ipv4Address GetUeAddr (void) const;
  uint16_t GetCellId (void) const;
  Ptr<EnbInfo> GetEnbInfo (void) const;
  Ptr<SgwInfo> GetSgwInfo (void) const;
  Ptr<PgwInfo> GetPgwInfo (void) const;
  uint64_t GetMmeUeS1Id (void) const;
  uint64_t GetEnbUeS1Id (void) const;
  EpcS11SapSgw* GetS11SapSgw (void) const;
  EpcS1apSapEnb* GetS1apSapEnb (void) const;
  Ptr<SliceController> GetSliceCtrl (void) const;
  std::list<BearerInfo> GetBearerList (void) const;
  //\}

  /**
   * Add an EPS bearer to the list of bearers for this UE.  The bearer will be
   * activated when the UE enters the ECM connected state.
   * \param bearer The bearer info.
   * \return The bearer ID.
   */
  uint8_t AddBearer (BearerInfo bearer);

  /**
   * Remove the bearer context for a specific bearer ID.
   * \param bearerId The bearer ID.
   */
  void RemoveBearer (uint8_t bearerId);

  /**
   * Add a TFT entry to the UE TFT classifier.
   * \param tft The bearer Traffic Flow Template.
   * \param teid The GTP tunnel ID.
   */
  void AddTft (Ptr<EpcTft> tft, uint32_t teid);

  /**
   * Classify the packet using the UE TFT classifier.
   * \param packet The IP packet to be classified.
   * \return The GTP tunnel ID for this packet.
   */
  uint32_t Classify (Ptr<Packet> packet);

  /**
   * Get the UE information from the global map for a specific IMSI.
   * \param imsi The IMSI identifier for this UE.
   * \return The UE information for this IMSI.
   */
  static Ptr<UeInfo> GetPointer (uint64_t imsi);

  /**
   * Get the UE information from the global map for a specific UE IPv4.
   * \param ipv4 The UE IPv4.
   * \return The UE information for this IP.
   */
  static Ptr<UeInfo> GetPointer (Ipv4Address ipv4);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /** \name Private member accessors. */
  //\{
  void SetEnbInfo (Ptr<EnbInfo> enbInfo, uint64_t enbUeS1Id);
  void SetSgwInfo (Ptr<SgwInfo> value);
  void SetPgwInfo (Ptr<PgwInfo> value);
  //\}

  /**
   * Register the UE information in global map for further usage.
   * \param ueInfo The UE information to save.
   */
  static void RegisterUeInfo (Ptr<UeInfo> ueInfo);

  // UE metadata.
  uint64_t               m_imsi;                 //!< UE IMSI.
  Ipv4Address            m_ueAddr;               //!< UE IP address.
  Ptr<EnbInfo>           m_enbInfo;              //!< Serving eNB info
  Ptr<SgwInfo>           m_sgwInfo;              //!< Serving S-GW info.
  Ptr<PgwInfo>           m_pgwInfo;              //!< Serving P-GW info.

  // Control-plane communication.
  Ptr<SliceController>   m_sliceCtrl;            //!< LTE logical slice ctrl.
  uint64_t               m_mmeUeS1Id;            //!< ID for S1-AP at MME.
  uint16_t               m_enbUeS1Id;            //!< ID for S1-AP at eNB.

  // Bearers and TFTs.
  uint16_t               m_bearerCounter;        //!< Number of bearers.
  std::list<BearerInfo>  m_bearersList;          //!< Bearer contexts.
  EpcTftClassifier       m_tftClassifier;        //!< P-GW TFT classifier.

  /** Map saving UE IMSI / UE information. */
  typedef std::map<uint64_t, Ptr<UeInfo> > ImsiUeInfoMap_t;

  /** Map saving UE IPv4 / UE information. */
  typedef std::map<Ipv4Address, Ptr<UeInfo> > Ipv4UeInfoMap_t;

  static ImsiUeInfoMap_t m_ueInfoByImsiMap;    //!< Global UE info map by IMSI.
  static Ipv4UeInfoMap_t m_ueInfoByIpv4Map;    //!< Global UE info map by IPv4.
};

} // namespace ns3
#endif // UE_INFO_H
