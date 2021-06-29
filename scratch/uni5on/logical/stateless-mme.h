/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic d Telecomunicacions de Catalunya (CTTC)
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
 *         Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef STATELESS_MME_H
#define STATELESS_MME_H

#include <map>
#include <list>
#include <ns3/lte-module.h>

namespace ns3 {

class Node;
class NetDevice;

/**
 * \ingroup uni5onLogical
 * The MME entity. The UNI5ON architecture expects one MME entity inside
 * each LTE slice controller. However, in this implementation we are using a
 * single shared stateless MME entity. The current ns-3 LTE implementation
 * limits the eNB connected to a single MME. Considering that in UNI5ON
 * architecture the eNBs share their resources with all slices, we decided to
 * use a single shared stateless MME entity. Then, internally it can forward
 * the control messages to the correct LTE slice controller.
 */
class StatelessMme : public Object
{
  friend class MemberEpcS1apSapMme<StatelessMme>;
  friend class MemberEpcS11SapMme<StatelessMme>;

public:
  StatelessMme ();          //!< Default constructor.
  virtual ~StatelessMme (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** \name Private member accessors. */
  //\{
  EpcS1apSapMme* GetS1apSapMme (void) const;
  EpcS11SapMme* GetS11SapMme (void) const;
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
    uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t gci,
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
};

} // namespace ns3
#endif // STATELESS_MME_H
