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

#ifndef SDRAN_CLOUD_H
#define SDRAN_CLOUD_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/lte-module.h>
#include "sdran-controller.h"

namespace ns3 {

class EpcController;

/**
 * This class represents the SDRAN cloud at the SDMN architecture.
 */
class SdranCloud : public Object
{
public:
  SdranCloud ();           //!< Default constructor.
  virtual ~SdranCloud ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  uint32_t GetId (void) const;
  uint32_t GetNSites (void) const;
  uint32_t GetNEnbs (void) const;
  Ptr<Node> GetSgwNode (void) const;
  NodeContainer GetEnbNodes (void) const;
  Ptr<SdranController> GetSdranCtrlApp (void) const;
  Ptr<OFSwitch13Device> GetSgwSwitchDevice (void) const;
  //\}

  // Implementing some of the EpcHelper methods that are redirected to here
  // from the EpcNetwork class.
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice,
                       uint16_t cellId);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);

  /**
   * Enable PCAP traces on the internal SDRAN OpenFlow network (user and
   * control planes), and on LTE EPC devices of S1-U interface.
   * \param prefix Filename prefix to use for pcap files.
   * \param promiscuous If true, enable promisc trace.
   */
  void EnablePcap (std::string prefix, bool promiscuous = false);

  /**
   * Get the SDRAN cloud pointer from the global map for this eNB node.
   * \param enb The eNB node pointer.
   * \return The SDRAN cloud pointer.
   */
  static Ptr<SdranCloud> GetPointer (Ptr<Node> enb);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /**
   * Register the SDRAN cloud into global map for further usage.
   * \param sdran The SDRAN cloud pointer.
   */
  static void RegisterSdranCloud (Ptr<SdranCloud> sdran);

  /**
   * Initilize the S1-U address static helper. This function is called by
   * object constructor, but the helper will be initializaed only once.
   */
  static void S1uAddrHelperInitialize ();

  uint32_t                      m_sdranId;        //!< SDRAN cloud id.
  uint32_t                      m_nSites;         //!< Number of cell sites.
  uint32_t                      m_nEnbs;          //!< Number of eNBs.
  Ptr<Node>                     m_sgwNode;        //!< S-GW user-plane node.
  NodeContainer                 m_enbNodes;       //!< eNB nodes.

  // OpenFlow switch helper.
  Ptr<OFSwitch13InternalHelper> m_ofSwitchHelper; //!< Switch helper.

  // Helper and attributes for S1-U interface.
  CsmaHelper                    m_csmaHelper;     //!< Connection helper.
  DataRate                      m_linkRate;       //!< Link data rate.
  Time                          m_linkDelay;      //!< Link delay.
  uint16_t                      m_linkMtu;        //!< Link MTU.

  // EPC user-plane devices.
  NetDeviceContainer            m_s1Devices;      //!< S1-U devices.

  // SDRAN controller.
  Ptr<SdranController>          m_sdranCtrlApp;   //!< SDRAN controller app.
  Ptr<Node>                     m_sdranCtrlNode;  //!< SDRAN controller node

  /** Map saving node / SDRAN pointer. */
  typedef std::map<Ptr<Node>, Ptr<SdranCloud> > NodeSdranMap_t;

  static Ipv4AddressHelper m_s1uAddrHelper; //!< Global S1-U address helper.
  static uint32_t          m_enbCounter;    //!< Global eNB counter.
  static uint32_t          m_sdranCounter;  //!< Global SDRAN cloud counter.
  static NodeSdranMap_t    m_enbSdranMap;   //!< Global SDRAN by eNB node map.
};

} // namespace ns3
#endif /* SDRAN_CLOUD_H */
