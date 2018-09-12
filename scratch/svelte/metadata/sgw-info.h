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
#include "../svelte-common.h"

namespace ns3 {

class SliceController;

/**
 * \ingroup svelteMeta
 * Metadata associated to a logical S-GW.
 */
class SgwInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param sgwId The S-GW ID.
   * \param s1uAddr The S-S1-U interface IP address.
   * \param s5Addr The S5 interface IP address.
   * \param s1uPortNo The port number for S1-U interface at the S-GW.
   * \param s5PortNo The port number for S5 interface at the S-GW.
   * \param infraSwIdx The OpenFlow backhaul switch index.
   * \param infraSwS1uPortNo The port number for S1-U interface at the switch.
   * \param infraSwS5PortNo The port number for S5 interface at the switch.
   * \param sliceCtrl The slice controller application.
   */
  SgwInfo (uint64_t sgwId, Ipv4Address s1uAddr, Ipv4Address s5Addr,
           uint32_t s1uPortNo, uint32_t s5PortNo, uint16_t infraSwIdx,
           uint32_t infraSwS1uPortNo, uint32_t infraSwS5PortNo,
           Ptr<SliceController> ctrlApp);
  virtual ~SgwInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for S-GW information.
   * \return The requested information.
   */
  //\{
  uint64_t              GetDpId             (void) const;
  uint16_t              GetInfraSwIdx       (void) const;
  uint32_t              GetInfraSwS1uPortNo (void) const;
  uint32_t              GetInfraSwS5PortNo  (void) const;
  Ipv4Address           GetS1uAddr          (void) const;
  uint32_t              GetS1uPortNo        (void) const;
  Ipv4Address           GetS5Addr           (void) const;
  uint32_t              GetS5PortNo         (void) const;
  uint64_t              GetSgwId            (void) const;
  Ptr<SliceController>  GetSliceController  (void) const;
  //\}

  /**
   * Get the S-GW information from the global map for a specific ID.
   * \param sgwId The ID for this S-GW.
   * \return The S-GW information for this ID.
   */
  static Ptr<SgwInfo> GetPointer (uint64_t sgwId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Register the S-GW information in global map for further usage.
   * \param sgwInfo The S-GW information to save.
   */
  static void RegisterSgwInfo (Ptr<SgwInfo> sgwInfo);

  // S-GW metadata.
  uint16_t               m_infraSwIdx;           //!< Backhaul switch index.
  uint32_t               m_infraSwS1uPortNo;     //!< Back switch S1-U port no.
  uint32_t               m_infraSwS5PortNo;      //!< Back switch S5 port no.
  Ipv4Address            m_s1uAddr;              //!< S-GW S1-U IP address.
  uint32_t               m_s1uPortNo;            //!< S-GW S1-U port no.
  Ipv4Address            m_s5Addr;               //!< S-GW S5 IP address.
  uint32_t               m_s5PortNo;             //!< S-GW S5 port no.
  uint64_t               m_sgwId;                //!< S-GW ID.
  Ptr<SliceController>   m_sliceCtrl;            //!< LTE logical slice ctrl.

  /** Map saving S-GW ID / S-GW information. */
  typedef std::map<uint64_t, Ptr<SgwInfo> > SgwIdSgwInfoMap_t;
  static SgwIdSgwInfoMap_t m_sgwInfoBySgwId;     //!< Global S-GW info map.
};

} // namespace ns3
#endif // SGW_INFO_H
