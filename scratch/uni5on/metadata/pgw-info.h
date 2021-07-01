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

class SliceController;

/**
 * \ingroup uni5onMeta
 * Metadata associated to a logical P-GW.
 */
class PgwInfo : public Object
{
  friend class SliceController;
  friend class SliceNetwork;

public:
  /**
   * Complete constructor.
   * \param pgwId The P-GW ID.
   * \param nTfts The number of TFT switches.
   * \param sgiPortNo The port number for SGi iface at the P-GW main switch.
   * \param infraSwIdx The OpenFlow transport switch index.
   * \param sliceCtrl The slice controller application.
   */
  PgwInfo (uint32_t pgwId, uint16_t nTfts, uint32_t sgiPortNo,
           uint16_t infraSwIdx, Ptr<SliceController> ctrlApp);
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
  uint16_t              GetInfraSwIdx   (void) const;
  uint32_t              GetPgwId        (void) const;
  Ptr<SliceController>  GetSliceCtrl    (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW TFT adaptive mechanism.
   * \return The requested information.
   */
  //\{
  uint16_t              GetCurLevel     (void) const;
  uint16_t              GetCurTfts      (void) const;
  uint16_t              GetMaxLevel     (void) const;
  uint16_t              GetMaxTfts      (void) const;
  //\}

  /**
   * \name Private member accessors for P-GW switch datapath information.
   * \param idx The internal switch index.
   * \param tableId The flow table ID.
   * \return The requested information.
   */
  //\{
  uint32_t      GetFlowTableCur         (uint16_t idx, uint8_t tableId) const;
  uint32_t      GetFlowTableMax         (uint16_t idx, uint8_t tableId) const;
  double        GetFlowTableUse         (uint16_t idx, uint8_t tableId) const;
  DataRate      GetEwmaCpuCur           (uint16_t idx) const;
  double        GetEwmaCpuUse           (uint16_t idx) const;
  DataRate      GetCpuMax               (uint16_t idx) const;
  //\}

  /**
   * \name Private member accessors for P-GW main switch information.
   * \param idx The internal TFT switch index.
   * \return The requested information.
   */
  //\{
  uint64_t      GetMainDpId             (void) const;
  uint32_t      GetMainInfraSwS5PortNo  (void) const;
  Ipv4Address   GetMainS5Addr           (void) const;
  uint32_t      GetMainS5PortNo         (void) const;
  uint32_t      GetMainSgiPortNo        (void) const;
  uint32_t      GetMainToTftPortNo      (uint16_t idx) const;
  //\}

  /**
   * \name Private member accessors for P-GW TFT switches information.
   * \param idx The internal TFT switch index.
   * \return The requested information.
   */
  //\{
  uint64_t      GetTftDpId              (uint16_t idx) const;
  uint32_t      GetTftInfraSwS5PortNo   (uint16_t idx) const;
  Ipv4Address   GetTftS5Addr            (uint16_t idx) const;
  uint32_t      GetTftS5PortNo          (uint16_t idx) const;
  uint32_t      GetTftToMainPortNo      (uint16_t idx) const;
  //\}

  /**
   * \name Private member accessors for P-GW TFT active switches aggregated
   *       datapath information.
   * \param tableId The flow table ID. The default value of 0 can be used as
   *        P-GW TFT switches have only one pipeline table.
   * \return The requested information.
   * \internal These methods iterate only over active TFT switches for
   *           collecting statistics.
   */
  //\{
  uint32_t      GetTftAvgFlowTableCur   (uint8_t tableId = 0) const;
  uint32_t      GetTftAvgFlowTableMax   (uint8_t tableId = 0) const;
  double        GetTftAvgFlowTableUse   (uint8_t tableId = 0) const;
  DataRate      GetTftAvgEwmaCpuCur     (void) const;
  double        GetTftAvgEwmaCpuUse     (void) const;
  DataRate      GetTftAvgCpuMax         (void) const;
  uint32_t      GetTftMaxFlowTableCur   (uint8_t tableId = 0) const;
  uint32_t      GetTftMaxFlowTableMax   (uint8_t tableId = 0) const;
  double        GetTftMaxFlowTableUse   (uint8_t tableId = 0) const;
  DataRate      GetTftMaxEwmaCpuCur     (void) const;
  double        GetTftMaxEwmaCpuUse     (void) const;
  DataRate      GetTftMaxCpuMax         (void) const;
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
   * Get the OpenFlow switch stats calculator.
   * \param idx The internal switch index.
   * \return The OpenFlow switch stats.
   */
  Ptr<OFSwitch13StatsCalculator> GetStats (uint16_t idx) const;

  /**
   * Save the metadata associated to a single P-GW OpenFlow switch attached to
   * the OpenFlow transport network.
   * \attention Invoke this method first for the P-GW MAIN switch, then for the
   *            P-GW TFT switches.
   * \param device The OpenFlow switch device.
   * \param s5Add The IP address assigned to the S5 network device on the P-GW
   *        switch.
   * \param s5PortNo The port number assigned to the S5 logical port on the
   *        P-GW switch that is connected to the S5 network device.
   * \param infraSwS5PortNo The port number assigned to the S5 physical port on
   *        the OpenFlow transport switch that is connected to the S5 network
   *        device on the P-GW switch.
   * \param mainToTftPortNo The port number assigned to the physical port on
   *        the main switch that is connected to this P-GW switch (when
   *        invoking this function for the P-GW main switch, this parameter can
   *        be left to the default value as it does not make sense).
   * \param tftToMainPortNo The port number assigned to the physical port on
   *        this switch that is connected to the P-GW main switch (when
   *        invoking this function for the P-GW main switch, this parameter can
   *        be left to the default value as it does not make sense).
   */
  void SaveSwitchInfo (Ptr<OFSwitch13Device> device, Ipv4Address s5Addr,
                       uint32_t s5PortNo, uint32_t infraSwS5PortNo,
                       uint32_t mainToTftPortNo = 0,
                       uint32_t tftToMainPortNo = 0);

  /**
   * Update the current TFT adaptive mechanism level.
   * \param value The value to set.
   */
  void SetCurLevel (uint16_t value);

  // P-GW metadata.
  OFSwitch13DeviceContainer m_devices;            //!< OpenFlow switch devices.
  uint16_t                  m_infraSwIdx;         //!< Transport switch index.
  std::vector<uint32_t>     m_infraSwS5PortNos;   //!< Back switch S5 port nos.
  std::vector<uint32_t>     m_mainToTftPortNos;   //!< Main port nos to TFTs.
  uint32_t                  m_pgwId;              //!< P-GW ID.
  std::vector<Ipv4Address>  m_s5Addrs;            //!< S5 dev IP addresses.
  std::vector<uint32_t>     m_s5PortNos;          //!< S5 port numbers.
  uint32_t                  m_sgiPortNo;          //!< SGi port number.
  Ptr<SliceController>      m_sliceCtrl;          //!< Slice controller.
  std::vector<uint32_t>     m_tftToMainPortNos;   //!< TFTs port nos to main.

  // TFT adaptive mechanism.
  uint16_t                  m_tftSwitches;        //!< Total # of TFT switches.
  uint8_t                   m_tftLevel;           //!< Current adaptive level.
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
