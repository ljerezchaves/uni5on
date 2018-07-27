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

#ifndef PGW_INFO_H
#define PGW_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include "../../slice-id.h"

namespace ns3 {

/**
 * \ingroup svelteLogical
 * Metadata associated to a logical P-GW.
 */
class PgwInfo : public Object
{
  friend class SliceNetwork;

public:
  /**
   * Complete constructor.
   * \param PgwId The ID for this P-GW.
   */
  PgwInfo (uint64_t pgwId);
  virtual ~PgwInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint64_t GetPgwId (void) const;
  SliceId GetSliceId (void) const;
  Ipv4Address GetS1uAddr (void) const;
  Ipv4Address GetS5Addr (void) const;
  uint32_t GetS1uPortNo (void) const;
  uint32_t GetS5PortNo (void) const;
  uint16_t GetInfraSwIdx (void) const;
  uint32_t GetInfraSwS1uPortNo (void) const;
  uint32_t GetInfraSwS5PortNo (void) const;
  //\}

  /**
   * Get the P-GW information from the global map for a specific ID.
   * \param pgwId The ID for this P-GW.
   * \return The P-GW information for this ID.
   */
  static Ptr<PgwInfo> GetPointer (uint64_t pgwId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /** \name Private member accessors. */
  //\{
  void SetSliceId (SliceId value);
  void SetS1uAddr (Ipv4Address value);
  void SetS5Addr (Ipv4Address value);
  void SetS1uPortNo (uint32_t value);
  void SetS5PortNo (uint32_t value);
  void SetInfraSwIdx (uint16_t value);
  void SetInfraSwS1uPortNo (uint32_t value);
  void SetInfraSwS5PortNo (uint32_t value);
  //\}

  /**
   * Register the P-GW information in global map for further usage.
   * \param pgwInfo The P-GW information to save.
   */
  static void RegisterPgwInfo (Ptr<PgwInfo> pgwInfo);

  // P-GW metadata.
  uint64_t                m_pgwId;              //!< P-GW ID (P-GW main dpId).
  SliceId                 m_sliceId;            //!< LTE logical slice ID.

  std::vector<uint64_t>   m_pgwDpIds;




  Ipv4Address            m_s1uAddr;              //!< P-GW S1-U IP address.
  Ipv4Address            m_s5Addr;               //!< P-GW S5 IP address.
  uint32_t               m_s1uPortNo;            //!< P-GW S1-U port no.
  uint32_t               m_s5PortNo;             //!< P-GW S5 port no.
  uint16_t               m_infraSwIdx;           //!< Backhaul switch index.
  uint32_t               m_infraSwS1uPortNo;     //!< Back switch S1-U port no.
  uint32_t               m_infraSwS5PortNo;      //!< Back switch S5 port no.

  /** Map saving P-GW ID / P-GW information. */
  typedef std::map<uint64_t, Ptr<PgwInfo> > PgwIdPgwInfo_t;
  static PgwIdPgwInfo_t  m_pgwInfoByPgwId;       //!< Global P-GW info map.
};

} // namespace ns3
#endif // PGW_INFO_H
