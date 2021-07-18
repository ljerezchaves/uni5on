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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef ENB_INFO_H
#define ENB_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class EnbApplication;

/**
 * \ingroup uni5onMeta
 * Metadata associated to an eNB.
 */
class EnbInfo : public Object
{
  friend class ScenarioConfig;

public:
  /**
   * Complete constructor.
   * \param cellId The cell identifier for this eNB.
   * \param device The OpenFlow switch device.
   * \param s1uAddr The eNB S1-U IP address.
   * \param infraSwIdx The OpenFlow transport switch index.
   * \param infraSwS1uPortNo The port number for S1-U interface at the switch.
   * \param s1uPortNo The port number for S1-U interface at the eNB.
   * \param appPortNo The port number for eNB application interface at the eNB.
   */
  EnbInfo (uint16_t cellId, Ptr<OFSwitch13Device> device, Ipv4Address s1uAddr,
           uint16_t infraSwIdx, uint32_t infraSwS1uPortNo, uint32_t s1uPortNo,
           uint32_t appPortNo);
  virtual ~EnbInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for eNB information.
   * \return The requested information.
   */
  //\{
  uint64_t                  GetDpId             (void) const;
  Ptr<EnbApplication>       GetApplication      (void) const;
  uint16_t                  GetCellId           (void) const;
  uint16_t                  GetInfraSwIdx       (void) const;
  uint32_t                  GetInfraSwS1uPortNo (void) const;
  EpcS1apSapEnb*            GetS1apSapEnb       (void) const;
  Ipv4Address               GetS1uAddr          (void) const;
  uint32_t                  GetS1uPortNo        (void) const;
  uint32_t                  GetAppPortNo        (void) const;
  //\}

  /**
   * Get the eNB information from the global map for a specific cell Id.
   * \param cellId The cell identifier for this eNB.
   * \return The eNb information for this cell ID.
   */
  static Ptr<EnbInfo> GetPointer (uint16_t cellId);

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
   * Set the internal application metadata.
   * \param value The application pointer.
   */
  void SetApplication (Ptr<EnbApplication> value);

  /**
   * Register the eNB information in global map for further usage.
   * \param enbInfo The eNB information to save.
   */
  static void RegisterEnbInfo (Ptr<EnbInfo> enbInfo);

  // eNB metadata.
  Ptr<OFSwitch13Device>     m_device;             //!< eNB OpenFlow device.
  Ptr<EnbApplication>       m_application;        //!< eNB application.
  uint16_t                  m_cellId;             //!< eNB cell ID.
  uint16_t                  m_infraSwIdx;         //!< Transport switch index.
  uint32_t                  m_infraSwS1uPortNo;   //!< Transport switch port no.
  Ipv4Address               m_s1uAddr;            //!< eNB S1-U IP address.
  uint32_t                  m_s1uPortNo;          //!< eNB S1-U port no.
  uint32_t                  m_appPortNo;          //!< eNB app port no.

  /** Map saving cell ID / eNB information. */
  typedef std::map<uint16_t, Ptr<EnbInfo>> CellIdEnbInfoMap_t;
  static CellIdEnbInfoMap_t m_enbInfoByCellId;    //!< Global eNB info map.
};

/**
 * Print the eNB metadata on an output stream.
 * \param os The output stream.
 * \param enbInfo The EnbInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the EnbInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const EnbInfo &enbInfo);

} // namespace ns3
#endif // ENB_INFO_H
