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

#ifndef EPC_S5_SAP_H
#define EPC_S5_SAP_H

#include <ns3/lte-module.h>
#include <list>

namespace ns3 {

class EpcS5Sap
{
public:
  virtual ~EpcS5Sap ();
};

/**
 * \ingroup sdmn
 * S-GW side of the S5 Service Access Point (SAP), provides the S-GW
 * methods to be called when an S5 message is received by the S-GW.
 */
class EpcS5SapSgw : public EpcS5Sap
{
public:
  /**
   * Send the Create Session Response message.
   * \param msg The message.
   */
  virtual void CreateSessionResponse (
    EpcS11SapMme::CreateSessionResponseMessage msg) = 0;

  /**
   * Send the Modify Bearer Response message.
   * \param msg The message.
   */
  virtual void ModifyBearerResponse (
    EpcS11SapMme::ModifyBearerResponseMessage msg) = 0;
};

/**
 * \ingroup sdmn
 * P-GW side of the S5 Service Access Point (SAP), provides the P-GW
 * methods to be called when an S5 message is received by the P-GW.
 */
class EpcS5SapPgw : public EpcS5Sap
{
public:
  /**
   * Forward the Create Session Request message from the S-GW to the P-GW.
   * \param msg The message.
   */
  virtual void CreateSessionRequest (
    EpcS11SapSgw::CreateSessionRequestMessage msg) = 0;

  /**
   * Forward the Modify Bearer Request message from the S-GW to the P-GW.
   * \param msg The message.
   */
  virtual void ModifyBearerRequest (
    EpcS11SapSgw::ModifyBearerRequestMessage msg) = 0;
};

/**
 * Template for the implementation of the EpcS5SapSgw as a member
 * of an owner class of type C to which all methods are forwarded.
 */
template <class C>
class MemberEpcS5SapSgw : public EpcS5SapSgw
{
public:
  MemberEpcS5SapSgw (C* owner);

  // Inherited from EpcS5SapSgw.
  virtual void CreateSessionResponse (
    EpcS11SapMme::CreateSessionResponseMessage msg);
  virtual void ModifyBearerResponse (
    EpcS11SapMme::ModifyBearerResponseMessage msg);

private:
  MemberEpcS5SapSgw ();
  C* m_owner;
};

template <class C>
MemberEpcS5SapSgw<C>::MemberEpcS5SapSgw (C* owner)
  : m_owner (owner)
{
}

template <class C>
MemberEpcS5SapSgw<C>::MemberEpcS5SapSgw ()
{
}

template <class C>
void MemberEpcS5SapSgw<C>::CreateSessionResponse (
  EpcS11SapMme::CreateSessionResponseMessage msg)
{
  m_owner->DoCreateSessionResponse (msg);
}

template <class C>
void MemberEpcS5SapSgw<C>::ModifyBearerResponse (
  EpcS11SapMme::ModifyBearerResponseMessage msg)
{
  m_owner->DoModifyBearerResponse (msg);
}

/**
 * Template for the implementation of the EpcS5SapPgw as a member
 * of an owner class of type C to which all methods are forwarded.
 */
template <class C>
class MemberEpcS5SapPgw : public EpcS5SapPgw
{
public:
  MemberEpcS5SapPgw (C* owner);

  // Inherited from EpcS5SapPgw.
  virtual void CreateSessionRequest (
    EpcS11SapSgw::CreateSessionRequestMessage msg);
  virtual void ModifyBearerRequest (
    EpcS11SapSgw::ModifyBearerRequestMessage msg);

private:
  MemberEpcS5SapPgw ();
  C* m_owner;
};

template <class C>
MemberEpcS5SapPgw<C>::MemberEpcS5SapPgw (C* owner)
  : m_owner (owner)
{
}

template <class C>
MemberEpcS5SapPgw<C>::MemberEpcS5SapPgw ()
{
}

template <class C>
void MemberEpcS5SapPgw<C>::CreateSessionRequest (
  EpcS11SapSgw::CreateSessionRequestMessage msg)
{
  m_owner->DoCreateSessionRequest (msg);
}

template <class C>
void MemberEpcS5SapPgw<C>::ModifyBearerRequest (
  EpcS11SapSgw::ModifyBearerRequestMessage msg)
{
  m_owner->DoModifyBearerRequest (msg);
}

} //namespace ns3
#endif /* EPC_S5_SAP_H */
