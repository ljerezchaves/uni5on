/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#ifndef ROUTING_INFO_H
#define ROUTING_INFO_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

class OpenFlowEpcController;
class RingController;

/** EPS context bearer */
typedef EpcS11SapMme::BearerContextCreated ContextBearer_t;

/** List of created context bearers */
typedef std::list<ContextBearer_t> BearerList_t;


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to a routing path between
 * two any switches in the OpenFlow network.
 */
class RoutingInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  RoutingInfo ();          //!< Default constructor
  virtual ~RoutingInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return True if the associated EPS bearer is of GBR type. */
  bool IsGbr (void) const;

  /** \return The EPS Bearer. */
  EpsBearer GetEpsBearer (void) const;

  /** \return The Bearer QoS information. */
  GbrQosInformation GetQosInfo (void) const;

  /** \return The GTP TEID. */
  uint32_t GetTeid (void) const;

  /** \return True when there is downlink traffic in this bearer. */
  bool HasDownlinkTraffic (void) const;
  
  /** \return True when there is uplink traffic in this bearer. */
  bool HasUplinkTraffic (void) const;

private:
  uint32_t          m_teid;         //!< GTP TEID
  uint16_t          m_sgwIdx;       //!< Sgw switch index
  uint16_t          m_enbIdx;       //!< eNB switch index
  Ipv4Address       m_sgwAddr;      //!< Sgw IPv4 address
  Ipv4Address       m_enbAddr;      //!< eNB IPv4 address
  int               m_priority;     //!< Flow priority
  int               m_timeout;      //!< Flow idle timeout
  bool              m_isDefault;    //!< This info is for default bearer
  bool              m_isInstalled;  //!< Rule is installed into switches
  bool              m_isActive;     //!< Application traffic is active
  ContextBearer_t   m_bearer;       //!< EPS bearer information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to meter rules.
 */
class MeterInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  MeterInfo ();          //!< Default constructor
  virtual ~MeterInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This MeterInfo object must be aggregated to rInfo.
   */
  MeterInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /** Get Dpctl commands to add or delete meter rules */
  //\{
  std::string GetDownAddCmd (void) const;
  std::string GetUpAddCmd (void) const;
  std::string GetDelCmd (void) const;
  //\}

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isInstalled;   //!< True when this meter is installed
  bool     m_hasDown;       //!< True for downlink meter
  bool     m_hasUp;         //!< True for uplink meter
  DataRate m_downDataRate;  //!< Downlink meter drop rate (bps)
  DataRate m_upDataRate;    //!< Uplink meter drop rate (bps)
  Ptr<RoutingInfo> m_rInfo; //!< Routing information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to GBR beares.
 */
class ReserveInfo : public Object
{
  friend class OpenFlowEpcController;
  friend class RingController;

public:
  ReserveInfo ();          //!< Default constructor
  virtual ~ReserveInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \attention This ReserveInfo object must be aggregated to rInfo.
   */
  ReserveInfo (Ptr<RoutingInfo> rInfo);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /** \return Downlink reserved data rate. */
  DataRate GetDownDataRate (void) const;

  /** \return Uplink reserved data rate. */
  DataRate GetUpDataRate (void) const;

private:
  uint32_t m_teid;          //!< GTP TEID
  bool     m_isReserved;    //!< True when resources are reserved
  bool     m_hasDown;       //!< True for downlink reserve
  bool     m_hasUp;         //!< True for uplink reserve
  DataRate m_downDataRate;  //!< Downlink reserved data rate
  DataRate m_upDataRate;    //!< Uplink reserved data rate
  Ptr<RoutingInfo> m_rInfo; //!< Routing information
};


// ------------------------------------------------------------------------ //
/**
 * \ingroup epcof
 * Metadata associated to a ring routing path between
 * two any switches in the OpenFlow ring network.
 */
class RingRoutingInfo : public Object
{
  friend class RingController;

public:
  /** Routing direction in the ring. */
  enum RoutingPath
  {
    CLOCK = 1,
    COUNTER = 2
  };

  RingRoutingInfo ();          //!< Default constructor
  virtual ~RingRoutingInfo (); //!< Dummy destructor, see DoDipose

  /**
   * Complete constructor.
   * \param rInfo RoutingInfo pointer.
   * \param downPath The path for downlink (uplink will get the inverse path).
   * \attention This RingRoutingInfo object must be aggregated to rInfo.
   */
  RingRoutingInfo (Ptr<RoutingInfo> rInfo, RoutingPath downPath);

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Invert the routing path
   * \param path The original routing path.
   * \return The inverse routing path.
   */
  static RoutingPath InvertPath (RoutingPath path);

  /** Destructor implementation */
  virtual void DoDispose ();

  /** \return RoutingInfo pointer. */
  Ptr<RoutingInfo> GetRoutingInfo ();

  /** \return True for downlink inverte path. */
  bool IsDownInv (void) const;

  /** \return True for uplink inverte path. */
  bool IsUpInv (void) const;

private:
  /** Invert down/up routing direction. */
  void InvertDownPath ();
  void InvertUpPath ();
  void ResetPaths ();

  Ptr<RoutingInfo> m_rInfo;     //!< Routing information
  RoutingPath      m_downPath;  //!< Downlink routing path
  RoutingPath      m_upPath;    //!< Uplink routing path
  bool             m_isDownInv; //!< True when down path is inverted
  bool             m_isUpInv;   //!< True when up path is inverted
};

};  // namespace ns3
#endif // ROUTING_INFO_H

