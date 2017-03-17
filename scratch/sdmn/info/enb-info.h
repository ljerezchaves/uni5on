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
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * \ingroup sdmnInfo
 * Metadata associated to an eNB.
 */
class EnbInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param cellId The cell identifier for this eNB.
   */
  EnbInfo (uint16_t cellId);
  virtual ~EnbInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint16_t GetCellId (void) const;
  Ipv4Address GetEnbS1uAddr (void) const;
  Ipv4Address GetSgwS1uAddr (void) const;
  EpcS1apSapEnb* GetS1apSapEnb (void) const;

  void SetEnbS1uAddr (Ipv4Address value);
  void SetSgwS1uAddr (Ipv4Address value);
  void SetS1apSapEnb (EpcS1apSapEnb* value);
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

  uint16_t               m_cellId;               //!< eNB cell ID.
  Ipv4Address            m_enbS1uAddr;           //!< eNB S1-U IP address.
  Ipv4Address            m_sgwS1uAddr;           //!< S-GW S1-U IP address.
  EpcS1apSapEnb*         m_s1apSapEnb;           //!< S1-AP eNB SAP provider.

  /** Map saving cell ID / eNB information. */
  typedef std::map<uint16_t, Ptr<EnbInfo> > CellIdEnbInfo_t;
  static CellIdEnbInfo_t m_enbInfoByCellId;      //!< Global eNB info map.
};

};  // namespace ns3
#endif // ENB_INFO_H

