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

class EpcController;
class RingController;

/** EPS context bearer */
typedef EpcS11SapMme::BearerContextCreated ContextBearer_t;

/** List of created context bearers */
typedef std::list<ContextBearer_t> BearerList_t;

/**
 * \ingroup sdmn
 * \defgroup sdmnInfo Metadata
 * Metadata used by the SDMN architecture.
 */
/**
 * \ingroup sdmnInfo
 * Metadata associated to a routing path between
 * two any switches in the OpenFlow network.
 */
class RoutingInfo : public Object
{
  friend class EpcController;
  friend class RingController;

public:
  /**
   * Complete constructor.
   * \param teid The TEID value.
   */
  RoutingInfo (uint32_t teid);
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDispose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  GbrQosInformation GetQosInfo          (void) const;
  EpsBearer::Qci    GetQciInfo          (void) const;
  EpsBearer         GetEpsBearer        (void) const;
  Ptr<EpcTft>       GetTft              (void) const;
  uint32_t          GetTeid             (void) const;
  uint64_t          GetImsi             (void) const;
  uint16_t          GetCellId           (void) const;
  uint64_t          GetEnbSwDpId        (void) const;
  uint64_t          GetPgwSwDpId        (void) const;
  Ipv4Address       GetEnbAddr          (void) const;
  Ipv4Address       GetPgwAddr          (void) const;
  uint16_t          GetPriority         (void) const;
  uint16_t          GetTimeout          (void) const;
  bool              HasDownlinkTraffic  (void) const;
  bool              HasUplinkTraffic    (void) const;
  bool              IsGbr               (void) const;
  bool              IsDefault           (void) const;
  bool              IsInstalled         (void) const;
  bool              IsActive            (void) const;
  //\}

  /**
   * Get the routing information from the global map for a specific TEID.
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  static Ptr<const RoutingInfo> GetConstPointer (uint32_t teid);

  /**
   * Get stored information for a specific EPS bearer.
   * \param teid The GTP tunnel ID.
   * \return The EpsBearer information for this teid.
   */
  static EpsBearer GetEpsBearer (uint32_t teid);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * Set the internal installed flag.
   * \param installed The value to set.
   */
  void SetInstalled (bool installed);

  /**
   * Set the internal active flag.
   * \param active The value to set.
   */
  void SetActive (bool active);

  /** Increase the priority value by one unit. */
  void IncreasePriority ();

  /**
   * Get the routing information from the global map for a specific TEID.
   * \param teid The GTP tunnel ID.
   * \return The routing information for this tunnel.
   */
  static Ptr<RoutingInfo> GetPointer (uint32_t teid);

  /**
   * Register the routing information in global map for further usage.
   * \param rInfo The routing information to save.
   */
  static void RegisterRoutingInfo (Ptr<RoutingInfo> rInfo);

private:
  uint32_t          m_teid;         //!< GTP TEID
  uint64_t          m_imsi;         //!< UE IMSI
  uint16_t          m_cellId;       //!< eNB cell ID
  uint64_t          m_pgwDpId;      //!< P-GW switch datapath id
  uint64_t          m_enbDpId;      //!< eNB switch datapath id
  Ipv4Address       m_pgwAddr;      //!< P-GW IPv4 address
  Ipv4Address       m_enbAddr;      //!< eNB IPv4 address
  uint16_t          m_priority;     //!< Flow priority
  uint16_t          m_timeout;      //!< Flow idle timeout
  bool              m_isDefault;    //!< This info is for default bearer
  bool              m_isInstalled;  //!< Rule is installed into switches
  bool              m_isActive;     //!< Application traffic is active
  ContextBearer_t   m_bearer;       //!< EPS bearer information

  /** Map saving TEID / routing information. */
  typedef std::map<uint32_t, Ptr<RoutingInfo> > TeidRoutingMap_t;
  static TeidRoutingMap_t m_globalInfoMap;  //!< Global routing info map.
};

};  // namespace ns3
#endif // ROUTING_INFO_H

