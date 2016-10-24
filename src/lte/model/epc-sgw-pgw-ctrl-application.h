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

#ifndef EPC_SGW_PGW_CTRL_APPLICATION_H
#define EPC_SGW_PGW_CTRL_APPLICATION_H

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

/**
 * \ingroup lte
 *
 * This application implements the control-plane of the SGW/PGW functionality.
 */
class EpcSgwPgwCtrlApplication : public Application
{
  friend class MemberEpcS11SapSgw<EpcSgwPgwCtrlApplication>;

public:
  // inherited from Object
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   * Constructor that binds the tap device to the callback methods.
   */
  EpcSgwPgwCtrlApplication ();

  /** 
   * Destructor
   */
  virtual ~EpcSgwPgwCtrlApplication (void);
  
  /** 
   * Set the MME side of the S11 SAP 
   * 
   * \param s the MME side of the S11 SAP 
   */
  void SetS11SapMme (EpcS11SapMme * s);

  /** 
   * 
   * \return the SGW side of the S11 SAP 
   */
  EpcS11SapSgw* GetS11SapSgw ();

  /** 
   * Let the SGW be aware of a new eNB 
   * 
   * \param cellId the cell identifier
   * \param enbAddr the address of the eNB
   * \param sgwAddr the address of the SGW
   */
  void AddEnb (uint16_t cellId, Ipv4Address enbAddr, Ipv4Address sgwAddr);

  /** 
   * Let the SGW be aware of a new UE
   * 
   * \param imsi the unique identifier of the UE
   */
  void AddUe (uint64_t imsi);

  /** 
   * set the address of a previously added UE
   * 
   * \param imsi the unique identifier of the UE
   * \param ueAddr the IPv4 address of the UE
   */
  void SetUeAddress (uint64_t imsi, Ipv4Address ueAddr);


  uint32_t GetTeid (Ipv4Address addr, Ptr<Packet> packet);

  Ipv4Address GetEnbAddr (Ipv4Address ueAddr);

private:

  // S11 SAP SGW methods
  void DoCreateSessionRequest (EpcS11SapSgw::CreateSessionRequestMessage msg);
  void DoModifyBearerRequest (EpcS11SapSgw::ModifyBearerRequestMessage msg);  

  void DoDeleteBearerCommand (EpcS11SapSgw::DeleteBearerCommandMessage req);
  void DoDeleteBearerResponse (EpcS11SapSgw::DeleteBearerResponseMessage req);


  /**
   * store info for each UE connected to this SGW
   */
  class UeInfo : public SimpleRefCount<UeInfo>
  {
public:
    UeInfo ();  

    /** 
     * 
     * \param tft the Traffic Flow Template of the new bearer to be added
     * \param epsBearerId the ID of the EPS Bearer to be activated
     * \param teid  the TEID of the new bearer
     */
    void AddBearer (Ptr<EpcTft> tft, uint8_t epsBearerId, uint32_t teid);

    /** 
     * \brief Function, deletes contexts of bearer on SGW and PGW side
     * \param bearerId, the Bearer Id whose contexts to be removed
     */
    void RemoveBearer (uint8_t bearerId);

    /**
     * 
     * 
     * \param p the IP packet from the internet to be classified
     * 
     * \return the corresponding bearer ID > 0 identifying the bearer
     * among all the bearers of this UE;  returns 0 if no bearers
     * matches with the previously declared TFTs
     */
    uint32_t Classify (Ptr<Packet> p);

    /** 
     * \return the address of the eNB to which the UE is connected
     */
    Ipv4Address GetEnbAddr ();

    /** 
     * set the address of the eNB to which the UE is connected
     * 
     * \param addr the address of the eNB
     */
    void SetEnbAddr (Ipv4Address addr);

    /** 
     * \return the address of the UE
     */
    Ipv4Address GetUeAddr ();

    /** 
     * set the address of the UE
     * 
     * \param addr the address of the UE
     */
    void SetUeAddr (Ipv4Address addr);


  private:
    EpcTftClassifier m_tftClassifier;
    Ipv4Address m_enbAddr;
    Ipv4Address m_ueAddr;
    std::map<uint8_t, uint32_t> m_teidByBearerIdMap;
  };

  /**
   * Map telling for each UE address the corresponding UE info 
   */
  std::map<Ipv4Address, Ptr<UeInfo> > m_ueInfoByAddrMap;

  /**
   * Map telling for each IMSI the corresponding UE info 
   */
  std::map<uint64_t, Ptr<UeInfo> > m_ueInfoByImsiMap;

  /**
   * TEID counter, used to allocate GTP teid values.
   * \internal
   * This counter is initialized at 0x0000000F, reserving the first values.
   */
  uint32_t m_teidCount;

  /**
   * MME side of the S11 SAP
   * 
   */
  EpcS11SapMme* m_s11SapMme;

  /**
   * SGW side of the S11 SAP
   * 
   */
  EpcS11SapSgw* m_s11SapSgw;

  struct EnbInfo
  {
    Ipv4Address enbAddr;
    Ipv4Address sgwAddr;    
  };

  std::map<uint16_t, EnbInfo> m_enbInfoByCellId;
};

} //namespace ns3

#endif /* EPC_SGW_PGW_CTRL_APPLICATION_H */

