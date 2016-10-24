/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *               2016 University of Campinas (Unicamp)
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
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef EPC_SGW_PGW_USER_APPLICATION_H
#define EPC_SGW_PGW_USER_APPLICATION_H

#include <ns3/address.h>
#include <ns3/socket.h>
#include <ns3/virtual-net-device.h>
#include <ns3/traced-callback.h>
#include <ns3/callback.h>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <ns3/eps-bearer.h>
#include <ns3/epc-tft.h>
#include <ns3/epc-tft-classifier.h>
#include <ns3/lte-common.h>
#include <ns3/application.h>
#include <ns3/epc-s1ap-sap.h>
#include <ns3/epc-s11-sap.h>
#include <map>

namespace ns3 {

class EpcSgwPgwCtrlApplication;

/**
 * \ingroup lte
 * This application implements the user-plane of the SGW/PGW functionality.
 */
class EpcSgwPgwUserApplication : public Application
{
  friend class EpcSgwPgwCtrlApplication;
  
public:
  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   * Constructor that binds the tap device to the callback methods.
   *
   * \param tunDevice TUN VirtualNetDevice used to tunnel IP packets from
   * the Gi interface of the PGW/SGW over the
   * internet over GTP-U/UDP/IP on the S1-U interface
   * \param s1uSocket socket used to send GTP-U packets to the eNBs
   * \param ctrlPlane Control plane of the gateway
   */
  EpcSgwPgwUserApplication (const Ptr<VirtualNetDevice> tunDevice,
                            const Ptr<Socket> s1uSocket,
                            Ptr<EpcSgwPgwCtrlApplication> ctrlPlane);

  /** 
   * Destructor
   */
  virtual ~EpcSgwPgwUserApplication (void);
  
  /** 
   * Method to be assigned to the callback of the Gi TUN VirtualNetDevice. It
   * is called when the SGW/PGW receives a data packet from the
   * internet (including IP headers) that is to be sent to the UE via
   * its associated eNB, tunneling IP over GTP-U/UDP/IP.
   * 
   * \param packet 
   * \param source 
   * \param dest 
   * \param protocolNumber 
   * \return true always 
   */
  bool RecvFromTunDevice (Ptr<Packet> packet, const Address& source,
                          const Address& dest, uint16_t protocolNumber);

  /** 
   * Method to be assigned to the recv callback of the S1-U socket. It
   * is called when the SGW/PGW receives a data packet from the eNB
   * that is to be forwarded to the internet. 
   * 
   * \param socket pointer to the S1-U socket
   */
  void RecvFromS1uSocket (Ptr<Socket> socket);

  /** 
   * Send a packet to the internet via the Gi interface.
   * \param packet The packet to send
   * \param teid The TEID value
   */
  void SendToTunDevice (Ptr<Packet> packet, uint32_t teid);

  /** 
   * Send a packet to the eNB via the S1-U interface.
   * \param packet packet to send
   * \param enbS1uAddress the address of the eNB
   * \param teid the Tunnel Enpoint IDentifier
   */
  void SendToS1uSocket (Ptr<Packet> packet, Ipv4Address enbS1uAddress, uint32_t teid);

private:

 /**
  * UDP socket to send and receive GTP-U packets to and from the S1-U interface
  */
  Ptr<Socket> m_s1uSocket;
  
  /**
   * TUN VirtualNetDevice used for tunneling/detunneling IP packets
   * from/to the internet over GTP-U/UDP/IP on the S1 interface 
   */
  Ptr<VirtualNetDevice> m_tunDevice;

  /**
   * UDP port to be used for GTP
   */
  uint16_t m_gtpuUdpPort;


  Ptr<EpcSgwPgwCtrlApplication> m_controlPlane;


  /** 
   * Trace source fired when a packet arrives this eNB from S1-U interface. 
   */
  TracedCallback<Ptr<const Packet> > m_rxS1uTrace;

  /** 
   * Trace source fired when a packet leaves this eNB over the S1-U interface.
   */
  TracedCallback<Ptr<const Packet> > m_txS1uTrace;
};

} //namespace ns3

#endif /* EPC_SGW_PGW_USER_APPLICATION_H */

