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

#ifndef SGW_INFO_H
#define SGW_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include "../../slice-id.h"

namespace ns3 {

/**
 * \ingroup svelteLogical
 * Metadata associated to a logical S-GW.
 */
class SgwInfo : public Object
{
  friend class SliceNetwork;

public:
  /**
   * Complete constructor.
   * \param sgwId The ID for this S-GW.
   */
  SgwInfo (uint64_t sgwId);
  virtual ~SgwInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint64_t GetSgwId (void) const;
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
   * Get the S-GW information from the global map for a specific ID.
   * \param sgwId The ID for this S-GW.
   * \return The S-GW information for this ID.
   */
  static Ptr<SgwInfo> GetPointer (uint64_t sgwId);

  /**
   * Get the S-GW information from the global map for a specific backhaul
   * switch index.
   * \param infaSwIdx The backhaul switch index.
   * \return The S-GW information.
   */
  static Ptr<SgwInfo> GetPointerBySwIdx (uint16_t infaSwIdx);

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
   * Register the S-GW information in global map for further usage.
   * \param sgwInfo The S-GW information to save.
   */
  static void RegisterSgwInfo (Ptr<SgwInfo> sgwInfo);

  // S-GW metadata.
  uint64_t               m_sgwId;                //!< S-GW ID.
  SliceId                m_sliceId;              //!< LTE logical slice ID.
  Ipv4Address            m_s1uAddr;              //!< S-GW S1-U IP address.
  Ipv4Address            m_s5Addr;               //!< S-GW S5 IP address.
  uint32_t               m_s1uPortNo;            //!< S-GW S1-U port no.
  uint32_t               m_s5PortNo;             //!< S-GW S5 port no.
  uint16_t               m_infraSwIdx;           //!< Backhaul switch index.
  uint32_t               m_infraSwS1uPortNo;     //!< Back switch S1-U port no.
  uint32_t               m_infraSwS5PortNo;      //!< Back switch S5 port no.

  /** Map saving S-GW ID / S-GW information. */
  typedef std::map<uint64_t, Ptr<SgwInfo> > SgwIdSgwInfo_t;
  static SgwIdSgwInfo_t  m_sgwInfoBySgwId;       //!< Global S-GW info map.
};

} // namespace ns3
#endif // SGW_INFO_H
