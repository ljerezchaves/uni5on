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

#ifndef PGW_INFO_H
#define PGW_INFO_H

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../svelte-common.h"

namespace ns3 {

/**
 * \ingroup svelteMeta
 * Metadata associated to a logical P-GW.
 */
class PgwInfo : public Object
{
  friend class SliceController;
  friend class SliceNetwork;

public:
  /**
   * Complete constructor.
   * \param PgwId The ID for this P-GW.
   */
  PgwInfo (uint64_t pgwId);
  virtual ~PgwInfo (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint64_t GetPgwId (void) const;
  SliceId GetSliceId (void) const;
  uint16_t GetInfraSwIdx (void) const;
  uint16_t GetNumTfts (void) const;
  uint32_t GetMainSgiPortNo (void) const;
  uint64_t GetMainDpId (void) const;
  Ipv4Address GetMainS5Addr (void) const;
  uint32_t GetMainS5PortNo (void) const;
  uint32_t GetMainInfraSwS5PortNo (void) const;
  uint32_t GetMainToTftPortNo (uint16_t idx) const;
  uint64_t GetTftDpId (uint16_t idx) const;
  Ipv4Address GetTftS5Addr (uint16_t idx) const;
  uint32_t GetTftS5PortNo (uint16_t idx) const;
  uint32_t GetTftInfraSwS5PortNo (uint16_t idx) const;
  uint32_t GetTftFlowTableSize (void) const;
  DataRate GetTftPipelineCapacity (void) const;
  double GetTftMaxEntries (void) const;
  double GetTftMaxLoad (void) const;
  double GetTftSumEntries (void) const;
  double GetTftSumLoad (void) const;
  double GetTftMaxTableUsage (void) const;
  double GetTftMaxLoadUsage (void) const;
  //\}

  /**
   * Get the P-GW information from the global map for a specific ID.
   * \param pgwId The ID for this P-GW.
   * \return The P-GW information for this ID.
   */
  static Ptr<PgwInfo> GetPointer (uint64_t pgwId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /** \name Private member accessors. */
  //\{
  void SetSliceId (SliceId value);
  void SetInfraSwIdx (uint16_t value);
  void SetNumTfts (uint16_t value);
  void SetMainSgiPortNo (uint32_t value);
  //\}

  /**
   * Update P-GW TFT switch statistics.
   */
  void UpdateTftStats (void);

  /**
   * Save the metadata associated to a single P-GW OpenFlow switch attached to
   * the OpenFlow backhaul network.
   * \attention Invoke this method first for the P-GW MAIN switch, then for the
   *            P-GW TFT switches.
   * \param dpId The OpenFlow switch datapath ID.
   * \param s5Add The IP address assigned to the S5 network device on the P-GW
   *        switch.
   * \param s5PortNo The port number assigned to the S5 logical port on the
   *        P-GW switch that is connected to the S5 network device.
   * \param infraSwS5PortNo The port number assigned to the S5 physical port on
   *        the OpenFlow backhaul switch that is connected to the S5 network
   *        device on the P-GW switch.
   * \param mainToTftPortNo The port number assigned to the physical port on
   *        the main switch that is connected to this P-GW switch (when
   *        invoking this function for the P-GW main switch, this parameter can
   *        be left to the default value as it does not make sense).
   */
  void SaveSwitchInfo (uint64_t dpId, Ipv4Address s5Addr, uint32_t s5PortNo,
                       uint32_t infraSwS5PortNo, uint32_t mainToTftPortNo = 0);

  /**
   * Register the P-GW information in global map for further usage.
   * \param pgwInfo The P-GW information to save.
   */
  static void RegisterPgwInfo (Ptr<PgwInfo> pgwInfo);

  /** Vector of OFSwitch13StatsCalculator */
  typedef std::vector<Ptr<OFSwitch13StatsCalculator> > StatsVector_t;

  // P-GW metadata.
  uint64_t                  m_pgwId;              //!< P-GW ID (P-GW main dpId).
  SliceId                   m_sliceId;            //!< LTE logical slice ID.
  uint16_t                  m_infraSwIdx;         //!< Backhaul switch index.
  uint32_t                  m_sgiPortNo;          //!< SGi port number.
  uint16_t                  m_nTfts;              //!< Number of TFT switches.
  std::vector<uint64_t>     m_dpIds;              //!< Switch datapath IDs.
  std::vector<Ipv4Address>  m_s5Addrs;            //!< S5 dev IP addresses.
  std::vector<uint32_t>     m_s5PortNos;          //!< S5 port numbers.
  std::vector<uint32_t>     m_infraSwS5PortNos;   //!< Back switch S5 port nos.
  std::vector<uint32_t>     m_mainToTftPortNos;   //!< Main port nos to TFTs.
  DataRate                  m_tftPipeCapacity;    //!< Min TFT pipe capacity.
  uint32_t                  m_tftFlowTableSize;   //!< Min TFT Flow table size.
  StatsVector_t             m_switchStats;        //!< Switch stats calculator.

  // TFT switch statistics.
  double                    m_tftMaxEntries;      //!< Max number flow entries.
  double                    m_tftMaxLoad;         //!< Max pipeline load.
  double                    m_tftSumEntries;      //!< Sum number flow entries.
  double                    m_tftSumLoad;         //!< Sum pipeline load.


  /** Map saving P-GW ID / P-GW information. */
  typedef std::map<uint64_t, Ptr<PgwInfo> > PgwIdPgwInfo_t;
  static PgwIdPgwInfo_t  m_pgwInfoByPgwId;       //!< Global P-GW info map.
};

} // namespace ns3
#endif // PGW_INFO_H
