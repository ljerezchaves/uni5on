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

#ifndef SVELTE_EPC_HELPER_H
#define SVELTE_EPC_HELPER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../statistics/admission-stats-calculator.h"
#include "../statistics/backhaul-stats-calculator.h"
#include "../statistics/pgw-tft-stats-calculator.h"
#include "../statistics/traffic-stats-calculator.h"
#include "../statistics/ue-rrc-stats-calculator.h"
#include "../svelte-common.h"

namespace ns3 {

class RadioNetwork;
class RingNetwork;
class SliceController;
class SliceNetwork;
class SvelteMme;
class TrafficHelper;

/**
 * \ingroup svelte
 * This class creates and configures the SVELTE architecture, including the
 * shared infrastructure and logical networks.
 */
class SvelteHelper : public EpcHelper
{
public:
  SvelteHelper ();          //!< Default constructor.
  virtual ~SvelteHelper (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces on the SVELTE infrastructure.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Print the LTE radio environment map.
   */
  void PrintLteRem (void);

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
   * Check the object factories for proper types.
   * \param controller The SliceController object factory.
   * \param network The SliceNetwork object factory.
   * \param traffic The TrafficHelper object factory.
   * \return true when all types are ok, false otherwise.
   */
  bool AreFactoriesOk (ObjectFactory &controller, ObjectFactory &network,
                       ObjectFactory &traffic) const;

  /**
   * Get the backahul switch index at which the given eNB should be connected.
   * \param cellId The eNB cell ID.
   * \return The backhaul switch index.
   */
  uint16_t GetEnbInfraSwIdx (uint16_t cellId);

  Ptr<RingNetwork>          m_backhaul;         //!< The backhaul network.
  Ptr<RadioNetwork>         m_radio;            //!< The LTE RAN network.
  Ptr<SvelteMme>            m_mme;              //!< SVELTE MME entity.

  // MTC network slice.
  ObjectFactory             m_mtcControllerFac; //!< MTC controller factory.
  ObjectFactory             m_mtcNetworkFac;    //!< MTC network factory.
  ObjectFactory             m_mtcTrafficFac;    //!< MTC traffic factory.
  Ptr<SliceController>      m_mtcController;    //!< MTC slice controller.
  Ptr<SliceNetwork>         m_mtcNetwork;       //!< MTC slice network.
  Ptr<TrafficHelper>        m_mtcTraffic;       //!< MTC slice traffic.

  // HTC network slice.
  ObjectFactory             m_htcControllerFac; //!< HTC controller factory.
  ObjectFactory             m_htcNetworkFac;    //!< HTC network factory.
  ObjectFactory             m_htcTrafficFac;    //!< HTC traffic factory.
  Ptr<SliceController>      m_htcController;    //!< HTC slice controller.
  Ptr<SliceNetwork>         m_htcNetwork;       //!< HTC slice network.
  Ptr<TrafficHelper>        m_htcTraffic;       //!< HTC slice traffic.

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
  Ptr<PgwTftStatsCalculator>      m_pgwTftStats;    //!< P-GW TFT stats.
  Ptr<TrafficStatsCalculator>     m_trafficStats;   //!< Traffic stats.
  Ptr<UeRrcStatsCalculator>       m_ueRrcStats;     //!< UE RRC stats.
};

} // namespace ns3
#endif  // SVELTE_EPC_HELPER_H
