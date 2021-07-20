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

#ifndef ENB_APPLICATION_H
#define ENB_APPLICATION_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/virtual-net-device-module.h>
#include <ns3/lte-module.h>
#include <ns3/csma-module.h>

namespace ns3 {

/**
 * \ingroup uni5onLogical
 * Custom eNB application that can handle connection with multiple S-GWs and
 * traffic aggregation within EPC bearers It also attachs/removes the GtpuTag
 * tag on packets entering.leaving the transport network over S1-U interface.
 */
class EnbApplication : public EpcEnbApplication
{
public:
  /**
   * Complete constructor.
   * \param lteSocket The socket to be used to send/receive IPv4 packets
   *                  to/from the LTE radio interface.
   * \param lteSocket6 The socket to be used to send/receive IPv6 packets
   *                   to/from the LTE radio interface.
   * \param s1uPortDev The local switch port to be used to send/receive packets
   *                   to/from the S1-U interface connected with the SGW.
   * \param enbS1uAddress The IPv4 address of the S1-U interface of this eNB.
   * \param cellId The identifier of the eNB.
   */
  EnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> lteSocket6,
                  Ptr<VirtualNetDevice> s1uPortDev, Ipv4Address enbS1uAddress,
                  uint16_t cellId);
  virtual ~EnbApplication (void); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Method to be assigned to the send callback of the VirtualNetDevice
   * implementing the OpenFlow S1-U logical port. It is called when the OpenFlow
   * switch sends a packet out over the logical port.
   * \param packet The packet received from the logical port.
   * \param source Ethernet source address.
   * \param dst Ethernet destination address.
   * \param protocolNo The type of payload contained in this packet.
   */
  bool RecvFromS1uLogicalPort (Ptr<Packet> packet, const Address& source,
                               const Address& dest, uint16_t protocolNo);

  // Inherited from EpcEnbApplication.
  void RecvFromS1uSocket (Ptr<Socket> socket);

protected:
  /** Destructor implementation. */
  void DoDispose (void);

  // Inherited from EpcEnbApplication.
  void DoInitialContextSetupRequest (
    uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
    std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList);

  void DoPathSwitchRequestAcknowledge (
    uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t cgi,
    std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem>
    erabToBeSwitchedInUplinkList);

  void DoUeContextRelease (uint16_t rnti);

  /**
   * Send a packet to the S-GW via the S1-U interface
   * \param packet packet to be sent
   * \param teid the Tunnel Enpoint IDentifier
   */
  void SendToS1uSocket (Ptr<Packet> packet, uint32_t teid);

private:
  /**
   * Adds the necessary Ethernet headers and trailers to a packet of data.
   * \param packet Packet to which header should be added.
   * \param source MAC source address from which packet should be sent.
   * \param dest MAC destination address to which packet should be sent.
   * \param protocolNo The type of payload contained in this packet.
   */
  void AddHeader (Ptr<Packet> packet, Mac48Address source = Mac48Address (),
                  Mac48Address dest = Mac48Address (),
                  uint16_t protocolNo = Ipv4L3Protocol::PROT_NUMBER);

  Ptr<VirtualNetDevice> m_s1uLogicalPort;    //!< OpenFlow logical port device.

  /**
   * Trace source fired when a packet arrives at this eNB from S1-U interface.
   */
  TracedCallback<Ptr<const Packet>> m_rxS1uTrace;

  /**
   * Trace source fired when a packet leaves this eNB over the S1-U interface.
   */
  TracedCallback<Ptr<const Packet>> m_txS1uTrace;

  /** Map telling for each S1-U TEID the corresponding S-GW S1-U address. */
  std::map<uint32_t, Ipv4Address> m_teidSgwAddrMap;
};

} //namespace ns3
#endif /* ENB_APPLICATION_H */
