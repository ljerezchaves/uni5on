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
 * Metadata associated to a UE.
 */
class UeInfo : public Object
{
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
  Ipv4Address GetUeAddress (void) const;
  Ipv4Address GetEnbAddress (void) const;
  uint64_t GetMmeUeS1Id (void) const;
  uint64_t GetEnbUeS1Id (void) const;
  uint16_t GetCellId (void) const;
  uint16_t GetBearerCounter (void) const;

  void SetUeAddress (Ipv4Address value);
  void SetEnbAddress (Ipv4Address value);
  void SetMmeUeS1Id (uint64_t value);
  void SetEnbUeS1Id (uint64_t value);
  void SetCellId (uint16_t value);
  //\}

  /**
   * Get the iterator for the begin of the bearer list.
   * \return The begin iterator.
   */
  std::list<BearerInfo>::const_iterator GetBearerListBegin (void) const;

  /**
    * Get the iterator for the end of the bearer list.
    * \return The end iterator.
    */
  std::list<BearerInfo>::const_iterator GetBearerListEnd (void) const;

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
   * Get the UE information from the global map for a specific IMSI.
   * \param imsi The IMSI identifier for this UE.
   * \return The UE information for this IMSI.
   */
  static Ptr<UeInfo> GetPointer (uint64_t imsi);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Register the UE information in global map for further usage.
   * \param ueInfo The UE information to save.
   */
  static void RegisterUeInfo (Ptr<UeInfo> ueInfo);

  uint64_t               m_imsi;                 //!< UE IMSI.
  Ipv4Address            m_ueAddr;               //!< UE IP address.
  Ipv4Address            m_enbAddr;              //!< eNB IP address.
  uint64_t               m_mmeUeS1Id;            //!< ID for S1-AP at MME.
  uint16_t               m_enbUeS1Id;            //!< ID for S1-AP at eNB.
  uint16_t               m_cellId;               //!< UE cell ID.
  uint16_t               m_bearerCounter;        //!< Number of bearers.
  std::list<BearerInfo>  m_bearersList;          //!< Bearer contexts.

  /** Map saving UE IMSI / UE information. */
  typedef std::map<uint64_t, Ptr<UeInfo> > ImsiUeInfoMap_t;
  static ImsiUeInfoMap_t m_ueInfoByImsiMap;      //!< Global UE info map.
};

};  // namespace ns3
#endif // UE_INFO_H

