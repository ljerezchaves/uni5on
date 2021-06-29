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

#ifndef SCENARIO_EPC_HELPER_H
#define SCENARIO_EPC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../statistics/admission-stats-calculator.h"
#include "../statistics/backhaul-stats-calculator.h"
#include "../statistics/lte-rrc-stats-calculator.h"
#include "../statistics/pgw-tft-stats-calculator.h"
#include "../statistics/traffic-stats-calculator.h"
#include "../uni5on-common.h"

namespace ns3 {

class RadioNetwork;
class RingNetwork;
class SliceController;
class SliceNetwork;
class Uni5onMme;
class TrafficHelper;

/**
 * \ingroup uni5on
 * This helper creates and configures the infrastructure and logical networks
 * for UNI5ON architecture simulations.
 */
class ScenarioHelper : public EpcHelper
{
public:
  /** The bitmap for PCAP configuration. */
  enum PcapConfig
  {
    PCSLCOFP  = (1U << 0),  //!< Slice OpenFlow control channels.
    PCSLCPGW  = (1U << 1),  //!< Slice P-GW internal interfaces.
    PCSLCSGI  = (1U << 2),  //!< Slice SGi interface (Internet).
    PCBACKOFP = (1U << 3),  //!< Backhaul OpenFlow control channels.
    PCBACKEPC = (1U << 4),  //!< Backhaul EPC interfaces.
    PCBACKSWT = (1U << 5),  //!< Backhaul switches interfaces.
    PCNOTUSED = (1U << 6),  //!< Flag not being used yet.
    PCPROMISC = (1U << 7),  //!< Enable promiscuous mode.
  };

  ScenarioHelper ();          //!< Default constructor.
  virtual ~ScenarioHelper (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Configure PCAP traces.
   * \param prefix Filename prefix to use for PCAP files.
   * \param config The PCAP configuration bitmap.
   *
   * \internal
   * We are using the following PCAP configuration bitmap:
   * \verbatim
   * 8 bits length: 0 0 0 0 0 0 0 0
   *                A B C D E F G H
   *
   * A: Enable promiscuous mode
   * B: Unused bit
   * C: Enable PCAP on OpenFlow switch ports at backhaul
   * D: Enable PCAP on EPC interfaces connected to the backhaul
   * E: Enable PCAP on OpenFlow control channel at backhaul
   * F: Enable PCAP on SGi intefaces connected to the Internet
   * G: Enable PCAP on P-GW internal interfaces at logical slices
   * H: Enable PCAP on OpenFlow control channels at logical slices
   * \endverbatim
   */
  void ConfigurePcap (std::string prefix, uint8_t config);

  /**
   * Check the PCAP configuration bitmap for the following flag.
   * \param config The PCAP configuration bitmap.
   * \param flag PCAP configuration flag.
   * \return True if the flag is set in PCAP config bitmap, false otherwise.
   */
  bool HasPcapFlag (uint8_t config, PcapConfig flag) const;

  /**
   * Print the LTE radio environment map.
   * \param enable If true, print the LTE REM.
   */
  void PrintLteRem (bool enable);

  // Inherited from EpcHelper.
  uint8_t ActivateEpsBearer (Ptr<NetDevice> ueLteDevice, uint64_t imsi,
                             Ptr<EpcTft> tft, EpsBearer bearer);
  void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice,
               uint16_t cellId);
  void AddUe (Ptr<NetDevice> ueLteDevice, uint64_t imsi);
  void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);
  Ptr<Node> GetPgwNode ();
  Ipv4InterfaceContainer AssignUeIpv4Address (NetDeviceContainer ueDevices);
  Ipv6InterfaceContainer AssignUeIpv6Address (NetDeviceContainer ueDevices);
  Ipv4Address GetUeDefaultGatewayAddress ();
  Ipv6Address GetUeDefaultGatewayAddress6 ();
  // Inherited from EpcHelper.

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

private:
  /**
   * Check the object factories for valid types.
   * \param controller The SliceController object factory.
   * \param network The SliceNetwork object factory.
   * \param traffic The TrafficHelper object factory.
   * \return true when all types are ok, false otherwise.
   */
  bool AreFactoriesOk (ObjectFactory &controller, ObjectFactory &network,
                       ObjectFactory &traffic) const;

  Ptr<RingNetwork>          m_backhaul;         //!< The backhaul network.
  Ptr<RadioNetwork>         m_radio;            //!< The LTE RAN network.
  Ptr<Uni5onMme>            m_mme;              //!< The MME entity.

  // MBB network slice.
  ObjectFactory             m_mbbControllerFac; //!< MBB controller factory.
  ObjectFactory             m_mbbNetworkFac;    //!< MBB network factory.
  ObjectFactory             m_mbbTrafficFac;    //!< MBB traffic factory.
  Ptr<SliceController>      m_mbbController;    //!< MBB slice controller.
  Ptr<SliceNetwork>         m_mbbNetwork;       //!< MBB slice network.
  Ptr<TrafficHelper>        m_mbbTraffic;       //!< MBB slice traffic.

  // MTC network slice.
  ObjectFactory             m_mtcControllerFac; //!< MTC controller factory.
  ObjectFactory             m_mtcNetworkFac;    //!< MTC network factory.
  ObjectFactory             m_mtcTrafficFac;    //!< MTC traffic factory.
  Ptr<SliceController>      m_mtcController;    //!< MTC slice controller.
  Ptr<SliceNetwork>         m_mtcNetwork;       //!< MTC slice network.
  Ptr<TrafficHelper>        m_mtcTraffic;       //!< MTC slice traffic.

  // TMP network slice.
  ObjectFactory             m_tmpControllerFac; //!< TMP controller factory.
  ObjectFactory             m_tmpNetworkFac;    //!< TMP network factory.
  ObjectFactory             m_tmpTrafficFac;    //!< TMP traffic factory.
  Ptr<SliceController>      m_tmpController;    //!< TMP slice controller.
  Ptr<SliceNetwork>         m_tmpNetwork;       //!< TMP slice network.
  Ptr<TrafficHelper>        m_tmpTraffic;       //!< TMP slice traffic.

  // Statistic calculators.
  Ptr<AdmissionStatsCalculator>   m_admissionStats; //!< Admission stats.
  Ptr<BackhaulStatsCalculator>    m_backhaulStats;  //!< Backhaul stats.
  Ptr<LteRrcStatsCalculator>      m_lteRrcStats;    //!< LTE RRC stats.
  Ptr<PgwTftStatsCalculator>      m_pgwTftStats;    //!< P-GW TFT stats.
  Ptr<TrafficStatsCalculator>     m_trafficStats;   //!< Traffic stats.
};

} // namespace ns3
#endif  // SCENARIO_EPC_HELPER_H
