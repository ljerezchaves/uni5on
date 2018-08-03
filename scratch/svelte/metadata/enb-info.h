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

#ifndef ENB_INFO_H
#define ENB_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>

namespace ns3 {

class SvelteEnbApplication;

/**
 * \ingroup svelteMeta
 * Metadata associated to an eNB.
 */
class EnbInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param cellId The cell identifier for this eNB.
   * \param s1uAddr The eNB S1-U IP address.
   * \param infraSwIdx The OpenFlow backhaul switch index.
   * \param infraSwS1uPortNo The port number for S1-U interface at the switch.
   * \param enbApp The eNB application.
   */
  EnbInfo (uint16_t cellId, Ipv4Address s1uAddr, uint16_t infraSwIdx,
           uint32_t infraSwS1uPortNo, Ptr<SvelteEnbApplication> enbApp);
  virtual ~EnbInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint16_t GetCellId (void) const;
  Ipv4Address GetS1uAddr (void) const;
  uint16_t GetInfraSwIdx (void) const;
  uint32_t GetInfraSwS1uPortNo (void) const;
  Ptr<SvelteEnbApplication> GetEnbApplication (void) const;
  EpcS1apSapEnb* GetS1apSapEnb (void) const;
  //\}

  /**
   * Get the eNB information from the global map for a specific cell Id.
   * \param cellId The cell identifier for this eNB.
   * \return The eNb information for this cell ID.
   */
  static Ptr<EnbInfo> GetPointer (uint16_t cellId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Register the eNB information in global map for further usage.
   * \param enbInfo The eNB information to save.
   */
  static void RegisterEnbInfo (Ptr<EnbInfo> enbInfo);

  // eNB metadata.
  uint16_t                  m_cellId;             //!< eNB cell ID.
  Ipv4Address               m_s1uAddr;            //!< eNB S1-U IP address.
  uint16_t                  m_infraSwIdx;         //!< Backhaul switch index.
  uint32_t                  m_infraSwS1uPortNo;   //!< Backhaul switch port no.
  Ptr<SvelteEnbApplication> m_enbApplication;     //!< eNB application.

  /** Map saving cell ID / eNB information. */
  typedef std::map<uint16_t, Ptr<EnbInfo> > CellIdEnbInfo_t;
  static CellIdEnbInfo_t    m_enbInfoByCellId;    //!< Global eNB info map.
};

} // namespace ns3
#endif // ENB_INFO_H
