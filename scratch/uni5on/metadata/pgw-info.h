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
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef PGW_INFO_H
#define PGW_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../uni5on-common.h"

namespace ns3 {

// Order for DL and UL switches in containers.
#define PGW_DL_IDX 0
#define PGW_UL_IDX 1

/**
 * \ingroup uni5onMeta
 * Metadata associated to a logical P-GW.
 */
class PgwInfo : public Object
{
  friend class SliceNetwork;

public:
  /**
   * Complete constructor.
   * \param pgwId The P-GW ID.
   * \param nTfts The number of TFT switches.
   */
  PgwInfo (uint32_t pgwId, uint16_t nTfts);
  virtual ~PgwInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for P-GW information.
   * \return The requested information.
   */
  //\{
  uint16_t      GetInfraSwIdx         (void) const;
  uint32_t      GetInfraSwS5PortNo    (void) const;
  uint16_t      GetNumTfts            (void) const;
  uint32_t      GetPgwId              (void) const;
  Ipv4Address   GetS5Addr             (void) const;
  Ipv4Address   GetSgiAddr            (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW UL/DL switch information.
   * \param idx The internal TFT switch index.
   * \return The requested information.
   */
  //\{
  uint64_t      GetDlDpId             (void) const;
  uint64_t      GetUlDpId             (void) const;
  uint32_t      GetDlSgiPortNo        (void) const;
  uint32_t      GetUlS5PortNo         (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW internal port numbers.
   * \param idx The internal TFT switch index.
   * \return The requested information.
   */
  //\{
  uint32_t      GetDlToTftPortNo      (uint16_t idx) const;
  uint32_t      GetUlToTftPortNo      (uint16_t idx) const;
  uint32_t      GetTftToDlPortNo      (uint16_t idx) const;
  uint32_t      GetTftToUlPortNo      (uint16_t idx) const;
  //\}

  /**
   * \name Private member accessors for P-GW TFT switch information.
   * \param idx The internal switch index.
   * \param tableId The flow table ID.
   * \return The requested information.
   */
  //\{
  uint32_t      GetTftFlowTableCur    (uint16_t idx, uint8_t tableId) const;
  uint32_t      GetTftFlowTableMax    (uint16_t idx, uint8_t tableId) const;
  double        GetTftFlowTableUse    (uint16_t idx, uint8_t tableId) const;
  DataRate      GetTftEwmaCpuCur      (uint16_t idx) const;
  double        GetTftEwmaCpuUse      (uint16_t idx) const;
  DataRate      GetTftCpuMax          (uint16_t idx) const;
  uint64_t      GetTftDpId            (uint16_t idx) const;
  //\}

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * Get the empty for the print operator <<.
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
   * Save the metadata associated to the P-GW TFT OpenFlow switch.
   * \param device The OpenFlow switch device.
   * \param tftToDlPortNo The port number assigned to the physical port on
   *        the TFT switch that is connected to the P-GW DL switch.
   * \param tftToUlPortNo The port number assigned to the physical port on
   *        the TFT switch that is connected to the P-GW UL switch.
   * \param dlToTftPortNo The port number assigned to the physical port on
   *        the DL switch that is connected to the P-GW TFT switch.
   * \param ulToTftPortNo The port number assigned to the physical port on
   *        the UL switch that is connected to the P-GW TFT switch.
   */
  void SaveTftInfo (
    Ptr<OFSwitch13Device> device, uint32_t tftToDlPortNo,
    uint32_t tftToUlPortNo, uint32_t dlToTftPortNo, uint32_t ulToTftPortNo);

  /**
   * Save the metadata associated to the P-GW UL/DL OpenFlow switches.
   * \param dlDevice The OpenFlow DL switch device.
   * \param ulDevice The OpenFlow UL switch device.
   * \param sgiPortNo The port number assigned to the SGi logical port on the
   *        P-GW DL switch that is connected to the SGi network device.
   * \param sgiAddr The IP address assigned to the SGi network device on the
   *        P-GW DL switch.
   * \param s5PortNo The port number assigned to the S5 logical port on the
   *        P-GW UL switch that is connected to the S5 network device.
   * \param s5Addr The IP address assigned to the S5 network device on the
   *        P-GW UL switch.
   * \param infraSwIdx The OpenFlow transport switch index.
   * \param infraSwS5PortNo The port number assigned to the S5 physical port on
   *        the OpenFlow transport switch that is connected to the S5 network
   *        device on the P-GW UL switch.
   */
  void SaveUlDlInfo (
    Ptr<OFSwitch13Device> dlDevice, Ptr<OFSwitch13Device> ulDevice,
    uint32_t sgiPortNo, Ipv4Address sgiAddr,
    uint32_t s5PortNo, Ipv4Address s5Addr,
    uint16_t infraSwIdx, uint32_t infraSwS5PortNo);

  // P-GW metadata.
  uint32_t                  m_pgwId;              //!< P-GW ID.
  uint16_t                  m_tftNum;             //!< Number of TFT switches.
  OFSwitch13DeviceContainer m_tftDevices;         //!< OpenFlow TTT devices.
  OFSwitch13DeviceContainer m_ulDlDevices;        //!< OpenFlow UL/DL devices.
  uint16_t                  m_infraSwIdx;         //!< Transport switch index.
  uint16_t                  m_infraSwS5PortNo;    //!< Transport S5 port no.
  Ipv4Address               m_s5Addr;             //!< P-GW S5 IP address.
  uint32_t                  m_s5PortNo;           //!< P-GW S5 port no.
  Ipv4Address               m_sgiAddr;            //!< P-GW SGi IP address.
  uint32_t                  m_sgiPortNo;          //!< P-GW SGi port no.
  std::vector<uint32_t>     m_dlToTftPortNos;     //!< DL port nos to TFTs.
  std::vector<uint32_t>     m_ulToTftPortNos;     //!< UL port nos to TFTs.
  std::vector<uint32_t>     m_tftToDlPortNos;     //!< TFT port nos to DL.
  std::vector<uint32_t>     m_tftToUlPortNos;     //!< TFT port nos to UL.
};

/**
 * Print the P-GW metadata on an output stream.
 * \param os The output stream.
 * \param pgwInfo The PgwInfo object.
 * \returns The output stream.
 * \internal Keep this method consistent with the PgwInfo::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const PgwInfo &pgwInfo);

} // namespace ns3
#endif // PGW_INFO_H
