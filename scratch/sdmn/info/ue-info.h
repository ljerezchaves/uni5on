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

namespace ns3 {

/**
 * \ingroup sdmnInfo
 * Metadata associated to a UE connected to a S-GW.
 */
class UeInfo : public Object
{
  friend class EpcController;

public:
  /**
   * Complete constructor.
   * \param imsi The IMSI identifier for this UE.
   */
  UeInfo (uint64_t imsi);
  virtual ~UeInfo (); //!< Dummy destructor, see DoDispose.

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
  uint64_t    GetImsi    (void) const;
  Ipv4Address GetUeAddr  (void) const;
  Ipv4Address GetEnbAddr (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  /**
   * Add a new bearer context for this UE.
   * \param epsBearerId The ID of the EPS Bearer to be activated.
   * \param teid The TEID of the new bearer.
   */
  void AddBearer (uint8_t epsBearerId, uint32_t teid);

  /**
   * Remove the bearer context for this UE.
   * \param bearerId The bearer id whose contexts to be removed.
   */
  void RemoveBearer (uint8_t bearerId);

  /**
   * Set the address of the UE.
   * \param addr The UE address.
   */
  void SetUeAddress (Ipv4Address addr);
  
  /**
   * Set the address of the eNB to which the UE is connected to.
   * \param addr The eNB address.
   */
  void SetEnbAddress (Ipv4Address addr);

  /**
   * Get the UE information from the global map for a specific IMSI.
   * \param imsi The IMSI identifier for this UE.
   * \return The UE information for this IMSI.
   */
  static Ptr<UeInfo> GetPointer (uint64_t imsi);

private:
  /**
   * Register the UE information in global map for further usage.
   * \param ueInfo The UE information to save.
   */
  static void RegisterUeInfo (Ptr<UeInfo> ueInfo);

  uint64_t                    m_imsi;               //!< UE IMSI
  Ipv4Address                 m_ueAddr;             //!< UE IP address
  Ipv4Address                 m_enbAddr;            //!< eNB IP address
  std::map<uint8_t, uint32_t> m_teidByBearerIdMap;  //!< List of bearers by ID

  /** Map saving UE IMSI / UE information. */
  typedef std::map<uint64_t, Ptr<UeInfo> > ImsiUeInfoMap_t;
  static ImsiUeInfoMap_t m_ueInfoByImsiMap; //!< Global UE info map.
};

};  // namespace ns3
#endif // UE_INFO_H

