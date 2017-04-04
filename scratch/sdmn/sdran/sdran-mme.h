/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *               2017 University of Campinas (Unicamp)
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef SDMN_MME_H
#define SDMN_MME_H

#include <map>
#include <list>
#include <ns3/lte-module.h>

namespace ns3 {

class Node;
class NetDevice;

/**
 * \ingroup sdmnSdran
 * This class implements the MME functionality. This is a stateless
 * implementation in terms of UE and eNB information, so we can have as many
 * instances we want at different places that they all will work over the same
 * data.
 */
class SdranMme : public Object
{
  friend class MemberEpcS1apSapMme<SdranMme>;
  friend class MemberEpcS11SapMme<SdranMme>;

public:
  SdranMme ();          //!< Default constructor.
  virtual ~SdranMme (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  EpcS1apSapMme* GetS1apSapMme (void) const;
  EpcS11SapMme* GetS11SapMme (void) const;

  void SetS11SapSgw (EpcS11SapSgw *sap);
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  // S1-AP SAP MME forwarded methods.
  void DoInitialUeMessage (
    uint64_t mmeUeS1Id, uint16_t enbUeS1Id, uint64_t imsi, uint16_t ecgi);
  void DoInitialContextSetupResponse (
    uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
    std::list<EpcS1apSapMme::ErabSetupItem> erabList);
  void DoPathSwitchRequest (
    uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t cgi,
    std::list<EpcS1apSapMme::ErabSwitchedInDownlinkItem> erabList);
  void DoErabReleaseIndication (
    uint64_t mmeUeS1Id, uint16_t enbUeS1Id,
    std::list<EpcS1apSapMme::ErabToBeReleasedIndication> erabList);

  // S11 SAP MME forwarded methods.
  void DoCreateSessionResponse (
    EpcS11SapMme::CreateSessionResponseMessage msg);
  void DoModifyBearerResponse (
    EpcS11SapMme::ModifyBearerResponseMessage msg);
  void DoDeleteBearerRequest (
    EpcS11SapMme::DeleteBearerRequestMessage msg);

  // SAP pointers.
  EpcS1apSapMme* m_s1apSapMme;
  EpcS11SapMme*  m_s11SapMme;
  EpcS11SapSgw*  m_s11SapSgw;
};

} // namespace ns3
#endif // SDMN_MME_H
