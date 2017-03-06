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
  friend class EpcController;
  friend class SdranCloud;
  friend class SdmnMme;

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

  /**
   * \name Private member accessors.
   * \return The requested field.
   */
  //\{
  uint16_t       GetCellId      (void) const;
  Ipv4Address    GetEnbAddress  (void) const;
  Ipv4Address    GetSgwAddress  (void) const;
  EpcS1apSapEnb* GetS1apSapEnb  (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * Set the address of the eNB to which the UE is connected to.
   * \param addr The eNB address.
   */
  void SetEnbAddress (Ipv4Address addr);

  /**
   * Set the address of the S-GW.
   * \param addr The S-GW address.
   */
  void SetSgwAddress (Ipv4Address addr);

  /**
   * Set the eNB side of the S1-AP SAP.
   * \param sap The eNB S1-AP SAP.
   */
  void SetS1apSapEnb (EpcS1apSapEnb *sap);

  /**
   * Get the eNB information from the global map for a specific cell Id.
   * \param cellId The cell identifier for this eNB.
   * \return The eNb information for this cell ID.
   */
  static Ptr<EnbInfo> GetPointer (uint16_t cellId);

private:
  /**
   * Register the eNB information in global map for further usage.
   * \param enbInfo The eNB information to save.
   */
  static void RegisterEnbInfo (Ptr<EnbInfo> enbInfo);

  uint16_t               m_cellId;               //!< eNB cell ID.
  Ipv4Address            m_enbAddr;              //!< eNB IP address.
  Ipv4Address            m_sgwAddr;              //!< S-GW IP address.
  EpcS1apSapEnb*         m_s1apSapEnb;           //!< S1-AP eNB SAP provider.

  /** Map saving cell ID / eNB information. */
  typedef std::map<uint16_t, Ptr<EnbInfo> > CellIdEnbInfo_t;
  static CellIdEnbInfo_t m_enbInfoByCellId;      //!< Global eNB info map.
};

};  // namespace ns3
#endif // ENB_INFO_H

