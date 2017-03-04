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

namespace ns3 {

class EpcController;

/**
 * This class represents the SDRAN cloud at the SDMN architecture.
 */
class SdranCloud : public Object
{
  friend class EpcNetwork;

public:
  SdranCloud ();           //!< Default constructor.
  virtual ~SdranCloud ();  //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \return The SDRAN Cloud ID. */
  uint32_t GetId (void) const;

  /**
   * Get the container of eNBs nodes created by this SDRAN cloud.
   * \return The container of eNB nodes.
   */
  NodeContainer GetEnbNodes (void) const;

  /**
   * Get the S-GW node associated to this SDRAN cloud.
   * \return The S-GW node pointer.
   */
  Ptr<Node> GetSgwNode ();

  /**
   * Get the S-GW OpenFlow switch device.
   * \return The OpenFlow switch device pointer.
   */
  Ptr<OFSwitch13Device> GetSgwSwitchDevice ();

  /**
   * Set the pointer to the commom LTE MME element.
   * \param mme The MME element.
   */
  void SetMme (Ptr<EpcMme> mme);

  // Implementing some of the EpcHelper methods that are redirected to here
  // from the EpcNetwork class.
  virtual void AddEnb (Ptr<Node> enbNode, Ptr<NetDevice> lteEnbNetDevice,
                       uint16_t cellId);
  virtual void AddX2Interface (Ptr<Node> enbNode1, Ptr<Node> enbNode2);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

  /**
   * Get the IP address for a given device.
   * \param device The network device.
   * \return The IP address assigned to this device.
   */
  Ipv4Address GetAddressForDevice (Ptr<NetDevice> device);

  /**
   * Get a pointer to the commom LTE MME element.
   * \return The MME element.
   */
  Ptr<EpcMme> GetMme ();

  /**
   * Get the SDRAN cloud pointer from the global map for this eNB node.
   * \param enb The eNB node pointer.
   * \return The SDRAN cloud pointer.
   */
  static Ptr<SdranCloud> GetPointer (Ptr<Node> enb);

private:
  /**
   * Register the SDRAN cloud into global map for further usage.
   * \param sdran The SDRAN cloud pointer.
   */
  static void RegisterSdranCloud (Ptr<SdranCloud> sdran);

  uint32_t                m_sdranId;        //!< SDRAN cloud id.
  uint32_t                m_nSites;         //!< Number of cell sites.
  uint32_t                m_nEnbs;          //!< Number of eNBs (3 * m_nSites).
  NodeContainer           m_enbNodes;       //!< eNB nodes.
  Ptr<Node>               m_sgwNode;        //!< S-GW node.
  NetDeviceContainer      m_s1Devices;      //!< S1-U devices.
  Ipv4Address             m_s1uNetworkAddr; //!< S1-U network address.
  Ipv4AddressHelper       m_s1uAddrHelper;  //!< S1-U address helper.
  Ipv4Address             m_sgwS1uAddr;     //!< S1-U S-GW IP address.
  Ptr<EpcController>      m_epcCtrlApp;     //!< EPC controller app.
  Ptr<EpcMme>             m_mme;            //!< LTE MME element.

  // Helper and connection attributes
  CsmaHelper              m_csmaHelper;     //!< Connection helper.
  DataRate                m_linkRate;       //!< Link data rate.
  Time                    m_linkDelay;      //!< Link delay.
  uint16_t                m_linkMtu;        //!< Link MTU.

  /** Map saving node / SDRAN pointer. */
  typedef std::map<Ptr<Node>, Ptr<SdranCloud> > NodeSdranMap_t;

  static uint32_t         m_enbCounter;     //!< Global eNB counter.
  static uint32_t         m_sdranCounter;   //!< Global SDRAN cloud counter.
  static NodeSdranMap_t   m_enbSdranMap;    //!< Global SDRAN by eNB node map.
};

} // namespace ns3
#endif /* SDRAN_CLOUD_H */
